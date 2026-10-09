Task : mutex/spinlock/contention

THE Main problem 

What is race condition ?
A race condition is a concurrency bug that occurs when the outcome of a program depends on the unpredictable timing or sequence of execution among multiple threads

also,
A race condition occurs when multiple threads access and modify shared data simultaneously without proper synchronization. Since the execution order depends on thread scheduling, the program may produce different outputs on different runs.

implementation of race condition in c++

#include <iostream>
#include <thread>
using namespace std;
int counter = 0;
void increment() {
    for (int i = 0; i < 1000000; i++) {
        counter++;
    }
}
int main() {
    thread t1(increment);
    thread t2(increment);
    t1.join();
    t2.join();
    cout << counter;
}

some outputs after running it 
1. 1000000
2. 195582
3. 173522

Eliminating the Race Condition
A race condition can be prevented by protecting the critical section with a mutex, ensuring that only one thread accesses the shared resource at a time.

Mutex = one thread goes inside critical section at a time and another thread will wait to come out for the first one .
Explanation: The mutex ensures that only one thread updates the shared counter at a time, preventing lost updates and producing a consistent output.

RESEARCH 
1. Mutex
A mutex (Mutual Exclusion) is a synchronization mechanism used to ensure that only one thread can access a shared resource or critical section at a time. When a thread acquires a mutex, other threads trying to acquire the same mutex must wait until the current owner releases it. This prevents race conditions and maintains data consistency in multithreaded programs.
Mutex algorithms are designed around important properties such as mutual exclusion, deadlock-freedom, starvation-freedom, and fairness. The choice of mutex implementation can affect system performance, especially when multiple threads compete for the same resource.
A major advantage of a mutex is that a waiting thread can be blocked and removed from CPU execution, allowing the processor to perform other useful work. However, blocking and waking a thread may introduce scheduling and context-switch overhead. Therefore, mutexes are generally useful when the critical section may take relatively longer to execute.
Reference
Raynal, M., & Taubenfeld, G. (2022). A visit to mutual exclusion in seven dates. Theoretical Computer Science, 919, 47–65.
Real Paper — ScienceDirect


implementing mutex in c++
#include <iostream>
#include <mutex>
#include <thread>
using namespace std;
int counter = 0;
mutex mtx;
void increment()
{
    for (int i = 0; i < 1000000; i++) {
        lock_guard<mutex> lock(mtx);
        counter++;
    }
}
int main()
{
    thread t1(increment);
    thread t2(increment);
    t1.join();
    t2.join();
    cout << "Counter = " << counter << endl;
    return 0;
}

O/P
1. 2000000 every time 



2. Spinlock
A spinlock is a synchronization mechanism in which a thread repeatedly checks whether a lock is available instead of immediately going to sleep. This repeated checking is called spinning or busy waiting.
Spinlocks can provide very low waiting latency when the critical section is extremely short because the waiting thread does not need to perform a context switch. However, the processor continues executing the waiting thread, which means CPU cycles are consumed even though the thread is not performing useful work.
The performance of a spinlock therefore depends strongly on the duration of the critical section and the amount of contention. With short critical sections and low contention, spinning can be efficient. With long critical sections or many competing threads, excessive spinning can waste CPU resources and increase system contention.
Reference
Anderson, T. E. (1990). The Performance of Spin Lock Alternatives for Shared-Memory Multiprocessors. IEEE Transactions on Parallel and Distributed Systems, 1(1), 6–16.



spinlock implementation in c++

#include <iostream>
#include <atomic>
#include <thread>

using namespace std;

int counter = 0;
atomic_flag lockFlag = ATOMIC_FLAG_INIT;

void spinLock() {
    
    while (lockFlag.test_and_set(memory_order_acquire)) {
        
    }
}

void spinUnlock() {
    lockFlag.clear(memory_order_release);
}

void work(int id) {

    spinLock();
   for(int i=0;i<1000;i++){
    counter++;
   }
    spinUnlock();
}
int main() {

    thread t1(work, 1);
    thread t2(work, 2);

    t1.join();
    t2.join();

    cout<<"count is "<<counter<<endl;

    return 0;
}
j 
o/p 1000 ALWAYS 




3. Blocking
Blocking occurs when a thread cannot continue execution because a required resource is currently unavailable. In synchronization, a thread may become blocked when another thread owns a mutex or when access to a shared resource is unavailable.
When a thread blocks on a mutex, the operating system can suspend that thread and allow another ready thread to use the CPU. Once the resource becomes available, the blocked thread can be awakened and scheduled again.
Blocking can reduce unnecessary CPU consumption compared with continuous spinning. However, it can introduce additional overhead because the operating system may need to perform scheduling, thread state changes, and context switches.
Therefore, there is an important trade-off:
•	Blocking: saves CPU resources but may introduce scheduling overhead.
•	Spinning: avoids some scheduling overhead but consumes CPU cycles while waiting.
Reference
Dinh, S., Li, J., Agrawal, K., Gill, C., & Lu, C. (2018). Blocking Analysis for Spin Locks in Real-Time Parallel Tasks. IEEE Transactions on Parallel and Distributed Systems, 29(4), 789–802.


4. Context Switching
A context switch occurs when the processor stops executing one thread or process and starts executing another. Before switching, the operating system needs to preserve the execution state of the current thread, including information such as registers and program state. It then restores the saved state of the next thread.
Context switching is necessary for multitasking and thread scheduling, but it introduces overhead because the processor spends time managing execution states instead of directly executing application work.
Context-switch overhead can also be affected by the program's memory behavior. The cost may depend on factors such as data size, memory access patterns, cache behavior, and operating-system activity.
In synchronization, context switching is particularly important when comparing mutexes and spinlocks. A blocked mutex may cause a waiting thread to be descheduled, while a spinlock allows the thread to continue executing while repeatedly checking the lock.
Reference
Li, C., Ding, C., & Shen, K. (2007). Quantifying the Cost of Context Switch. Proceedings of the Workshop on Experimental Computer Science.


5. Contention
Contention occurs when multiple threads simultaneously compete for the same shared resource. In a multithreaded program, contention commonly occurs when several threads attempt to acquire the same mutex or spinlock.
Low contention means that threads rarely have to wait for the resource. As contention increases, more threads compete for the same lock, increasing waiting time and potentially reducing overall system performance.
Contention affects mutexes and spinlocks differently. With a mutex, additional competing threads may become blocked and removed from CPU execution. With a spinlock, competing threads may continue consuming CPU cycles while waiting.
Therefore, contention is an important factor when determining which synchronization mechanism performs better.
Reference
Wieder, A., & Brandenburg, B. B. (2013). On Spin Locks in AUTOSAR: Blocking Analysis of FIFO, Unordered, and Priority-Ordered Spin Locks.


6. CPU Utilization
CPU utilization represents how much of the processor's available execution capacity is being used. In concurrent programs, synchronization mechanisms can have a significant effect on CPU utilization.
A spinlock can result in high CPU utilization because waiting threads continue executing instructions while checking the lock. This can be useful when the expected waiting period is very short, but under high contention it can result in CPU cycles being spent on busy waiting instead of useful computation.
A blocking mutex behaves differently. When a thread cannot acquire the lock, it can be suspended, allowing the CPU to execute another ready thread. This can improve CPU efficiency, although the associated scheduling and context-switch overhead may increase latency.
Research on hybrid spin/sleep synchronization explores how systems can balance response time, CPU consumption, and energy efficiency instead of choosing exclusively between spinning and blocking.
Reference
Federico, M., Marotta, R., & Quaglia, F. (2025). Spin/Sleep Proactive-Awakening Locks for Alternative Performance/Energy Trade-Offs.
 









