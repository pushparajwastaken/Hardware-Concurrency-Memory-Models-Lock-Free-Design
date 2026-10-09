# Case Study: Low-Latency Concurrent Systems

## Scenario
A producer thread publishes market data, while a consumer thread reads the data and calculates a signal.

## Challenge
If the consumer observes a ready flag without the required synchronization, the program may not guarantee that it observes the corresponding data correctly.

## Solution
Use a correctly designed synchronization protocol. A release store and an acquire load that reads from that release operation can establish a happens-before relationship for preceding writes.

## Key Lessons
- Atomicity does not automatically synchronize unrelated data.
- Choose memory ordering based on correctness requirements.
- Test performance only after correctness is established.
- Real systems may require a queue, sequence counter, or other repeated-publication protocol.
