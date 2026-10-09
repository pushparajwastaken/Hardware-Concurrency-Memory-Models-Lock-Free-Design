# ABA Problem: Technical References

## Primary Academic Sources

1. **Maged M. Michael and Michael L. Scott**, "Simple, Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms," *PODC 1996*. 
   - The canonical reference for the Michael-Scott queue.
   - First paper to formally address ABA in lock-free queues using tagged pointers.
   - DOI: 10.1145/248052.248106

2. **Maged M. Michael**, "Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects," *IEEE Transactions on Parallel and Distributed Systems*, Vol. 15, No. 6, June 2004.
   - The definitive reference for hazard pointers as an ABA solution.
   - DOI: 10.1109/TPDS.2004.8

3. **Maged M. Michael**, "ABA Prevention Using Single-Word Instructions," *IBM Research Report RC23089*, 2004.
   - Shows how to pack a tag into the unused bits of a pointer on 64-bit systems.

4. **Tim Harris**, "A Pragmatic Implementation of Non-Blocking Linked Lists," *DISC 2001*.
   - Practical tagged-pointer implementation for lock-free linked lists.

## Industry Standards

5. **SEI CERT C Coding Standard, CON09-C**: "Avoid the ABA problem when using lock-free algorithms."
   - Provides concrete noncompliant code example (lock-free stack) and compliant solution.
   - URL: https://wiki.sei.cmu.edu/confluence/display/c/CON09-C

6. **C++ Standard (P2530R3)**: `std::hazard_pointer` (C++26).
   - Standard library support for hazard pointers.
   - URL: https://wg21.link/P2530R3

## Intel Architecture Reference

7. **Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 3A, Section 8.2**: Memory Ordering.
   - Explains why x86-64 has strong memory ordering, making `acquire`/`release` cheap.
   - Critical for understanding why tagged pointers are efficient on x86-64.