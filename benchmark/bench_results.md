# Benchmark Results

## rs-bpe (Rust)

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

## c_bpe (C)

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_encode_cl100k[small]` | 10 | 2.000 μs | 2.369 μs | 2.200 μs | 2.592 μs | 300.000 ns | 422,067 | 220.0 ns | 185185 |
| `test_encode_cl100k[medium]` | 3,450 | 882.500 μs | 1.010 ms | 923.500 μs | 194.280 μs | 114.600 μs | 990 | 267.7 ns | 549 |
| `test_decode_cl100k[small]` | 10 | 100.000 ns | 172.663 ns | 200.000 ns | 252.781 ns | 100.000 ns | 5,791,612 | 20.0 ns | 200000 |
| `test_decode_o200k[small]` | 10 | 100.000 ns | 177.395 ns | 200.000 ns | 453.085 ns | 100.000 ns | 5,637,122 | 20.0 ns | 200000 |
| `test_roundtrip_cl100k[small]` | 10 | 2.200 μs | 2.575 μs | 2.400 μs | 1.789 μs | 300.000 ns | 388,367 | 240.0 ns | 185185 |
| `test_count_cl100k[small]` | 10 | 1.500 μs | 1.729 μs | 1.600 μs | 1.307 μs | 100.000 ns | 578,344 | 160.0 ns | 200000 |
| `test_count_till_limit_cl100k[small]` | 10 | 1.600 μs | 1.881 μs | 1.800 μs | 1.684 μs | 100.000 ns | 531,518 | 180.0 ns | 200000 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 45.000 μs | 92.168 μs | 67.600 μs | 470.152 μs | 15.200 μs | 10,850 | 67.6 ns | 3695 |
| `test_encode_batch_cl100k[medium_x10]` | 34,500 | 4.684 ms | 5.462 ms | 5.170 ms | 1.174 ms | 481.100 μs | 183 | 149.9 ns | 89 |
| `test_encode_o200k[small]` | 10 | 2.000 μs | 2.346 μs | 2.100 μs | 1.521 μs | 400.000 ns | 426,320 | 210.0 ns | 172413 |
| `test_encode_o200k[medium]` | 3,490 | 883.100 μs | 1.024 ms | 929.600 μs | 232.207 μs | 133.300 μs | 976 | 266.4 ns | 542 |
| `test_roundtrip_o200k[small]` | 10 | 2.100 μs | 2.471 μs | 2.300 μs | 1.686 μs | 200.000 ns | 404,771 | 230.0 ns | 185185 |

## Comparison (median, c_bpe vs rs-bpe)

| Benchmark | rs-bpe | c_bpe | ratio |
| --- | --- | --- | --- |
| `encode_cl100k[small]` | 1.370 μs | 2.200 μs | 1.61× |
| `encode_cl100k[medium]` | 460.250 μs | 923.500 μs | 2.01× |
| `decode_cl100k[small]` | 251.000 ns | 200.000 ns | **0.80×** |
| `roundtrip_cl100k[small]` | 1.940 μs | 2.400 μs | 1.24× |
| `count_cl100k[small]` | 1.430 μs | 1.600 μs | 1.12× |
| `encode_batch_cl100k[small_x100]` | 171.100 μs | 67.600 μs | **0.40×** |
| `encode_batch_cl100k[medium_x10]` | 5.622 ms | 5.170 ms | **0.92×** |
| `encode_o200k[small]` | 1.680 μs | 2.100 μs | 1.25× |
| `encode_o200k[medium]` | 551.350 μs | 929.600 μs | 1.69× |
| `roundtrip_o200k[small]` | 1.970 μs | 2.300 μs | 1.17× |
