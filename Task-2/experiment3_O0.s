	.file	"experiment3.cpp"
	.intel_syntax noprefix
	.text
	.globl	"_Z11branch_testi"
	.def	"_Z11branch_testi";	.scl	2;	.type	32;	.endef
	.seh_proc	"_Z11branch_testi"
"_Z11branch_testi":
.LFB2790:
	push	rbp
	.seh_pushreg	rbp
	mov	rbp, rsp
	.seh_setframe	rbp, 0
	.seh_endprologue
	mov	DWORD PTR 16[rbp], ecx
	cmp	DWORD PTR 16[rbp], 100
	jle	.L2
	mov	eax, DWORD PTR 16[rbp]
	add	eax, 10
	jmp	.L3
.L2:
	mov	eax, DWORD PTR 16[rbp]
	sub	eax, 10
.L3:
	pop	rbp
	ret
	.seh_endproc
	.ident	"GCC: (Rev3, Built by MSYS2 project) 16.2.0"
