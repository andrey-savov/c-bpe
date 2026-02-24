# Benchmark Results

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_encode_cl100k[small]` | 10 | 1.270 μs | 1.443 μs | 1.370 μs | 594.025 ns | 50.000 ns | 692,966 | 144.3 ns | 78741 |
| `test_encode_cl100k[medium]` | 3,435 | 430.900 μs | 498.160 μs | 460.250 μs | 116.253 μs | 59.000 μs | 2,007 | 145.0 ns | 2326 |
| `test_decode_cl100k[small]` | 10 | 244.000 ns | 263.283 ns | 251.000 ns | 72.161 ns | 6.000 ns | 3,798,190 | 26.3 ns | 40651 |
| `test_decode_o200k[small]` | 10 | 245.000 ns | 267.466 ns | 251.000 ns | 82.430 ns | 5.000 ns | 3,738,788 | 26.7 ns | 40651 |
| `test_roundtrip_cl100k[small]` | 10 | 1.650 μs | 2.025 μs | 1.940 μs | 689.786 ns | 79.998 ns | 493,754 | 202.5 ns | 59881 |
| `test_count_cl100k[small]` | 10 | 1.170 μs | 1.460 μs | 1.430 μs | 582.963 ns | 210.002 ns | 684,700 | 146.0 ns | 84746 |
| `test_count_till_limit_cl100k[small]` | 10 | 1.180 μs | 1.458 μs | 1.430 μs | 503.826 ns | 210.002 ns | 685,656 | 145.8 ns | 84746 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 163.100 μs | 182.140 μs | 171.100 μs | 42.567 μs | 12.250 μs | 5,490 | 182.1 ns | 6128 |
| `test_encode_batch_cl100k[medium_x10]` | 34,350 | 5.201 ms | 5.860 ms | 5.622 ms | 761.496 μs | 574.250 μs | 171 | 170.6 ns | 192 |
| `test_encode_o200k[small]` | 10 | 1.410 μs | 1.763 μs | 1.680 μs | 696.199 ns | 79.998 ns | 567,242 | 176.3 ns | 70423 |
| `test_encode_o200k[medium]` | 3,472 | 519.600 μs | 599.228 μs | 551.350 μs | 134.823 μs | 79.100 μs | 1,669 | 172.6 ns | 1918 |
| `test_roundtrip_o200k[small]` | 10 | 1.690 μs | 2.061 μs | 1.970 μs | 674.528 ns | 120.001 ns | 485,238 | 206.1 ns | 59172 |
