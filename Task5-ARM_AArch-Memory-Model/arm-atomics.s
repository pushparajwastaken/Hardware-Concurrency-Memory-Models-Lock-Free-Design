	.arch armv8-a
	.file	"arm-atomics.cpp"
	.text
	.align	2
	.p2align 4,,11
	.global	load_relaxed
	.type	load_relaxed, %function
load_relaxed:
.LFB313:
	.cfi_startproc
	adrp	x0, .LANCHOR0
	add	x0, x0, :lo12:.LANCHOR0
	ldr	w0, [x0]
	ret
	.cfi_endproc
.LFE313:
	.size	load_relaxed, .-load_relaxed
	.align	2
	.p2align 4,,11
	.global	load_acquire
	.type	load_acquire, %function
load_acquire:
.LFB314:
	.cfi_startproc
	adrp	x0, .LANCHOR0
	add	x0, x0, :lo12:.LANCHOR0
	ldar	w0, [x0]
	ret
	.cfi_endproc
.LFE314:
	.size	load_acquire, .-load_acquire
	.align	2
	.p2align 4,,11
	.global	load_seq_cst
	.type	load_seq_cst, %function
load_seq_cst:
.LFB315:
	.cfi_startproc
	adrp	x0, .LANCHOR0
	add	x0, x0, :lo12:.LANCHOR0
	ldar	w0, [x0]
	ret
	.cfi_endproc
.LFE315:
	.size	load_seq_cst, .-load_seq_cst
	.align	2
	.p2align 4,,11
	.global	store_relaxed
	.type	store_relaxed, %function
store_relaxed:
.LFB316:
	.cfi_startproc
	adrp	x1, .LANCHOR0
	add	x1, x1, :lo12:.LANCHOR0
	str	w0, [x1]
	ret
	.cfi_endproc
.LFE316:
	.size	store_relaxed, .-store_relaxed
	.align	2
	.p2align 4,,11
	.global	store_release
	.type	store_release, %function
store_release:
.LFB317:
	.cfi_startproc
	adrp	x1, .LANCHOR0
	add	x1, x1, :lo12:.LANCHOR0
	stlr	w0, [x1]
	ret
	.cfi_endproc
.LFE317:
	.size	store_release, .-store_release
	.align	2
	.p2align 4,,11
	.global	store_seq_cst
	.type	store_seq_cst, %function
store_seq_cst:
.LFB318:
	.cfi_startproc
	adrp	x1, .LANCHOR0
	add	x1, x1, :lo12:.LANCHOR0
	stlr	w0, [x1]
	ret
	.cfi_endproc
.LFE318:
	.size	store_seq_cst, .-store_seq_cst
	.global	__aarch64_ldadd4_relax
	.align	2
	.p2align 4,,11
	.global	fetch_add_relaxed
	.type	fetch_add_relaxed, %function
fetch_add_relaxed:
.LFB319:
	.cfi_startproc
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	adrp	x1, .LANCHOR0
	mov	w0, 1
	mov	x29, sp
	add	x1, x1, :lo12:.LANCHOR0
	bl	__aarch64_ldadd4_relax
	ldp	x29, x30, [sp], 16
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE319:
	.size	fetch_add_relaxed, .-fetch_add_relaxed
	.global	__aarch64_ldadd4_acq_rel
	.align	2
	.p2align 4,,11
	.global	fetch_add_acq_rel
	.type	fetch_add_acq_rel, %function
fetch_add_acq_rel:
.LFB320:
	.cfi_startproc
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	adrp	x1, .LANCHOR0
	mov	w0, 1
	mov	x29, sp
	add	x1, x1, :lo12:.LANCHOR0
	bl	__aarch64_ldadd4_acq_rel
	ldp	x29, x30, [sp], 16
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE320:
	.size	fetch_add_acq_rel, .-fetch_add_acq_rel
	.align	2
	.p2align 4,,11
	.global	fetch_add_seq_cst
	.type	fetch_add_seq_cst, %function
fetch_add_seq_cst:
.LFB321:
	.cfi_startproc
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	adrp	x1, .LANCHOR0
	mov	w0, 1
	mov	x29, sp
	add	x1, x1, :lo12:.LANCHOR0
	bl	__aarch64_ldadd4_acq_rel
	ldp	x29, x30, [sp], 16
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE321:
	.size	fetch_add_seq_cst, .-fetch_add_seq_cst
	.global	__aarch64_swp4_acq_rel
	.align	2
	.p2align 4,,11
	.global	exchange_seq_cst
	.type	exchange_seq_cst, %function
exchange_seq_cst:
.LFB322:
	.cfi_startproc
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	adrp	x1, .LANCHOR0
	add	x1, x1, :lo12:.LANCHOR0
	mov	x29, sp
	bl	__aarch64_swp4_acq_rel
	ldp	x29, x30, [sp], 16
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE322:
	.size	exchange_seq_cst, .-exchange_seq_cst
	.global	__aarch64_cas4_acq_rel
	.align	2
	.p2align 4,,11
	.global	cas_seq_cst
	.type	cas_seq_cst, %function
cas_seq_cst:
.LFB323:
	.cfi_startproc
	stp	x29, x30, [sp, -32]!
	.cfi_def_cfa_offset 32
	.cfi_offset 29, -32
	.cfi_offset 30, -24
	adrp	x2, .LANCHOR0
	add	x2, x2, :lo12:.LANCHOR0
	mov	x29, sp
	str	x19, [sp, 16]
	.cfi_offset 19, -16
	mov	w19, w0
	bl	__aarch64_cas4_acq_rel
	cmp	w0, w19
	ldr	x19, [sp, 16]
	cset	w0, eq
	ldp	x29, x30, [sp], 32
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 19
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE323:
	.size	cas_seq_cst, .-cas_seq_cst
	.align	2
	.p2align 4,,11
	.global	fence_seq_cst
	.type	fence_seq_cst, %function
fence_seq_cst:
.LFB324:
	.cfi_startproc
	dmb	ish
	ret
	.cfi_endproc
.LFE324:
	.size	fence_seq_cst, .-fence_seq_cst
	.align	2
	.p2align 4,,11
	.global	fence_acquire
	.type	fence_acquire, %function
fence_acquire:
.LFB325:
	.cfi_startproc
	dmb	ishld
	ret
	.cfi_endproc
.LFE325:
	.size	fence_acquire, .-fence_acquire
	.align	2
	.p2align 4,,11
	.global	fence_release
	.type	fence_release, %function
fence_release:
.LFB326:
	.cfi_startproc
	dmb	ish
	ret
	.cfi_endproc
.LFE326:
	.size	fence_release, .-fence_release
	.align	2
	.p2align 4,,11
	.global	compiler_only_barrier
	.type	compiler_only_barrier, %function
compiler_only_barrier:
.LFB327:
	.cfi_startproc
	ret
	.cfi_endproc
.LFE327:
	.size	compiler_only_barrier, .-compiler_only_barrier
	.align	2
	.p2align 4,,11
	.global	dmb_ish
	.type	dmb_ish, %function
dmb_ish:
.LFB328:
	.cfi_startproc
#APP
// 49 "/mnt/user-data/outputs/arm-atomics.cpp" 1
	dmb ish
// 0 "" 2
#NO_APP
	ret
	.cfi_endproc
.LFE328:
	.size	dmb_ish, .-dmb_ish
	.align	2
	.p2align 4,,11
	.global	dmb_ishld
	.type	dmb_ishld, %function
dmb_ishld:
.LFB329:
	.cfi_startproc
#APP
// 50 "/mnt/user-data/outputs/arm-atomics.cpp" 1
	dmb ishld
// 0 "" 2
#NO_APP
	ret
	.cfi_endproc
.LFE329:
	.size	dmb_ishld, .-dmb_ishld
	.align	2
	.p2align 4,,11
	.global	dmb_ishst
	.type	dmb_ishst, %function
dmb_ishst:
.LFB330:
	.cfi_startproc
#APP
// 51 "/mnt/user-data/outputs/arm-atomics.cpp" 1
	dmb ishst
// 0 "" 2
#NO_APP
	ret
	.cfi_endproc
.LFE330:
	.size	dmb_ishst, .-dmb_ishst
	.align	2
	.p2align 4,,11
	.global	sb_thread_relaxed
	.type	sb_thread_relaxed, %function
sb_thread_relaxed:
.LFB331:
	.cfi_startproc
	adrp	x0, .LANCHOR0
	add	x0, x0, :lo12:.LANCHOR0
	add	x1, x0, 8
	mov	w2, 1
	str	w2, [x1]
	add	x0, x0, 16
	ldr	w0, [x0]
	ret
	.cfi_endproc
.LFE331:
	.size	sb_thread_relaxed, .-sb_thread_relaxed
	.align	2
	.p2align 4,,11
	.global	sb_thread_seq_cst
	.type	sb_thread_seq_cst, %function
sb_thread_seq_cst:
.LFB332:
	.cfi_startproc
	adrp	x0, .LANCHOR0
	add	x0, x0, :lo12:.LANCHOR0
	add	x1, x0, 8
	mov	w2, 1
	stlr	w2, [x1]
	add	x0, x0, 16
	ldar	w0, [x0]
	ret
	.cfi_endproc
.LFE332:
	.size	sb_thread_seq_cst, .-sb_thread_seq_cst
	.align	2
	.p2align 4,,11
	.global	sb_thread_fence
	.type	sb_thread_fence, %function
sb_thread_fence:
.LFB333:
	.cfi_startproc
	adrp	x0, .LANCHOR0
	add	x0, x0, :lo12:.LANCHOR0
	add	x1, x0, 8
	mov	w2, 1
	str	w2, [x1]
	dmb	ish
	add	x0, x0, 16
	ldr	w0, [x0]
	ret
	.cfi_endproc
.LFE333:
	.size	sb_thread_fence, .-sb_thread_fence
	.align	2
	.p2align 4,,11
	.global	mp_writer_relaxed
	.type	mp_writer_relaxed, %function
mp_writer_relaxed:
.LFB334:
	.cfi_startproc
	adrp	x0, .LANCHOR0
	add	x0, x0, :lo12:.LANCHOR0
	add	x1, x0, 24
	mov	w2, 1
	str	w2, [x1]
	add	x0, x0, 32
	str	w2, [x0]
	ret
	.cfi_endproc
.LFE334:
	.size	mp_writer_relaxed, .-mp_writer_relaxed
	.align	2
	.p2align 4,,11
	.global	mp_writer_release
	.type	mp_writer_release, %function
mp_writer_release:
.LFB335:
	.cfi_startproc
	adrp	x0, .LANCHOR0
	add	x0, x0, :lo12:.LANCHOR0
	add	x1, x0, 24
	mov	w2, 1
	str	w2, [x1]
	add	x0, x0, 32
	stlr	w2, [x0]
	ret
	.cfi_endproc
.LFE335:
	.size	mp_writer_release, .-mp_writer_release
	.align	2
	.p2align 4,,11
	.global	mp_reader_relaxed
	.type	mp_reader_relaxed, %function
mp_reader_relaxed:
.LFB336:
	.cfi_startproc
	adrp	x1, .LANCHOR0
	add	x1, x1, :lo12:.LANCHOR0
	add	x0, x1, 32
	ldr	w2, [x0]
	add	x1, x1, 24
	ldr	w0, [x1]
	add	w2, w2, w2, lsl 2
	add	w0, w0, w2, lsl 1
	ret
	.cfi_endproc
.LFE336:
	.size	mp_reader_relaxed, .-mp_reader_relaxed
	.align	2
	.p2align 4,,11
	.global	mp_reader_acquire
	.type	mp_reader_acquire, %function
mp_reader_acquire:
.LFB337:
	.cfi_startproc
	adrp	x1, .LANCHOR0
	add	x1, x1, :lo12:.LANCHOR0
	add	x0, x1, 32
	ldar	w2, [x0]
	add	x1, x1, 24
	ldr	w0, [x1]
	add	w2, w2, w2, lsl 2
	add	w0, w0, w2, lsl 1
	ret
	.cfi_endproc
.LFE337:
	.size	mp_reader_acquire, .-mp_reader_acquire
	.global	flag
	.global	data
	.global	Y
	.global	X
	.global	a
	.bss
	.align	3
	.set	.LANCHOR0,. + 0
	.type	a, %object
	.size	a, 4
a:
	.zero	4
	.zero	4
	.type	X, %object
	.size	X, 4
X:
	.zero	4
	.zero	4
	.type	Y, %object
	.size	Y, 4
Y:
	.zero	4
	.zero	4
	.type	data, %object
	.size	data, 4
data:
	.zero	4
	.zero	4
	.type	flag, %object
	.size	flag, 4
flag:
	.zero	4
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
