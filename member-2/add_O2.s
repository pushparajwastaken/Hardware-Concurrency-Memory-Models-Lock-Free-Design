	.file	"add.cpp"
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
	sub	rsp, 40
	.seh_stackalloc	40
	.seh_endprologue
	call	"__main"
	mov	rcx, QWORD PTR .refptr._ZSt4cout[rip]
	mov	edx, 11
	call	"_ZNSolsEi"
	xor	eax, eax
	add	rsp, 40
	ret
	.seh_endproc
	.def	"__main";	.scl	2;	.type	32;	.endef
	.ident	"GCC: (Rev3, Built by MSYS2 project) 16.2.0"
	.def	"_ZNSolsEi";	.scl	2;	.type	32;	.endef
	.section	.rdata$.refptr._ZSt4cout, "dr"
	.p2align	3, 0
	.globl	.refptr._ZSt4cout
	.linkonce	discard
.refptr._ZSt4cout:
	.quad	"_ZSt4cout"
