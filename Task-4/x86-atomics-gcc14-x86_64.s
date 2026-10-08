# Compiler: GCC 14.2.0 (Debian 14.2.0-19)
# Command: g++ -std=c++20 -O2 -S -masm=intel x86-atomics.cpp
# Target: compiler default x86-64 Linux target; no explicit -march.
# Generated from x86-atomics.cpp; unrelated directives and function metadata omitted.

load_relaxed:
    mov eax, DWORD PTR x[rip]
    ret

load_acquire:
    mov eax, DWORD PTR x[rip]
    ret

store_relaxed:
    mov DWORD PTR x[rip], edi
    ret

store_release:
    mov DWORD PTR x[rip], edi
    ret

store_sc:
    xchg edi, DWORD PTR x[rip]
    ret

fetch_add_relaxed:
    mov eax, edi
    lock xadd DWORD PTR x[rip], eax
    ret

fetch_add_sc:
    mov eax, edi
    lock xadd DWORD PTR x[rip], eax
    ret

fence_sc:
    lock or QWORD PTR [rsp], 0
    ret
