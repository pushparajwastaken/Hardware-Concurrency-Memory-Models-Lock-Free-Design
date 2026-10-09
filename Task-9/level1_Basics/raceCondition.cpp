#include <iostream>
#include <thread>
#include <vector>
using namespace std;
// SHARED memory 
long long counter = 0;

void increment(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        counter = counter + 1;  //this is not atomic
        // Machine level  3 steps:
        // 1. load  (READ)
        // 2. register (MODIFY)
        // 3. register (WRITE)
    }
}

int main() {
    const int NUM_THREADS = 4;
    const int ITERATIONS = 1000000;

    vector<thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(increment, ITERATIONS);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Expected: 4 * 1,000,000 = 4,000,000
    cout << "Expected: " << (NUM_THREADS * ITERATIONS) << endl;
    cout << "Actual:   " << counter << endl;

    if (counter != NUM_THREADS * ITERATIONS) {
        cout << "\n>>> RACE CONDITION DETECTED! Data corrupted. <<<\n";
    }
    return 0;
}