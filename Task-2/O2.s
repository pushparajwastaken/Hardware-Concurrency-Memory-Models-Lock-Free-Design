	.file	"main.cpp"
	.intel_syntax noprefix
	.text
	.p2align 4
	.globl	"_Z3addii"
	.def	"_Z3addii";	.scl	2;	.type	32;	.endef
	.seh_proc	"_Z3addii"
"_Z3addii":
.LFB2832:
	.seh_endprologue
	lea	eax, [rcx+rdx]
	ret
	.seh_endproc
	.section	.text.startup,"x"
	.p2align 4
	.globl	"main"
	.def	"main";	.scl	2;	.type	32;	.endef
	.seh_proc	"main"
"main":
.LFB2833:
	sub	rsp, 56
	.seh_stackalloc	56
	.seh_endprologue
	call	"__main"
	mov	rcx, QWORD PTR .refptr._ZSt4cout[rip]
	mov	edx, 30
	call	"_ZNSolsEi"
	mov	BYTE PTR 47[rsp], 10
	mov	rdx, QWORD PTR [rax]
	mov	rdx, QWORD PTR -24[rdx]
	cmp	QWORD PTR 16[rax+rdx], 0
	je	.L4
	lea	rdx, 47[rsp]
	mov	r8d, 1
	mov	rcx, rax
	call	"_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_x"
.L5:
	xor	eax, eax
	add	rsp, 56
	ret
.L4:
	mov	edx, 10
	mov	rcx, rax
	call	"_ZNSo3putEc"
	jmp	.L5
	.seh_endproc
	.def	"__main";	.scl	2;	.type	32;	.endef
	.ident	"GCC: (Rev3, Built by MSYS2 project) 16.2.0"
	.def	"_ZNSolsEi";	.scl	2;	.type	32;	.endef
	.def	"_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_x";	.scl	2;	.type	32;	.endef
	.def	"_ZNSo3putEc";	.scl	2;	.type	32;	.endef
	.section	.rdata$.refptr._ZSt4cout, "dr"
	.p2align	3, 0
	.globl	.refptr._ZSt4cout
	.linkonce	discard
.refptr._ZSt4cout:
	.quad	"_ZSt4cout"
