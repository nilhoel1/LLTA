//===- LoopBoundPlugin.cpp - Clang Plugin for Loop Bound Annotations -----===//
//
// This plugin parses #pragma loop_bound(lower, upper) annotations and emits
// them as LLVM loop metadata for WCET analysis.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Attr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Token.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/FileSystem.h"
#include <map>
#include <vector>

using namespace clang;

namespace {

// Struct to store loop bound information
struct LoopBoundInfo {
  unsigned LowerBound;
  unsigned UpperBound;
};

// Global map to store loop bounds keyed by the location after the pragma
// We use a pointer so it persists across different compilation stages
static std::map<unsigned, LoopBoundInfo> *LoopBoundMap = nullptr;

// Global flag for verbose output
static bool VerboseOutput = false;

// Structure to store loop bound with location for JSON export
struct LoopBoundExport {
  std::string FileName;
  unsigned Line;
  unsigned Column;
  unsigned LowerBound;
  unsigned UpperBound;
};

// Global vector to collect all loop bounds for export
static std::vector<LoopBoundExport> *LoopBoundsToExport = nullptr;

// Pragma handler for #pragma loop_bound(lower, upper)
class LoopBoundPragmaHandler : public PragmaHandler {
public:
  LoopBoundPragmaHandler() : PragmaHandler("loop_bound") {
    if (VerboseOutput)
      llvm::errs() << "[LoopBoundPlugin] Pragma handler registered\n";
    if (!LoopBoundMap) {
      LoopBoundMap = new std::map<unsigned, LoopBoundInfo>();
      if (VerboseOutput)
        llvm::errs() << "[LoopBoundPlugin] Created new LoopBoundMap\n";
    }
    if (!LoopBoundsToExport) {
      LoopBoundsToExport = new std::vector<LoopBoundExport>();
      if (VerboseOutput)
        llvm::errs() << "[LoopBoundPlugin] Created new LoopBoundsToExport\n";
    }
  }  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) override {
    SourceLocation PragmaLoc = PragmaTok.getLocation();
    if (VerboseOutput)
      llvm::errs() << "[LoopBoundPlugin] HandlePragma called at "
                   << PragmaLoc.printToString(PP.getSourceManager()) << "\n";

    // Get the next token (should be '(')
    Token Tok;
    PP.Lex(Tok);
    if (Tok.isNot(tok::l_paren)) {
      PP.Diag(Tok.getLocation(), diag::err_expected) << "(";
      return;
    }

    // Get the lower bound value
    PP.Lex(Tok);
    if (Tok.isNot(tok::numeric_constant)) {
      PP.Diag(Tok.getLocation(), diag::err_expected) << "numeric constant";
      return;
    }

    // Parse the lower bound numeric value
    llvm::SmallString<64> SpellingBuffer;
    bool Invalid = false;
    StringRef LowerSpelling = PP.getSpelling(Tok, SpellingBuffer, &Invalid);
    if (Invalid) {
      PP.Diag(Tok.getLocation(), diag::err_expected) << "numeric constant";
      return;
    }

    unsigned LowerBound = 0;
    if (LowerSpelling.getAsInteger(10, LowerBound)) {
      PP.Diag(Tok.getLocation(), diag::err_expected) << "numeric constant";
      return;
    }

    // Get the comma
    PP.Lex(Tok);
    if (Tok.isNot(tok::comma)) {
      PP.Diag(Tok.getLocation(), diag::err_expected) << ",";
      return;
    }

    // Get the upper bound value
    PP.Lex(Tok);
    if (Tok.isNot(tok::numeric_constant)) {
      PP.Diag(Tok.getLocation(), diag::err_expected) << "numeric constant";
      return;
    }

    // Parse the upper bound numeric value
    SpellingBuffer.clear();
    StringRef UpperSpelling = PP.getSpelling(Tok, SpellingBuffer, &Invalid);
    if (Invalid) {
      PP.Diag(Tok.getLocation(), diag::err_expected) << "numeric constant";
      return;
    }

    unsigned UpperBound = 0;
    if (UpperSpelling.getAsInteger(10, UpperBound)) {
      PP.Diag(Tok.getLocation(), diag::err_expected) << "numeric constant";
      return;
    }

    // Get the closing ')'
    PP.Lex(Tok);
    if (Tok.isNot(tok::r_paren)) {
      PP.Diag(Tok.getLocation(), diag::err_expected) << ")";
      return;
    }

    // Expect end of pragma directive - just skip any remaining tokens
    PP.Lex(Tok);
    if (Tok.isNot(tok::eod)) {
      // Silently consume extra tokens
    }

    // Store the bound information keyed by the file offset of the pragma location
    SourceManager &SM = PP.getSourceManager();
    unsigned Offset = SM.getFileOffset(PragmaLoc);

    LoopBoundInfo Info;
    Info.LowerBound = LowerBound;
    Info.UpperBound = UpperBound;
    (*LoopBoundMap)[Offset] = Info;

    if (VerboseOutput) {
      llvm::errs() << "[LoopBoundPlugin] Stored loop_bound pragma at offset " << Offset
                   << " with lower: " << LowerBound
                   << ", upper: " << UpperBound << "\n";
      llvm::errs().flush();
    }
  }

};

// AST visitor to attach loop metadata
class LoopBoundVisitor : public RecursiveASTVisitor<LoopBoundVisitor> {
public:
  explicit LoopBoundVisitor(ASTContext &Context, Sema *S)
      : Context(Context), SemaPtr(S) {}  bool VisitForStmt(ForStmt *S) {
    if (VerboseOutput)
      llvm::errs() << "[LoopBoundPlugin] Found for loop at "
                   << S->getBeginLoc().printToString(Context.getSourceManager()) << "\n";
    ProcessLoop(S, S->getBeginLoc());
    return true;
  }

  bool VisitWhileStmt(WhileStmt *S) {
    ProcessLoop(S, S->getBeginLoc());
    return true;
  }

  bool VisitDoStmt(DoStmt *S) {
    ProcessLoop(S, S->getBeginLoc());
    return true;
  }

private:
  void ProcessLoop(Stmt *S, SourceLocation Loc) {
    if (!LoopBoundMap)
      return;

    SourceManager &SM = Context.getSourceManager();

    // Search backwards from the loop location to find a pragma
    // Pragmas are typically on the line before the loop
    unsigned LoopOffset = SM.getFileOffset(Loc);

    // Look for a pragma within ~200 bytes before the loop
    // (to account for whitespace and newlines)
    LoopBoundInfo *BoundInfo = nullptr;
    unsigned BestOffset = 0;

    for (auto &Entry : *LoopBoundMap) {
      unsigned PragmaOffset = Entry.first;
      // Pragma should be before the loop, but not too far before
      if (PragmaOffset < LoopOffset && (LoopOffset - PragmaOffset) < 200) {
        // Take the closest pragma
        if (PragmaOffset > BestOffset) {
          BestOffset = PragmaOffset;
          BoundInfo = &Entry.second;
        }
      }
    }

    if (BoundInfo) {
      if (VerboseOutput) {
        llvm::errs() << "[LoopBoundPlugin] Attaching loop bounds (lower: " << BoundInfo->LowerBound
                     << ", upper: " << BoundInfo->UpperBound << ") to loop at "
                     << Loc.printToString(SM) << "\n";
        llvm::errs().flush();
      }

      // Create loop attributes using Clang's attribute mechanism
      // We'll use the loop hint attributes that Clang already supports
      AttachLoopMetadata(S, BoundInfo->LowerBound, BoundInfo->UpperBound);

      // Remove the pragma from the map so it's not reused
      LoopBoundMap->erase(BestOffset);
    }
  }

  void AttachLoopMetadata(Stmt *S, unsigned Lower, unsigned Upper) {
    // Export loop bounds to JSON file for consumption by LLVM IR pass
    SourceLocation Loc = S->getBeginLoc();
    SourceManager &SM = Context.getSourceManager();

    if (VerboseOutput) {
      llvm::errs() << "[LoopBoundPlugin] Storing loop bounds (lower: " << Lower
                   << ", upper: " << Upper << ") for loop at "
                   << Loc.printToString(SM) << "\n";
    }

    // Store loop bound with location information
    if (LoopBoundsToExport) {
      LoopBoundExport Export;
      Export.FileName = SM.getFilename(Loc).str();
      Export.Line = SM.getSpellingLineNumber(Loc);
      Export.Column = SM.getSpellingColumnNumber(Loc);
      Export.LowerBound = Lower;
      Export.UpperBound = Upper;
      LoopBoundsToExport->push_back(Export);
    }
  }

  ASTContext &Context;
  Sema *SemaPtr;
};

class LoopBoundASTConsumer : public ASTConsumer {
public:
  explicit LoopBoundASTConsumer(ASTContext &Context, Sema *S)
      : Visitor(Context, S) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    if (VerboseOutput)
      llvm::errs() << "[LoopBoundPlugin] Traversing AST for loops\n";
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    if (VerboseOutput)
      llvm::errs() << "[LoopBoundPlugin] AST traversal complete\n";

    // Export loop bounds to JSON file
    ExportLoopBoundsJSON(Context);
  }

  void ExportLoopBoundsJSON(ASTContext &Context) {
    if (!LoopBoundsToExport || LoopBoundsToExport->empty()) {
      if (VerboseOutput)
        llvm::errs() << "[LoopBoundPlugin] No loop bounds to export\n";
      return;
    }

    // Build JSON array
    llvm::json::Array LoopBoundsArray;
    for (const auto &LB : *LoopBoundsToExport) {
      llvm::json::Object LoopObj;
      LoopObj["file"] = LB.FileName;
      LoopObj["line"] = LB.Line;
      LoopObj["column"] = LB.Column;
      LoopObj["lower_bound"] = LB.LowerBound;
      LoopObj["upper_bound"] = LB.UpperBound;
      LoopBoundsArray.push_back(std::move(LoopObj));
    }

    // Create JSON object
    llvm::json::Object Root;
    Root["loop_bounds"] = std::move(LoopBoundsArray);

    // Get the main source file name
    SourceManager &SM = Context.getSourceManager();
    FileID MainFileID = SM.getMainFileID();
    StringRef MainFileName = SM.getFileEntryRefForID(MainFileID)->getName();

    // Generate output filename: source.c -> source.c.loop_bounds.json
    std::string OutputFileName = MainFileName.str() + ".loop_bounds.json";

    // Write JSON to file
    std::error_code EC;
    llvm::raw_fd_ostream OutFile(OutputFileName, EC);
    if (EC) {
      llvm::errs() << "[LoopBoundPlugin] Error opening file " << OutputFileName
                   << ": " << EC.message() << "\n";
      return;
    }

    OutFile << llvm::formatv("{0:2}", llvm::json::Value(std::move(Root)));
    OutFile.close();

    if (VerboseOutput)
      llvm::errs() << "[LoopBoundPlugin] Exported " << LoopBoundsToExport->size()
                   << " loop bounds to " << OutputFileName << "\n";
  }

private:
  LoopBoundVisitor Visitor;
};

class LoopBoundPluginAction : public PluginASTAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                   StringRef) override {
    if (VerboseOutput)
      llvm::errs() << "[LoopBoundPlugin] Plugin action CreateASTConsumer called\n";
    // Register the pragma handler
    Preprocessor &PP = CI.getPreprocessor();
    PP.AddPragmaHandler(new LoopBoundPragmaHandler());
    if (VerboseOutput)
      llvm::errs() << "[LoopBoundPlugin] Pragma handler added to preprocessor\n";
    // Pass nullptr for Sema since we don't need it anymore
    return std::make_unique<LoopBoundASTConsumer>(CI.getASTContext(), nullptr);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    for (const auto &arg : args) {
      if (arg == "verbose" || arg == "-v") {
        VerboseOutput = true;
      } else if (arg == "help" || arg == "-h") {
        llvm::errs() << "LoopBoundPlugin usage:\n"
                     << "  -plugin-arg-loop-bound verbose  Enable verbose output\n"
                     << "  -plugin-arg-loop-bound help     Show this help\n";
      }
    }
    if (VerboseOutput)
      llvm::errs() << "[LoopBoundPlugin] ParseArgs called with verbose output enabled\n";
    return true;
  }
};} // namespace

static FrontendPluginRegistry::Add<LoopBoundPluginAction>
    X("loop-bound", "Parse loop bound pragmas and emit metadata");

// Add module initialization to verify the plugin is loaded
__attribute__((constructor))
static void InitPlugin() {
  if (VerboseOutput)
    llvm::errs() << "[LoopBoundPlugin] Plugin module loaded!\n";
}
