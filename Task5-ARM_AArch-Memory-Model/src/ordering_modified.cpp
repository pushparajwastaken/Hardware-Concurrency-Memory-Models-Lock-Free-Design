#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#endif

// Set either of these to 1 to prevent CPU reordering
#define USE_CPU_FENCE              0
#define USE_SINGLE_HW_THREAD       0  // Supported on Linux, but not Cygwin or PS3

#if USE_SINGLE_HW_THREAD
#include <sched.h>
#endif

// ---- Experiment config ----
#define WARMUP_RUNS    200000   // untimed, uncounted runs before measuring
#define PIN_THREADS    1        // 1 = pin each worker thread to its own logical CPU
// Logical CPU indices. On most Intel/AMD Windows setups with SMT:
//   (0,1) = two hyperthreads of the SAME physical core
//   (0,2) = two DIFFERENT physical cores
// Never use the same index for both (that gives zero reorders).
#define CORE_THREAD1   0
#define CORE_THREAD2   2


//-------------------------------------
//  MersenneTwister
//  A thread-safe random number generator with good randomness
//  in a small number of instructions. We'll use it to introduce
//  random timing delays.
//-------------------------------------
#define MT_IA  397
#define MT_LEN 624

class MersenneTwister
{
    unsigned int m_buffer[MT_LEN];
    int m_index;

public:
    MersenneTwister(unsigned int seed);
    // Declare noinline so that the function call acts as a compiler barrier:
    unsigned int integer() __attribute__((noinline));
};

MersenneTwister::MersenneTwister(unsigned int seed)
{
    // Initialize by filling with the seed, then iterating
    // the algorithm a bunch of times to shuffle things up.
    for (int i = 0; i < MT_LEN; i++)
        m_buffer[i] = seed;
    m_index = 0;
    for (int i = 0; i < MT_LEN * 100; i++)
        integer();
}

unsigned int MersenneTwister::integer()
{
    // Indices
    int i = m_index;
    int i2 = m_index + 1; if (i2 >= MT_LEN) i2 = 0; // wrap-around
    int j = m_index + MT_IA; if (j >= MT_LEN) j -= MT_LEN; // wrap-around

    // Twist
    unsigned int s = (m_buffer[i] & 0x80000000) | (m_buffer[i2] & 0x7fffffff);
    unsigned int r = m_buffer[j] ^ (s >> 1) ^ ((s & 1) * 0x9908B0DF);
    m_buffer[m_index] = r;
    m_index = i2;

    // Swizzle
    r ^= (r >> 11);
    r ^= (r << 7) & 0x9d2c5680UL;
    r ^= (r << 15) & 0xefc60000UL;
    r ^= (r >> 18);
    return r;
}


//-------------------------------------
//  Main program, as decribed in the post
//-------------------------------------
static void pinThisThread(int core)
{
#if PIN_THREADS
#ifdef _WIN32
    if (SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1 << core) == 0)
        fprintf(stderr, "Warning: could not pin thread to CPU %d\n", core);
#else
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    if (pthread_setaffinity_np(pthread_self(), sizeof(set), &set) != 0)
        fprintf(stderr, "Warning: could not pin thread to CPU %d\n", core);
#endif
#endif
}

sem_t beginSema1;
sem_t beginSema2;
sem_t endSema;

int X, Y;
int r1, r2;

void *thread1Func(void *param)
{
    pinThisThread(CORE_THREAD1);
    MersenneTwister random(1);
    for (;;)
    {
        sem_wait(&beginSema1);  // Wait for signal
        while (random.integer() % 8 != 0) {}  // Random delay

        // ----- THE TRANSACTION! -----
        X = 1;
#if USE_CPU_FENCE
        asm volatile("mfence" ::: "memory");  // Prevent CPU reordering
#else
        asm volatile("" ::: "memory");  // Prevent compiler reordering
#endif
        r1 = Y;

        sem_post(&endSema);  // Notify transaction complete
    }
    return NULL;  // Never returns
};

void *thread2Func(void *param)
{
    pinThisThread(CORE_THREAD2);
    MersenneTwister random(2);
    for (;;)
    {
        sem_wait(&beginSema2);  // Wait for signal
        while (random.integer() % 8 != 0) {}  // Random delay

        // ----- THE TRANSACTION! -----
        Y = 1;
#if USE_CPU_FENCE
        asm volatile("mfence" ::: "memory");  // Prevent CPU reordering
#else
        asm volatile("" ::: "memory");  // Prevent compiler reordering
#endif
        r2 = X;

        sem_post(&endSema);  // Notify transaction complete
    }
    return NULL;  // Never returns
};

#define TOTAL_RUNS       1000000
#define SAMPLE_INTERVAL  1000      // log cumulative reorders every N runs

static double now_sec()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main()
{
    // Initialize the semaphores
    sem_init(&beginSema1, 0, 0);
    sem_init(&beginSema2, 0, 0);
    sem_init(&endSema, 0, 0);

    // Spawn the threads
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, thread1Func, NULL);
    pthread_create(&thread2, NULL, thread2Func, NULL);

#if USE_SINGLE_HW_THREAD
    // Force thread affinities to the same cpu core.
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(0, &cpus);
    pthread_setaffinity_np(thread1, sizeof(cpu_set_t), &cpus);
    pthread_setaffinity_np(thread2, sizeof(cpu_set_t), &cpus);
#endif

    // Data collection (kept in memory, no printing in the hot loop)
    std::vector<int> sampleRuns, sampleCumReorders;   // graph 1
    std::vector<int> perSecReorders, perSecRuns;      // graph 2

    int detected = 0;

    // Warm-up: same experiment, nothing recorded
    for (int i = 0; i < WARMUP_RUNS; i++)
    {
        X = 0;
        Y = 0;
        sem_post(&beginSema1);
        sem_post(&beginSema2);
        sem_wait(&endSema);
        sem_wait(&endSema);
    }
    printf("Warm-up of %d runs done, starting measurement\n", WARMUP_RUNS);

    double t0 = now_sec();

    for (int iterations = 1; iterations <= TOTAL_RUNS; iterations++)
    {
        // Reset X and Y
        X = 0;
        Y = 0;
        // Signal both threads
        sem_post(&beginSema1);
        sem_post(&beginSema2);
        // Wait for both threads
        sem_wait(&endSema);
        sem_wait(&endSema);

        // Which wall-clock second are we in?
        size_t sec = (size_t)(now_sec() - t0);
        while (perSecReorders.size() <= sec) {
            perSecReorders.push_back(0);
            perSecRuns.push_back(0);
        }
        perSecRuns[sec]++;

        // Check if there was a simultaneous reorder
        if (r1 == 0 && r2 == 0)
        {
            detected++;
            perSecReorders[sec]++;
        }

        if (iterations % SAMPLE_INTERVAL == 0) {
            sampleRuns.push_back(iterations);
            sampleCumReorders.push_back(detected);
        }
        if (iterations % 100000 == 0)
            printf("%d runs done, %d reorders so far\n", iterations, detected);
    }
    double total = now_sec() - t0;

    // Graph 1 data: cumulative reorders vs runs
    FILE *f1 = fopen("reorders_vs_runs.txt", "w");
    fprintf(f1, "# runs cumulative_reorders\n");
    for (size_t i = 0; i < sampleRuns.size(); i++)
        fprintf(f1, "%d %d\n", sampleRuns[i], sampleCumReorders[i]);
    fclose(f1);

    // Graph 2 data: reorders per second
    // (last second is partial; total runtime is in the header)
    FILE *f2 = fopen("reorders_per_second.txt", "w");
    fprintf(f2, "# total_runtime_sec %.3f total_runs %d total_reorders %d "
            "warmup %d pinned %d cores %d,%d\n",
            total, TOTAL_RUNS, detected, WARMUP_RUNS, PIN_THREADS,
            CORE_THREAD1, CORE_THREAD2);
    fprintf(f2, "# second reorders runs_in_that_second\n");
    for (size_t i = 0; i < perSecReorders.size(); i++)
        fprintf(f2, "%zu %d %d\n", i + 1, perSecReorders[i], perSecRuns[i]);
    fclose(f2);

    printf("Done: %d reorders in %d runs (%.2f s). Data written to "
           "reorders_vs_runs.txt and reorders_per_second.txt\n",
           detected, TOTAL_RUNS, total);
    return 0;
}