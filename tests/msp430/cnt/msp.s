	.file	"llvm-link"
	.file	1 "/Users/nilsholscher/workspaces/msp430_template" "src/main.c"
	.text
	.globl	main                            ; -- Begin function main
	.p2align	1
	.type	main,@function
main:                                   ; @main
.Lfunc_begin0:
	.cfi_sections .debug_frame
	.cfi_startproc
; %bb.0:                                ; %entry
	.loc	1 61 4 prologue_end             ; src/main.c:61:4
	call	#InitSeed
	.loc	1 67 4                          ; src/main.c:67:4
	mov	#Array, r12
	call	#Test
	.loc	1 69 4                          ; src/main.c:69:4
	mov	#1, r12
	ret
.Ltmp0:
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        ; -- End function
	.globl	InitSeed                        ; -- Begin function InitSeed
	.p2align	1
	.type	InitSeed,@function
InitSeed:                               ; @InitSeed
.Lfunc_begin1:
	.cfi_startproc
; %bb.0:                                ; %entry
	.loc	1 152 9 prologue_end            ; src/main.c:152:9
	clr	&Seed
	.loc	1 154 4                         ; src/main.c:154:4
	clr	r12
	ret
.Ltmp1:
.Lfunc_end1:
	.size	InitSeed, .Lfunc_end1-InitSeed
	.cfi_endproc
                                        ; -- End function
	.globl	Test                            ; -- Begin function Test
	.p2align	1
	.type	Test,@function
Test:                                   ; @Test
.Lfunc_begin2:
	.loc	1 79 0                          ; src/main.c:79:0
	.cfi_startproc
; %bb.0:                                ; %entry
	;DEBUG_VALUE: Test:Array <- $r12
	push	r10
	.cfi_def_cfa_offset 4
	.cfi_offset r10, -4
	mov	r12, r10
.Ltmp2:
	;DEBUG_VALUE: Test:Array <- $r10
	.loc	1 87 4 prologue_end             ; src/main.c:87:4
	call	#Initialize
.Ltmp3:
	;DEBUG_VALUE: Test:StartTime <- 1000
	.loc	1 91 4                          ; src/main.c:91:4
	mov	r10, r12
	call	#Sum
.Ltmp4:
	;DEBUG_VALUE: Test:StopTime <- 1500
	;DEBUG_VALUE: Test:TotalTime <- undef
	.loc	1 111 4                         ; src/main.c:111:4
	clr	r12
	.loc	1 111 4 epilogue_begin is_stmt 0 ; src/main.c:111:4
	pop	r10
.Ltmp5:
	.cfi_def_cfa_offset 2
	.cfi_restore r10
	ret
.Ltmp6:
.Lfunc_end2:
	.size	Test, .Lfunc_end2-Test
	.cfi_endproc
                                        ; -- End function
	.globl	Initialize                      ; -- Begin function Initialize
	.p2align	1
	.type	Initialize,@function
Initialize:                             ; @Initialize
.Lfunc_begin3:
	.loc	1 123 0 is_stmt 1               ; src/main.c:123:0
	.cfi_startproc
; %bb.0:                                ; %entry
	;DEBUG_VALUE: Initialize:Array <- $r12
	push	r7
	.cfi_def_cfa_offset 4
	push	r8
	.cfi_def_cfa_offset 6
	push	r9
	.cfi_def_cfa_offset 8
	push	r10
	.cfi_def_cfa_offset 10
	.cfi_offset r7, -4
	.cfi_offset r8, -6
	.cfi_offset r9, -8
	.cfi_offset r10, -10
	.loc	1 123 0 prologue_end            ; src/main.c:123:0
	mov	r12, r10
.Ltmp7:
	;DEBUG_VALUE: Initialize:Array <- $r10
	clr	r9
.Ltmp8:
	;DEBUG_VALUE: Initialize:OuterIndex <- 0
.LBB3_1:                                ; %for.body
                                        ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB3_2 Depth 2
	;DEBUG_VALUE: Initialize:OuterIndex <- $r9
	;DEBUG_VALUE: Initialize:InnerIndex <- 0
	clr	r8
.Ltmp9:
.LBB3_2:                                ; %for.body3
                                        ;   Parent Loop BB3_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	;DEBUG_VALUE: Initialize:OuterIndex <- $r9
	;DEBUG_VALUE: Initialize:InnerIndex <- [DW_OP_consts 2, DW_OP_div, DW_OP_stack_value] $r8
	.loc	1 134 40                        ; src/main.c:134:40
	mov	r10, r7
	add	r8, r7
	.loc	1 134 42 is_stmt 0              ; src/main.c:134:42
	call	#RandomInteger
	.loc	1 134 40                        ; src/main.c:134:40
	mov	r12, 0(r7)
.Ltmp10:
	;DEBUG_VALUE: Initialize:InnerIndex <- [DW_OP_consts 2, DW_OP_div, DW_OP_consts 1, DW_OP_plus, DW_OP_stack_value] $r8
	.loc	1 132 39 is_stmt 1              ; src/main.c:132:39
	incd	r8
.Ltmp11:
	.loc	1 132 7 is_stmt 0               ; src/main.c:132:7
	cmp	#20, r8
	jne	.LBB3_2
.Ltmp12:
; %bb.3:                                ; %for.inc5
                                        ;   in Loop: Header=BB3_1 Depth=1
	;DEBUG_VALUE: Initialize:OuterIndex <- $r9
	.loc	1 130 4 is_stmt 1               ; src/main.c:130:4
	add	#20, r10
.Ltmp13:
	.loc	1 130 57 is_stmt 0              ; src/main.c:130:57
	inc	r9
.Ltmp14:
	;DEBUG_VALUE: Initialize:OuterIndex <- $r9
	.loc	1 130 4                         ; src/main.c:130:4
	cmp	#10, r9
	jne	.LBB3_1
.Ltmp15:
; %bb.4:                                ; %for.end7
	;DEBUG_VALUE: Initialize:OuterIndex <- $r9
	.loc	1 138 4 is_stmt 1               ; src/main.c:138:4
	clr	r12
	.loc	1 138 4 epilogue_begin is_stmt 0 ; src/main.c:138:4
	pop	r10
	.cfi_def_cfa_offset 8
	pop	r9
.Ltmp16:
	.cfi_def_cfa_offset 6
	pop	r8
	.cfi_def_cfa_offset 4
	pop	r7
	.cfi_def_cfa_offset 2
	.cfi_restore r7
	.cfi_restore r8
	.cfi_restore r9
	.cfi_restore r10
	ret
.Ltmp17:
.Lfunc_end3:
	.size	Initialize, .Lfunc_end3-Initialize
	.cfi_endproc
                                        ; -- End function
	.globl	Sum                             ; -- Begin function Sum
	.p2align	1
	.type	Sum,@function
Sum:                                    ; @Sum
.Lfunc_begin4:
	.loc	1 162 0 is_stmt 1               ; src/main.c:162:0
	.cfi_startproc
; %bb.0:                                ; %entry
	;DEBUG_VALUE: Sum:Array <- $r12
	push	r8
	.cfi_def_cfa_offset 4
	push	r9
	.cfi_def_cfa_offset 6
	push	r10
	.cfi_def_cfa_offset 8
	.cfi_offset r8, -4
	.cfi_offset r9, -6
	.cfi_offset r10, -8
	.loc	1 162 0 prologue_end            ; src/main.c:162:0
	clr	r13
.Ltmp18:
	;DEBUG_VALUE: Sum:Ptotal <- 0
	;DEBUG_VALUE: Sum:Ntotal <- 0
	;DEBUG_VALUE: Sum:Pcnt <- 0
	;DEBUG_VALUE: Sum:Ncnt <- 0
	;DEBUG_VALUE: Sum:Outer <- 0
	clr	r11
	clr	r14
	clr	r15
	clr	r10
	jmp	.LBB4_2
.Ltmp19:
.LBB4_1:                                ; %for.inc13
                                        ;   in Loop: Header=BB4_2 Depth=1
	;DEBUG_VALUE: Sum:Outer <- $r11
	;DEBUG_VALUE: Sum:Ncnt <- $r13
	;DEBUG_VALUE: Sum:Pcnt <- $r10
	;DEBUG_VALUE: Sum:Ntotal <- $r15
	;DEBUG_VALUE: Sum:Ptotal <- $r14
	;DEBUG_VALUE: Sum:Ptotal <- $r14
	;DEBUG_VALUE: Sum:Ntotal <- $r15
	;DEBUG_VALUE: Sum:Pcnt <- $r10
	;DEBUG_VALUE: Sum:Ncnt <- $r13
	.loc	1 179 3                         ; src/main.c:179:3
	add	#20, r12
.Ltmp20:
	.loc	1 179 41 is_stmt 0              ; src/main.c:179:41
	inc	r11
.Ltmp21:
	;DEBUG_VALUE: Sum:Outer <- $r11
	.loc	1 179 3                         ; src/main.c:179:3
	cmp	#10, r11
	jeq	.LBB4_7
.Ltmp22:
.LBB4_2:                                ; %for.body
                                        ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB4_5 Depth 2
	;DEBUG_VALUE: Sum:Ptotal <- $r14
	;DEBUG_VALUE: Sum:Ntotal <- $r15
	;DEBUG_VALUE: Sum:Pcnt <- $r10
	;DEBUG_VALUE: Sum:Ncnt <- $r13
	;DEBUG_VALUE: Sum:Outer <- $r11
	;DEBUG_VALUE: Sum:Inner <- 0
	.loc	1 0 3                           ; src/main.c:0:3
	clr	r9
	jmp	.LBB4_5
.Ltmp23:
.LBB4_3:                                ; %if.else
                                        ;   in Loop: Header=BB4_5 Depth=2
	;DEBUG_VALUE: Sum:Inner <- [DW_OP_consts 2, DW_OP_div, DW_OP_stack_value] $r9
	;DEBUG_VALUE: Sum:Outer <- $r11
	;DEBUG_VALUE: Sum:Ncnt <- $r13
	;DEBUG_VALUE: Sum:Pcnt <- $r10
	;DEBUG_VALUE: Sum:Ntotal <- $r15
	;DEBUG_VALUE: Sum:Ptotal <- $r14
	.loc	1 201 11 is_stmt 1              ; src/main.c:201:11
	add	0(r8), r15
.Ltmp24:
	;DEBUG_VALUE: Sum:Ntotal <- $r15
	.loc	1 203 8                         ; src/main.c:203:8
	inc	r13
.Ltmp25:
	;DEBUG_VALUE: Sum:Ncnt <- $r13
.LBB4_4:                                ; %for.inc
                                        ;   in Loop: Header=BB4_5 Depth=2
	;DEBUG_VALUE: Sum:Inner <- [DW_OP_consts 2, DW_OP_div, DW_OP_stack_value] $r9
	;DEBUG_VALUE: Sum:Outer <- $r11
	;DEBUG_VALUE: Sum:Ncnt <- $r13
	;DEBUG_VALUE: Sum:Pcnt <- $r10
	;DEBUG_VALUE: Sum:Ntotal <- $r15
	;DEBUG_VALUE: Sum:Ptotal <- $r14
	;DEBUG_VALUE: Sum:Ptotal <- $r14
	;DEBUG_VALUE: Sum:Ntotal <- $r15
	;DEBUG_VALUE: Sum:Pcnt <- $r10
	;DEBUG_VALUE: Sum:Ncnt <- $r13
	;DEBUG_VALUE: Sum:Inner <- [DW_OP_consts 2, DW_OP_div, DW_OP_consts 1, DW_OP_plus, DW_OP_stack_value] $r9
	.loc	1 181 27                        ; src/main.c:181:27
	incd	r9
.Ltmp26:
	.loc	1 181 5 is_stmt 0               ; src/main.c:181:5
	cmp	#20, r9
	jeq	.LBB4_1
.Ltmp27:
.LBB4_5:                                ; %for.body3
                                        ;   Parent Loop BB4_2 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	;DEBUG_VALUE: Sum:Outer <- $r11
	;DEBUG_VALUE: Sum:Ncnt <- $r13
	;DEBUG_VALUE: Sum:Pcnt <- $r10
	;DEBUG_VALUE: Sum:Ntotal <- $r15
	;DEBUG_VALUE: Sum:Ptotal <- $r14
	;DEBUG_VALUE: Sum:Ptotal <- $r14
	;DEBUG_VALUE: Sum:Ntotal <- $r15
	;DEBUG_VALUE: Sum:Pcnt <- $r10
	;DEBUG_VALUE: Sum:Ncnt <- $r13
	;DEBUG_VALUE: Sum:Inner <- [DW_OP_consts 2, DW_OP_div, DW_OP_stack_value] $r9
	.loc	1 189 6 is_stmt 1               ; src/main.c:189:6
	mov	r12, r8
	add	r9, r8
	.loc	1 189 26 is_stmt 0              ; src/main.c:189:26
	tst	0(r8)
	jge	.LBB4_3
.Ltmp28:
; %bb.6:                                ; %if.then
                                        ;   in Loop: Header=BB4_5 Depth=2
	;DEBUG_VALUE: Sum:Inner <- [DW_OP_consts 2, DW_OP_div, DW_OP_stack_value] $r9
	;DEBUG_VALUE: Sum:Outer <- $r11
	;DEBUG_VALUE: Sum:Ncnt <- $r13
	;DEBUG_VALUE: Sum:Pcnt <- $r10
	;DEBUG_VALUE: Sum:Ntotal <- $r15
	;DEBUG_VALUE: Sum:Ptotal <- $r14
	.loc	1 193 11 is_stmt 1              ; src/main.c:193:11
	add	0(r8), r14
.Ltmp29:
	;DEBUG_VALUE: Sum:Ptotal <- $r14
	.loc	1 195 8                         ; src/main.c:195:8
	inc	r10
.Ltmp30:
	;DEBUG_VALUE: Sum:Pcnt <- $r10
	.loc	1 197 2                         ; src/main.c:197:2
	jmp	.LBB4_4
.Ltmp31:
.LBB4_7:                                ; %for.end15
	;DEBUG_VALUE: Sum:Outer <- $r11
	;DEBUG_VALUE: Sum:Ncnt <- $r13
	;DEBUG_VALUE: Sum:Pcnt <- $r10
	;DEBUG_VALUE: Sum:Ntotal <- $r15
	;DEBUG_VALUE: Sum:Ptotal <- $r14
	.loc	1 211 10                        ; src/main.c:211:10
	mov	r10, &Poscnt
	.loc	1 209 12                        ; src/main.c:209:12
	mov	r14, &Postotal
	.loc	1 213 12                        ; src/main.c:213:12
	mov	r15, &Negtotal
	.loc	1 215 10                        ; src/main.c:215:10
	mov	r13, &Negcnt
	.loc	1 217 1 epilogue_begin          ; src/main.c:217:1
	pop	r10
.Ltmp32:
	.cfi_def_cfa_offset 6
	pop	r9
	.cfi_def_cfa_offset 4
	pop	r8
	.cfi_def_cfa_offset 2
	.cfi_restore r8
	.cfi_restore r9
	.cfi_restore r10
	ret
.Ltmp33:
.Lfunc_end4:
	.size	Sum, .Lfunc_end4-Sum
	.cfi_endproc
                                        ; -- End function
	.globl	RandomInteger                   ; -- Begin function RandomInteger
	.p2align	1
	.type	RandomInteger,@function
RandomInteger:                          ; @RandomInteger
.Lfunc_begin5:
	.cfi_startproc
; %bb.0:                                ; %entry
	.loc	1 253 13 prologue_end           ; src/main.c:253:13
	mov	&Seed, r12
	.loc	1 253 18 is_stmt 0              ; src/main.c:253:18
	mov	#133, r13
	call	#__mspabi_mpyi
	.loc	1 253 25                        ; src/main.c:253:25
	add	#81, r12
	.loc	1 253 31                        ; src/main.c:253:31
	mov	#8095, r13
	call	#__mspabi_remi
	.loc	1 253 9                         ; src/main.c:253:9
	mov	r12, &Seed
	.loc	1 255 4 is_stmt 1               ; src/main.c:255:4
	ret
.Ltmp34:
.Lfunc_end5:
	.size	RandomInteger, .Lfunc_end5-RandomInteger
	.cfi_endproc
                                        ; -- End function
	.type	Array,@object                   ; @Array
	.section	.bss,"aw",@nobits
	.globl	Array
	.p2align	1, 0x0
Array:
	.zero	200
	.size	Array, 200

	.type	Seed,@object                    ; @Seed
	.globl	Seed
	.p2align	1, 0x0
Seed:
	.short	0                               ; 0x0
	.size	Seed, 2

	.type	Postotal,@object                ; @Postotal
	.globl	Postotal
	.p2align	1, 0x0
Postotal:
	.short	0                               ; 0x0
	.size	Postotal, 2

	.type	Poscnt,@object                  ; @Poscnt
	.globl	Poscnt
	.p2align	1, 0x0
Poscnt:
	.short	0                               ; 0x0
	.size	Poscnt, 2

	.type	Negtotal,@object                ; @Negtotal
	.globl	Negtotal
	.p2align	1, 0x0
Negtotal:
	.short	0                               ; 0x0
	.size	Negtotal, 2

	.type	Negcnt,@object                  ; @Negcnt
	.globl	Negcnt
	.p2align	1, 0x0
Negcnt:
	.short	0                               ; 0x0
	.size	Negcnt, 2

	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
	.long	.Lfunc_begin2-.Lfunc_begin0
	.long	.Ltmp2-.Lfunc_begin0
	.short	1                               ; Loc expr size
	.byte	92                              ; DW_OP_reg12
	.long	.Ltmp2-.Lfunc_begin0
	.long	.Ltmp5-.Lfunc_begin0
	.short	1                               ; Loc expr size
	.byte	90                              ; DW_OP_reg10
	.long	0
	.long	0
.Ldebug_loc1:
	.long	.Ltmp3-.Lfunc_begin0
	.long	.Lfunc_end2-.Lfunc_begin0
	.short	4                               ; Loc expr size
	.byte	17                              ; DW_OP_consts
	.byte	232                             ; 1000
	.byte	7                               ; 
	.byte	159                             ; DW_OP_stack_value
	.long	0
	.long	0
.Ldebug_loc2:
	.long	.Ltmp4-.Lfunc_begin0
	.long	.Lfunc_end2-.Lfunc_begin0
	.short	4                               ; Loc expr size
	.byte	17                              ; DW_OP_consts
	.byte	220                             ; 1500
	.byte	11                              ; 
	.byte	159                             ; DW_OP_stack_value
	.long	0
	.long	0
.Ldebug_loc3:
	.long	.Lfunc_begin3-.Lfunc_begin0
	.long	.Ltmp7-.Lfunc_begin0
	.short	1                               ; Loc expr size
	.byte	92                              ; DW_OP_reg12
	.long	.Ltmp7-.Lfunc_begin0
	.long	.Ltmp8-.Lfunc_begin0
	.short	1                               ; Loc expr size
	.byte	90                              ; DW_OP_reg10
	.long	0
	.long	0
.Ldebug_loc4:
	.long	.Ltmp8-.Lfunc_begin0
	.long	.Ltmp9-.Lfunc_begin0
	.short	3                               ; Loc expr size
	.byte	17                              ; DW_OP_consts
	.byte	0                               ; 0
	.byte	159                             ; DW_OP_stack_value
	.long	.Ltmp9-.Lfunc_begin0
	.long	.Ltmp10-.Lfunc_begin0
	.short	6                               ; Loc expr size
	.byte	120                             ; DW_OP_breg8
	.byte	0                               ; 0
	.byte	17                              ; DW_OP_consts
	.byte	2                               ; 2
	.byte	27                              ; DW_OP_div
	.byte	159                             ; DW_OP_stack_value
	.long	.Ltmp10-.Lfunc_begin0
	.long	.Ltmp11-.Lfunc_begin0
	.short	9                               ; Loc expr size
	.byte	120                             ; DW_OP_breg8
	.byte	0                               ; 0
	.byte	17                              ; DW_OP_consts
	.byte	2                               ; 2
	.byte	27                              ; DW_OP_div
	.byte	17                              ; DW_OP_consts
	.byte	1                               ; 1
	.byte	34                              ; DW_OP_plus
	.byte	159                             ; DW_OP_stack_value
	.long	0
	.long	0
.Ldebug_loc5:
	.long	.Lfunc_begin4-.Lfunc_begin0
	.long	.Ltmp19-.Lfunc_begin0
	.short	1                               ; Loc expr size
	.byte	92                              ; DW_OP_reg12
	.long	0
	.long	0
.Ldebug_loc6:
	.long	.Ltmp18-.Lfunc_begin0
	.long	.Ltmp19-.Lfunc_begin0
	.short	3                               ; Loc expr size
	.byte	17                              ; DW_OP_consts
	.byte	0                               ; 0
	.byte	159                             ; DW_OP_stack_value
	.long	.Ltmp19-.Lfunc_begin0
	.long	.Lfunc_end4-.Lfunc_begin0
	.short	1                               ; Loc expr size
	.byte	94                              ; DW_OP_reg14
	.long	0
	.long	0
.Ldebug_loc7:
	.long	.Ltmp18-.Lfunc_begin0
	.long	.Ltmp19-.Lfunc_begin0
	.short	3                               ; Loc expr size
	.byte	17                              ; DW_OP_consts
	.byte	0                               ; 0
	.byte	159                             ; DW_OP_stack_value
	.long	.Ltmp19-.Lfunc_begin0
	.long	.Lfunc_end4-.Lfunc_begin0
	.short	1                               ; Loc expr size
	.byte	95                              ; DW_OP_reg15
	.long	0
	.long	0
.Ldebug_loc8:
	.long	.Ltmp18-.Lfunc_begin0
	.long	.Ltmp19-.Lfunc_begin0
	.short	3                               ; Loc expr size
	.byte	17                              ; DW_OP_consts
	.byte	0                               ; 0
	.byte	159                             ; DW_OP_stack_value
	.long	.Ltmp19-.Lfunc_begin0
	.long	.Ltmp32-.Lfunc_begin0
	.short	1                               ; Loc expr size
	.byte	90                              ; DW_OP_reg10
	.long	0
	.long	0
.Ldebug_loc9:
	.long	.Ltmp18-.Lfunc_begin0
	.long	.Ltmp19-.Lfunc_begin0
	.short	3                               ; Loc expr size
	.byte	17                              ; DW_OP_consts
	.byte	0                               ; 0
	.byte	159                             ; DW_OP_stack_value
	.long	.Ltmp19-.Lfunc_begin0
	.long	.Lfunc_end4-.Lfunc_begin0
	.short	1                               ; Loc expr size
	.byte	93                              ; DW_OP_reg13
	.long	0
	.long	0
.Ldebug_loc10:
	.long	.Ltmp18-.Lfunc_begin0
	.long	.Ltmp19-.Lfunc_begin0
	.short	3                               ; Loc expr size
	.byte	17                              ; DW_OP_consts
	.byte	0                               ; 0
	.byte	159                             ; DW_OP_stack_value
	.long	.Ltmp19-.Lfunc_begin0
	.long	.Lfunc_end4-.Lfunc_begin0
	.short	1                               ; Loc expr size
	.byte	91                              ; DW_OP_reg11
	.long	0
	.long	0
.Ldebug_loc11:
	.long	.Ltmp22-.Lfunc_begin0
	.long	.Ltmp23-.Lfunc_begin0
	.short	3                               ; Loc expr size
	.byte	17                              ; DW_OP_consts
	.byte	0                               ; 0
	.byte	159                             ; DW_OP_stack_value
	.long	.Ltmp23-.Lfunc_begin0
	.long	.Ltmp25-.Lfunc_begin0
	.short	6                               ; Loc expr size
	.byte	121                             ; DW_OP_breg9
	.byte	0                               ; 0
	.byte	17                              ; DW_OP_consts
	.byte	2                               ; 2
	.byte	27                              ; DW_OP_div
	.byte	159                             ; DW_OP_stack_value
	.long	.Ltmp25-.Lfunc_begin0
	.long	.Ltmp26-.Lfunc_begin0
	.short	9                               ; Loc expr size
	.byte	121                             ; DW_OP_breg9
	.byte	0                               ; 0
	.byte	17                              ; DW_OP_consts
	.byte	2                               ; 2
	.byte	27                              ; DW_OP_div
	.byte	17                              ; DW_OP_consts
	.byte	1                               ; 1
	.byte	34                              ; DW_OP_plus
	.byte	159                             ; DW_OP_stack_value
	.long	.Ltmp27-.Lfunc_begin0
	.long	.Ltmp31-.Lfunc_begin0
	.short	6                               ; Loc expr size
	.byte	121                             ; DW_OP_breg9
	.byte	0                               ; 0
	.byte	17                              ; DW_OP_consts
	.byte	2                               ; 2
	.byte	27                              ; DW_OP_div
	.byte	159                             ; DW_OP_stack_value
	.long	0
	.long	0
	.section	.debug_abbrev,"",@progbits
	.byte	1                               ; Abbreviation Code
	.byte	17                              ; DW_TAG_compile_unit
	.byte	1                               ; DW_CHILDREN_yes
	.byte	37                              ; DW_AT_producer
	.byte	14                              ; DW_FORM_strp
	.byte	19                              ; DW_AT_language
	.byte	5                               ; DW_FORM_data2
	.byte	3                               ; DW_AT_name
	.byte	14                              ; DW_FORM_strp
	.byte	16                              ; DW_AT_stmt_list
	.byte	23                              ; DW_FORM_sec_offset
	.byte	27                              ; DW_AT_comp_dir
	.byte	14                              ; DW_FORM_strp
	.byte	17                              ; DW_AT_low_pc
	.byte	1                               ; DW_FORM_addr
	.byte	18                              ; DW_AT_high_pc
	.byte	6                               ; DW_FORM_data4
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	2                               ; Abbreviation Code
	.byte	52                              ; DW_TAG_variable
	.byte	0                               ; DW_CHILDREN_no
	.byte	3                               ; DW_AT_name
	.byte	14                              ; DW_FORM_strp
	.byte	73                              ; DW_AT_type
	.byte	19                              ; DW_FORM_ref4
	.byte	63                              ; DW_AT_external
	.byte	25                              ; DW_FORM_flag_present
	.byte	58                              ; DW_AT_decl_file
	.byte	11                              ; DW_FORM_data1
	.byte	59                              ; DW_AT_decl_line
	.byte	11                              ; DW_FORM_data1
	.byte	2                               ; DW_AT_location
	.byte	24                              ; DW_FORM_exprloc
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	3                               ; Abbreviation Code
	.byte	36                              ; DW_TAG_base_type
	.byte	0                               ; DW_CHILDREN_no
	.byte	3                               ; DW_AT_name
	.byte	14                              ; DW_FORM_strp
	.byte	62                              ; DW_AT_encoding
	.byte	11                              ; DW_FORM_data1
	.byte	11                              ; DW_AT_byte_size
	.byte	11                              ; DW_FORM_data1
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	4                               ; Abbreviation Code
	.byte	22                              ; DW_TAG_typedef
	.byte	0                               ; DW_CHILDREN_no
	.byte	73                              ; DW_AT_type
	.byte	19                              ; DW_FORM_ref4
	.byte	3                               ; DW_AT_name
	.byte	14                              ; DW_FORM_strp
	.byte	58                              ; DW_AT_decl_file
	.byte	11                              ; DW_FORM_data1
	.byte	59                              ; DW_AT_decl_line
	.byte	11                              ; DW_FORM_data1
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	5                               ; Abbreviation Code
	.byte	1                               ; DW_TAG_array_type
	.byte	1                               ; DW_CHILDREN_yes
	.byte	73                              ; DW_AT_type
	.byte	19                              ; DW_FORM_ref4
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	6                               ; Abbreviation Code
	.byte	33                              ; DW_TAG_subrange_type
	.byte	0                               ; DW_CHILDREN_no
	.byte	73                              ; DW_AT_type
	.byte	19                              ; DW_FORM_ref4
	.byte	55                              ; DW_AT_count
	.byte	11                              ; DW_FORM_data1
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	7                               ; Abbreviation Code
	.byte	36                              ; DW_TAG_base_type
	.byte	0                               ; DW_CHILDREN_no
	.byte	3                               ; DW_AT_name
	.byte	14                              ; DW_FORM_strp
	.byte	11                              ; DW_AT_byte_size
	.byte	11                              ; DW_FORM_data1
	.byte	62                              ; DW_AT_encoding
	.byte	11                              ; DW_FORM_data1
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	8                               ; Abbreviation Code
	.byte	46                              ; DW_TAG_subprogram
	.byte	0                               ; DW_CHILDREN_no
	.byte	17                              ; DW_AT_low_pc
	.byte	1                               ; DW_FORM_addr
	.byte	18                              ; DW_AT_high_pc
	.byte	6                               ; DW_FORM_data4
	.byte	64                              ; DW_AT_frame_base
	.byte	24                              ; DW_FORM_exprloc
	.byte	3                               ; DW_AT_name
	.byte	14                              ; DW_FORM_strp
	.byte	58                              ; DW_AT_decl_file
	.byte	11                              ; DW_FORM_data1
	.byte	59                              ; DW_AT_decl_line
	.byte	11                              ; DW_FORM_data1
	.byte	39                              ; DW_AT_prototyped
	.byte	25                              ; DW_FORM_flag_present
	.byte	73                              ; DW_AT_type
	.byte	19                              ; DW_FORM_ref4
	.byte	63                              ; DW_AT_external
	.byte	25                              ; DW_FORM_flag_present
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	9                               ; Abbreviation Code
	.byte	46                              ; DW_TAG_subprogram
	.byte	1                               ; DW_CHILDREN_yes
	.byte	17                              ; DW_AT_low_pc
	.byte	1                               ; DW_FORM_addr
	.byte	18                              ; DW_AT_high_pc
	.byte	6                               ; DW_FORM_data4
	.byte	64                              ; DW_AT_frame_base
	.byte	24                              ; DW_FORM_exprloc
	.byte	3                               ; DW_AT_name
	.byte	14                              ; DW_FORM_strp
	.byte	58                              ; DW_AT_decl_file
	.byte	11                              ; DW_FORM_data1
	.byte	59                              ; DW_AT_decl_line
	.byte	11                              ; DW_FORM_data1
	.byte	39                              ; DW_AT_prototyped
	.byte	25                              ; DW_FORM_flag_present
	.byte	73                              ; DW_AT_type
	.byte	19                              ; DW_FORM_ref4
	.byte	63                              ; DW_AT_external
	.byte	25                              ; DW_FORM_flag_present
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	10                              ; Abbreviation Code
	.byte	5                               ; DW_TAG_formal_parameter
	.byte	0                               ; DW_CHILDREN_no
	.byte	2                               ; DW_AT_location
	.byte	23                              ; DW_FORM_sec_offset
	.byte	3                               ; DW_AT_name
	.byte	14                              ; DW_FORM_strp
	.byte	58                              ; DW_AT_decl_file
	.byte	11                              ; DW_FORM_data1
	.byte	59                              ; DW_AT_decl_line
	.byte	11                              ; DW_FORM_data1
	.byte	73                              ; DW_AT_type
	.byte	19                              ; DW_FORM_ref4
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	11                              ; Abbreviation Code
	.byte	52                              ; DW_TAG_variable
	.byte	0                               ; DW_CHILDREN_no
	.byte	2                               ; DW_AT_location
	.byte	23                              ; DW_FORM_sec_offset
	.byte	3                               ; DW_AT_name
	.byte	14                              ; DW_FORM_strp
	.byte	58                              ; DW_AT_decl_file
	.byte	11                              ; DW_FORM_data1
	.byte	59                              ; DW_AT_decl_line
	.byte	11                              ; DW_FORM_data1
	.byte	73                              ; DW_AT_type
	.byte	19                              ; DW_FORM_ref4
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	12                              ; Abbreviation Code
	.byte	52                              ; DW_TAG_variable
	.byte	0                               ; DW_CHILDREN_no
	.byte	2                               ; DW_AT_location
	.byte	24                              ; DW_FORM_exprloc
	.byte	3                               ; DW_AT_name
	.byte	14                              ; DW_FORM_strp
	.byte	58                              ; DW_AT_decl_file
	.byte	11                              ; DW_FORM_data1
	.byte	59                              ; DW_AT_decl_line
	.byte	11                              ; DW_FORM_data1
	.byte	73                              ; DW_AT_type
	.byte	19                              ; DW_FORM_ref4
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	13                              ; Abbreviation Code
	.byte	46                              ; DW_TAG_subprogram
	.byte	1                               ; DW_CHILDREN_yes
	.byte	17                              ; DW_AT_low_pc
	.byte	1                               ; DW_FORM_addr
	.byte	18                              ; DW_AT_high_pc
	.byte	6                               ; DW_FORM_data4
	.byte	64                              ; DW_AT_frame_base
	.byte	24                              ; DW_FORM_exprloc
	.byte	3                               ; DW_AT_name
	.byte	14                              ; DW_FORM_strp
	.byte	58                              ; DW_AT_decl_file
	.byte	11                              ; DW_FORM_data1
	.byte	59                              ; DW_AT_decl_line
	.byte	11                              ; DW_FORM_data1
	.byte	39                              ; DW_AT_prototyped
	.byte	25                              ; DW_FORM_flag_present
	.byte	63                              ; DW_AT_external
	.byte	25                              ; DW_FORM_flag_present
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	14                              ; Abbreviation Code
	.byte	15                              ; DW_TAG_pointer_type
	.byte	0                               ; DW_CHILDREN_no
	.byte	73                              ; DW_AT_type
	.byte	19                              ; DW_FORM_ref4
	.byte	0                               ; EOM(1)
	.byte	0                               ; EOM(2)
	.byte	0                               ; EOM(3)
	.section	.debug_info,"",@progbits
.Lcu_begin0:
	.long	.Ldebug_info_end0-.Ldebug_info_start0 ; Length of Unit
.Ldebug_info_start0:
	.short	4                               ; DWARF version number
	.long	.debug_abbrev                   ; Offset Into Abbrev. Section
	.byte	4                               ; Address Size (in bytes)
	.byte	1                               ; Abbrev [1] 0xb:0x203 DW_TAG_compile_unit
	.long	.Linfo_string0                  ; DW_AT_producer
	.short	12                              ; DW_AT_language
	.long	.Linfo_string1                  ; DW_AT_name
	.long	.Lline_table_start0             ; DW_AT_stmt_list
	.long	.Linfo_string2                  ; DW_AT_comp_dir
	.long	.Lfunc_begin0                   ; DW_AT_low_pc
	.long	.Lfunc_end5-.Lfunc_begin0       ; DW_AT_high_pc
	.byte	2                               ; Abbrev [2] 0x26:0x11 DW_TAG_variable
	.long	.Linfo_string3                  ; DW_AT_name
	.long	55                              ; DW_AT_type
                                        ; DW_AT_external
	.byte	1                               ; DW_AT_decl_file
	.byte	47                              ; DW_AT_decl_line
	.byte	5                               ; DW_AT_location
	.byte	3
	.long	Seed
	.byte	3                               ; Abbrev [3] 0x37:0x7 DW_TAG_base_type
	.long	.Linfo_string4                  ; DW_AT_name
	.byte	5                               ; DW_AT_encoding
	.byte	2                               ; DW_AT_byte_size
	.byte	2                               ; Abbrev [2] 0x3e:0x11 DW_TAG_variable
	.long	.Linfo_string5                  ; DW_AT_name
	.long	79                              ; DW_AT_type
                                        ; DW_AT_external
	.byte	1                               ; DW_AT_decl_file
	.byte	49                              ; DW_AT_decl_line
	.byte	5                               ; DW_AT_location
	.byte	3
	.long	Array
	.byte	4                               ; Abbrev [4] 0x4f:0xb DW_TAG_typedef
	.long	90                              ; DW_AT_type
	.long	.Linfo_string7                  ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	25                              ; DW_AT_decl_line
	.byte	5                               ; Abbrev [5] 0x5a:0x12 DW_TAG_array_type
	.long	55                              ; DW_AT_type
	.byte	6                               ; Abbrev [6] 0x5f:0x6 DW_TAG_subrange_type
	.long	108                             ; DW_AT_type
	.byte	10                              ; DW_AT_count
	.byte	6                               ; Abbrev [6] 0x65:0x6 DW_TAG_subrange_type
	.long	108                             ; DW_AT_type
	.byte	10                              ; DW_AT_count
	.byte	0                               ; End Of Children Mark
	.byte	7                               ; Abbrev [7] 0x6c:0x7 DW_TAG_base_type
	.long	.Linfo_string6                  ; DW_AT_name
	.byte	8                               ; DW_AT_byte_size
	.byte	7                               ; DW_AT_encoding
	.byte	2                               ; Abbrev [2] 0x73:0x11 DW_TAG_variable
	.long	.Linfo_string8                  ; DW_AT_name
	.long	55                              ; DW_AT_type
                                        ; DW_AT_external
	.byte	1                               ; DW_AT_decl_file
	.byte	51                              ; DW_AT_decl_line
	.byte	5                               ; DW_AT_location
	.byte	3
	.long	Postotal
	.byte	2                               ; Abbrev [2] 0x84:0x11 DW_TAG_variable
	.long	.Linfo_string9                  ; DW_AT_name
	.long	55                              ; DW_AT_type
                                        ; DW_AT_external
	.byte	1                               ; DW_AT_decl_file
	.byte	51                              ; DW_AT_decl_line
	.byte	5                               ; DW_AT_location
	.byte	3
	.long	Negtotal
	.byte	2                               ; Abbrev [2] 0x95:0x11 DW_TAG_variable
	.long	.Linfo_string10                 ; DW_AT_name
	.long	55                              ; DW_AT_type
                                        ; DW_AT_external
	.byte	1                               ; DW_AT_decl_file
	.byte	51                              ; DW_AT_decl_line
	.byte	5                               ; DW_AT_location
	.byte	3
	.long	Poscnt
	.byte	2                               ; Abbrev [2] 0xa6:0x11 DW_TAG_variable
	.long	.Linfo_string11                 ; DW_AT_name
	.long	55                              ; DW_AT_type
                                        ; DW_AT_external
	.byte	1                               ; DW_AT_decl_file
	.byte	51                              ; DW_AT_decl_line
	.byte	5                               ; DW_AT_location
	.byte	3
	.long	Negcnt
	.byte	8                               ; Abbrev [8] 0xb7:0x15 DW_TAG_subprogram
	.long	.Lfunc_begin0                   ; DW_AT_low_pc
	.long	.Lfunc_end0-.Lfunc_begin0       ; DW_AT_high_pc
	.byte	1                               ; DW_AT_frame_base
	.byte	81
	.long	.Linfo_string12                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	57                              ; DW_AT_decl_line
                                        ; DW_AT_prototyped
	.long	55                              ; DW_AT_type
                                        ; DW_AT_external
	.byte	8                               ; Abbrev [8] 0xcc:0x15 DW_TAG_subprogram
	.long	.Lfunc_begin1                   ; DW_AT_low_pc
	.long	.Lfunc_end1-.Lfunc_begin1       ; DW_AT_high_pc
	.byte	1                               ; DW_AT_frame_base
	.byte	81
	.long	.Linfo_string13                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	148                             ; DW_AT_decl_line
                                        ; DW_AT_prototyped
	.long	55                              ; DW_AT_type
                                        ; DW_AT_external
	.byte	9                               ; Abbrev [9] 0xe1:0x43 DW_TAG_subprogram
	.long	.Lfunc_begin2                   ; DW_AT_low_pc
	.long	.Lfunc_end2-.Lfunc_begin2       ; DW_AT_high_pc
	.byte	1                               ; DW_AT_frame_base
	.byte	81
	.long	.Linfo_string14                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	77                              ; DW_AT_decl_line
                                        ; DW_AT_prototyped
	.long	55                              ; DW_AT_type
                                        ; DW_AT_external
	.byte	10                              ; Abbrev [10] 0xf6:0xf DW_TAG_formal_parameter
	.long	.Ldebug_loc0                    ; DW_AT_location
	.long	.Linfo_string5                  ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	77                              ; DW_AT_decl_line
	.long	501                             ; DW_AT_type
	.byte	11                              ; Abbrev [11] 0x105:0xf DW_TAG_variable
	.long	.Ldebug_loc1                    ; DW_AT_location
	.long	.Linfo_string18                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	81                              ; DW_AT_decl_line
	.long	518                             ; DW_AT_type
	.byte	11                              ; Abbrev [11] 0x114:0xf DW_TAG_variable
	.long	.Ldebug_loc2                    ; DW_AT_location
	.long	.Linfo_string20                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	81                              ; DW_AT_decl_line
	.long	518                             ; DW_AT_type
	.byte	0                               ; End Of Children Mark
	.byte	9                               ; Abbrev [9] 0x124:0x41 DW_TAG_subprogram
	.long	.Lfunc_begin3                   ; DW_AT_low_pc
	.long	.Lfunc_end3-.Lfunc_begin3       ; DW_AT_high_pc
	.byte	1                               ; DW_AT_frame_base
	.byte	81
	.long	.Linfo_string15                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	121                             ; DW_AT_decl_line
                                        ; DW_AT_prototyped
	.long	55                              ; DW_AT_type
                                        ; DW_AT_external
	.byte	10                              ; Abbrev [10] 0x139:0xf DW_TAG_formal_parameter
	.long	.Ldebug_loc3                    ; DW_AT_location
	.long	.Linfo_string5                  ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	121                             ; DW_AT_decl_line
	.long	501                             ; DW_AT_type
	.byte	12                              ; Abbrev [12] 0x148:0xd DW_TAG_variable
	.byte	1                               ; DW_AT_location
	.byte	89
	.long	.Linfo_string21                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	125                             ; DW_AT_decl_line
	.long	55                              ; DW_AT_type
	.byte	11                              ; Abbrev [11] 0x155:0xf DW_TAG_variable
	.long	.Ldebug_loc4                    ; DW_AT_location
	.long	.Linfo_string22                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	125                             ; DW_AT_decl_line
	.long	55                              ; DW_AT_type
	.byte	0                               ; End Of Children Mark
	.byte	13                              ; Abbrev [13] 0x165:0x7b DW_TAG_subprogram
	.long	.Lfunc_begin4                   ; DW_AT_low_pc
	.long	.Lfunc_end4-.Lfunc_begin4       ; DW_AT_high_pc
	.byte	1                               ; DW_AT_frame_base
	.byte	81
	.long	.Linfo_string16                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	160                             ; DW_AT_decl_line
                                        ; DW_AT_prototyped
                                        ; DW_AT_external
	.byte	10                              ; Abbrev [10] 0x176:0xf DW_TAG_formal_parameter
	.long	.Ldebug_loc5                    ; DW_AT_location
	.long	.Linfo_string5                  ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	160                             ; DW_AT_decl_line
	.long	501                             ; DW_AT_type
	.byte	11                              ; Abbrev [11] 0x185:0xf DW_TAG_variable
	.long	.Ldebug_loc6                    ; DW_AT_location
	.long	.Linfo_string23                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	168                             ; DW_AT_decl_line
	.long	55                              ; DW_AT_type
	.byte	11                              ; Abbrev [11] 0x194:0xf DW_TAG_variable
	.long	.Ldebug_loc7                    ; DW_AT_location
	.long	.Linfo_string24                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	170                             ; DW_AT_decl_line
	.long	55                              ; DW_AT_type
	.byte	11                              ; Abbrev [11] 0x1a3:0xf DW_TAG_variable
	.long	.Ldebug_loc8                    ; DW_AT_location
	.long	.Linfo_string25                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	172                             ; DW_AT_decl_line
	.long	55                              ; DW_AT_type
	.byte	11                              ; Abbrev [11] 0x1b2:0xf DW_TAG_variable
	.long	.Ldebug_loc9                    ; DW_AT_location
	.long	.Linfo_string26                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	174                             ; DW_AT_decl_line
	.long	55                              ; DW_AT_type
	.byte	11                              ; Abbrev [11] 0x1c1:0xf DW_TAG_variable
	.long	.Ldebug_loc10                   ; DW_AT_location
	.long	.Linfo_string27                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	164                             ; DW_AT_decl_line
	.long	55                              ; DW_AT_type
	.byte	11                              ; Abbrev [11] 0x1d0:0xf DW_TAG_variable
	.long	.Ldebug_loc11                   ; DW_AT_location
	.long	.Linfo_string28                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	164                             ; DW_AT_decl_line
	.long	55                              ; DW_AT_type
	.byte	0                               ; End Of Children Mark
	.byte	8                               ; Abbrev [8] 0x1e0:0x15 DW_TAG_subprogram
	.long	.Lfunc_begin5                   ; DW_AT_low_pc
	.long	.Lfunc_end5-.Lfunc_begin5       ; DW_AT_high_pc
	.byte	1                               ; DW_AT_frame_base
	.byte	81
	.long	.Linfo_string17                 ; DW_AT_name
	.byte	1                               ; DW_AT_decl_file
	.byte	249                             ; DW_AT_decl_line
                                        ; DW_AT_prototyped
	.long	55                              ; DW_AT_type
                                        ; DW_AT_external
	.byte	14                              ; Abbrev [14] 0x1f5:0x5 DW_TAG_pointer_type
	.long	506                             ; DW_AT_type
	.byte	5                               ; Abbrev [5] 0x1fa:0xc DW_TAG_array_type
	.long	55                              ; DW_AT_type
	.byte	6                               ; Abbrev [6] 0x1ff:0x6 DW_TAG_subrange_type
	.long	108                             ; DW_AT_type
	.byte	10                              ; DW_AT_count
	.byte	0                               ; End Of Children Mark
	.byte	3                               ; Abbrev [3] 0x206:0x7 DW_TAG_base_type
	.long	.Linfo_string19                 ; DW_AT_name
	.byte	5                               ; DW_AT_encoding
	.byte	4                               ; DW_AT_byte_size
	.byte	0                               ; End Of Children Mark
.Ldebug_info_end0:
	.section	.debug_str,"MS",@progbits,1
.Linfo_string0:
	.asciz	"clang version 22.0.0git (git@github.com:OMA-NVM/llvm-project.git fca9ff2383868608326c25ec508dffeb3c1b30e9)" ; string offset=0
.Linfo_string1:
	.asciz	"src/main.c"                    ; string offset=107
.Linfo_string2:
	.asciz	"/Users/nilsholscher/workspaces/msp430_template" ; string offset=118
.Linfo_string3:
	.asciz	"Seed"                          ; string offset=165
.Linfo_string4:
	.asciz	"int"                           ; string offset=170
.Linfo_string5:
	.asciz	"Array"                         ; string offset=174
.Linfo_string6:
	.asciz	"__ARRAY_SIZE_TYPE__"           ; string offset=180
.Linfo_string7:
	.asciz	"matrix"                        ; string offset=200
.Linfo_string8:
	.asciz	"Postotal"                      ; string offset=207
.Linfo_string9:
	.asciz	"Negtotal"                      ; string offset=216
.Linfo_string10:
	.asciz	"Poscnt"                        ; string offset=225
.Linfo_string11:
	.asciz	"Negcnt"                        ; string offset=232
.Linfo_string12:
	.asciz	"main"                          ; string offset=239
.Linfo_string13:
	.asciz	"InitSeed"                      ; string offset=244
.Linfo_string14:
	.asciz	"Test"                          ; string offset=253
.Linfo_string15:
	.asciz	"Initialize"                    ; string offset=258
.Linfo_string16:
	.asciz	"Sum"                           ; string offset=269
.Linfo_string17:
	.asciz	"RandomInteger"                 ; string offset=273
.Linfo_string18:
	.asciz	"StartTime"                     ; string offset=287
.Linfo_string19:
	.asciz	"long"                          ; string offset=297
.Linfo_string20:
	.asciz	"StopTime"                      ; string offset=302
.Linfo_string21:
	.asciz	"OuterIndex"                    ; string offset=311
.Linfo_string22:
	.asciz	"InnerIndex"                    ; string offset=322
.Linfo_string23:
	.asciz	"Ptotal"                        ; string offset=333
.Linfo_string24:
	.asciz	"Ntotal"                        ; string offset=340
.Linfo_string25:
	.asciz	"Pcnt"                          ; string offset=347
.Linfo_string26:
	.asciz	"Ncnt"                          ; string offset=352
.Linfo_string27:
	.asciz	"Outer"                         ; string offset=357
.Linfo_string28:
	.asciz	"Inner"                         ; string offset=363
	.ident	"clang version 22.0.0git (git@github.com:OMA-NVM/llvm-project.git fca9ff2383868608326c25ec508dffeb3c1b30e9)"
	.section	".note.GNU-stack","",@progbits
	.section	.debug_line,"",@progbits
.Lline_table_start0:
