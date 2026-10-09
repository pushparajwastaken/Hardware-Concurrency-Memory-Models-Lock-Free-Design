# Experiment: Relaxed vs Sequential Consistency

## Objective
Compare the Store Buffering outcome under relaxed and sequentially consistent atomic operations.

## Compile
```bash
g++ -std=c++20 -O2 -pthread practical/store_buffering.cpp -o store_buffering
```

## Run
```bash
./store_buffering 100000 relaxed
./store_buffering 100000 seq_cst
```

## Expected Interpretation
- Relaxed: both loads returning zero is permitted, but may not occur in a finite run.
- Sequentially consistent: both loads returning zero is forbidden for this test.

## Actual Results
Record the output from your own machine here. Do not enter expected values as measured results.

## Limitations
This experiment observes program outcomes; it does not directly inspect the CPU's internal store buffer.
