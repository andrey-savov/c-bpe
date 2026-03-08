# Benchmark Results

## rs_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_import_cl100k` | n/a | 314.600 μs | 840.831 μs | 647.200 μs | 440.909 μs | 732.925 μs | 1,189 | n/a | 3197 |
| `test_import_o200k` | n/a | 348.500 μs | 845.114 μs | 703.300 μs | 406.878 μs | 709.800 μs | 1,183 | n/a | 2976 |
| `test_encode_cl100k[small]` | 10 | 1.300 μs | 3.183 μs | 3.470 μs | 2.101 μs | 2.750 μs | 314,179 | 318.3 ns | 76924 |
| `test_encode_cl100k[medium]` | 5,157 | 628.300 μs | 1.250 ms | 912.100 μs | 592.555 μs | 1.014 ms | 800 | 242.3 ns | 1313 |
| `test_encode_cl100k[large]` | 257,850 | 39.926 ms | 58.111 ms | 54.403 ms | 13.531 ms | 17.329 ms | 17 | 225.4 ns | 24 |
| `test_decode_cl100k[small]` | 10 | 243.000 ns | 639.913 ns | 785.000 ns | 304.566 ns | 586.000 ns | 1,562,712 | 64.0 ns | 41153 |
| `test_decode_cl100k[medium]` | 5,157 | 80.700 μs | 143.732 μs | 89.100 μs | 87.552 μs | 134.700 μs | 6,957 | 27.9 ns | 12361 |
| `test_decode_cl100k[large]` | 257,850 | 4.185 ms | 7.790 ms | 7.565 ms | 2.513 ms | 4.738 ms | 128 | 30.2 ns | 242 |
| `test_roundtrip_cl100k[small]` | 10 | 1.540 μs | 3.358 μs | 2.270 μs | 2.218 μs | 3.160 μs | 297,776 | 335.8 ns | 54054 |
| `test_roundtrip_cl100k[medium]` | 5,157 | 704.000 μs | 1.381 ms | 961.500 μs | 657.967 μs | 1.161 ms | 724 | 267.8 ns | 1438 |
| `test_roundtrip_cl100k[large]` | 257,850 | 46.287 ms | 68.681 ms | 64.798 ms | 13.731 ms | 25.170 ms | 15 | 266.4 ns | 21 |
| `test_count_cl100k[small]` | 10 | 1.020 μs | 2.053 μs | 1.280 μs | 1.475 μs | 1.920 μs | 487,011 | 205.3 ns | 98040 |
| `test_count_cl100k[medium]` | 5,157 | 693.700 μs | 1.244 ms | 958.600 μs | 576.843 μs | 1.109 ms | 804 | 241.2 ns | 1439 |
| `test_count_cl100k[large]` | 257,850 | 42.365 ms | 72.738 ms | 74.493 ms | 12.268 ms | 16.723 ms | 14 | 282.1 ns | 27 |
| `test_count_till_limit_cl100k[small]` | 10 | 1.150 μs | 2.214 μs | 1.330 μs | 1.598 μs | 2.080 μs | 451,581 | 221.4 ns | 86957 |
| `test_count_till_limit_cl100k[medium]` | 50 | 4.600 μs | 12.076 μs | 7.800 μs | 10.660 μs | 11.700 μs | 82,811 | 241.5 ns | 172416 |
| `test_count_till_limit_cl100k[large]` | 50 | 5.850 μs | 12.465 μs | 12.050 μs | 8.529 μs | 10.650 μs | 80,227 | 249.3 ns | 85471 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 135.100 μs | 278.417 μs | 181.700 μs | 152.316 μs | 255.800 μs | 3,592 | 278.4 ns | 6614 |
| `test_encode_batch_cl100k[medium_x10]` | 51,570 | 7.858 ms | 15.213 ms | 15.015 ms | 5.038 ms | 9.532 ms | 66 | 295.0 ns | 148 |
| `test_encode_batch_cl100k[large_x2]` | 515,700 | 96.249 ms | 146.003 ms | 153.256 ms | 30.145 ms | 42.657 ms | 7 | 283.1 ns | 12 |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 2.763 ms | 4.236 ms | 3.392 ms | 2.417 ms | 1.808 ms | 236 | 4236.2 ns | 5 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 51,570 | 12.595 ms | 17.499 ms | 15.194 ms | 5.274 ms | 9.906 ms | 57 | 339.3 ns | 5 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 515,700 | 148.607 ms | 157.941 ms | 156.137 ms | 9.957 ms | 15.801 ms | 6 | 306.3 ns | 5 |
| `test_decode_o200k[small]` | 10 | 274.995 ns | 661.863 ns | 524.997 ns | 777.807 ns | 581.247 ns | 1,510,887 | 66.2 ns | 192311 |
| `test_decode_o200k[medium]` | 5,212 | 65.200 μs | 155.094 μs | 107.300 μs | 80.798 μs | 140.000 μs | 6,448 | 29.8 ns | 12020 |
| `test_decode_o200k[large]` | 260,600 | 3.561 ms | 7.295 ms | 6.791 ms | 2.758 ms | 4.773 ms | 137 | 28.0 ns | 234 |
| `test_encode_o200k[small]` | 10 | 1.320 μs | 2.825 μs | 1.760 μs | 1.900 μs | 2.680 μs | 354,001 | 282.5 ns | 62894 |
| `test_encode_o200k[medium]` | 5,212 | 817.800 μs | 1.473 ms | 1.138 ms | 677.984 μs | 1.332 ms | 679 | 282.6 ns | 1220 |
| `test_encode_o200k[large]` | 260,600 | 57.613 ms | 85.686 ms | 87.893 ms | 13.800 ms | 12.974 ms | 12 | 328.8 ns | 22 |
| `test_roundtrip_o200k[small]` | 10 | 1.590 μs | 3.941 μs | 3.340 μs | 2.283 μs | 3.520 μs | 253,746 | 394.1 ns | 51021 |
| `test_roundtrip_o200k[medium]` | 5,212 | 725.500 μs | 1.027 ms | 823.750 μs | 517.713 μs | 270.550 μs | 973 | 197.1 ns | 1364 |
| `test_roundtrip_o200k[large]` | 260,600 | 39.277 ms | 45.841 ms | 43.772 ms | 5.282 ms | 8.685 ms | 22 | 175.9 ns | 26 |

## c_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_import_cl100k` | n/a | 304.300 μs | 659.866 μs | 485.600 μs | 346.547 μs | 484.600 μs | 1,515 | n/a | 3348 |
| `test_import_o200k` | n/a | 318.700 μs | 761.325 μs | 603.300 μs | 372.333 μs | 634.550 μs | 1,314 | n/a | 3180 |
| `test_encode_cl100k[small]` | 10 | 689.994 ns | 1.638 μs | 1.080 μs | 1.206 μs | 1.590 μs | 610,613 | 163.8 ns | 104166 |
| `test_encode_cl100k[medium]` | 5,160 | 477.500 μs | 1.133 ms | 1.311 ms | 448.707 μs | 836.575 μs | 883 | 219.5 ns | 2605 |
| `test_encode_cl100k[large]` | 258,000 | 25.270 ms | 42.249 ms | 40.568 ms | 12.234 ms | 16.102 ms | 24 | 163.8 ns | 47 |
| `test_decode_cl100k[small]` | 10 | 143.000 ns | 288.065 ns | 190.000 ns | 232.174 ns | 306.000 ns | 3,471,436 | 28.8 ns | 53764 |
| `test_decode_cl100k[medium]` | 5,160 | 46.000 μs | 100.403 μs | 68.500 μs | 55.703 μs | 79.200 μs | 9,960 | 19.5 ns | 21740 |
| `test_decode_cl100k[large]` | 258,000 | 2.600 ms | 5.212 ms | 4.183 ms | 2.103 ms | 3.859 ms | 192 | 20.2 ns | 318 |
| `test_roundtrip_cl100k[small]` | 10 | 1.220 μs | 2.605 μs | 2.580 μs | 1.641 μs | 2.310 μs | 383,902 | 260.5 ns | 82645 |
| `test_roundtrip_cl100k[medium]` | 5,160 | 443.500 μs | 1.094 ms | 958.200 μs | 501.240 μs | 904.100 μs | 914 | 212.0 ns | 2309 |
| `test_roundtrip_cl100k[large]` | 258,000 | 30.576 ms | 59.648 ms | 58.706 ms | 13.580 ms | 20.069 ms | 17 | 231.2 ns | 33 |
| `test_count_cl100k[small]` | 10 | 770.006 ns | 1.702 μs | 2.090 μs | 1.341 μs | 1.400 μs | 587,712 | 170.2 ns | 128207 |
| `test_count_cl100k[medium]` | 5,160 | 417.100 μs | 806.427 μs | 642.500 μs | 387.987 μs | 692.775 μs | 1,240 | 156.3 ns | 2415 |
| `test_count_cl100k[large]` | 258,000 | 22.366 ms | 34.796 ms | 31.591 ms | 9.230 ms | 12.295 ms | 29 | 134.9 ns | 37 |
| `test_count_till_limit_cl100k[small]` | 10 | 1.160 μs | 2.234 μs | 1.480 μs | 1.561 μs | 1.820 μs | 447,645 | 223.4 ns | 86208 |
| `test_count_till_limit_cl100k[medium]` | 50 | 4.300 μs | 10.066 μs | 6.450 μs | 7.409 μs | 9.700 μs | 99,347 | 201.3 ns | 116279 |
| `test_count_till_limit_cl100k[large]` | 50 | 5.700 μs | 10.645 μs | 6.500 μs | 11.379 μs | 9.900 μs | 93,943 | 212.9 ns | 114944 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 64.400 μs | 150.412 μs | 95.500 μs | 84.551 μs | 145.800 μs | 6,648 | 150.4 ns | 15175 |
| `test_encode_batch_cl100k[medium_x10]` | 51,600 | 3.842 ms | 7.311 ms | 6.424 ms | 2.587 ms | 3.243 ms | 137 | 141.7 ns | 251 |
| `test_encode_batch_cl100k[large_x2]` | 516,000 | 86.397 ms | 113.448 ms | 114.165 ms | 15.301 ms | 18.363 ms | 9 | 219.9 ns | 16 |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 230.200 μs | 487.140 μs | 328.500 μs | 423.477 μs | 369.950 μs | 2,053 | 487.1 ns | 5 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 51,600 | 1.347 ms | 2.201 ms | 2.125 ms | 625.528 μs | 717.475 μs | 454 | 42.7 ns | 5 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 516,000 | 38.859 ms | 56.836 ms | 60.198 ms | 12.716 ms | 19.723 ms | 18 | 110.1 ns | 5 |
| `test_decode_o200k[small]` | 10 | 154.000 ns | 171.030 ns | 157.000 ns | 89.851 ns | 1.001 ns | 5,846,937 | 17.1 ns | 63695 |
| `test_decode_o200k[medium]` | 5,215 | 46.500 μs | 53.423 μs | 48.100 μs | 21.059 μs | 2.400 μs | 18,718 | 10.2 ns | 21414 |
| `test_decode_o200k[large]` | 260,750 | 2.507 ms | 2.953 ms | 2.842 ms | 384.456 μs | 296.000 μs | 339 | 11.3 ns | 409 |
| `test_encode_o200k[small]` | 10 | 679.994 ns | 808.041 ns | 750.006 ns | 503.565 ns | 50.000 ns | 1,237,562 | 80.8 ns | 144929 |
| `test_encode_o200k[medium]` | 5,215 | 421.600 μs | 508.371 μs | 450.400 μs | 176.694 μs | 57.750 μs | 1,967 | 97.5 ns | 2359 |
| `test_encode_o200k[large]` | 260,750 | 23.079 ms | 27.548 ms | 26.649 ms | 3.425 ms | 4.979 ms | 36 | 105.6 ns | 42 |
| `test_roundtrip_o200k[small]` | 10 | 849.995 ns | 1.073 μs | 969.996 ns | 693.819 ns | 100.001 ns | 932,106 | 107.3 ns | 116279 |
| `test_roundtrip_o200k[medium]` | 5,215 | 476.600 μs | 563.538 μs | 499.300 μs | 212.785 μs | 32.750 μs | 1,775 | 108.1 ns | 2097 |
| `test_roundtrip_o200k[large]` | 260,750 | 25.650 ms | 29.834 ms | 29.877 ms | 2.506 ms | 3.131 ms | 34 | 114.4 ns | 40 |

## Comparison (median, rs_bpe vs c_bpe)

| Benchmark | rs_bpe | c_bpe | ratio |
| --- | --- | --- | --- |
| `test_count_cl100k[large]` | 74.493 ms | 31.591 ms | 2.36× |
| `test_count_cl100k[medium]` | 958.600 μs | 642.500 μs | 1.49× |
| `test_count_cl100k[small]` | 1.280 μs | 2.090 μs | **0.61×** |
| `test_count_till_limit_cl100k[large]` | 12.050 μs | 6.500 μs | 1.85× |
| `test_count_till_limit_cl100k[medium]` | 7.800 μs | 6.450 μs | 1.21× |
| `test_count_till_limit_cl100k[small]` | 1.330 μs | 1.480 μs | **0.90×** |
| `test_decode_cl100k[large]` | 7.565 ms | 4.183 ms | 1.81× |
| `test_decode_cl100k[medium]` | 89.100 μs | 68.500 μs | 1.30× |
| `test_decode_cl100k[small]` | 785.000 ns | 190.000 ns | 4.13× |
| `test_decode_o200k[large]` | 6.791 ms | 2.842 ms | 2.39× |
| `test_decode_o200k[medium]` | 107.300 μs | 48.100 μs | 2.23× |
| `test_decode_o200k[small]` | 524.997 ns | 157.000 ns | 3.34× |
| `test_encode_batch_cl100k[large_x2]` | 153.256 ms | 114.165 ms | 1.34× |
| `test_encode_batch_cl100k[medium_x10]` | 15.015 ms | 6.424 ms | 2.34× |
| `test_encode_batch_cl100k[small_x100]` | 181.700 μs | 95.500 μs | 1.90× |
| `test_encode_batch_parallel_cl100k[large_x2]` | 156.137 ms | 60.198 ms | 2.59× |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 15.194 ms | 2.125 ms | 7.15× |
| `test_encode_batch_parallel_cl100k[small_x100]` | 3.392 ms | 328.500 μs | 10.33× |
| `test_encode_cl100k[large]` | 54.403 ms | 40.568 ms | 1.34× |
| `test_encode_cl100k[medium]` | 912.100 μs | 1.311 ms | **0.70×** |
| `test_encode_cl100k[small]` | 3.470 μs | 1.080 μs | 3.21× |
| `test_encode_o200k[large]` | 87.893 ms | 26.649 ms | 3.30× |
| `test_encode_o200k[medium]` | 1.138 ms | 450.400 μs | 2.53× |
| `test_encode_o200k[small]` | 1.760 μs | 750.006 ns | 2.35× |
| `test_import_cl100k` | 647.200 μs | 485.600 μs | 1.33× |
| `test_import_o200k` | 703.300 μs | 603.300 μs | 1.17× |
| `test_roundtrip_cl100k[large]` | 64.798 ms | 58.706 ms | 1.10× |
| `test_roundtrip_cl100k[medium]` | 961.500 μs | 958.200 μs | 1.00× |
| `test_roundtrip_cl100k[small]` | 2.270 μs | 2.580 μs | **0.88×** |
| `test_roundtrip_o200k[large]` | 43.772 ms | 29.877 ms | 1.47× |
| `test_roundtrip_o200k[medium]` | 823.750 μs | 499.300 μs | 1.65× |
| `test_roundtrip_o200k[small]` | 3.340 μs | 969.996 ns | 3.44× |
