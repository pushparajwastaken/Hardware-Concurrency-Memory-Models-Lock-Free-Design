Benchmark Environment

Processor: 13th Gen Intel(R) Core(TM) i5-13500H (2.60 GHz)
RAM: 16.0 GB (15.7 GB usable)
Operating System: Windows 11
Compiler: MinGW-w64 GCC
Iterations per thread: 1,000,000
Thread counts: 1, 2, 4, 8
Runs per configuration: 4

Objective
The objective of this experiment was to compare the execution performance of Mutex, Spinlock and Atomic synchronization under different levels of thread contention.

Methodology
Each thread performed 1,000,000 increments on a shared counter. The experiment was performed using 1, 2, 4 and 8 threads. Each configuration was executed four times, and execution time was measured using C++ high_resolution_clock. The average of the four runs was used for comparison.

Observation
Execution time increased as the number of threads increased for all three approaches. The increase was most significant for the spinlock.

compartive data >>
1 thread:
Mutex    19.00 ms
Spinlock 13.75 ms
Atomic    6.75 ms

8 threads:
Mutex    558.75 ms
Spinlock 1572.00 ms
Atomic   119.50 ms

<> why spinlock takes more time ??
With more threads competing for the same lock, contention increased. A spinlock keeps waiting threads actively checking the lock instead of blocking, so under higher contention the threads can spend more time repeatedly attempting to acquire the lock. In our experiment, this was reflected by the large increase in spinlock execution time, especially at 4 and 8 threads.
The mutex also showed increasing execution time as contention increased, but its increase was lower than the spinlock in this workload.

MAIN RESULT TABLE (AVG)
| Threads | Mutex (ms) | Spinlock (ms) | Atomic (ms) |
     
| 1       | 19.00      |   13.75         | 6.75      
| 2       | 80.25      |   102.00        | 22.25 
| 4       | 144.00     |   462.00        | 50.00 
| 8       | 558.75     |   1572.00       | 119.50 

conclusion >>>
The experiment showed that synchronization performance changes as the number of competing threads increases. All three approaches showed higher execution time with increasing thread count. The spinlock showed the largest increase, reaching an average of 1572 ms at 8 threads, compared with 558.75 ms for the mutex and 119.50 ms for the atomic implementation. Under the tested workload and hardware, the atomic approach produced the lowest execution time, while the spinlock was most affected by increased contention.

