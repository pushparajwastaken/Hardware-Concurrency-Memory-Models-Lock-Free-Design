	.file	"experiment2.cpp"
	.intel_syntax noprefix
	.text
	.section	.text$_ZNKSt5ctypeIcE8do_widenEc,"x"
	.linkonce discard
	.align 2
	.p2align 4
	.globl	"_ZNKSt5ctypeIcE8do_widenEc"
	.def	"_ZNKSt5ctypeIcE8do_widenEc";	.scl	2;	.type	32;	.endef
	.seh_proc	"_ZNKSt5ctypeIcE8do_widenEc"
"_ZNKSt5ctypeIcE8do_widenEc":
.LFB2576:
	.seh_endprologue
	mov	eax, edx
	ret
	.seh_endproc
	.text
	.p2align 4
	.def	"_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_.isra.0";	.scl	3;	.type	32;	.endef
	.seh_proc	"_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_.isra.0"
"_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_.isra.0":
.LFB3428:
	push	rbx
	.seh_pushreg	rbx
	sub	rsp, 48
	.seh_stackalloc	48
	.seh_endprologue
	mov	rax, QWORD PTR [rcx]
	mov	rax, QWORD PTR -24[rax]
	mov	rbx, rcx
	mov	rcx, QWORD PTR 240[rcx+rax]
	test	rcx, rcx
	je	.L8
	cmp	BYTE PTR 56[rcx], 0
	je	.L5
	movsx	edx, BYTE PTR 67[rcx]
.L6:
	mov	rcx, rbx
	call	"_ZNSo3putEc"
	mov	rcx, rax
	add	rsp, 48
	pop	rbx
	jmp	"_ZNSo5flushEv"
.L5:
	mov	QWORD PTR 40[rsp], rcx
	call	"_ZNKSt5ctypeIcE13_M_widen_initEv"
	mov	rcx, QWORD PTR 40[rsp]
	mov	edx, 10
	lea	r8, "_ZNKSt5ctypeIcE8do_widenEc"[rip]
	mov	rax, QWORD PTR [rcx]
	mov	rax, QWORD PTR 48[rax]
	cmp	rax, r8
	je	.L6
	mov	edx, 10
	call	rax
	movsx	edx, al
	jmp	.L6
.L8:
	call	"_ZSt16__throw_bad_castv"
	nop
	.seh_endproc
	.p2align 4
	.globl	"_Z11independentiiii"
	.def	"_Z11independentiiii";	.scl	2;	.type	32;	.endef
	.seh_proc	"_Z11independentiiii"
"_Z11independentiiii":
.LFB2832:
	.seh_endprologue
	lea	eax, [rcx+rdx]
	add	r8d, r9d
	imul	eax, r8d
	ret
	.seh_endproc
	.p2align 4
	.globl	"_Z9dependentii"
	.def	"_Z9dependentii";	.scl	2;	.type	32;	.endef
	.seh_proc	"_Z9dependentii"
"_Z9dependentii":
.LFB2833:
	.seh_endprologue
	lea	eax, 10[rcx+rdx]
	add	eax, eax
	ret
	.seh_endproc
	.section	.text.startup,"x"
	.p2align 4
	.globl	"main"
	.def	"main";	.scl	2;	.type	32;	.endef
	.seh_proc	"main"
"main":
.LFB2834:
	push	rbx
	.seh_pushreg	rbx
	sub	rsp, 32
	.seh_stackalloc	32
	.seh_endprologue
	call	"__main"
	mov	rbx, QWORD PTR .refptr._ZSt4cout[rip]
	mov	edx, 168
	mov	rcx, rbx
	call	"_ZNSolsEi"
	mov	rcx, rax
	call	"_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_.isra.0"
	mov	edx, 80
	mov	rcx, rbx
	call	"_ZNSolsEi"
	mov	rcx, rax
	call	"_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_.isra.0"
	xor	eax, eax
	add	rsp, 32
	pop	rbx
	ret
	.seh_endproc
	.def	"__main";	.scl	2;	.type	32;	.endef
	.ident	"GCC: (Rev3, Built by MSYS2 project) 16.2.0"
	.def	"_ZNSo3putEc";	.scl	2;	.type	32;	.endef
	.def	"_ZNSo5flushEv";	.scl	2;	.type	32;	.endef
	.def	"_ZNKSt5ctypeIcE13_M_widen_initEv";	.scl	2;	.type	32;	.endef
	.def	"_ZSt16__throw_bad_castv";	.scl	2;	.type	32;	.endef
	.def	"_ZNSolsEi";	.scl	2;	.type	32;	.endef
	.section	.rdata$.refptr._ZSt4cout, "dr"
	.p2align	3, 0
	.globl	.refptr._ZSt4cout
	.linkonce	discard
.refptr._ZSt4cout:
	.quad	"_ZSt4cout"
