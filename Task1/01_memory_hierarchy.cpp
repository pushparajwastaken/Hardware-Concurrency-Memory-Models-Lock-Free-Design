/*
 * Concept : CPU -> Cache -> RAM memory hierarchy
 * Overview: Measures how access time changes as data grows from L1 cache to RAM,
 *           compares row-major and column-major traversal, and shows that the
 *           64-byte cache line is the unit of memory transfer.
 */

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace std;
using namespace std::chrono;

using Clock = steady_clock;

double nsPerAccess(size_t bytes) {
    size_t n = bytes / sizeof(size_t);
    vector<size_t> order(n);
    iota(order.begin(), order.end(), 0);

    mt19937_64 rng(12345);
    for (size_t i = n - 1; i > 0; --i) {
        size_t j = rng() % i;
        swap(order[i], order[j]);
    }

    const size_t steps = 20000000;
    size_t p = 0;
    for (size_t i = 0; i < n; ++i) p = order[p];

    auto start = Clock::now();
    for (size_t i = 0; i < steps; ++i) p = order[p];
    auto end = Clock::now();

    volatile size_t sink = p;
    (void)sink;
    return duration<double, nano>(end - start).count() / steps;
}

void latencyTest() {
    cout << "[A] Access latency vs working-set size\n";
    cout << "    size        ns/access\n";
    for (size_t kb = 4; kb <= 131072; kb *= 2) {
        string label = kb >= 1024 ? to_string(kb / 1024) + " MB" : to_string(kb) + " KB";
        cout << "    " << setw(8) << label << "  " << fixed << setprecision(2)
             << setw(8) << nsPerAccess(kb * 1024) << "\n";
    }
}

void matrixTest() {
    const size_t N = 4096;
    vector<int> matrix(N * N, 1);

    auto t0 = Clock::now();
    long long rowSum = 0;
    for (size_t r = 0; r < N; ++r)
        for (size_t c = 0; c < N; ++c) rowSum += matrix[r * N + c];
    auto t1 = Clock::now();

    long long colSum = 0;
    for (size_t c = 0; c < N; ++c)
        for (size_t r = 0; r < N; ++r) colSum += matrix[r * N + c];
    auto t2 = Clock::now();

    double rowMs = duration<double, milli>(t1 - t0).count();
    double colMs = duration<double, milli>(t2 - t1).count();

    cout << "\n[B] 4096 x 4096 matrix sum\n";
    cout << "    row-major    : " << fixed << setprecision(1) << rowMs << " ms\n";
    cout << "    column-major : " << colMs << " ms (" << colMs / rowMs << "x slower)\n";
    cout << "    checksum     : " << (rowSum == colSum ? "OK" : "MISMATCH") << "\n";
}

void strideTest() {
    const size_t n = 16 * 1024 * 1024;
    const int passes = 10;
    vector<int> arr(n, 1);

    cout << "\n[C] Stride test on a 64 MB array\n";
    cout << "    stride  touched-elements  time(ms)\n";
    for (size_t stride : {1, 2, 4, 8, 16, 32, 64}) {
        auto start = Clock::now();
        for (int p = 0; p < passes; ++p)
            for (size_t i = 0; i < n; i += stride) arr[i] *= 3;
        auto end = Clock::now();
        cout << "    " << setw(6) << stride << "  " << setw(16) << n / stride << "  "
             << fixed << setprecision(1) << setw(8)
             << duration<double, milli>(end - start).count() << "\n";
    }
}

int main() {
    latencyTest();
    matrixTest();
    strideTest();
    return 0;
}
