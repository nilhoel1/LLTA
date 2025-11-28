; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

@Array = dso_local global [10 x [10 x i16]] zeroinitializer, align 2, !dbg !0
@Seed = dso_local global i16 0, align 2, !dbg !5
@Postotal = dso_local global i16 0, align 2, !dbg !8
@Poscnt = dso_local global i16 0, align 2, !dbg !12
@Negtotal = dso_local global i16 0, align 2, !dbg !10
@Negcnt = dso_local global i16 0, align 2, !dbg !14

; Function Attrs: noinline nounwind
define dso_local i16 @main() #0 !dbg !24 {
entry:
  %call = call i16 @InitSeed(), !dbg !27
  %call1 = call i16 @Test(ptr noundef nonnull @Array), !dbg !28
  ret i16 1, !dbg !29
}

; Function Attrs: noinline nounwind
define dso_local i16 @InitSeed() #0 !dbg !30 {
entry:
  store i16 0, ptr @Seed, align 2, !dbg !31
  ret i16 0, !dbg !32
}

; Function Attrs: noinline nounwind
define dso_local i16 @Test(ptr noundef %Array) #0 !dbg !33 {
entry:
    #dbg_value(ptr %Array, !40, !DIExpression(), !41)
  %call = call i16 @Initialize(ptr noundef %Array), !dbg !42
    #dbg_value(i32 1000, !43, !DIExpression(), !41)
  call void @Sum(ptr noundef %Array), !dbg !45
    #dbg_value(i32 1500, !46, !DIExpression(), !41)
    #dbg_value(float poison, !47, !DIExpression(), !41)
  ret i16 0, !dbg !49
}

; Function Attrs: noinline nounwind
define dso_local i16 @Initialize(ptr noundef %Array) #0 !dbg !50 {
entry:
    #dbg_value(ptr %Array, !51, !DIExpression(), !52)
    #dbg_value(i16 0, !53, !DIExpression(), !52)
  br label %for.body, !dbg !54

for.body:                                         ; preds = %for.inc5, %entry
  %OuterIndex.02 = phi i16 [ 0, %entry ], [ %inc6, %for.inc5 ]
    #dbg_value(i16 %OuterIndex.02, !53, !DIExpression(), !52)
    #dbg_value(i16 0, !56, !DIExpression(), !52)
  br label %for.body3, !dbg !57

for.body3:                                        ; preds = %for.inc, %for.body
  %InnerIndex.01 = phi i16 [ 0, %for.body ], [ %inc, %for.inc ]
    #dbg_value(i16 %InnerIndex.01, !56, !DIExpression(), !52)
  %call = call i16 @RandomInteger(), !dbg !60
  %arrayidx4 = getelementptr inbounds [10 x i16], ptr %Array, i16 %OuterIndex.02, i16 %InnerIndex.01, !dbg !62
  store i16 %call, ptr %arrayidx4, align 2, !dbg !63
  br label %for.inc, !dbg !62

for.inc:                                          ; preds = %for.body3
  %inc = add nuw nsw i16 %InnerIndex.01, 1, !dbg !64
    #dbg_value(i16 %inc, !56, !DIExpression(), !52)
  %exitcond = icmp ne i16 %inc, 10, !dbg !65
  br i1 %exitcond, label %for.body3, label %for.end, !dbg !57, !llvm.loop !66

for.end:                                          ; preds = %for.inc
  br label %for.inc5, !dbg !67

for.inc5:                                         ; preds = %for.end
  %inc6 = add nuw nsw i16 %OuterIndex.02, 1, !dbg !69
    #dbg_value(i16 %inc6, !53, !DIExpression(), !52)
  %exitcond3 = icmp ne i16 %inc6, 10, !dbg !70
  br i1 %exitcond3, label %for.body, label %for.end7, !dbg !54, !llvm.loop !71

for.end7:                                         ; preds = %for.inc5
  ret i16 0, !dbg !73
}

; Function Attrs: noinline nounwind
define dso_local void @Sum(ptr noundef %Array) #0 !dbg !74 {
entry:
    #dbg_value(ptr %Array, !77, !DIExpression(), !78)
    #dbg_value(i16 0, !79, !DIExpression(), !78)
    #dbg_value(i16 0, !80, !DIExpression(), !78)
    #dbg_value(i16 0, !81, !DIExpression(), !78)
    #dbg_value(i16 0, !82, !DIExpression(), !78)
    #dbg_value(i16 0, !83, !DIExpression(), !78)
  br label %for.body, !dbg !84

for.body:                                         ; preds = %for.inc13, %entry
  %Ncnt.010 = phi i16 [ 0, %entry ], [ %Ncnt.1.lcssa, %for.inc13 ]
  %Outer.09 = phi i16 [ 0, %entry ], [ %inc14, %for.inc13 ]
  %Ptotal.08 = phi i16 [ 0, %entry ], [ %Ptotal.1.lcssa, %for.inc13 ]
  %Ntotal.07 = phi i16 [ 0, %entry ], [ %Ntotal.1.lcssa, %for.inc13 ]
  %Pcnt.06 = phi i16 [ 0, %entry ], [ %Pcnt.1.lcssa, %for.inc13 ]
    #dbg_value(i16 %Ncnt.010, !82, !DIExpression(), !78)
    #dbg_value(i16 %Outer.09, !83, !DIExpression(), !78)
    #dbg_value(i16 %Ptotal.08, !79, !DIExpression(), !78)
    #dbg_value(i16 %Ntotal.07, !80, !DIExpression(), !78)
    #dbg_value(i16 %Pcnt.06, !81, !DIExpression(), !78)
    #dbg_value(i16 0, !86, !DIExpression(), !78)
    #dbg_value(i16 %Ncnt.010, !82, !DIExpression(), !78)
    #dbg_value(i16 %Ptotal.08, !79, !DIExpression(), !78)
    #dbg_value(i16 %Ntotal.07, !80, !DIExpression(), !78)
    #dbg_value(i16 %Pcnt.06, !81, !DIExpression(), !78)
  br label %for.body3, !dbg !87

for.body3:                                        ; preds = %for.inc, %for.body
  %Ncnt.15 = phi i16 [ %Ncnt.010, %for.body ], [ %Ncnt.2, %for.inc ]
  %Inner.04 = phi i16 [ 0, %for.body ], [ %inc12, %for.inc ]
  %Ptotal.13 = phi i16 [ %Ptotal.08, %for.body ], [ %Ptotal.2, %for.inc ]
  %Ntotal.12 = phi i16 [ %Ntotal.07, %for.body ], [ %Ntotal.2, %for.inc ]
  %Pcnt.11 = phi i16 [ %Pcnt.06, %for.body ], [ %Pcnt.2, %for.inc ]
    #dbg_value(i16 %Ncnt.15, !82, !DIExpression(), !78)
    #dbg_value(i16 %Inner.04, !86, !DIExpression(), !78)
    #dbg_value(i16 %Ptotal.13, !79, !DIExpression(), !78)
    #dbg_value(i16 %Ntotal.12, !80, !DIExpression(), !78)
    #dbg_value(i16 %Pcnt.11, !81, !DIExpression(), !78)
  %arrayidx4 = getelementptr inbounds [10 x i16], ptr %Array, i16 %Outer.09, i16 %Inner.04, !dbg !90
  %0 = load i16, ptr %arrayidx4, align 2, !dbg !90
  %cmp5 = icmp slt i16 %0, 0, !dbg !93
  br i1 %cmp5, label %if.then, label %if.else, !dbg !93

if.then:                                          ; preds = %for.body3
  %arrayidx7 = getelementptr inbounds [10 x i16], ptr %Array, i16 %Outer.09, i16 %Inner.04, !dbg !94
  %1 = load i16, ptr %arrayidx7, align 2, !dbg !94
  %add = add nsw i16 %Ptotal.13, %1, !dbg !96
    #dbg_value(i16 %add, !79, !DIExpression(), !78)
  %inc = add nsw i16 %Pcnt.11, 1, !dbg !97
    #dbg_value(i16 %inc, !81, !DIExpression(), !78)
  br label %if.end, !dbg !98

if.else:                                          ; preds = %for.body3
  %arrayidx9 = getelementptr inbounds [10 x i16], ptr %Array, i16 %Outer.09, i16 %Inner.04, !dbg !99
  %2 = load i16, ptr %arrayidx9, align 2, !dbg !99
  %add10 = add nsw i16 %Ntotal.12, %2, !dbg !101
    #dbg_value(i16 %add10, !80, !DIExpression(), !78)
  %inc11 = add nsw i16 %Ncnt.15, 1, !dbg !102
    #dbg_value(i16 %inc11, !82, !DIExpression(), !78)
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %Pcnt.2 = phi i16 [ %inc, %if.then ], [ %Pcnt.11, %if.else ], !dbg !78
  %Ntotal.2 = phi i16 [ %Ntotal.12, %if.then ], [ %add10, %if.else ], !dbg !78
  %Ptotal.2 = phi i16 [ %add, %if.then ], [ %Ptotal.13, %if.else ], !dbg !78
  %Ncnt.2 = phi i16 [ %Ncnt.15, %if.then ], [ %inc11, %if.else ], !dbg !78
    #dbg_value(i16 %Ncnt.2, !82, !DIExpression(), !78)
    #dbg_value(i16 %Ptotal.2, !79, !DIExpression(), !78)
    #dbg_value(i16 %Ntotal.2, !80, !DIExpression(), !78)
    #dbg_value(i16 %Pcnt.2, !81, !DIExpression(), !78)
  br label %for.inc, !dbg !103

for.inc:                                          ; preds = %if.end
  %inc12 = add nuw nsw i16 %Inner.04, 1, !dbg !104
    #dbg_value(i16 %Ncnt.2, !82, !DIExpression(), !78)
    #dbg_value(i16 %inc12, !86, !DIExpression(), !78)
    #dbg_value(i16 %Ptotal.2, !79, !DIExpression(), !78)
    #dbg_value(i16 %Ntotal.2, !80, !DIExpression(), !78)
    #dbg_value(i16 %Pcnt.2, !81, !DIExpression(), !78)
  %exitcond = icmp ne i16 %inc12, 10, !dbg !105
  br i1 %exitcond, label %for.body3, label %for.end, !dbg !87, !llvm.loop !106

for.end:                                          ; preds = %for.inc
  %Pcnt.1.lcssa = phi i16 [ %Pcnt.2, %for.inc ], !dbg !78
  %Ntotal.1.lcssa = phi i16 [ %Ntotal.2, %for.inc ], !dbg !108
  %Ptotal.1.lcssa = phi i16 [ %Ptotal.2, %for.inc ], !dbg !78
  %Ncnt.1.lcssa = phi i16 [ %Ncnt.2, %for.inc ], !dbg !109
  br label %for.inc13, !dbg !107

for.inc13:                                        ; preds = %for.end
  %inc14 = add nuw nsw i16 %Outer.09, 1, !dbg !110
    #dbg_value(i16 %Ncnt.1.lcssa, !82, !DIExpression(), !78)
    #dbg_value(i16 %inc14, !83, !DIExpression(), !78)
    #dbg_value(i16 %Ptotal.1.lcssa, !79, !DIExpression(), !78)
    #dbg_value(i16 %Ntotal.1.lcssa, !80, !DIExpression(), !78)
    #dbg_value(i16 %Pcnt.1.lcssa, !81, !DIExpression(), !78)
  %exitcond11 = icmp ne i16 %inc14, 10, !dbg !111
  br i1 %exitcond11, label %for.body, label %for.end15, !dbg !84, !llvm.loop !112

for.end15:                                        ; preds = %for.inc13
  %Pcnt.0.lcssa = phi i16 [ %Pcnt.1.lcssa, %for.inc13 ], !dbg !114
  %Ntotal.0.lcssa = phi i16 [ %Ntotal.1.lcssa, %for.inc13 ], !dbg !108
  %Ptotal.0.lcssa = phi i16 [ %Ptotal.1.lcssa, %for.inc13 ], !dbg !115
  %Ncnt.0.lcssa = phi i16 [ %Ncnt.1.lcssa, %for.inc13 ], !dbg !109
  store i16 %Ptotal.0.lcssa, ptr @Postotal, align 2, !dbg !116
  store i16 %Pcnt.0.lcssa, ptr @Poscnt, align 2, !dbg !117
  store i16 %Ntotal.0.lcssa, ptr @Negtotal, align 2, !dbg !118
  store i16 %Ncnt.0.lcssa, ptr @Negcnt, align 2, !dbg !119
  ret void, !dbg !120
}

; Function Attrs: noinline nounwind
define dso_local i16 @RandomInteger() #0 !dbg !121 {
entry:
  %0 = load i16, ptr @Seed, align 2, !dbg !122
  %mul = mul nsw i16 %0, 133, !dbg !123
  %add = add nsw i16 %mul, 81, !dbg !124
  %rem = srem i16 %add, 8095, !dbg !125
  store i16 %rem, ptr @Seed, align 2, !dbg !126
  ret i16 %rem, !dbg !127
}

attributes #0 = { noinline nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" }

!llvm.dbg.cu = !{!2}
!llvm.ident = !{!20}
!llvm.module.flags = !{!21, !22, !23}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "Array", scope: !2, file: !3, line: 49, type: !16, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "clang version 22.0.0git (git@github.com:OMA-NVM/llvm-project.git fca9ff2383868608326c25ec508dffeb3c1b30e9)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !4, splitDebugInlining: false, nameTableKind: None)
!3 = !DIFile(filename: "src/main.c", directory: "/Users/nilsholscher/workspaces/msp430_template")
!4 = !{!5, !0, !8, !10, !12, !14}
!5 = !DIGlobalVariableExpression(var: !6, expr: !DIExpression())
!6 = distinct !DIGlobalVariable(name: "Seed", scope: !2, file: !3, line: 47, type: !7, isLocal: false, isDefinition: true)
!7 = !DIBasicType(name: "int", size: 16, encoding: DW_ATE_signed)
!8 = !DIGlobalVariableExpression(var: !9, expr: !DIExpression())
!9 = distinct !DIGlobalVariable(name: "Postotal", scope: !2, file: !3, line: 51, type: !7, isLocal: false, isDefinition: true)
!10 = !DIGlobalVariableExpression(var: !11, expr: !DIExpression())
!11 = distinct !DIGlobalVariable(name: "Negtotal", scope: !2, file: !3, line: 51, type: !7, isLocal: false, isDefinition: true)
!12 = !DIGlobalVariableExpression(var: !13, expr: !DIExpression())
!13 = distinct !DIGlobalVariable(name: "Poscnt", scope: !2, file: !3, line: 51, type: !7, isLocal: false, isDefinition: true)
!14 = !DIGlobalVariableExpression(var: !15, expr: !DIExpression())
!15 = distinct !DIGlobalVariable(name: "Negcnt", scope: !2, file: !3, line: 51, type: !7, isLocal: false, isDefinition: true)
!16 = !DIDerivedType(tag: DW_TAG_typedef, name: "matrix", file: !3, line: 25, baseType: !17)
!17 = !DICompositeType(tag: DW_TAG_array_type, baseType: !7, size: 1600, elements: !18)
!18 = !{!19, !19}
!19 = !DISubrange(count: 10)
!20 = !{!"clang version 22.0.0git (git@github.com:OMA-NVM/llvm-project.git fca9ff2383868608326c25ec508dffeb3c1b30e9)"}
!21 = !{i32 7, !"Dwarf Version", i32 4}
!22 = !{i32 2, !"Debug Info Version", i32 3}
!23 = !{i32 1, !"wchar_size", i32 2}
!24 = distinct !DISubprogram(name: "main", scope: !3, file: !3, line: 57, type: !25, scopeLine: 59, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2)
!25 = !DISubroutineType(types: !26)
!26 = !{!7}
!27 = !DILocation(line: 61, column: 4, scope: !24)
!28 = !DILocation(line: 67, column: 4, scope: !24)
!29 = !DILocation(line: 69, column: 4, scope: !24)
!30 = distinct !DISubprogram(name: "InitSeed", scope: !3, file: !3, line: 148, type: !25, scopeLine: 150, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2)
!31 = !DILocation(line: 152, column: 9, scope: !30)
!32 = !DILocation(line: 154, column: 4, scope: !30)
!33 = distinct !DISubprogram(name: "Test", scope: !3, file: !3, line: 77, type: !34, scopeLine: 79, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !39)
!34 = !DISubroutineType(types: !35)
!35 = !{!7, !36}
!36 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !37, size: 16)
!37 = !DICompositeType(tag: DW_TAG_array_type, baseType: !7, size: 160, elements: !38)
!38 = !{!19}
!39 = !{}
!40 = !DILocalVariable(name: "Array", arg: 1, scope: !33, file: !3, line: 77, type: !36)
!41 = !DILocation(line: 0, scope: !33)
!42 = !DILocation(line: 87, column: 4, scope: !33)
!43 = !DILocalVariable(name: "StartTime", scope: !33, file: !3, line: 81, type: !44)
!44 = !DIBasicType(name: "long", size: 32, encoding: DW_ATE_signed)
!45 = !DILocation(line: 91, column: 4, scope: !33)
!46 = !DILocalVariable(name: "StopTime", scope: !33, file: !3, line: 81, type: !44)
!47 = !DILocalVariable(name: "TotalTime", scope: !33, file: !3, line: 83, type: !48)
!48 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!49 = !DILocation(line: 111, column: 4, scope: !33)
!50 = distinct !DISubprogram(name: "Initialize", scope: !3, file: !3, line: 121, type: !34, scopeLine: 123, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !39)
!51 = !DILocalVariable(name: "Array", arg: 1, scope: !50, file: !3, line: 121, type: !36)
!52 = !DILocation(line: 0, scope: !50)
!53 = !DILocalVariable(name: "OuterIndex", scope: !50, file: !3, line: 125, type: !7)
!54 = !DILocation(line: 130, column: 4, scope: !55)
!55 = distinct !DILexicalBlock(scope: !50, file: !3, line: 130, column: 4)
!56 = !DILocalVariable(name: "InnerIndex", scope: !50, file: !3, line: 125, type: !7)
!57 = !DILocation(line: 132, column: 7, scope: !58)
!58 = distinct !DILexicalBlock(scope: !59, file: !3, line: 132, column: 7)
!59 = distinct !DILexicalBlock(scope: !55, file: !3, line: 130, column: 4)
!60 = !DILocation(line: 134, column: 42, scope: !61)
!61 = distinct !DILexicalBlock(scope: !58, file: !3, line: 132, column: 7)
!62 = !DILocation(line: 134, column: 10, scope: !61)
!63 = !DILocation(line: 134, column: 40, scope: !61)
!64 = !DILocation(line: 132, column: 60, scope: !61)
!65 = !DILocation(line: 132, column: 39, scope: !61)
!66 = distinct !{!66, !57, !67, !68}
!67 = !DILocation(line: 134, column: 56, scope: !58)
!68 = !{!"llvm.loop.mustprogress"}
!69 = !DILocation(line: 130, column: 57, scope: !59)
!70 = !DILocation(line: 130, column: 36, scope: !59)
!71 = distinct !{!71, !54, !72, !68}
!72 = !DILocation(line: 134, column: 56, scope: !55)
!73 = !DILocation(line: 138, column: 4, scope: !50)
!74 = distinct !DISubprogram(name: "Sum", scope: !3, file: !3, line: 160, type: !75, scopeLine: 162, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !39)
!75 = !DISubroutineType(types: !76)
!76 = !{null, !36}
!77 = !DILocalVariable(name: "Array", arg: 1, scope: !74, file: !3, line: 160, type: !36)
!78 = !DILocation(line: 0, scope: !74)
!79 = !DILocalVariable(name: "Ptotal", scope: !74, file: !3, line: 168, type: !7)
!80 = !DILocalVariable(name: "Ntotal", scope: !74, file: !3, line: 170, type: !7)
!81 = !DILocalVariable(name: "Pcnt", scope: !74, file: !3, line: 172, type: !7)
!82 = !DILocalVariable(name: "Ncnt", scope: !74, file: !3, line: 174, type: !7)
!83 = !DILocalVariable(name: "Outer", scope: !74, file: !3, line: 164, type: !7)
!84 = !DILocation(line: 179, column: 3, scope: !85)
!85 = distinct !DILexicalBlock(scope: !74, file: !3, line: 179, column: 3)
!86 = !DILocalVariable(name: "Inner", scope: !74, file: !3, line: 164, type: !7)
!87 = !DILocation(line: 181, column: 5, scope: !88)
!88 = distinct !DILexicalBlock(scope: !89, file: !3, line: 181, column: 5)
!89 = distinct !DILexicalBlock(scope: !85, file: !3, line: 179, column: 3)
!90 = !DILocation(line: 189, column: 6, scope: !91)
!91 = distinct !DILexicalBlock(scope: !92, file: !3, line: 189, column: 6)
!92 = distinct !DILexicalBlock(scope: !88, file: !3, line: 181, column: 5)
!93 = !DILocation(line: 189, column: 26, scope: !91)
!94 = !DILocation(line: 193, column: 14, scope: !95)
!95 = distinct !DILexicalBlock(scope: !91, file: !3, line: 189, column: 31)
!96 = !DILocation(line: 193, column: 11, scope: !95)
!97 = !DILocation(line: 195, column: 8, scope: !95)
!98 = !DILocation(line: 197, column: 2, scope: !95)
!99 = !DILocation(line: 201, column: 14, scope: !100)
!100 = distinct !DILexicalBlock(scope: !91, file: !3, line: 199, column: 7)
!101 = !DILocation(line: 201, column: 11, scope: !100)
!102 = !DILocation(line: 203, column: 8, scope: !100)
!103 = !DILocation(line: 189, column: 28, scope: !91)
!104 = !DILocation(line: 181, column: 43, scope: !92)
!105 = !DILocation(line: 181, column: 27, scope: !92)
!106 = distinct !{!106, !87, !107, !68}
!107 = !DILocation(line: 205, column: 2, scope: !88)
!108 = !DILocation(line: 170, column: 7, scope: !74)
!109 = !DILocation(line: 174, column: 7, scope: !74)
!110 = !DILocation(line: 179, column: 41, scope: !89)
!111 = !DILocation(line: 179, column: 25, scope: !89)
!112 = distinct !{!112, !84, !113, !68}
!113 = !DILocation(line: 205, column: 2, scope: !85)
!114 = !DILocation(line: 172, column: 7, scope: !74)
!115 = !DILocation(line: 168, column: 7, scope: !74)
!116 = !DILocation(line: 209, column: 12, scope: !74)
!117 = !DILocation(line: 211, column: 10, scope: !74)
!118 = !DILocation(line: 213, column: 12, scope: !74)
!119 = !DILocation(line: 215, column: 10, scope: !74)
!120 = !DILocation(line: 217, column: 1, scope: !74)
!121 = distinct !DISubprogram(name: "RandomInteger", scope: !3, file: !3, line: 249, type: !25, scopeLine: 251, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2)
!122 = !DILocation(line: 253, column: 13, scope: !121)
!123 = !DILocation(line: 253, column: 18, scope: !121)
!124 = !DILocation(line: 253, column: 25, scope: !121)
!125 = !DILocation(line: 253, column: 31, scope: !121)
!126 = !DILocation(line: 253, column: 9, scope: !121)
!127 = !DILocation(line: 255, column: 4, scope: !121)
