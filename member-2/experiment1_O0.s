	.file	"experiment1.cpp"
	.intel_syntax noprefix
	.text
	.globl	"_Z3fooii"
	.def	"_Z3fooii";	.scl	2;	.type	32;	.endef
	.seh_proc	"_Z3fooii"
"_Z3fooii":
.LFB2790:
	push	rbp
	.seh_pushreg	rbp
	mov	rbp, rsp
	.seh_setframe	rbp, 0
	sub	rsp, 16
	.seh_stackalloc	16
	.seh_endprologue
	mov	DWORD PTR 16[rbp], ecx
	mov	DWORD PTR 24[rbp], edx
	mov	eax, DWORD PTR 16[rbp]
	add	eax, 1
	mov	DWORD PTR -4[rbp], eax
	mov	eax, DWORD PTR 24[rbp]
	add	eax, 2
	mov	DWORD PTR -8[rbp], eax
	mov	eax, DWORD PTR -4[rbp]
	imul	eax, DWORD PTR -8[rbp]
	add	rsp, 16
	pop	rbp
	ret
	.seh_endproc
	.globl	"main"
	.def	"main";	.scl	2;	.type	32;	.endef
	.seh_proc	"main"
"main":
.LFB2791:
	push	rbp
	.seh_pushreg	rbp
	mov	rbp, rsp
	.seh_setframe	rbp, 0
	sub	rsp, 32
	.seh_stackalloc	32
	.seh_endprologue
	call	"__main"
	mov	edx, 20
	mov	ecx, 10
	call	"_Z3fooii"
	mov	edx, eax
	mov	rax, QWORD PTR .refptr._ZSt4cout[rip]
	mov	rcx, rax
	call	"_ZNSolsEi"
	mov	edx, 10
	mov	rcx, rax
	call	"_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_c"
	mov	eax, 0
	add	rsp, 32
	pop	rbp
	ret
	.seh_endproc
	.def	"__main";	.scl	2;	.type	32;	.endef
	.ident	"GCC: (Rev3, Built by MSYS2 project) 16.2.0"
	.def	"_ZNSolsEi";	.scl	2;	.type	32;	.endef
	.def	"_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_c";	.scl	2;	.type	32;	.endef
	.section	.rdata$.refptr._ZSt4cout, "dr"
	.p2align	3, 0
	.globl	.refptr._ZSt4cout
	.linkonce	discard
.refptr._ZSt4cout:
	.quad	"_ZSt4cout"
