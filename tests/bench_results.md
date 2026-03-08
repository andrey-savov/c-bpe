# Benchmark Results

## rs_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 8.225 ms | 9.113 ms | 9.085 ms | 490.474 μs | 547.082 μs | 110 | 9113.2 ns | 121 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 46,030 | 3.145 ms | 3.522 ms | 3.302 ms | 510.394 μs | 408.769 μs | 284 | 76.5 ns | 319 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 460,300 | 38.089 ms | 40.202 ms | 40.079 ms | 1.386 ms | 2.349 ms | 25 | 87.3 ns | 27 |

## c_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 71.557 μs | 356.195 μs | 267.724 μs | 3.493 ms | 125.843 μs | 2,807 | 356.2 ns | 12005 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 46,060 | 1.905 ms | 2.648 ms | 2.310 ms | 2.913 ms | 416.947 μs | 378 | 57.5 ns | 526 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 460,600 | 29.946 ms | 32.483 ms | 32.254 ms | 1.629 ms | 861.863 μs | 31 | 70.5 ns | 34 |

## Comparison (median, rs_bpe vs c_bpe)

| Benchmark | rs_bpe | c_bpe | ratio |
| --- | --- | --- | --- |
| `test_encode_batch_parallel_cl100k[large_x2]` | 40.079 ms | 32.254 ms | 1.24× |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 3.302 ms | 2.310 ms | 1.43× |
| `test_encode_batch_parallel_cl100k[small_x100]` | 9.085 ms | 267.724 μs | 33.93× |
