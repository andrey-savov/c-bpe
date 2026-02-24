# Benchmark Results

| Benchmark | min | mean | median | stddev | IQR | ops/s | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `test_encode_cl100k[small]` | 1.350 μs | 1.508 μs | 1.410 μs | 537.242 ns | 109.998 ns | 663,021 | 74075 |
| `test_decode_cl100k_small` | 257.000 ns | 281.616 ns | 262.000 ns | 81.351 ns | 10.000 ns | 3,550,935 | 38911 |
| `test_roundtrip_cl100k[small]` | 1.640 μs | 1.857 μs | 1.720 μs | 662.971 ns | 120.001 ns | 538,561 | 61729 |
| `test_count_cl100k[small]` | 1.140 μs | 1.278 μs | 1.170 μs | 1.371 μs | 50.000 ns | 782,362 | 87720 |
| `test_count_till_limit_cl100k[small]` | 1.140 μs | 1.271 μs | 1.190 μs | 447.860 ns | 40.000 ns | 786,779 | 87720 |
| `test_encode_batch_cl100k[small_x100]` | 137.100 μs | 152.421 μs | 141.000 μs | 37.926 μs | 7.500 μs | 6,561 | 7289 |
| `test_encode_o200k[small]` | 1.270 μs | 1.505 μs | 1.400 μs | 561.375 ns | 60.000 ns | 664,552 | 78741 |
| `test_roundtrip_o200k[small]` | 1.630 μs | 1.855 μs | 1.700 μs | 733.053 ns | 90.001 ns | 539,178 | 61729 |
