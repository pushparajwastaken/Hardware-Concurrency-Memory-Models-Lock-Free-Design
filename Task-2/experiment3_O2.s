	.file	"experiment3.cpp"
	.intel_syntax noprefix
	.text
	.p2align 4
	.globl	"_Z11branch_testi"
	.def	"_Z11branch_testi";	.scl	2;	.type	32;	.endef
	.seh_proc	"_Z11branch_testi"
"_Z11branch_testi":
.LFB2832:
	.seh_endprologue
	lea	edx, 10[rcx]
	lea	eax, -10[rcx]
	cmp	ecx, 101
	cmovge	eax, edx
	ret
	.seh_endproc
	.ident	"GCC: (Rev3, Built by MSYS2 project) 16.2.0"
