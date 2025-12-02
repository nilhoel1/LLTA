#include "MIRPasses/AdressResolverPass.h"
#include "MIRPasses/AsmDumpAndCheckPass.h"
#include "TimingAnalysisResults.h"
#include "Utility/Options.h"
#include "llvm/CodeGen/GlobalISel/IRTranslator.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include <fstream>
#include <map>

namespace llvm {

char AdressResolverPass::ID = 1;

/**
 * @brief Construct a new Adress Resolver Pass:: Adress Resolver Pass
 * object
 *
 * @paramTAR
 */
AdressResolverPass::AdressResolverPass(TimingAnalysisResults &TAR)
    : MachineFunctionPass(ID), TAR(TAR) {}

/**
 * @brief Checks if unknown Instructions were found.
 *        Always returns false.
 *
 * @return false
 */
bool AdressResolverPass::doFinalization(Module &M) { return false; }

/*datastructure used for matching parsed address information with instructions*/
struct InstructionLineRelation {
  int Address;
  std::string MachineCode;
  std::string AssemblerCode;
  MachineInstr *Instruction;
};
static std::multimap<int, InstructionLineRelation> InstructionRelationData;

/**
 * @brief Checks if unknown Instructions were found.
 *        Always returns false.
 *
 * @return false
 */
bool AdressResolverPass::doInitialization(Module &M) {
  parseFile(M.getModuleIdentifier());
  if (DebugPrints)
    outs() << "DumpFilename: " << DumpFilename << "\n";
  return false;
}

bool AdressResolverPass::runOnMachineFunction(MachineFunction &F) {
  int InstructionLine = 0;
  std::string Opcode = "";
  std::string InstructionLineString = "";
  std::pair<std::multimap<int, InstructionLineRelation>::iterator,
            std::multimap<int, InstructionLineRelation>::iterator>
      LineFind;
  int NumberObjdumpEntries = InstructionRelationData.size();
  int NumberInstructionsNotMapped = 0;
  int NumberInstructionMapped = 0;
  /*iterate over instructions*/
  // for(Module::iterator F = M.begin(), E = M.end(); F != E; ++F){
  for (MachineFunction::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
    for (MachineBasicBlock::iterator I = BB->begin(), E = BB->end(); I != E;
         ++I) {
      /*save debug location*/
      DILocation *Location = I->getDebugLoc().get();
      /*match instruction to address information*/
      if (Location != NULL) {

        InstructionLine = Location->getLine();
        // Opcode = I->getOpcodeName();
        InstructionLineString = std::to_string(InstructionLine);
        LineFind = InstructionRelationData.equal_range(InstructionLine);
        auto *Inst = dyn_cast<MachineInstr>(I->getIterator());
        for (auto It = LineFind.first; It != LineFind.second; ++It) {
          It->second.Instruction = Inst;
        }
        if (LineFind.first == LineFind.second) {
          InstructionLineRelation InstLiRel =
              InstructionLineRelation{-1, "", "", Inst};
          InstructionRelationData.insert(
              std::pair<int, InstructionLineRelation>(InstructionLine,
                                                      InstLiRel));
          NumberInstructionsNotMapped++;
        } else {
          NumberInstructionMapped++;
        }
      }
    }
  }
  //}
  /*output*/
  llvm::outs() << "\n";

  for (auto It = InstructionRelationData.begin();
       It != InstructionRelationData.end(); ++It) {
    if (It->second.Instruction != NULL) {
      if (It->second.Address > 0) {
        // std::stringstream Sstream;
        // Sstream << std::hex << It->second.Address;
        // llvm::outs() << "Line: " << It->first << " Instruction: "
        // << It->second.Instruction->getOpcode() << " Function: "
        // << It->second.Instruction->getMF()->getName() << " - Address: 0x"
        // << Sstream.str() << " MachineCode: " << It->second.MachineCode
        // << " AssemblerCode: " << It->second.AssemblerCode <<"\n";
      } else {
        // llvm::outs() << "Line: " << It->first << " Instruction: " <<
        // It->second.Instruction->getOpcode() << " Function: " <<
        // It->second.Instruction->getMF()->getName() <<
        // " - no address information found\n";
      }
    }
  }
  if (DebugPrints)
    llvm::outs() << "\n"
                 << NumberInstructionMapped << " instructions mapped to "
                 << NumberObjdumpEntries << " address information entries\n"
                 << NumberInstructionsNotMapped
                 << " instructions could not be mapped to address "
                    "information\nTotal information entries: "
                 << InstructionRelationData.size() << "\n";
  return false;
}
/*scans file for address information and saves it in the CompilerData
 * datastructure*/
void AdressResolverPass::parseFile(std::string ModuleIdentifier) {
  /*parse debug output*/
  if (DebugPrints)
    llvm::outs() << "Scanning " << ModuleIdentifier
                 << "for address and line information\n";

  int LineNumber = -1;
  int NewLineNumber;
  std::string Line;
  std::ifstream ConfigFile(DumpFilename.c_str());
  std::string Subline;
  /*scans each line of the file for information*/
  if (ConfigFile.is_open()) {
    while (!ConfigFile.eof()) {
      getline(ConfigFile, Line);
      /*check if line contains a debug line number*/
      NewLineNumber = lineHasLineNumber(Line);
      /*if a new line number is available update it, else check if address
       * information can be parsed; if it cannot be parsed reset line number,
       * because the block is over*/
      if (NewLineNumber > 0) {
        LineNumber = NewLineNumber;
      } else if (!parseLine(Line, LineNumber)) {
        /*skip comment lines starting with ; or #*/
        if (Line.find(";", 0) == std::string::npos) {
          if (Line.find("#", 0) == std::string::npos) {
            LineNumber = -1;
          }
        }
      }
    }
  } else {
    if (DebugPrints)
      llvm::outs() << "Could not parse information from the input file.\n";
  }
}
/*parses a line with address information, if the line contains it; returns true
 * if line is successfully parsed*/
bool AdressResolverPass::parseLine(std::string Line, int LineNumber) {

  if (LineNumber > 0) {
    std::tuple<int, std::string, std::string> AddressInformation;
    int AddressCast = 0;
    std::string MachineCode = "";
    std::string AssemblerCode = "";
    /*find possible Address*/
    std::string::size_type StartPos = 0;
    StartPos = Line.find(":", StartPos);
    if (StartPos > 0 && StartPos != std::string::npos) {
      std::string PossibleAddress = Line.substr(0, StartPos);
      if (isHex(PossibleAddress)) {
        AddressCast = std::stoi(PossibleAddress, NULL, 16);
      }

      /*if an address is found, the line contains information to parse*/
      if (AddressCast > 0) {

        bool CodeNotOver = true;
        char *CLine;
        CLine = new char[Line.substr(StartPos + 2, Line.length()).size() + 1];
        strcpy(CLine, Line.substr(StartPos + 2, Line.length()).c_str());
        char *Tokens = strtok(CLine, " ");
        std::string TTokens = "";
        /*parse machine code*/
        while (Tokens != NULL && CodeNotOver) {
          TTokens = Tokens;
          /*machine code consists of 2 character blocks, if the block is bigger
           * the machine code is over*/
          if (TTokens.length() != 2) {
            CodeNotOver = false;
            MachineCode = MachineCode.substr(0, MachineCode.length() -
                                                    1); // cut last whitespace
          } else {
            MachineCode = MachineCode + Tokens + " ";
            Tokens = strtok(NULL, " ");
          }
        }
        /*parse assembler code, remove the first tab character and tab character
         * in between*/
        if (Tokens != NULL) {
          AssemblerCode = Tokens;

          /*check if the token still contains machine code, due to Tokens not
           * seperating correctly*/
          TTokens = AssemblerCode.substr(0, 2);
          if (isHex(TTokens)) {
            MachineCode = MachineCode + " " + TTokens;
            AssemblerCode = AssemblerCode.substr(2, AssemblerCode.length());
          }
          AssemblerCode = AssemblerCode.substr(1, AssemblerCode.length()) +
                          " "; // cut first whitespace in token

          /*remove possible tab character in assembler code*/
          StartPos = AssemblerCode.find("\t", 0);
          if (StartPos != std::string::npos && StartPos != 0) {
            AssemblerCode =
                AssemblerCode.substr(0, StartPos) + " " +
                AssemblerCode.substr(StartPos + 1, AssemblerCode.length());
          } else if (StartPos == 0) {
            AssemblerCode = AssemblerCode.substr(1, AssemblerCode.length());
          }
          Tokens = strtok(NULL, " ");
        }
        /*parse rest of assembler code */
        CodeNotOver = true;
        while (Tokens != NULL && CodeNotOver) {
          TTokens = Tokens;
          StartPos = TTokens.find("#", 0);
          if (StartPos == std::string::npos) {
            AssemblerCode = AssemblerCode + Tokens + " ";
            Tokens = strtok(NULL, " ");
          } else {
            CodeNotOver = false;
          }
        }
        AssemblerCode = AssemblerCode.substr(0, AssemblerCode.length() -
                                                    1); // cut last whitespace
        /*save parsed object*/
        InstructionLineRelation InstLiRel = InstructionLineRelation{
            AddressCast, MachineCode, AssemblerCode, NULL};
        InstructionRelationData.insert(
            std::pair<int, InstructionLineRelation>(LineNumber, InstLiRel));
        return true;
      }
    }
  }
  return false;
}
/*returns line number of a debug file if available, else returns -1*/
int AdressResolverPass::lineHasLineNumber(std::string Line) {

  int LineNumber = 0;
  std::string::size_type StartPos = 0;

  /*try to find filename in line*/
  StartPos = Line.find(".c", StartPos);
  if (StartPos > 0 && StartPos != std::string::npos) {
    std::string PossibleLineNumber = Line.substr(StartPos, Line.length());
    /*try to cast line number after filename*/
    if (PossibleLineNumber.substr(0, 3) == ".c:") {
      LineNumber =
          std::stoi(PossibleLineNumber.substr(3, PossibleLineNumber.length()));
      return LineNumber;
    }
  }
  return -1;
}
/*returns if a string consists only of hex literals or whitespace*/
bool AdressResolverPass::isHex(std::string &In) {
  for (char C : In) {
    if (C != ' ') {
      if (!isxdigit(C))
        return false;
    }
  }
  return true;
}

MachineFunctionPass *createAdressResolverPass(TimingAnalysisResults &TAR) {
  return new AdressResolverPass(TAR);
}
} // namespace llvm
