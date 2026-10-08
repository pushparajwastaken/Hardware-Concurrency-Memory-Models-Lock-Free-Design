#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>

using namespace std;

const int ITERATIONS = 1000000;

int mutexCounter = 0;
mutex mtx;

int spinCounter = 0;
atomic_flag lockFlag = ATOMIC_FLAG_INIT;

atomic<int> atomicCounter = 0;


// Mutex

void mutexWork()
{
    for (int i = 0; i < ITERATIONS; i++)
    {
        lock_guard<mutex> lock(mtx);
        mutexCounter++;
    }
}


// Spinlock

void spinLock()
{
    while (lockFlag.test_and_set())
    {
    }
}

void spinUnlock()
{
    lockFlag.clear();
}

void spinWork()
{
    for (int i = 0; i < ITERATIONS; i++)
    {
        spinLock();

        spinCounter++;

        spinUnlock();
    }
}


// Atomic

void atomicWork()
{
    for (int i = 0; i < ITERATIONS; i++)
    {
        atomicCounter++;
    }
}


int main()
{
    int threadCounts[] = {1, 2, 4, 8};

    for (int threadsCount : threadCounts)
    {
        cout << "\nThreads = " << threadsCount << endl;

        // Mutex test
        mutexCounter = 0;

        auto start = chrono::high_resolution_clock::now();

        vector<thread> threads;

        for (int i = 0; i < threadsCount; i++)
        {
            threads.push_back(thread(mutexWork));
        }

        for (auto &t : threads)
        {
            t.join();
        }

        auto end = chrono::high_resolution_clock::now();

        cout << "Mutex    >> Time = "
             << chrono::duration_cast<chrono::milliseconds>
                (end - start).count()
             << " ms" << endl;


        // Spinlock test
        spinCounter = 0;
        lockFlag.clear();

        start = chrono::high_resolution_clock::now();

        threads.clear();

        for (int i = 0; i < threadsCount; i++)
        {
            threads.push_back(thread(spinWork));
        }

        for (auto &t : threads)
        {
            t.join();
        }

        end = chrono::high_resolution_clock::now();

        cout << "Spinlock >> Time = "
             << chrono::duration_cast<chrono::milliseconds>
                (end - start).count()
             << " ms" << endl;


        // Atomic test
        atomicCounter = 0;

        start = chrono::high_resolution_clock::now();

        threads.clear();

        for (int i = 0; i < threadsCount; i++)
        {
            threads.push_back(thread(atomicWork));
        }

        for (auto &t : threads)
        {
            t.join();
        }

        end = chrono::high_resolution_clock::now();

        cout << "Atomic   >> Time = "
             << chrono::duration_cast<chrono::milliseconds>
                (end - start).count()
             << " ms" << endl;
    }

    return 0;
}