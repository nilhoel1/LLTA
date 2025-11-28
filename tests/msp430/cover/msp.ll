; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

; Function Attrs: noinline nounwind
define dso_local i16 @swi120(i16 noundef %c) #0 !dbg !6 {
entry:
    #dbg_value(i16 %c, !11, !DIExpression(), !12)
    #dbg_value(i16 0, !13, !DIExpression(), !12)
  br label %for.body, !dbg !14

for.body:                                         ; preds = %for.inc, %entry
  %c.addr.02 = phi i16 [ %c, %entry ], [ %c.addr.1, %for.inc ]
  %i.01 = phi i16 [ 0, %entry ], [ %inc239, %for.inc ]
    #dbg_value(i16 %c.addr.02, !11, !DIExpression(), !12)
    #dbg_value(i16 %i.01, !13, !DIExpression(), !12)
  switch i16 %i.01, label %sw.default [
    i16 0, label %sw.bb
    i16 1, label %sw.bb1
    i16 2, label %sw.bb3
    i16 3, label %sw.bb5
    i16 4, label %sw.bb7
    i16 5, label %sw.bb9
    i16 6, label %sw.bb11
    i16 7, label %sw.bb13
    i16 8, label %sw.bb15
    i16 9, label %sw.bb17
    i16 10, label %sw.bb19
    i16 11, label %sw.bb21
    i16 12, label %sw.bb23
    i16 13, label %sw.bb25
    i16 14, label %sw.bb27
    i16 15, label %sw.bb29
    i16 16, label %sw.bb31
    i16 17, label %sw.bb33
    i16 18, label %sw.bb35
    i16 19, label %sw.bb37
    i16 20, label %sw.bb39
    i16 21, label %sw.bb41
    i16 22, label %sw.bb43
    i16 23, label %sw.bb45
    i16 24, label %sw.bb47
    i16 25, label %sw.bb49
    i16 26, label %sw.bb51
    i16 27, label %sw.bb53
    i16 28, label %sw.bb55
    i16 29, label %sw.bb57
    i16 30, label %sw.bb59
    i16 31, label %sw.bb61
    i16 32, label %sw.bb63
    i16 33, label %sw.bb65
    i16 34, label %sw.bb67
    i16 35, label %sw.bb69
    i16 36, label %sw.bb71
    i16 37, label %sw.bb73
    i16 38, label %sw.bb75
    i16 39, label %sw.bb77
    i16 40, label %sw.bb79
    i16 41, label %sw.bb81
    i16 42, label %sw.bb83
    i16 43, label %sw.bb85
    i16 44, label %sw.bb87
    i16 45, label %sw.bb89
    i16 46, label %sw.bb91
    i16 47, label %sw.bb93
    i16 48, label %sw.bb95
    i16 49, label %sw.bb97
    i16 50, label %sw.bb99
    i16 51, label %sw.bb101
    i16 52, label %sw.bb103
    i16 53, label %sw.bb105
    i16 54, label %sw.bb107
    i16 55, label %sw.bb109
    i16 56, label %sw.bb111
    i16 57, label %sw.bb113
    i16 58, label %sw.bb115
    i16 59, label %sw.bb117
    i16 60, label %sw.bb119
    i16 61, label %sw.bb121
    i16 62, label %sw.bb123
    i16 63, label %sw.bb125
    i16 64, label %sw.bb127
    i16 65, label %sw.bb129
    i16 66, label %sw.bb131
    i16 67, label %sw.bb133
    i16 68, label %sw.bb135
    i16 69, label %sw.bb137
    i16 70, label %sw.bb139
    i16 71, label %sw.bb141
    i16 72, label %sw.bb143
    i16 73, label %sw.bb145
    i16 74, label %sw.bb147
    i16 75, label %sw.bb149
    i16 76, label %sw.bb151
    i16 77, label %sw.bb153
    i16 78, label %sw.bb155
    i16 79, label %sw.bb157
    i16 80, label %sw.bb159
    i16 81, label %sw.bb161
    i16 82, label %sw.bb163
    i16 83, label %sw.bb165
    i16 84, label %sw.bb167
    i16 85, label %sw.bb169
    i16 86, label %sw.bb171
    i16 87, label %sw.bb173
    i16 88, label %sw.bb175
    i16 89, label %sw.bb177
    i16 90, label %sw.bb179
    i16 91, label %sw.bb181
    i16 92, label %sw.bb183
    i16 93, label %sw.bb185
    i16 94, label %sw.bb187
    i16 95, label %sw.bb189
    i16 96, label %sw.bb191
    i16 97, label %sw.bb193
    i16 98, label %sw.bb195
    i16 99, label %sw.bb197
    i16 100, label %sw.bb199
    i16 101, label %sw.bb201
    i16 102, label %sw.bb203
    i16 103, label %sw.bb205
    i16 104, label %sw.bb207
    i16 105, label %sw.bb209
    i16 106, label %sw.bb211
    i16 107, label %sw.bb213
    i16 108, label %sw.bb215
    i16 109, label %sw.bb217
    i16 110, label %sw.bb219
    i16 111, label %sw.bb221
    i16 112, label %sw.bb223
    i16 113, label %sw.bb225
    i16 114, label %sw.bb227
    i16 115, label %sw.bb229
    i16 116, label %sw.bb231
    i16 117, label %sw.bb233
    i16 118, label %sw.bb235
    i16 119, label %sw.bb237
  ], !dbg !16

sw.bb:                                            ; preds = %for.body
  %inc = add nsw i16 %c.addr.02, 1, !dbg !19
    #dbg_value(i16 %inc, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !21

sw.bb1:                                           ; preds = %for.body
  %inc2 = add nsw i16 %c.addr.02, 1, !dbg !22
    #dbg_value(i16 %inc2, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !23

sw.bb3:                                           ; preds = %for.body
  %inc4 = add nsw i16 %c.addr.02, 1, !dbg !24
    #dbg_value(i16 %inc4, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !25

sw.bb5:                                           ; preds = %for.body
  %inc6 = add nsw i16 %c.addr.02, 1, !dbg !26
    #dbg_value(i16 %inc6, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !27

sw.bb7:                                           ; preds = %for.body
  %inc8 = add nsw i16 %c.addr.02, 1, !dbg !28
    #dbg_value(i16 %inc8, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !29

sw.bb9:                                           ; preds = %for.body
  %inc10 = add nsw i16 %c.addr.02, 1, !dbg !30
    #dbg_value(i16 %inc10, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !31

sw.bb11:                                          ; preds = %for.body
  %inc12 = add nsw i16 %c.addr.02, 1, !dbg !32
    #dbg_value(i16 %inc12, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !33

sw.bb13:                                          ; preds = %for.body
  %inc14 = add nsw i16 %c.addr.02, 1, !dbg !34
    #dbg_value(i16 %inc14, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !35

sw.bb15:                                          ; preds = %for.body
  %inc16 = add nsw i16 %c.addr.02, 1, !dbg !36
    #dbg_value(i16 %inc16, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !37

sw.bb17:                                          ; preds = %for.body
  %inc18 = add nsw i16 %c.addr.02, 1, !dbg !38
    #dbg_value(i16 %inc18, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !39

sw.bb19:                                          ; preds = %for.body
  %inc20 = add nsw i16 %c.addr.02, 1, !dbg !40
    #dbg_value(i16 %inc20, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !41

sw.bb21:                                          ; preds = %for.body
  %inc22 = add nsw i16 %c.addr.02, 1, !dbg !42
    #dbg_value(i16 %inc22, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !43

sw.bb23:                                          ; preds = %for.body
  %inc24 = add nsw i16 %c.addr.02, 1, !dbg !44
    #dbg_value(i16 %inc24, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !45

sw.bb25:                                          ; preds = %for.body
  %inc26 = add nsw i16 %c.addr.02, 1, !dbg !46
    #dbg_value(i16 %inc26, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !47

sw.bb27:                                          ; preds = %for.body
  %inc28 = add nsw i16 %c.addr.02, 1, !dbg !48
    #dbg_value(i16 %inc28, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !49

sw.bb29:                                          ; preds = %for.body
  %inc30 = add nsw i16 %c.addr.02, 1, !dbg !50
    #dbg_value(i16 %inc30, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !51

sw.bb31:                                          ; preds = %for.body
  %inc32 = add nsw i16 %c.addr.02, 1, !dbg !52
    #dbg_value(i16 %inc32, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !53

sw.bb33:                                          ; preds = %for.body
  %inc34 = add nsw i16 %c.addr.02, 1, !dbg !54
    #dbg_value(i16 %inc34, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !55

sw.bb35:                                          ; preds = %for.body
  %inc36 = add nsw i16 %c.addr.02, 1, !dbg !56
    #dbg_value(i16 %inc36, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !57

sw.bb37:                                          ; preds = %for.body
  %inc38 = add nsw i16 %c.addr.02, 1, !dbg !58
    #dbg_value(i16 %inc38, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !59

sw.bb39:                                          ; preds = %for.body
  %inc40 = add nsw i16 %c.addr.02, 1, !dbg !60
    #dbg_value(i16 %inc40, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !61

sw.bb41:                                          ; preds = %for.body
  %inc42 = add nsw i16 %c.addr.02, 1, !dbg !62
    #dbg_value(i16 %inc42, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !63

sw.bb43:                                          ; preds = %for.body
  %inc44 = add nsw i16 %c.addr.02, 1, !dbg !64
    #dbg_value(i16 %inc44, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !65

sw.bb45:                                          ; preds = %for.body
  %inc46 = add nsw i16 %c.addr.02, 1, !dbg !66
    #dbg_value(i16 %inc46, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !67

sw.bb47:                                          ; preds = %for.body
  %inc48 = add nsw i16 %c.addr.02, 1, !dbg !68
    #dbg_value(i16 %inc48, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !69

sw.bb49:                                          ; preds = %for.body
  %inc50 = add nsw i16 %c.addr.02, 1, !dbg !70
    #dbg_value(i16 %inc50, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !71

sw.bb51:                                          ; preds = %for.body
  %inc52 = add nsw i16 %c.addr.02, 1, !dbg !72
    #dbg_value(i16 %inc52, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !73

sw.bb53:                                          ; preds = %for.body
  %inc54 = add nsw i16 %c.addr.02, 1, !dbg !74
    #dbg_value(i16 %inc54, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !75

sw.bb55:                                          ; preds = %for.body
  %inc56 = add nsw i16 %c.addr.02, 1, !dbg !76
    #dbg_value(i16 %inc56, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !77

sw.bb57:                                          ; preds = %for.body
  %inc58 = add nsw i16 %c.addr.02, 1, !dbg !78
    #dbg_value(i16 %inc58, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !79

sw.bb59:                                          ; preds = %for.body
  %inc60 = add nsw i16 %c.addr.02, 1, !dbg !80
    #dbg_value(i16 %inc60, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !81

sw.bb61:                                          ; preds = %for.body
  %inc62 = add nsw i16 %c.addr.02, 1, !dbg !82
    #dbg_value(i16 %inc62, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !83

sw.bb63:                                          ; preds = %for.body
  %inc64 = add nsw i16 %c.addr.02, 1, !dbg !84
    #dbg_value(i16 %inc64, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !85

sw.bb65:                                          ; preds = %for.body
  %inc66 = add nsw i16 %c.addr.02, 1, !dbg !86
    #dbg_value(i16 %inc66, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !87

sw.bb67:                                          ; preds = %for.body
  %inc68 = add nsw i16 %c.addr.02, 1, !dbg !88
    #dbg_value(i16 %inc68, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !89

sw.bb69:                                          ; preds = %for.body
  %inc70 = add nsw i16 %c.addr.02, 1, !dbg !90
    #dbg_value(i16 %inc70, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !91

sw.bb71:                                          ; preds = %for.body
  %inc72 = add nsw i16 %c.addr.02, 1, !dbg !92
    #dbg_value(i16 %inc72, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !93

sw.bb73:                                          ; preds = %for.body
  %inc74 = add nsw i16 %c.addr.02, 1, !dbg !94
    #dbg_value(i16 %inc74, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !95

sw.bb75:                                          ; preds = %for.body
  %inc76 = add nsw i16 %c.addr.02, 1, !dbg !96
    #dbg_value(i16 %inc76, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !97

sw.bb77:                                          ; preds = %for.body
  %inc78 = add nsw i16 %c.addr.02, 1, !dbg !98
    #dbg_value(i16 %inc78, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !99

sw.bb79:                                          ; preds = %for.body
  %inc80 = add nsw i16 %c.addr.02, 1, !dbg !100
    #dbg_value(i16 %inc80, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !101

sw.bb81:                                          ; preds = %for.body
  %inc82 = add nsw i16 %c.addr.02, 1, !dbg !102
    #dbg_value(i16 %inc82, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !103

sw.bb83:                                          ; preds = %for.body
  %inc84 = add nsw i16 %c.addr.02, 1, !dbg !104
    #dbg_value(i16 %inc84, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !105

sw.bb85:                                          ; preds = %for.body
  %inc86 = add nsw i16 %c.addr.02, 1, !dbg !106
    #dbg_value(i16 %inc86, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !107

sw.bb87:                                          ; preds = %for.body
  %inc88 = add nsw i16 %c.addr.02, 1, !dbg !108
    #dbg_value(i16 %inc88, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !109

sw.bb89:                                          ; preds = %for.body
  %inc90 = add nsw i16 %c.addr.02, 1, !dbg !110
    #dbg_value(i16 %inc90, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !111

sw.bb91:                                          ; preds = %for.body
  %inc92 = add nsw i16 %c.addr.02, 1, !dbg !112
    #dbg_value(i16 %inc92, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !113

sw.bb93:                                          ; preds = %for.body
  %inc94 = add nsw i16 %c.addr.02, 1, !dbg !114
    #dbg_value(i16 %inc94, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !115

sw.bb95:                                          ; preds = %for.body
  %inc96 = add nsw i16 %c.addr.02, 1, !dbg !116
    #dbg_value(i16 %inc96, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !117

sw.bb97:                                          ; preds = %for.body
  %inc98 = add nsw i16 %c.addr.02, 1, !dbg !118
    #dbg_value(i16 %inc98, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !119

sw.bb99:                                          ; preds = %for.body
  %inc100 = add nsw i16 %c.addr.02, 1, !dbg !120
    #dbg_value(i16 %inc100, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !121

sw.bb101:                                         ; preds = %for.body
  %inc102 = add nsw i16 %c.addr.02, 1, !dbg !122
    #dbg_value(i16 %inc102, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !123

sw.bb103:                                         ; preds = %for.body
  %inc104 = add nsw i16 %c.addr.02, 1, !dbg !124
    #dbg_value(i16 %inc104, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !125

sw.bb105:                                         ; preds = %for.body
  %inc106 = add nsw i16 %c.addr.02, 1, !dbg !126
    #dbg_value(i16 %inc106, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !127

sw.bb107:                                         ; preds = %for.body
  %inc108 = add nsw i16 %c.addr.02, 1, !dbg !128
    #dbg_value(i16 %inc108, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !129

sw.bb109:                                         ; preds = %for.body
  %inc110 = add nsw i16 %c.addr.02, 1, !dbg !130
    #dbg_value(i16 %inc110, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !131

sw.bb111:                                         ; preds = %for.body
  %inc112 = add nsw i16 %c.addr.02, 1, !dbg !132
    #dbg_value(i16 %inc112, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !133

sw.bb113:                                         ; preds = %for.body
  %inc114 = add nsw i16 %c.addr.02, 1, !dbg !134
    #dbg_value(i16 %inc114, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !135

sw.bb115:                                         ; preds = %for.body
  %inc116 = add nsw i16 %c.addr.02, 1, !dbg !136
    #dbg_value(i16 %inc116, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !137

sw.bb117:                                         ; preds = %for.body
  %inc118 = add nsw i16 %c.addr.02, 1, !dbg !138
    #dbg_value(i16 %inc118, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !139

sw.bb119:                                         ; preds = %for.body
  %inc120 = add nsw i16 %c.addr.02, 1, !dbg !140
    #dbg_value(i16 %inc120, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !141

sw.bb121:                                         ; preds = %for.body
  %inc122 = add nsw i16 %c.addr.02, 1, !dbg !142
    #dbg_value(i16 %inc122, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !143

sw.bb123:                                         ; preds = %for.body
  %inc124 = add nsw i16 %c.addr.02, 1, !dbg !144
    #dbg_value(i16 %inc124, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !145

sw.bb125:                                         ; preds = %for.body
  %inc126 = add nsw i16 %c.addr.02, 1, !dbg !146
    #dbg_value(i16 %inc126, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !147

sw.bb127:                                         ; preds = %for.body
  %inc128 = add nsw i16 %c.addr.02, 1, !dbg !148
    #dbg_value(i16 %inc128, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !149

sw.bb129:                                         ; preds = %for.body
  %inc130 = add nsw i16 %c.addr.02, 1, !dbg !150
    #dbg_value(i16 %inc130, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !151

sw.bb131:                                         ; preds = %for.body
  %inc132 = add nsw i16 %c.addr.02, 1, !dbg !152
    #dbg_value(i16 %inc132, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !153

sw.bb133:                                         ; preds = %for.body
  %inc134 = add nsw i16 %c.addr.02, 1, !dbg !154
    #dbg_value(i16 %inc134, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !155

sw.bb135:                                         ; preds = %for.body
  %inc136 = add nsw i16 %c.addr.02, 1, !dbg !156
    #dbg_value(i16 %inc136, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !157

sw.bb137:                                         ; preds = %for.body
  %inc138 = add nsw i16 %c.addr.02, 1, !dbg !158
    #dbg_value(i16 %inc138, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !159

sw.bb139:                                         ; preds = %for.body
  %inc140 = add nsw i16 %c.addr.02, 1, !dbg !160
    #dbg_value(i16 %inc140, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !161

sw.bb141:                                         ; preds = %for.body
  %inc142 = add nsw i16 %c.addr.02, 1, !dbg !162
    #dbg_value(i16 %inc142, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !163

sw.bb143:                                         ; preds = %for.body
  %inc144 = add nsw i16 %c.addr.02, 1, !dbg !164
    #dbg_value(i16 %inc144, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !165

sw.bb145:                                         ; preds = %for.body
  %inc146 = add nsw i16 %c.addr.02, 1, !dbg !166
    #dbg_value(i16 %inc146, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !167

sw.bb147:                                         ; preds = %for.body
  %inc148 = add nsw i16 %c.addr.02, 1, !dbg !168
    #dbg_value(i16 %inc148, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !169

sw.bb149:                                         ; preds = %for.body
  %inc150 = add nsw i16 %c.addr.02, 1, !dbg !170
    #dbg_value(i16 %inc150, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !171

sw.bb151:                                         ; preds = %for.body
  %inc152 = add nsw i16 %c.addr.02, 1, !dbg !172
    #dbg_value(i16 %inc152, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !173

sw.bb153:                                         ; preds = %for.body
  %inc154 = add nsw i16 %c.addr.02, 1, !dbg !174
    #dbg_value(i16 %inc154, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !175

sw.bb155:                                         ; preds = %for.body
  %inc156 = add nsw i16 %c.addr.02, 1, !dbg !176
    #dbg_value(i16 %inc156, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !177

sw.bb157:                                         ; preds = %for.body
  %inc158 = add nsw i16 %c.addr.02, 1, !dbg !178
    #dbg_value(i16 %inc158, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !179

sw.bb159:                                         ; preds = %for.body
  %inc160 = add nsw i16 %c.addr.02, 1, !dbg !180
    #dbg_value(i16 %inc160, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !181

sw.bb161:                                         ; preds = %for.body
  %inc162 = add nsw i16 %c.addr.02, 1, !dbg !182
    #dbg_value(i16 %inc162, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !183

sw.bb163:                                         ; preds = %for.body
  %inc164 = add nsw i16 %c.addr.02, 1, !dbg !184
    #dbg_value(i16 %inc164, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !185

sw.bb165:                                         ; preds = %for.body
  %inc166 = add nsw i16 %c.addr.02, 1, !dbg !186
    #dbg_value(i16 %inc166, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !187

sw.bb167:                                         ; preds = %for.body
  %inc168 = add nsw i16 %c.addr.02, 1, !dbg !188
    #dbg_value(i16 %inc168, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !189

sw.bb169:                                         ; preds = %for.body
  %inc170 = add nsw i16 %c.addr.02, 1, !dbg !190
    #dbg_value(i16 %inc170, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !191

sw.bb171:                                         ; preds = %for.body
  %inc172 = add nsw i16 %c.addr.02, 1, !dbg !192
    #dbg_value(i16 %inc172, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !193

sw.bb173:                                         ; preds = %for.body
  %inc174 = add nsw i16 %c.addr.02, 1, !dbg !194
    #dbg_value(i16 %inc174, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !195

sw.bb175:                                         ; preds = %for.body
  %inc176 = add nsw i16 %c.addr.02, 1, !dbg !196
    #dbg_value(i16 %inc176, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !197

sw.bb177:                                         ; preds = %for.body
  %inc178 = add nsw i16 %c.addr.02, 1, !dbg !198
    #dbg_value(i16 %inc178, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !199

sw.bb179:                                         ; preds = %for.body
  %inc180 = add nsw i16 %c.addr.02, 1, !dbg !200
    #dbg_value(i16 %inc180, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !201

sw.bb181:                                         ; preds = %for.body
  %inc182 = add nsw i16 %c.addr.02, 1, !dbg !202
    #dbg_value(i16 %inc182, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !203

sw.bb183:                                         ; preds = %for.body
  %inc184 = add nsw i16 %c.addr.02, 1, !dbg !204
    #dbg_value(i16 %inc184, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !205

sw.bb185:                                         ; preds = %for.body
  %inc186 = add nsw i16 %c.addr.02, 1, !dbg !206
    #dbg_value(i16 %inc186, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !207

sw.bb187:                                         ; preds = %for.body
  %inc188 = add nsw i16 %c.addr.02, 1, !dbg !208
    #dbg_value(i16 %inc188, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !209

sw.bb189:                                         ; preds = %for.body
  %inc190 = add nsw i16 %c.addr.02, 1, !dbg !210
    #dbg_value(i16 %inc190, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !211

sw.bb191:                                         ; preds = %for.body
  %inc192 = add nsw i16 %c.addr.02, 1, !dbg !212
    #dbg_value(i16 %inc192, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !213

sw.bb193:                                         ; preds = %for.body
  %inc194 = add nsw i16 %c.addr.02, 1, !dbg !214
    #dbg_value(i16 %inc194, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !215

sw.bb195:                                         ; preds = %for.body
  %inc196 = add nsw i16 %c.addr.02, 1, !dbg !216
    #dbg_value(i16 %inc196, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !217

sw.bb197:                                         ; preds = %for.body
  %inc198 = add nsw i16 %c.addr.02, 1, !dbg !218
    #dbg_value(i16 %inc198, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !219

sw.bb199:                                         ; preds = %for.body
  %inc200 = add nsw i16 %c.addr.02, 1, !dbg !220
    #dbg_value(i16 %inc200, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !221

sw.bb201:                                         ; preds = %for.body
  %inc202 = add nsw i16 %c.addr.02, 1, !dbg !222
    #dbg_value(i16 %inc202, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !223

sw.bb203:                                         ; preds = %for.body
  %inc204 = add nsw i16 %c.addr.02, 1, !dbg !224
    #dbg_value(i16 %inc204, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !225

sw.bb205:                                         ; preds = %for.body
  %inc206 = add nsw i16 %c.addr.02, 1, !dbg !226
    #dbg_value(i16 %inc206, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !227

sw.bb207:                                         ; preds = %for.body
  %inc208 = add nsw i16 %c.addr.02, 1, !dbg !228
    #dbg_value(i16 %inc208, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !229

sw.bb209:                                         ; preds = %for.body
  %inc210 = add nsw i16 %c.addr.02, 1, !dbg !230
    #dbg_value(i16 %inc210, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !231

sw.bb211:                                         ; preds = %for.body
  %inc212 = add nsw i16 %c.addr.02, 1, !dbg !232
    #dbg_value(i16 %inc212, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !233

sw.bb213:                                         ; preds = %for.body
  %inc214 = add nsw i16 %c.addr.02, 1, !dbg !234
    #dbg_value(i16 %inc214, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !235

sw.bb215:                                         ; preds = %for.body
  %inc216 = add nsw i16 %c.addr.02, 1, !dbg !236
    #dbg_value(i16 %inc216, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !237

sw.bb217:                                         ; preds = %for.body
  %inc218 = add nsw i16 %c.addr.02, 1, !dbg !238
    #dbg_value(i16 %inc218, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !239

sw.bb219:                                         ; preds = %for.body
  %inc220 = add nsw i16 %c.addr.02, 1, !dbg !240
    #dbg_value(i16 %inc220, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !241

sw.bb221:                                         ; preds = %for.body
  %inc222 = add nsw i16 %c.addr.02, 1, !dbg !242
    #dbg_value(i16 %inc222, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !243

sw.bb223:                                         ; preds = %for.body
  %inc224 = add nsw i16 %c.addr.02, 1, !dbg !244
    #dbg_value(i16 %inc224, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !245

sw.bb225:                                         ; preds = %for.body
  %inc226 = add nsw i16 %c.addr.02, 1, !dbg !246
    #dbg_value(i16 %inc226, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !247

sw.bb227:                                         ; preds = %for.body
  %inc228 = add nsw i16 %c.addr.02, 1, !dbg !248
    #dbg_value(i16 %inc228, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !249

sw.bb229:                                         ; preds = %for.body
  %inc230 = add nsw i16 %c.addr.02, 1, !dbg !250
    #dbg_value(i16 %inc230, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !251

sw.bb231:                                         ; preds = %for.body
  %inc232 = add nsw i16 %c.addr.02, 1, !dbg !252
    #dbg_value(i16 %inc232, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !253

sw.bb233:                                         ; preds = %for.body
  %inc234 = add nsw i16 %c.addr.02, 1, !dbg !254
    #dbg_value(i16 %inc234, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !255

sw.bb235:                                         ; preds = %for.body
  %inc236 = add nsw i16 %c.addr.02, 1, !dbg !256
    #dbg_value(i16 %inc236, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !257

sw.bb237:                                         ; preds = %for.body
  %inc238 = add nsw i16 %c.addr.02, 1, !dbg !258
    #dbg_value(i16 %inc238, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !259

sw.default:                                       ; preds = %for.body
  %dec = add nsw i16 %c.addr.02, -1, !dbg !260
    #dbg_value(i16 %dec, !11, !DIExpression(), !12)
  br label %sw.epilog, !dbg !261

sw.epilog:                                        ; preds = %sw.default, %sw.bb237, %sw.bb235, %sw.bb233, %sw.bb231, %sw.bb229, %sw.bb227, %sw.bb225, %sw.bb223, %sw.bb221, %sw.bb219, %sw.bb217, %sw.bb215, %sw.bb213, %sw.bb211, %sw.bb209, %sw.bb207, %sw.bb205, %sw.bb203, %sw.bb201, %sw.bb199, %sw.bb197, %sw.bb195, %sw.bb193, %sw.bb191, %sw.bb189, %sw.bb187, %sw.bb185, %sw.bb183, %sw.bb181, %sw.bb179, %sw.bb177, %sw.bb175, %sw.bb173, %sw.bb171, %sw.bb169, %sw.bb167, %sw.bb165, %sw.bb163, %sw.bb161, %sw.bb159, %sw.bb157, %sw.bb155, %sw.bb153, %sw.bb151, %sw.bb149, %sw.bb147, %sw.bb145, %sw.bb143, %sw.bb141, %sw.bb139, %sw.bb137, %sw.bb135, %sw.bb133, %sw.bb131, %sw.bb129, %sw.bb127, %sw.bb125, %sw.bb123, %sw.bb121, %sw.bb119, %sw.bb117, %sw.bb115, %sw.bb113, %sw.bb111, %sw.bb109, %sw.bb107, %sw.bb105, %sw.bb103, %sw.bb101, %sw.bb99, %sw.bb97, %sw.bb95, %sw.bb93, %sw.bb91, %sw.bb89, %sw.bb87, %sw.bb85, %sw.bb83, %sw.bb81, %sw.bb79, %sw.bb77, %sw.bb75, %sw.bb73, %sw.bb71, %sw.bb69, %sw.bb67, %sw.bb65, %sw.bb63, %sw.bb61, %sw.bb59, %sw.bb57, %sw.bb55, %sw.bb53, %sw.bb51, %sw.bb49, %sw.bb47, %sw.bb45, %sw.bb43, %sw.bb41, %sw.bb39, %sw.bb37, %sw.bb35, %sw.bb33, %sw.bb31, %sw.bb29, %sw.bb27, %sw.bb25, %sw.bb23, %sw.bb21, %sw.bb19, %sw.bb17, %sw.bb15, %sw.bb13, %sw.bb11, %sw.bb9, %sw.bb7, %sw.bb5, %sw.bb3, %sw.bb1, %sw.bb
  %c.addr.1 = phi i16 [ %dec, %sw.default ], [ %inc238, %sw.bb237 ], [ %inc236, %sw.bb235 ], [ %inc234, %sw.bb233 ], [ %inc232, %sw.bb231 ], [ %inc230, %sw.bb229 ], [ %inc228, %sw.bb227 ], [ %inc226, %sw.bb225 ], [ %inc224, %sw.bb223 ], [ %inc222, %sw.bb221 ], [ %inc220, %sw.bb219 ], [ %inc218, %sw.bb217 ], [ %inc216, %sw.bb215 ], [ %inc214, %sw.bb213 ], [ %inc212, %sw.bb211 ], [ %inc210, %sw.bb209 ], [ %inc208, %sw.bb207 ], [ %inc206, %sw.bb205 ], [ %inc204, %sw.bb203 ], [ %inc202, %sw.bb201 ], [ %inc200, %sw.bb199 ], [ %inc198, %sw.bb197 ], [ %inc196, %sw.bb195 ], [ %inc194, %sw.bb193 ], [ %inc192, %sw.bb191 ], [ %inc190, %sw.bb189 ], [ %inc188, %sw.bb187 ], [ %inc186, %sw.bb185 ], [ %inc184, %sw.bb183 ], [ %inc182, %sw.bb181 ], [ %inc180, %sw.bb179 ], [ %inc178, %sw.bb177 ], [ %inc176, %sw.bb175 ], [ %inc174, %sw.bb173 ], [ %inc172, %sw.bb171 ], [ %inc170, %sw.bb169 ], [ %inc168, %sw.bb167 ], [ %inc166, %sw.bb165 ], [ %inc164, %sw.bb163 ], [ %inc162, %sw.bb161 ], [ %inc160, %sw.bb159 ], [ %inc158, %sw.bb157 ], [ %inc156, %sw.bb155 ], [ %inc154, %sw.bb153 ], [ %inc152, %sw.bb151 ], [ %inc150, %sw.bb149 ], [ %inc148, %sw.bb147 ], [ %inc146, %sw.bb145 ], [ %inc144, %sw.bb143 ], [ %inc142, %sw.bb141 ], [ %inc140, %sw.bb139 ], [ %inc138, %sw.bb137 ], [ %inc136, %sw.bb135 ], [ %inc134, %sw.bb133 ], [ %inc132, %sw.bb131 ], [ %inc130, %sw.bb129 ], [ %inc128, %sw.bb127 ], [ %inc126, %sw.bb125 ], [ %inc124, %sw.bb123 ], [ %inc122, %sw.bb121 ], [ %inc120, %sw.bb119 ], [ %inc118, %sw.bb117 ], [ %inc116, %sw.bb115 ], [ %inc114, %sw.bb113 ], [ %inc112, %sw.bb111 ], [ %inc110, %sw.bb109 ], [ %inc108, %sw.bb107 ], [ %inc106, %sw.bb105 ], [ %inc104, %sw.bb103 ], [ %inc102, %sw.bb101 ], [ %inc100, %sw.bb99 ], [ %inc98, %sw.bb97 ], [ %inc96, %sw.bb95 ], [ %inc94, %sw.bb93 ], [ %inc92, %sw.bb91 ], [ %inc90, %sw.bb89 ], [ %inc88, %sw.bb87 ], [ %inc86, %sw.bb85 ], [ %inc84, %sw.bb83 ], [ %inc82, %sw.bb81 ], [ %inc80, %sw.bb79 ], [ %inc78, %sw.bb77 ], [ %inc76, %sw.bb75 ], [ %inc74, %sw.bb73 ], [ %inc72, %sw.bb71 ], [ %inc70, %sw.bb69 ], [ %inc68, %sw.bb67 ], [ %inc66, %sw.bb65 ], [ %inc64, %sw.bb63 ], [ %inc62, %sw.bb61 ], [ %inc60, %sw.bb59 ], [ %inc58, %sw.bb57 ], [ %inc56, %sw.bb55 ], [ %inc54, %sw.bb53 ], [ %inc52, %sw.bb51 ], [ %inc50, %sw.bb49 ], [ %inc48, %sw.bb47 ], [ %inc46, %sw.bb45 ], [ %inc44, %sw.bb43 ], [ %inc42, %sw.bb41 ], [ %inc40, %sw.bb39 ], [ %inc38, %sw.bb37 ], [ %inc36, %sw.bb35 ], [ %inc34, %sw.bb33 ], [ %inc32, %sw.bb31 ], [ %inc30, %sw.bb29 ], [ %inc28, %sw.bb27 ], [ %inc26, %sw.bb25 ], [ %inc24, %sw.bb23 ], [ %inc22, %sw.bb21 ], [ %inc20, %sw.bb19 ], [ %inc18, %sw.bb17 ], [ %inc16, %sw.bb15 ], [ %inc14, %sw.bb13 ], [ %inc12, %sw.bb11 ], [ %inc10, %sw.bb9 ], [ %inc8, %sw.bb7 ], [ %inc6, %sw.bb5 ], [ %inc4, %sw.bb3 ], [ %inc2, %sw.bb1 ], [ %inc, %sw.bb ], !dbg !262
    #dbg_value(i16 %c.addr.1, !11, !DIExpression(), !12)
  br label %for.inc, !dbg !263

for.inc:                                          ; preds = %sw.epilog
  %inc239 = add nuw nsw i16 %i.01, 1, !dbg !264
    #dbg_value(i16 %c.addr.1, !11, !DIExpression(), !12)
    #dbg_value(i16 %inc239, !13, !DIExpression(), !12)
  %exitcond = icmp ne i16 %inc239, 120, !dbg !265
  br i1 %exitcond, label %for.body, label %for.end, !dbg !14, !llvm.loop !266

for.end:                                          ; preds = %for.inc
  %c.addr.0.lcssa = phi i16 [ %c.addr.1, %for.inc ]
  ret i16 %c.addr.0.lcssa, !dbg !269
}

; Function Attrs: noinline nounwind
define dso_local i16 @swi50(i16 noundef %c) #0 !dbg !270 {
entry:
    #dbg_value(i16 %c, !271, !DIExpression(), !272)
    #dbg_value(i16 0, !273, !DIExpression(), !272)
  br label %for.body, !dbg !274

for.body:                                         ; preds = %for.inc, %entry
  %c.addr.02 = phi i16 [ %c, %entry ], [ %c.addr.1, %for.inc ]
  %i.01 = phi i16 [ 0, %entry ], [ %inc119, %for.inc ]
    #dbg_value(i16 %c.addr.02, !271, !DIExpression(), !272)
    #dbg_value(i16 %i.01, !273, !DIExpression(), !272)
  switch i16 %i.01, label %sw.default [
    i16 0, label %sw.bb
    i16 1, label %sw.bb1
    i16 2, label %sw.bb3
    i16 3, label %sw.bb5
    i16 4, label %sw.bb7
    i16 5, label %sw.bb9
    i16 6, label %sw.bb11
    i16 7, label %sw.bb13
    i16 8, label %sw.bb15
    i16 9, label %sw.bb17
    i16 10, label %sw.bb19
    i16 11, label %sw.bb21
    i16 12, label %sw.bb23
    i16 13, label %sw.bb25
    i16 14, label %sw.bb27
    i16 15, label %sw.bb29
    i16 16, label %sw.bb31
    i16 17, label %sw.bb33
    i16 18, label %sw.bb35
    i16 19, label %sw.bb37
    i16 20, label %sw.bb39
    i16 21, label %sw.bb41
    i16 22, label %sw.bb43
    i16 23, label %sw.bb45
    i16 24, label %sw.bb47
    i16 25, label %sw.bb49
    i16 26, label %sw.bb51
    i16 27, label %sw.bb53
    i16 28, label %sw.bb55
    i16 29, label %sw.bb57
    i16 30, label %sw.bb59
    i16 31, label %sw.bb61
    i16 32, label %sw.bb63
    i16 33, label %sw.bb65
    i16 34, label %sw.bb67
    i16 35, label %sw.bb69
    i16 36, label %sw.bb71
    i16 37, label %sw.bb73
    i16 38, label %sw.bb75
    i16 39, label %sw.bb77
    i16 40, label %sw.bb79
    i16 41, label %sw.bb81
    i16 42, label %sw.bb83
    i16 43, label %sw.bb85
    i16 44, label %sw.bb87
    i16 45, label %sw.bb89
    i16 46, label %sw.bb91
    i16 47, label %sw.bb93
    i16 48, label %sw.bb95
    i16 49, label %sw.bb97
    i16 50, label %sw.bb99
    i16 51, label %sw.bb101
    i16 52, label %sw.bb103
    i16 53, label %sw.bb105
    i16 54, label %sw.bb107
    i16 55, label %sw.bb109
    i16 56, label %sw.bb111
    i16 57, label %sw.bb113
    i16 58, label %sw.bb115
    i16 59, label %sw.bb117
  ], !dbg !276

sw.bb:                                            ; preds = %for.body
  %inc = add nsw i16 %c.addr.02, 1, !dbg !279
    #dbg_value(i16 %inc, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !281

sw.bb1:                                           ; preds = %for.body
  %inc2 = add nsw i16 %c.addr.02, 1, !dbg !282
    #dbg_value(i16 %inc2, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !283

sw.bb3:                                           ; preds = %for.body
  %inc4 = add nsw i16 %c.addr.02, 1, !dbg !284
    #dbg_value(i16 %inc4, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !285

sw.bb5:                                           ; preds = %for.body
  %inc6 = add nsw i16 %c.addr.02, 1, !dbg !286
    #dbg_value(i16 %inc6, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !287

sw.bb7:                                           ; preds = %for.body
  %inc8 = add nsw i16 %c.addr.02, 1, !dbg !288
    #dbg_value(i16 %inc8, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !289

sw.bb9:                                           ; preds = %for.body
  %inc10 = add nsw i16 %c.addr.02, 1, !dbg !290
    #dbg_value(i16 %inc10, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !291

sw.bb11:                                          ; preds = %for.body
  %inc12 = add nsw i16 %c.addr.02, 1, !dbg !292
    #dbg_value(i16 %inc12, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !293

sw.bb13:                                          ; preds = %for.body
  %inc14 = add nsw i16 %c.addr.02, 1, !dbg !294
    #dbg_value(i16 %inc14, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !295

sw.bb15:                                          ; preds = %for.body
  %inc16 = add nsw i16 %c.addr.02, 1, !dbg !296
    #dbg_value(i16 %inc16, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !297

sw.bb17:                                          ; preds = %for.body
  %inc18 = add nsw i16 %c.addr.02, 1, !dbg !298
    #dbg_value(i16 %inc18, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !299

sw.bb19:                                          ; preds = %for.body
  %inc20 = add nsw i16 %c.addr.02, 1, !dbg !300
    #dbg_value(i16 %inc20, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !301

sw.bb21:                                          ; preds = %for.body
  %inc22 = add nsw i16 %c.addr.02, 1, !dbg !302
    #dbg_value(i16 %inc22, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !303

sw.bb23:                                          ; preds = %for.body
  %inc24 = add nsw i16 %c.addr.02, 1, !dbg !304
    #dbg_value(i16 %inc24, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !305

sw.bb25:                                          ; preds = %for.body
  %inc26 = add nsw i16 %c.addr.02, 1, !dbg !306
    #dbg_value(i16 %inc26, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !307

sw.bb27:                                          ; preds = %for.body
  %inc28 = add nsw i16 %c.addr.02, 1, !dbg !308
    #dbg_value(i16 %inc28, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !309

sw.bb29:                                          ; preds = %for.body
  %inc30 = add nsw i16 %c.addr.02, 1, !dbg !310
    #dbg_value(i16 %inc30, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !311

sw.bb31:                                          ; preds = %for.body
  %inc32 = add nsw i16 %c.addr.02, 1, !dbg !312
    #dbg_value(i16 %inc32, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !313

sw.bb33:                                          ; preds = %for.body
  %inc34 = add nsw i16 %c.addr.02, 1, !dbg !314
    #dbg_value(i16 %inc34, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !315

sw.bb35:                                          ; preds = %for.body
  %inc36 = add nsw i16 %c.addr.02, 1, !dbg !316
    #dbg_value(i16 %inc36, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !317

sw.bb37:                                          ; preds = %for.body
  %inc38 = add nsw i16 %c.addr.02, 1, !dbg !318
    #dbg_value(i16 %inc38, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !319

sw.bb39:                                          ; preds = %for.body
  %inc40 = add nsw i16 %c.addr.02, 1, !dbg !320
    #dbg_value(i16 %inc40, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !321

sw.bb41:                                          ; preds = %for.body
  %inc42 = add nsw i16 %c.addr.02, 1, !dbg !322
    #dbg_value(i16 %inc42, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !323

sw.bb43:                                          ; preds = %for.body
  %inc44 = add nsw i16 %c.addr.02, 1, !dbg !324
    #dbg_value(i16 %inc44, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !325

sw.bb45:                                          ; preds = %for.body
  %inc46 = add nsw i16 %c.addr.02, 1, !dbg !326
    #dbg_value(i16 %inc46, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !327

sw.bb47:                                          ; preds = %for.body
  %inc48 = add nsw i16 %c.addr.02, 1, !dbg !328
    #dbg_value(i16 %inc48, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !329

sw.bb49:                                          ; preds = %for.body
  %inc50 = add nsw i16 %c.addr.02, 1, !dbg !330
    #dbg_value(i16 %inc50, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !331

sw.bb51:                                          ; preds = %for.body
  %inc52 = add nsw i16 %c.addr.02, 1, !dbg !332
    #dbg_value(i16 %inc52, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !333

sw.bb53:                                          ; preds = %for.body
  %inc54 = add nsw i16 %c.addr.02, 1, !dbg !334
    #dbg_value(i16 %inc54, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !335

sw.bb55:                                          ; preds = %for.body
  %inc56 = add nsw i16 %c.addr.02, 1, !dbg !336
    #dbg_value(i16 %inc56, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !337

sw.bb57:                                          ; preds = %for.body
  %inc58 = add nsw i16 %c.addr.02, 1, !dbg !338
    #dbg_value(i16 %inc58, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !339

sw.bb59:                                          ; preds = %for.body
  %inc60 = add nsw i16 %c.addr.02, 1, !dbg !340
    #dbg_value(i16 %inc60, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !341

sw.bb61:                                          ; preds = %for.body
  %inc62 = add nsw i16 %c.addr.02, 1, !dbg !342
    #dbg_value(i16 %inc62, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !343

sw.bb63:                                          ; preds = %for.body
  %inc64 = add nsw i16 %c.addr.02, 1, !dbg !344
    #dbg_value(i16 %inc64, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !345

sw.bb65:                                          ; preds = %for.body
  %inc66 = add nsw i16 %c.addr.02, 1, !dbg !346
    #dbg_value(i16 %inc66, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !347

sw.bb67:                                          ; preds = %for.body
  %inc68 = add nsw i16 %c.addr.02, 1, !dbg !348
    #dbg_value(i16 %inc68, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !349

sw.bb69:                                          ; preds = %for.body
  %inc70 = add nsw i16 %c.addr.02, 1, !dbg !350
    #dbg_value(i16 %inc70, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !351

sw.bb71:                                          ; preds = %for.body
  %inc72 = add nsw i16 %c.addr.02, 1, !dbg !352
    #dbg_value(i16 %inc72, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !353

sw.bb73:                                          ; preds = %for.body
  %inc74 = add nsw i16 %c.addr.02, 1, !dbg !354
    #dbg_value(i16 %inc74, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !355

sw.bb75:                                          ; preds = %for.body
  %inc76 = add nsw i16 %c.addr.02, 1, !dbg !356
    #dbg_value(i16 %inc76, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !357

sw.bb77:                                          ; preds = %for.body
  %inc78 = add nsw i16 %c.addr.02, 1, !dbg !358
    #dbg_value(i16 %inc78, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !359

sw.bb79:                                          ; preds = %for.body
  %inc80 = add nsw i16 %c.addr.02, 1, !dbg !360
    #dbg_value(i16 %inc80, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !361

sw.bb81:                                          ; preds = %for.body
  %inc82 = add nsw i16 %c.addr.02, 1, !dbg !362
    #dbg_value(i16 %inc82, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !363

sw.bb83:                                          ; preds = %for.body
  %inc84 = add nsw i16 %c.addr.02, 1, !dbg !364
    #dbg_value(i16 %inc84, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !365

sw.bb85:                                          ; preds = %for.body
  %inc86 = add nsw i16 %c.addr.02, 1, !dbg !366
    #dbg_value(i16 %inc86, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !367

sw.bb87:                                          ; preds = %for.body
  %inc88 = add nsw i16 %c.addr.02, 1, !dbg !368
    #dbg_value(i16 %inc88, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !369

sw.bb89:                                          ; preds = %for.body
  %inc90 = add nsw i16 %c.addr.02, 1, !dbg !370
    #dbg_value(i16 %inc90, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !371

sw.bb91:                                          ; preds = %for.body
  %inc92 = add nsw i16 %c.addr.02, 1, !dbg !372
    #dbg_value(i16 %inc92, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !373

sw.bb93:                                          ; preds = %for.body
  %inc94 = add nsw i16 %c.addr.02, 1, !dbg !374
    #dbg_value(i16 %inc94, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !375

sw.bb95:                                          ; preds = %for.body
  %inc96 = add nsw i16 %c.addr.02, 1, !dbg !376
    #dbg_value(i16 %inc96, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !377

sw.bb97:                                          ; preds = %for.body
  %inc98 = add nsw i16 %c.addr.02, 1, !dbg !378
    #dbg_value(i16 %inc98, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !379

sw.bb99:                                          ; preds = %for.body
  %inc100 = add nsw i16 %c.addr.02, 1, !dbg !380
    #dbg_value(i16 %inc100, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !381

sw.bb101:                                         ; preds = %for.body
  %inc102 = add nsw i16 %c.addr.02, 1, !dbg !382
    #dbg_value(i16 %inc102, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !383

sw.bb103:                                         ; preds = %for.body
  %inc104 = add nsw i16 %c.addr.02, 1, !dbg !384
    #dbg_value(i16 %inc104, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !385

sw.bb105:                                         ; preds = %for.body
  %inc106 = add nsw i16 %c.addr.02, 1, !dbg !386
    #dbg_value(i16 %inc106, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !387

sw.bb107:                                         ; preds = %for.body
  %inc108 = add nsw i16 %c.addr.02, 1, !dbg !388
    #dbg_value(i16 %inc108, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !389

sw.bb109:                                         ; preds = %for.body
  %inc110 = add nsw i16 %c.addr.02, 1, !dbg !390
    #dbg_value(i16 %inc110, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !391

sw.bb111:                                         ; preds = %for.body
  %inc112 = add nsw i16 %c.addr.02, 1, !dbg !392
    #dbg_value(i16 %inc112, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !393

sw.bb113:                                         ; preds = %for.body
  %inc114 = add nsw i16 %c.addr.02, 1, !dbg !394
    #dbg_value(i16 %inc114, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !395

sw.bb115:                                         ; preds = %for.body
  %inc116 = add nsw i16 %c.addr.02, 1, !dbg !396
    #dbg_value(i16 %inc116, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !397

sw.bb117:                                         ; preds = %for.body
  %inc118 = add nsw i16 %c.addr.02, 1, !dbg !398
    #dbg_value(i16 %inc118, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !399

sw.default:                                       ; preds = %for.body
  %dec = add nsw i16 %c.addr.02, -1, !dbg !400
    #dbg_value(i16 %dec, !271, !DIExpression(), !272)
  br label %sw.epilog, !dbg !401

sw.epilog:                                        ; preds = %sw.default, %sw.bb117, %sw.bb115, %sw.bb113, %sw.bb111, %sw.bb109, %sw.bb107, %sw.bb105, %sw.bb103, %sw.bb101, %sw.bb99, %sw.bb97, %sw.bb95, %sw.bb93, %sw.bb91, %sw.bb89, %sw.bb87, %sw.bb85, %sw.bb83, %sw.bb81, %sw.bb79, %sw.bb77, %sw.bb75, %sw.bb73, %sw.bb71, %sw.bb69, %sw.bb67, %sw.bb65, %sw.bb63, %sw.bb61, %sw.bb59, %sw.bb57, %sw.bb55, %sw.bb53, %sw.bb51, %sw.bb49, %sw.bb47, %sw.bb45, %sw.bb43, %sw.bb41, %sw.bb39, %sw.bb37, %sw.bb35, %sw.bb33, %sw.bb31, %sw.bb29, %sw.bb27, %sw.bb25, %sw.bb23, %sw.bb21, %sw.bb19, %sw.bb17, %sw.bb15, %sw.bb13, %sw.bb11, %sw.bb9, %sw.bb7, %sw.bb5, %sw.bb3, %sw.bb1, %sw.bb
  %c.addr.1 = phi i16 [ %dec, %sw.default ], [ %inc118, %sw.bb117 ], [ %inc116, %sw.bb115 ], [ %inc114, %sw.bb113 ], [ %inc112, %sw.bb111 ], [ %inc110, %sw.bb109 ], [ %inc108, %sw.bb107 ], [ %inc106, %sw.bb105 ], [ %inc104, %sw.bb103 ], [ %inc102, %sw.bb101 ], [ %inc100, %sw.bb99 ], [ %inc98, %sw.bb97 ], [ %inc96, %sw.bb95 ], [ %inc94, %sw.bb93 ], [ %inc92, %sw.bb91 ], [ %inc90, %sw.bb89 ], [ %inc88, %sw.bb87 ], [ %inc86, %sw.bb85 ], [ %inc84, %sw.bb83 ], [ %inc82, %sw.bb81 ], [ %inc80, %sw.bb79 ], [ %inc78, %sw.bb77 ], [ %inc76, %sw.bb75 ], [ %inc74, %sw.bb73 ], [ %inc72, %sw.bb71 ], [ %inc70, %sw.bb69 ], [ %inc68, %sw.bb67 ], [ %inc66, %sw.bb65 ], [ %inc64, %sw.bb63 ], [ %inc62, %sw.bb61 ], [ %inc60, %sw.bb59 ], [ %inc58, %sw.bb57 ], [ %inc56, %sw.bb55 ], [ %inc54, %sw.bb53 ], [ %inc52, %sw.bb51 ], [ %inc50, %sw.bb49 ], [ %inc48, %sw.bb47 ], [ %inc46, %sw.bb45 ], [ %inc44, %sw.bb43 ], [ %inc42, %sw.bb41 ], [ %inc40, %sw.bb39 ], [ %inc38, %sw.bb37 ], [ %inc36, %sw.bb35 ], [ %inc34, %sw.bb33 ], [ %inc32, %sw.bb31 ], [ %inc30, %sw.bb29 ], [ %inc28, %sw.bb27 ], [ %inc26, %sw.bb25 ], [ %inc24, %sw.bb23 ], [ %inc22, %sw.bb21 ], [ %inc20, %sw.bb19 ], [ %inc18, %sw.bb17 ], [ %inc16, %sw.bb15 ], [ %inc14, %sw.bb13 ], [ %inc12, %sw.bb11 ], [ %inc10, %sw.bb9 ], [ %inc8, %sw.bb7 ], [ %inc6, %sw.bb5 ], [ %inc4, %sw.bb3 ], [ %inc2, %sw.bb1 ], [ %inc, %sw.bb ], !dbg !402
    #dbg_value(i16 %c.addr.1, !271, !DIExpression(), !272)
  br label %for.inc, !dbg !403

for.inc:                                          ; preds = %sw.epilog
  %inc119 = add nuw nsw i16 %i.01, 1, !dbg !404
    #dbg_value(i16 %c.addr.1, !271, !DIExpression(), !272)
    #dbg_value(i16 %inc119, !273, !DIExpression(), !272)
  %exitcond = icmp ne i16 %inc119, 50, !dbg !405
  br i1 %exitcond, label %for.body, label %for.end, !dbg !274, !llvm.loop !406

for.end:                                          ; preds = %for.inc
  %c.addr.0.lcssa = phi i16 [ %c.addr.1, %for.inc ]
  ret i16 %c.addr.0.lcssa, !dbg !408
}

; Function Attrs: noinline nounwind
define dso_local i16 @swi10(i16 noundef %c) #0 !dbg !409 {
entry:
    #dbg_value(i16 %c, !410, !DIExpression(), !411)
    #dbg_value(i16 0, !412, !DIExpression(), !411)
  br label %for.body, !dbg !413

for.body:                                         ; preds = %for.inc, %entry
  %c.addr.02 = phi i16 [ %c, %entry ], [ %c.addr.1, %for.inc ]
  %i.01 = phi i16 [ 0, %entry ], [ %inc19, %for.inc ]
    #dbg_value(i16 %c.addr.02, !410, !DIExpression(), !411)
    #dbg_value(i16 %i.01, !412, !DIExpression(), !411)
  switch i16 %i.01, label %sw.default [
    i16 0, label %sw.bb
    i16 1, label %sw.bb1
    i16 2, label %sw.bb3
    i16 3, label %sw.bb5
    i16 4, label %sw.bb7
    i16 5, label %sw.bb9
    i16 6, label %sw.bb11
    i16 7, label %sw.bb13
    i16 8, label %sw.bb15
    i16 9, label %sw.bb17
  ], !dbg !415

sw.bb:                                            ; preds = %for.body
  %inc = add nsw i16 %c.addr.02, 1, !dbg !418
    #dbg_value(i16 %inc, !410, !DIExpression(), !411)
  br label %sw.epilog, !dbg !420

sw.bb1:                                           ; preds = %for.body
  %inc2 = add nsw i16 %c.addr.02, 1, !dbg !421
    #dbg_value(i16 %inc2, !410, !DIExpression(), !411)
  br label %sw.epilog, !dbg !422

sw.bb3:                                           ; preds = %for.body
  %inc4 = add nsw i16 %c.addr.02, 1, !dbg !423
    #dbg_value(i16 %inc4, !410, !DIExpression(), !411)
  br label %sw.epilog, !dbg !424

sw.bb5:                                           ; preds = %for.body
  %inc6 = add nsw i16 %c.addr.02, 1, !dbg !425
    #dbg_value(i16 %inc6, !410, !DIExpression(), !411)
  br label %sw.epilog, !dbg !426

sw.bb7:                                           ; preds = %for.body
  %inc8 = add nsw i16 %c.addr.02, 1, !dbg !427
    #dbg_value(i16 %inc8, !410, !DIExpression(), !411)
  br label %sw.epilog, !dbg !428

sw.bb9:                                           ; preds = %for.body
  %inc10 = add nsw i16 %c.addr.02, 1, !dbg !429
    #dbg_value(i16 %inc10, !410, !DIExpression(), !411)
  br label %sw.epilog, !dbg !430

sw.bb11:                                          ; preds = %for.body
  %inc12 = add nsw i16 %c.addr.02, 1, !dbg !431
    #dbg_value(i16 %inc12, !410, !DIExpression(), !411)
  br label %sw.epilog, !dbg !432

sw.bb13:                                          ; preds = %for.body
  %inc14 = add nsw i16 %c.addr.02, 1, !dbg !433
    #dbg_value(i16 %inc14, !410, !DIExpression(), !411)
  br label %sw.epilog, !dbg !434

sw.bb15:                                          ; preds = %for.body
  %inc16 = add nsw i16 %c.addr.02, 1, !dbg !435
    #dbg_value(i16 %inc16, !410, !DIExpression(), !411)
  br label %sw.epilog, !dbg !436

sw.bb17:                                          ; preds = %for.body
  %inc18 = add nsw i16 %c.addr.02, 1, !dbg !437
    #dbg_value(i16 %inc18, !410, !DIExpression(), !411)
  br label %sw.epilog, !dbg !438

sw.default:                                       ; preds = %for.body
  %dec = add nsw i16 %c.addr.02, -1, !dbg !439
    #dbg_value(i16 %dec, !410, !DIExpression(), !411)
  br label %sw.epilog, !dbg !440

sw.epilog:                                        ; preds = %sw.default, %sw.bb17, %sw.bb15, %sw.bb13, %sw.bb11, %sw.bb9, %sw.bb7, %sw.bb5, %sw.bb3, %sw.bb1, %sw.bb
  %c.addr.1 = phi i16 [ %dec, %sw.default ], [ %inc18, %sw.bb17 ], [ %inc16, %sw.bb15 ], [ %inc14, %sw.bb13 ], [ %inc12, %sw.bb11 ], [ %inc10, %sw.bb9 ], [ %inc8, %sw.bb7 ], [ %inc6, %sw.bb5 ], [ %inc4, %sw.bb3 ], [ %inc2, %sw.bb1 ], [ %inc, %sw.bb ], !dbg !441
    #dbg_value(i16 %c.addr.1, !410, !DIExpression(), !411)
  br label %for.inc, !dbg !442

for.inc:                                          ; preds = %sw.epilog
  %inc19 = add nuw nsw i16 %i.01, 1, !dbg !443
    #dbg_value(i16 %c.addr.1, !410, !DIExpression(), !411)
    #dbg_value(i16 %inc19, !412, !DIExpression(), !411)
  %exitcond = icmp ne i16 %inc19, 10, !dbg !444
  br i1 %exitcond, label %for.body, label %for.end, !dbg !413, !llvm.loop !445

for.end:                                          ; preds = %for.inc
  %c.addr.0.lcssa = phi i16 [ %c.addr.1, %for.inc ]
  ret i16 %c.addr.0.lcssa, !dbg !447
}

; Function Attrs: noinline nounwind
define dso_local i16 @main() #0 !dbg !448 {
entry:
  %cnt = alloca i16, align 2
    #dbg_declare(ptr %cnt, !451, !DIExpression(), !453)
  store volatile i16 0, ptr %cnt, align 2, !dbg !453
  %0 = load volatile i16, ptr %cnt, align 2, !dbg !454
  %call = call i16 @swi10(i16 noundef %0), !dbg !455
  store volatile i16 %call, ptr %cnt, align 2, !dbg !456
  %1 = load volatile i16, ptr %cnt, align 2, !dbg !457
  %call1 = call i16 @swi50(i16 noundef %1), !dbg !458
  store volatile i16 %call1, ptr %cnt, align 2, !dbg !459
  %2 = load volatile i16, ptr %cnt, align 2, !dbg !460
  %call2 = call i16 @swi120(i16 noundef %2), !dbg !461
  store volatile i16 %call2, ptr %cnt, align 2, !dbg !462
  %3 = load volatile i16, ptr %cnt, align 2, !dbg !463
  ret i16 %3, !dbg !464
}

attributes #0 = { noinline nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" }

!llvm.dbg.cu = !{!0}
!llvm.ident = !{!2}
!llvm.module.flags = !{!3, !4, !5}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 22.0.0git (git@github.com:OMA-NVM/llvm-project.git fca9ff2383868608326c25ec508dffeb3c1b30e9)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "src/main.c", directory: "/Users/nilsholscher/workspaces/msp430_template")
!2 = !{!"clang version 22.0.0git (git@github.com:OMA-NVM/llvm-project.git fca9ff2383868608326c25ec508dffeb3c1b30e9)"}
!3 = !{i32 7, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 2}
!6 = distinct !DISubprogram(name: "swi120", scope: !1, file: !1, line: 1, type: !7, scopeLine: 2, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !10)
!7 = !DISubroutineType(types: !8)
!8 = !{!9, !9}
!9 = !DIBasicType(name: "int", size: 16, encoding: DW_ATE_signed)
!10 = !{}
!11 = !DILocalVariable(name: "c", arg: 1, scope: !6, file: !1, line: 1, type: !9)
!12 = !DILocation(line: 0, scope: !6)
!13 = !DILocalVariable(name: "i", scope: !6, file: !1, line: 3, type: !9)
!14 = !DILocation(line: 5, column: 2, scope: !15)
!15 = distinct !DILexicalBlock(scope: !6, file: !1, line: 5, column: 2)
!16 = !DILocation(line: 6, column: 3, scope: !17)
!17 = distinct !DILexicalBlock(scope: !18, file: !1, line: 5, column: 24)
!18 = distinct !DILexicalBlock(scope: !15, file: !1, line: 5, column: 2)
!19 = !DILocation(line: 7, column: 13, scope: !20)
!20 = distinct !DILexicalBlock(scope: !17, file: !1, line: 6, column: 14)
!21 = !DILocation(line: 7, column: 17, scope: !20)
!22 = !DILocation(line: 8, column: 13, scope: !20)
!23 = !DILocation(line: 8, column: 17, scope: !20)
!24 = !DILocation(line: 9, column: 13, scope: !20)
!25 = !DILocation(line: 9, column: 17, scope: !20)
!26 = !DILocation(line: 10, column: 13, scope: !20)
!27 = !DILocation(line: 10, column: 17, scope: !20)
!28 = !DILocation(line: 11, column: 13, scope: !20)
!29 = !DILocation(line: 11, column: 17, scope: !20)
!30 = !DILocation(line: 12, column: 13, scope: !20)
!31 = !DILocation(line: 12, column: 17, scope: !20)
!32 = !DILocation(line: 13, column: 13, scope: !20)
!33 = !DILocation(line: 13, column: 17, scope: !20)
!34 = !DILocation(line: 14, column: 13, scope: !20)
!35 = !DILocation(line: 14, column: 17, scope: !20)
!36 = !DILocation(line: 15, column: 13, scope: !20)
!37 = !DILocation(line: 15, column: 17, scope: !20)
!38 = !DILocation(line: 16, column: 13, scope: !20)
!39 = !DILocation(line: 16, column: 17, scope: !20)
!40 = !DILocation(line: 17, column: 14, scope: !20)
!41 = !DILocation(line: 17, column: 18, scope: !20)
!42 = !DILocation(line: 18, column: 14, scope: !20)
!43 = !DILocation(line: 18, column: 18, scope: !20)
!44 = !DILocation(line: 19, column: 14, scope: !20)
!45 = !DILocation(line: 19, column: 18, scope: !20)
!46 = !DILocation(line: 20, column: 14, scope: !20)
!47 = !DILocation(line: 20, column: 18, scope: !20)
!48 = !DILocation(line: 21, column: 14, scope: !20)
!49 = !DILocation(line: 21, column: 18, scope: !20)
!50 = !DILocation(line: 22, column: 14, scope: !20)
!51 = !DILocation(line: 22, column: 18, scope: !20)
!52 = !DILocation(line: 23, column: 14, scope: !20)
!53 = !DILocation(line: 23, column: 18, scope: !20)
!54 = !DILocation(line: 24, column: 14, scope: !20)
!55 = !DILocation(line: 24, column: 18, scope: !20)
!56 = !DILocation(line: 25, column: 14, scope: !20)
!57 = !DILocation(line: 25, column: 18, scope: !20)
!58 = !DILocation(line: 26, column: 14, scope: !20)
!59 = !DILocation(line: 26, column: 18, scope: !20)
!60 = !DILocation(line: 27, column: 14, scope: !20)
!61 = !DILocation(line: 27, column: 18, scope: !20)
!62 = !DILocation(line: 28, column: 14, scope: !20)
!63 = !DILocation(line: 28, column: 18, scope: !20)
!64 = !DILocation(line: 29, column: 14, scope: !20)
!65 = !DILocation(line: 29, column: 18, scope: !20)
!66 = !DILocation(line: 30, column: 14, scope: !20)
!67 = !DILocation(line: 30, column: 18, scope: !20)
!68 = !DILocation(line: 31, column: 14, scope: !20)
!69 = !DILocation(line: 31, column: 18, scope: !20)
!70 = !DILocation(line: 32, column: 14, scope: !20)
!71 = !DILocation(line: 32, column: 18, scope: !20)
!72 = !DILocation(line: 33, column: 14, scope: !20)
!73 = !DILocation(line: 33, column: 18, scope: !20)
!74 = !DILocation(line: 34, column: 14, scope: !20)
!75 = !DILocation(line: 34, column: 18, scope: !20)
!76 = !DILocation(line: 35, column: 14, scope: !20)
!77 = !DILocation(line: 35, column: 18, scope: !20)
!78 = !DILocation(line: 36, column: 14, scope: !20)
!79 = !DILocation(line: 36, column: 18, scope: !20)
!80 = !DILocation(line: 37, column: 14, scope: !20)
!81 = !DILocation(line: 37, column: 18, scope: !20)
!82 = !DILocation(line: 38, column: 14, scope: !20)
!83 = !DILocation(line: 38, column: 18, scope: !20)
!84 = !DILocation(line: 39, column: 14, scope: !20)
!85 = !DILocation(line: 39, column: 18, scope: !20)
!86 = !DILocation(line: 40, column: 14, scope: !20)
!87 = !DILocation(line: 40, column: 18, scope: !20)
!88 = !DILocation(line: 41, column: 14, scope: !20)
!89 = !DILocation(line: 41, column: 18, scope: !20)
!90 = !DILocation(line: 42, column: 14, scope: !20)
!91 = !DILocation(line: 42, column: 18, scope: !20)
!92 = !DILocation(line: 43, column: 14, scope: !20)
!93 = !DILocation(line: 43, column: 18, scope: !20)
!94 = !DILocation(line: 44, column: 14, scope: !20)
!95 = !DILocation(line: 44, column: 18, scope: !20)
!96 = !DILocation(line: 45, column: 14, scope: !20)
!97 = !DILocation(line: 45, column: 18, scope: !20)
!98 = !DILocation(line: 46, column: 14, scope: !20)
!99 = !DILocation(line: 46, column: 18, scope: !20)
!100 = !DILocation(line: 47, column: 14, scope: !20)
!101 = !DILocation(line: 47, column: 18, scope: !20)
!102 = !DILocation(line: 48, column: 14, scope: !20)
!103 = !DILocation(line: 48, column: 18, scope: !20)
!104 = !DILocation(line: 49, column: 14, scope: !20)
!105 = !DILocation(line: 49, column: 18, scope: !20)
!106 = !DILocation(line: 50, column: 14, scope: !20)
!107 = !DILocation(line: 50, column: 18, scope: !20)
!108 = !DILocation(line: 51, column: 14, scope: !20)
!109 = !DILocation(line: 51, column: 18, scope: !20)
!110 = !DILocation(line: 52, column: 14, scope: !20)
!111 = !DILocation(line: 52, column: 18, scope: !20)
!112 = !DILocation(line: 53, column: 14, scope: !20)
!113 = !DILocation(line: 53, column: 18, scope: !20)
!114 = !DILocation(line: 54, column: 14, scope: !20)
!115 = !DILocation(line: 54, column: 18, scope: !20)
!116 = !DILocation(line: 55, column: 14, scope: !20)
!117 = !DILocation(line: 55, column: 18, scope: !20)
!118 = !DILocation(line: 56, column: 14, scope: !20)
!119 = !DILocation(line: 56, column: 18, scope: !20)
!120 = !DILocation(line: 57, column: 14, scope: !20)
!121 = !DILocation(line: 57, column: 18, scope: !20)
!122 = !DILocation(line: 58, column: 14, scope: !20)
!123 = !DILocation(line: 58, column: 18, scope: !20)
!124 = !DILocation(line: 59, column: 14, scope: !20)
!125 = !DILocation(line: 59, column: 18, scope: !20)
!126 = !DILocation(line: 60, column: 14, scope: !20)
!127 = !DILocation(line: 60, column: 18, scope: !20)
!128 = !DILocation(line: 61, column: 14, scope: !20)
!129 = !DILocation(line: 61, column: 18, scope: !20)
!130 = !DILocation(line: 62, column: 14, scope: !20)
!131 = !DILocation(line: 62, column: 18, scope: !20)
!132 = !DILocation(line: 63, column: 14, scope: !20)
!133 = !DILocation(line: 63, column: 18, scope: !20)
!134 = !DILocation(line: 64, column: 14, scope: !20)
!135 = !DILocation(line: 64, column: 18, scope: !20)
!136 = !DILocation(line: 65, column: 14, scope: !20)
!137 = !DILocation(line: 65, column: 18, scope: !20)
!138 = !DILocation(line: 66, column: 14, scope: !20)
!139 = !DILocation(line: 66, column: 18, scope: !20)
!140 = !DILocation(line: 67, column: 14, scope: !20)
!141 = !DILocation(line: 67, column: 18, scope: !20)
!142 = !DILocation(line: 68, column: 14, scope: !20)
!143 = !DILocation(line: 68, column: 18, scope: !20)
!144 = !DILocation(line: 69, column: 14, scope: !20)
!145 = !DILocation(line: 69, column: 18, scope: !20)
!146 = !DILocation(line: 70, column: 14, scope: !20)
!147 = !DILocation(line: 70, column: 18, scope: !20)
!148 = !DILocation(line: 71, column: 14, scope: !20)
!149 = !DILocation(line: 71, column: 18, scope: !20)
!150 = !DILocation(line: 72, column: 14, scope: !20)
!151 = !DILocation(line: 72, column: 18, scope: !20)
!152 = !DILocation(line: 73, column: 14, scope: !20)
!153 = !DILocation(line: 73, column: 18, scope: !20)
!154 = !DILocation(line: 74, column: 14, scope: !20)
!155 = !DILocation(line: 74, column: 18, scope: !20)
!156 = !DILocation(line: 75, column: 14, scope: !20)
!157 = !DILocation(line: 75, column: 18, scope: !20)
!158 = !DILocation(line: 76, column: 14, scope: !20)
!159 = !DILocation(line: 76, column: 18, scope: !20)
!160 = !DILocation(line: 77, column: 14, scope: !20)
!161 = !DILocation(line: 77, column: 18, scope: !20)
!162 = !DILocation(line: 78, column: 14, scope: !20)
!163 = !DILocation(line: 78, column: 18, scope: !20)
!164 = !DILocation(line: 79, column: 14, scope: !20)
!165 = !DILocation(line: 79, column: 18, scope: !20)
!166 = !DILocation(line: 80, column: 14, scope: !20)
!167 = !DILocation(line: 80, column: 18, scope: !20)
!168 = !DILocation(line: 81, column: 14, scope: !20)
!169 = !DILocation(line: 81, column: 18, scope: !20)
!170 = !DILocation(line: 82, column: 14, scope: !20)
!171 = !DILocation(line: 82, column: 18, scope: !20)
!172 = !DILocation(line: 83, column: 14, scope: !20)
!173 = !DILocation(line: 83, column: 18, scope: !20)
!174 = !DILocation(line: 84, column: 14, scope: !20)
!175 = !DILocation(line: 84, column: 18, scope: !20)
!176 = !DILocation(line: 85, column: 14, scope: !20)
!177 = !DILocation(line: 85, column: 18, scope: !20)
!178 = !DILocation(line: 86, column: 14, scope: !20)
!179 = !DILocation(line: 86, column: 18, scope: !20)
!180 = !DILocation(line: 87, column: 14, scope: !20)
!181 = !DILocation(line: 87, column: 18, scope: !20)
!182 = !DILocation(line: 88, column: 14, scope: !20)
!183 = !DILocation(line: 88, column: 18, scope: !20)
!184 = !DILocation(line: 89, column: 14, scope: !20)
!185 = !DILocation(line: 89, column: 18, scope: !20)
!186 = !DILocation(line: 90, column: 14, scope: !20)
!187 = !DILocation(line: 90, column: 18, scope: !20)
!188 = !DILocation(line: 91, column: 14, scope: !20)
!189 = !DILocation(line: 91, column: 18, scope: !20)
!190 = !DILocation(line: 92, column: 14, scope: !20)
!191 = !DILocation(line: 92, column: 18, scope: !20)
!192 = !DILocation(line: 93, column: 14, scope: !20)
!193 = !DILocation(line: 93, column: 18, scope: !20)
!194 = !DILocation(line: 94, column: 14, scope: !20)
!195 = !DILocation(line: 94, column: 18, scope: !20)
!196 = !DILocation(line: 95, column: 14, scope: !20)
!197 = !DILocation(line: 95, column: 18, scope: !20)
!198 = !DILocation(line: 96, column: 14, scope: !20)
!199 = !DILocation(line: 96, column: 18, scope: !20)
!200 = !DILocation(line: 97, column: 14, scope: !20)
!201 = !DILocation(line: 97, column: 18, scope: !20)
!202 = !DILocation(line: 98, column: 14, scope: !20)
!203 = !DILocation(line: 98, column: 18, scope: !20)
!204 = !DILocation(line: 99, column: 14, scope: !20)
!205 = !DILocation(line: 99, column: 18, scope: !20)
!206 = !DILocation(line: 100, column: 14, scope: !20)
!207 = !DILocation(line: 100, column: 18, scope: !20)
!208 = !DILocation(line: 101, column: 14, scope: !20)
!209 = !DILocation(line: 101, column: 18, scope: !20)
!210 = !DILocation(line: 102, column: 14, scope: !20)
!211 = !DILocation(line: 102, column: 18, scope: !20)
!212 = !DILocation(line: 103, column: 14, scope: !20)
!213 = !DILocation(line: 103, column: 18, scope: !20)
!214 = !DILocation(line: 104, column: 14, scope: !20)
!215 = !DILocation(line: 104, column: 18, scope: !20)
!216 = !DILocation(line: 105, column: 14, scope: !20)
!217 = !DILocation(line: 105, column: 18, scope: !20)
!218 = !DILocation(line: 106, column: 14, scope: !20)
!219 = !DILocation(line: 106, column: 18, scope: !20)
!220 = !DILocation(line: 107, column: 15, scope: !20)
!221 = !DILocation(line: 107, column: 19, scope: !20)
!222 = !DILocation(line: 108, column: 15, scope: !20)
!223 = !DILocation(line: 108, column: 19, scope: !20)
!224 = !DILocation(line: 109, column: 15, scope: !20)
!225 = !DILocation(line: 109, column: 19, scope: !20)
!226 = !DILocation(line: 110, column: 15, scope: !20)
!227 = !DILocation(line: 110, column: 19, scope: !20)
!228 = !DILocation(line: 111, column: 15, scope: !20)
!229 = !DILocation(line: 111, column: 19, scope: !20)
!230 = !DILocation(line: 112, column: 15, scope: !20)
!231 = !DILocation(line: 112, column: 19, scope: !20)
!232 = !DILocation(line: 113, column: 15, scope: !20)
!233 = !DILocation(line: 113, column: 19, scope: !20)
!234 = !DILocation(line: 114, column: 15, scope: !20)
!235 = !DILocation(line: 114, column: 19, scope: !20)
!236 = !DILocation(line: 115, column: 15, scope: !20)
!237 = !DILocation(line: 115, column: 19, scope: !20)
!238 = !DILocation(line: 116, column: 15, scope: !20)
!239 = !DILocation(line: 116, column: 19, scope: !20)
!240 = !DILocation(line: 117, column: 15, scope: !20)
!241 = !DILocation(line: 117, column: 19, scope: !20)
!242 = !DILocation(line: 118, column: 15, scope: !20)
!243 = !DILocation(line: 118, column: 19, scope: !20)
!244 = !DILocation(line: 119, column: 15, scope: !20)
!245 = !DILocation(line: 119, column: 19, scope: !20)
!246 = !DILocation(line: 120, column: 15, scope: !20)
!247 = !DILocation(line: 120, column: 19, scope: !20)
!248 = !DILocation(line: 121, column: 15, scope: !20)
!249 = !DILocation(line: 121, column: 19, scope: !20)
!250 = !DILocation(line: 122, column: 15, scope: !20)
!251 = !DILocation(line: 122, column: 19, scope: !20)
!252 = !DILocation(line: 123, column: 15, scope: !20)
!253 = !DILocation(line: 123, column: 19, scope: !20)
!254 = !DILocation(line: 124, column: 15, scope: !20)
!255 = !DILocation(line: 124, column: 19, scope: !20)
!256 = !DILocation(line: 125, column: 15, scope: !20)
!257 = !DILocation(line: 125, column: 19, scope: !20)
!258 = !DILocation(line: 126, column: 15, scope: !20)
!259 = !DILocation(line: 126, column: 19, scope: !20)
!260 = !DILocation(line: 127, column: 14, scope: !20)
!261 = !DILocation(line: 127, column: 18, scope: !20)
!262 = !DILocation(line: 0, scope: !20)
!263 = !DILocation(line: 129, column: 2, scope: !17)
!264 = !DILocation(line: 5, column: 20, scope: !18)
!265 = !DILocation(line: 5, column: 13, scope: !18)
!266 = distinct !{!266, !14, !267, !268}
!267 = !DILocation(line: 129, column: 2, scope: !15)
!268 = !{!"llvm.loop.mustprogress"}
!269 = !DILocation(line: 130, column: 2, scope: !6)
!270 = distinct !DISubprogram(name: "swi50", scope: !1, file: !1, line: 134, type: !7, scopeLine: 135, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !10)
!271 = !DILocalVariable(name: "c", arg: 1, scope: !270, file: !1, line: 134, type: !9)
!272 = !DILocation(line: 0, scope: !270)
!273 = !DILocalVariable(name: "i", scope: !270, file: !1, line: 136, type: !9)
!274 = !DILocation(line: 138, column: 2, scope: !275)
!275 = distinct !DILexicalBlock(scope: !270, file: !1, line: 138, column: 2)
!276 = !DILocation(line: 139, column: 3, scope: !277)
!277 = distinct !DILexicalBlock(scope: !278, file: !1, line: 138, column: 23)
!278 = distinct !DILexicalBlock(scope: !275, file: !1, line: 138, column: 2)
!279 = !DILocation(line: 140, column: 13, scope: !280)
!280 = distinct !DILexicalBlock(scope: !277, file: !1, line: 139, column: 14)
!281 = !DILocation(line: 140, column: 17, scope: !280)
!282 = !DILocation(line: 141, column: 13, scope: !280)
!283 = !DILocation(line: 141, column: 17, scope: !280)
!284 = !DILocation(line: 142, column: 13, scope: !280)
!285 = !DILocation(line: 142, column: 17, scope: !280)
!286 = !DILocation(line: 143, column: 13, scope: !280)
!287 = !DILocation(line: 143, column: 17, scope: !280)
!288 = !DILocation(line: 144, column: 13, scope: !280)
!289 = !DILocation(line: 144, column: 17, scope: !280)
!290 = !DILocation(line: 145, column: 13, scope: !280)
!291 = !DILocation(line: 145, column: 17, scope: !280)
!292 = !DILocation(line: 146, column: 13, scope: !280)
!293 = !DILocation(line: 146, column: 17, scope: !280)
!294 = !DILocation(line: 147, column: 13, scope: !280)
!295 = !DILocation(line: 147, column: 17, scope: !280)
!296 = !DILocation(line: 148, column: 13, scope: !280)
!297 = !DILocation(line: 148, column: 17, scope: !280)
!298 = !DILocation(line: 149, column: 13, scope: !280)
!299 = !DILocation(line: 149, column: 17, scope: !280)
!300 = !DILocation(line: 150, column: 14, scope: !280)
!301 = !DILocation(line: 150, column: 18, scope: !280)
!302 = !DILocation(line: 151, column: 14, scope: !280)
!303 = !DILocation(line: 151, column: 18, scope: !280)
!304 = !DILocation(line: 152, column: 14, scope: !280)
!305 = !DILocation(line: 152, column: 18, scope: !280)
!306 = !DILocation(line: 153, column: 14, scope: !280)
!307 = !DILocation(line: 153, column: 18, scope: !280)
!308 = !DILocation(line: 154, column: 14, scope: !280)
!309 = !DILocation(line: 154, column: 18, scope: !280)
!310 = !DILocation(line: 155, column: 14, scope: !280)
!311 = !DILocation(line: 155, column: 18, scope: !280)
!312 = !DILocation(line: 156, column: 14, scope: !280)
!313 = !DILocation(line: 156, column: 18, scope: !280)
!314 = !DILocation(line: 157, column: 14, scope: !280)
!315 = !DILocation(line: 157, column: 18, scope: !280)
!316 = !DILocation(line: 158, column: 14, scope: !280)
!317 = !DILocation(line: 158, column: 18, scope: !280)
!318 = !DILocation(line: 159, column: 14, scope: !280)
!319 = !DILocation(line: 159, column: 18, scope: !280)
!320 = !DILocation(line: 160, column: 14, scope: !280)
!321 = !DILocation(line: 160, column: 18, scope: !280)
!322 = !DILocation(line: 161, column: 14, scope: !280)
!323 = !DILocation(line: 161, column: 18, scope: !280)
!324 = !DILocation(line: 162, column: 14, scope: !280)
!325 = !DILocation(line: 162, column: 18, scope: !280)
!326 = !DILocation(line: 163, column: 14, scope: !280)
!327 = !DILocation(line: 163, column: 18, scope: !280)
!328 = !DILocation(line: 164, column: 14, scope: !280)
!329 = !DILocation(line: 164, column: 18, scope: !280)
!330 = !DILocation(line: 165, column: 14, scope: !280)
!331 = !DILocation(line: 165, column: 18, scope: !280)
!332 = !DILocation(line: 166, column: 14, scope: !280)
!333 = !DILocation(line: 166, column: 18, scope: !280)
!334 = !DILocation(line: 167, column: 14, scope: !280)
!335 = !DILocation(line: 167, column: 18, scope: !280)
!336 = !DILocation(line: 168, column: 14, scope: !280)
!337 = !DILocation(line: 168, column: 18, scope: !280)
!338 = !DILocation(line: 169, column: 14, scope: !280)
!339 = !DILocation(line: 169, column: 18, scope: !280)
!340 = !DILocation(line: 170, column: 14, scope: !280)
!341 = !DILocation(line: 170, column: 18, scope: !280)
!342 = !DILocation(line: 171, column: 14, scope: !280)
!343 = !DILocation(line: 171, column: 18, scope: !280)
!344 = !DILocation(line: 172, column: 14, scope: !280)
!345 = !DILocation(line: 172, column: 18, scope: !280)
!346 = !DILocation(line: 173, column: 14, scope: !280)
!347 = !DILocation(line: 173, column: 18, scope: !280)
!348 = !DILocation(line: 174, column: 14, scope: !280)
!349 = !DILocation(line: 174, column: 18, scope: !280)
!350 = !DILocation(line: 175, column: 14, scope: !280)
!351 = !DILocation(line: 175, column: 18, scope: !280)
!352 = !DILocation(line: 176, column: 14, scope: !280)
!353 = !DILocation(line: 176, column: 18, scope: !280)
!354 = !DILocation(line: 177, column: 14, scope: !280)
!355 = !DILocation(line: 177, column: 18, scope: !280)
!356 = !DILocation(line: 178, column: 14, scope: !280)
!357 = !DILocation(line: 178, column: 18, scope: !280)
!358 = !DILocation(line: 179, column: 14, scope: !280)
!359 = !DILocation(line: 179, column: 18, scope: !280)
!360 = !DILocation(line: 180, column: 14, scope: !280)
!361 = !DILocation(line: 180, column: 18, scope: !280)
!362 = !DILocation(line: 181, column: 14, scope: !280)
!363 = !DILocation(line: 181, column: 18, scope: !280)
!364 = !DILocation(line: 182, column: 14, scope: !280)
!365 = !DILocation(line: 182, column: 18, scope: !280)
!366 = !DILocation(line: 183, column: 14, scope: !280)
!367 = !DILocation(line: 183, column: 18, scope: !280)
!368 = !DILocation(line: 184, column: 14, scope: !280)
!369 = !DILocation(line: 184, column: 18, scope: !280)
!370 = !DILocation(line: 185, column: 14, scope: !280)
!371 = !DILocation(line: 185, column: 18, scope: !280)
!372 = !DILocation(line: 186, column: 14, scope: !280)
!373 = !DILocation(line: 186, column: 18, scope: !280)
!374 = !DILocation(line: 187, column: 14, scope: !280)
!375 = !DILocation(line: 187, column: 18, scope: !280)
!376 = !DILocation(line: 188, column: 14, scope: !280)
!377 = !DILocation(line: 188, column: 18, scope: !280)
!378 = !DILocation(line: 189, column: 14, scope: !280)
!379 = !DILocation(line: 189, column: 18, scope: !280)
!380 = !DILocation(line: 190, column: 14, scope: !280)
!381 = !DILocation(line: 190, column: 18, scope: !280)
!382 = !DILocation(line: 191, column: 14, scope: !280)
!383 = !DILocation(line: 191, column: 18, scope: !280)
!384 = !DILocation(line: 192, column: 14, scope: !280)
!385 = !DILocation(line: 192, column: 18, scope: !280)
!386 = !DILocation(line: 193, column: 14, scope: !280)
!387 = !DILocation(line: 193, column: 18, scope: !280)
!388 = !DILocation(line: 194, column: 14, scope: !280)
!389 = !DILocation(line: 194, column: 18, scope: !280)
!390 = !DILocation(line: 195, column: 14, scope: !280)
!391 = !DILocation(line: 195, column: 18, scope: !280)
!392 = !DILocation(line: 196, column: 14, scope: !280)
!393 = !DILocation(line: 196, column: 18, scope: !280)
!394 = !DILocation(line: 197, column: 14, scope: !280)
!395 = !DILocation(line: 197, column: 18, scope: !280)
!396 = !DILocation(line: 198, column: 14, scope: !280)
!397 = !DILocation(line: 198, column: 18, scope: !280)
!398 = !DILocation(line: 199, column: 14, scope: !280)
!399 = !DILocation(line: 199, column: 18, scope: !280)
!400 = !DILocation(line: 200, column: 14, scope: !280)
!401 = !DILocation(line: 200, column: 18, scope: !280)
!402 = !DILocation(line: 0, scope: !280)
!403 = !DILocation(line: 202, column: 2, scope: !277)
!404 = !DILocation(line: 138, column: 19, scope: !278)
!405 = !DILocation(line: 138, column: 13, scope: !278)
!406 = distinct !{!406, !274, !407, !268}
!407 = !DILocation(line: 202, column: 2, scope: !275)
!408 = !DILocation(line: 203, column: 2, scope: !270)
!409 = distinct !DISubprogram(name: "swi10", scope: !1, file: !1, line: 207, type: !7, scopeLine: 208, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !10)
!410 = !DILocalVariable(name: "c", arg: 1, scope: !409, file: !1, line: 207, type: !9)
!411 = !DILocation(line: 0, scope: !409)
!412 = !DILocalVariable(name: "i", scope: !409, file: !1, line: 209, type: !9)
!413 = !DILocation(line: 211, column: 2, scope: !414)
!414 = distinct !DILexicalBlock(scope: !409, file: !1, line: 211, column: 2)
!415 = !DILocation(line: 212, column: 3, scope: !416)
!416 = distinct !DILexicalBlock(scope: !417, file: !1, line: 211, column: 23)
!417 = distinct !DILexicalBlock(scope: !414, file: !1, line: 211, column: 2)
!418 = !DILocation(line: 213, column: 13, scope: !419)
!419 = distinct !DILexicalBlock(scope: !416, file: !1, line: 212, column: 14)
!420 = !DILocation(line: 213, column: 17, scope: !419)
!421 = !DILocation(line: 214, column: 13, scope: !419)
!422 = !DILocation(line: 214, column: 17, scope: !419)
!423 = !DILocation(line: 215, column: 13, scope: !419)
!424 = !DILocation(line: 215, column: 17, scope: !419)
!425 = !DILocation(line: 216, column: 13, scope: !419)
!426 = !DILocation(line: 216, column: 17, scope: !419)
!427 = !DILocation(line: 217, column: 13, scope: !419)
!428 = !DILocation(line: 217, column: 17, scope: !419)
!429 = !DILocation(line: 218, column: 13, scope: !419)
!430 = !DILocation(line: 218, column: 17, scope: !419)
!431 = !DILocation(line: 219, column: 13, scope: !419)
!432 = !DILocation(line: 219, column: 17, scope: !419)
!433 = !DILocation(line: 220, column: 13, scope: !419)
!434 = !DILocation(line: 220, column: 17, scope: !419)
!435 = !DILocation(line: 221, column: 13, scope: !419)
!436 = !DILocation(line: 221, column: 17, scope: !419)
!437 = !DILocation(line: 222, column: 13, scope: !419)
!438 = !DILocation(line: 222, column: 17, scope: !419)
!439 = !DILocation(line: 223, column: 14, scope: !419)
!440 = !DILocation(line: 223, column: 18, scope: !419)
!441 = !DILocation(line: 0, scope: !419)
!442 = !DILocation(line: 225, column: 2, scope: !416)
!443 = !DILocation(line: 211, column: 19, scope: !417)
!444 = !DILocation(line: 211, column: 13, scope: !417)
!445 = distinct !{!445, !413, !446, !268}
!446 = !DILocation(line: 225, column: 2, scope: !414)
!447 = !DILocation(line: 226, column: 2, scope: !409)
!448 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 229, type: !449, scopeLine: 230, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !10)
!449 = !DISubroutineType(types: !450)
!450 = !{!9}
!451 = !DILocalVariable(name: "cnt", scope: !448, file: !1, line: 231, type: !452)
!452 = !DIDerivedType(tag: DW_TAG_volatile_type, baseType: !9)
!453 = !DILocation(line: 231, column: 15, scope: !448)
!454 = !DILocation(line: 233, column: 12, scope: !448)
!455 = !DILocation(line: 233, column: 6, scope: !448)
!456 = !DILocation(line: 233, column: 5, scope: !448)
!457 = !DILocation(line: 234, column: 12, scope: !448)
!458 = !DILocation(line: 234, column: 6, scope: !448)
!459 = !DILocation(line: 234, column: 5, scope: !448)
!460 = !DILocation(line: 235, column: 13, scope: !448)
!461 = !DILocation(line: 235, column: 6, scope: !448)
!462 = !DILocation(line: 235, column: 5, scope: !448)
!463 = !DILocation(line: 239, column: 9, scope: !448)
!464 = !DILocation(line: 239, column: 2, scope: !448)
