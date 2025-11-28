; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

@WDTCTL = external dso_local global i16, align 2
@PM5CTL0 = external dso_local global i16, align 2
@PADIR_L = external dso_local global i8, align 1
@PAOUT_L = external dso_local global i8, align 1

; Function Attrs: noinline nounwind
define dso_local i16 @main() #0 !dbg !6 {
entry:
  %i = alloca i16, align 2
  store volatile i16 23168, ptr @WDTCTL, align 2, !dbg !11
  %0 = load volatile i16, ptr @PM5CTL0, align 2, !dbg !12
  %and = and i16 %0, -2, !dbg !12
  store volatile i16 %and, ptr @PM5CTL0, align 2, !dbg !12
  %1 = load volatile i8, ptr @PADIR_L, align 1, !dbg !13
  %2 = or i8 %1, 1, !dbg !13
  store volatile i8 %2, ptr @PADIR_L, align 1, !dbg !13
  br label %for.cond, !dbg !14

for.cond:                                         ; preds = %do.end, %entry
    #dbg_declare(ptr %i, !15, !DIExpression(), !21)
  %3 = load volatile i8, ptr @PAOUT_L, align 1, !dbg !22
  %4 = xor i8 %3, 1, !dbg !22
  store volatile i8 %4, ptr @PAOUT_L, align 1, !dbg !22
  store volatile i16 -15536, ptr %i, align 2, !dbg !23
  br label %do.body, !dbg !24

do.body:                                          ; preds = %do.cond, %for.cond
  %5 = load volatile i16, ptr %i, align 2, !dbg !25
  %dec = add i16 %5, -1, !dbg !25
  store volatile i16 %dec, ptr %i, align 2, !dbg !25
  br label %do.cond, !dbg !26

do.cond:                                          ; preds = %do.body
  %6 = load volatile i16, ptr %i, align 2, !dbg !27
  %cmp.not = icmp eq i16 %6, 0, !dbg !28
  br i1 %cmp.not, label %do.end, label %do.body, !dbg !26, !llvm.loop !29

do.end:                                           ; preds = %do.cond
  br label %for.cond, !dbg !32, !llvm.loop !33
}

attributes #0 = { noinline nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" }

!llvm.dbg.cu = !{!0}
!llvm.ident = !{!2}
!llvm.module.flags = !{!3, !4, !5}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 22.0.0git (git@github.com:OMA-NVM/llvm-project.git fca9ff2383868608326c25ec508dffeb3c1b30e9)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "src/blink.c", directory: "/Users/nilsholscher/workspaces/msp430_template")
!2 = !{!"clang version 22.0.0git (git@github.com:OMA-NVM/llvm-project.git fca9ff2383868608326c25ec508dffeb3c1b30e9)"}
!3 = !{i32 7, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 2}
!6 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 3, type: !7, scopeLine: 3, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !10)
!7 = !DISubroutineType(types: !8)
!8 = !{!9}
!9 = !DIBasicType(name: "int", size: 16, encoding: DW_ATE_signed)
!10 = !{}
!11 = !DILocation(line: 4, column: 12, scope: !6)
!12 = !DILocation(line: 5, column: 13, scope: !6)
!13 = !DILocation(line: 7, column: 11, scope: !6)
!14 = !DILocation(line: 9, column: 5, scope: !6)
!15 = !DILocalVariable(name: "i", scope: !16, file: !1, line: 10, type: !19)
!16 = distinct !DILexicalBlock(scope: !17, file: !1, line: 9, column: 13)
!17 = distinct !DILexicalBlock(scope: !18, file: !1, line: 9, column: 5)
!18 = distinct !DILexicalBlock(scope: !6, file: !1, line: 9, column: 5)
!19 = !DIDerivedType(tag: DW_TAG_volatile_type, baseType: !20)
!20 = !DIBasicType(name: "unsigned int", size: 16, encoding: DW_ATE_unsigned)
!21 = !DILocation(line: 10, column: 31, scope: !16)
!22 = !DILocation(line: 12, column: 15, scope: !16)
!23 = !DILocation(line: 14, column: 11, scope: !16)
!24 = !DILocation(line: 15, column: 9, scope: !16)
!25 = !DILocation(line: 15, column: 13, scope: !16)
!26 = !DILocation(line: 15, column: 12, scope: !16)
!27 = !DILocation(line: 16, column: 15, scope: !16)
!28 = !DILocation(line: 16, column: 17, scope: !16)
!29 = distinct !{!29, !24, !30, !31}
!30 = !DILocation(line: 16, column: 21, scope: !16)
!31 = !{!"llvm.loop.mustprogress"}
!32 = !DILocation(line: 9, column: 5, scope: !17)
!33 = distinct !{!33, !34, !35}
!34 = !DILocation(line: 9, column: 5, scope: !18)
!35 = !DILocation(line: 17, column: 5, scope: !18)
