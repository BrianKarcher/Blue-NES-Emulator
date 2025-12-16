Using if statements:
=== Sequential Access Test ===
Sequential Test Results:
  Total operations: 51100
  Time elapsed: 3.57 ms
  Throughput: 14.32 million ops/sec
  Latency: 69.82 ns/op

=== Random Access Test ===
Random Test Results:
  Total operations: 1000000
  Time elapsed: 9.90 ms
  Throughput: 101.05 million ops/sec
  Latency: 9.90 ns/op

=== Strided Access Test (stride=4096) ===
Strided Test Results:
  Total operations: 7000
  Time elapsed: 0.03 ms
  Throughput: 259.26 million ops/sec
  Latency: 3.86 ns/op

=== Strided Access Test (stride=8192) ===
Strided Test Results:
  Total operations: 3000
  Time elapsed: 0.01 ms
  Throughput: 250.00 million ops/sec
  Latency: 4.00 ns/op

=== Multi-threaded Test (4 threads) ===
Multi-threaded Test Results:
  Total operations: 400000
  Time elapsed: 14.49 ms
  Throughput: 27.60 million ops/sec
  Latency: 36.23 ns/op

=== Load Test Complete ===