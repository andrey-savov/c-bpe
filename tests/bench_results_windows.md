# Benchmark Results

## rs_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_encode_cl100k[small]` | 10 | 1.440 μs | 1.582 μs | 1.490 μs | 684.006 ns | 50.000 ns | 632,051 | 158.2 ns | 69445 |
| `test_encode_cl100k[medium]` | 5,157 | 742.800 μs | 848.702 μs | 788.600 μs | 164.967 μs | 92.100 μs | 1,178 | 164.6 ns | 1342 |
| `test_encode_cl100k[large]` | 257,850 | 39.504 ms | 42.201 ms | 41.689 ms | 1.560 ms | 1.455 ms | 24 | 163.7 ns | 26 |
| `test_decode_cl100k[small]` | 10 | 273.680 ns | 301.688 ns | 284.213 ns | 133.524 ns | 5.269 ns | 3,314,688 | 30.2 ns | 192307 |
| `test_decode_cl100k[medium]` | 5,157 | 65.800 μs | 72.164 μs | 67.600 μs | 18.493 μs | 3.100 μs | 13,857 | 14.0 ns | 15129 |
| `test_decode_cl100k[large]` | 257,850 | 3.348 ms | 3.928 ms | 3.837 ms | 371.343 μs | 247.075 μs | 255 | 15.2 ns | 297 |
| `test_roundtrip_cl100k[small]` | 10 | 1.770 μs | 1.922 μs | 1.820 μs | 538.262 ns | 59.989 ns | 520,180 | 192.2 ns | 56498 |
| `test_roundtrip_cl100k[medium]` | 5,157 | 800.700 μs | 905.377 μs | 843.600 μs | 167.101 μs | 97.700 μs | 1,105 | 175.6 ns | 1250 |
| `test_roundtrip_cl100k[large]` | 257,850 | 44.115 ms | 46.857 ms | 46.648 ms | 1.334 ms | 1.607 ms | 21 | 181.7 ns | 23 |
| `test_count_cl100k[small]` | 10 | 1.200 μs | 1.309 μs | 1.230 μs | 550.623 ns | 10.012 ns | 764,043 | 130.9 ns | 82645 |
| `test_count_cl100k[medium]` | 5,157 | 672.200 μs | 779.902 μs | 715.600 μs | 182.569 μs | 86.500 μs | 1,282 | 151.2 ns | 1453 |
| `test_count_cl100k[large]` | 257,850 | 36.566 ms | 38.464 ms | 38.105 ms | 1.570 ms | 1.948 ms | 26 | 149.2 ns | 29 |
| `test_count_till_limit_cl100k[small]` | 10 | 1.200 μs | 1.314 μs | 1.230 μs | 529.039 ns | 20.000 ns | 760,784 | 131.4 ns | 82645 |
| `test_count_till_limit_cl100k[medium]` | 50 | 5.300 μs | 5.906 μs | 5.600 μs | 3.297 μs | 200.002 ns | 169,323 | 118.1 ns | 188682 |
| `test_count_till_limit_cl100k[large]` | 50 | 5.300 μs | 5.885 μs | 5.600 μs | 2.401 μs | 200.002 ns | 169,909 | 117.7 ns | 188682 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 147.100 μs | 160.087 μs | 149.400 μs | 38.401 μs | 3.900 μs | 6,247 | 160.1 ns | 6799 |
| `test_encode_batch_cl100k[medium_x10]` | 51,570 | 7.404 ms | 8.268 ms | 8.068 ms | 736.872 μs | 789.875 μs | 121 | 160.3 ns | 135 |
| `test_encode_batch_cl100k[large_x2]` | 515,700 | 80.663 ms | 84.478 ms | 83.095 ms | 3.154 ms | 5.508 ms | 12 | 163.8 ns | 13 |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 1.116 ms | 1.653 ms | 1.616 ms | 239.337 μs | 326.750 μs | 605 | 1653.0 ns | 828 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 51,570 | 6.668 ms | 7.543 ms | 7.384 ms | 705.340 μs | 610.325 μs | 133 | 146.3 ns | 151 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 515,700 | 72.190 ms | 75.250 ms | 74.114 ms | 3.368 ms | 3.842 ms | 13 | 145.9 ns | 15 |
| `test_decode_o200k[small]` | 10 | 249.000 ns | 277.986 ns | 262.000 ns | 93.091 ns | 12.000 ns | 3,597,301 | 27.8 ns | 40161 |
| `test_decode_o200k[medium]` | 5,212 | 67.300 μs | 76.694 μs | 69.700 μs | 26.671 μs | 5.200 μs | 13,039 | 14.7 ns | 14859 |
| `test_decode_o200k[large]` | 260,600 | 3.489 ms | 4.229 ms | 4.003 ms | 576.891 μs | 671.325 μs | 236 | 16.2 ns | 289 |
| `test_encode_o200k[small]` | 10 | 1.410 μs | 1.833 μs | 1.470 μs | 1.125 μs | 220.002 ns | 545,661 | 183.3 ns | 70923 |
| `test_encode_o200k[medium]` | 5,212 | 785.500 μs | 936.165 μs | 845.900 μs | 234.546 μs | 119.000 μs | 1,068 | 179.6 ns | 1280 |
| `test_encode_o200k[large]` | 260,600 | 43.490 ms | 47.403 ms | 47.029 ms | 2.040 ms | 2.262 ms | 21 | 181.9 ns | 23 |
| `test_roundtrip_o200k[small]` | 10 | 1.680 μs | 2.146 μs | 1.750 μs | 1.216 μs | 250.002 ns | 465,917 | 214.6 ns | 59524 |
| `test_roundtrip_o200k[medium]` | 5,212 | 867.100 μs | 1.054 ms | 946.700 μs | 262.353 μs | 193.925 μs | 949 | 202.2 ns | 1167 |
| `test_roundtrip_o200k[large]` | 260,600 | 52.191 ms | 59.132 ms | 56.336 ms | 8.350 ms | 8.317 ms | 17 | 226.9 ns | 21 |

## c_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_encode_cl100k[small]` | 10 | 709.994 ns | 803.940 ns | 760.006 ns | 307.093 ns | 29.989 ns | 1,243,873 | 80.4 ns | 140847 |
| `test_encode_cl100k[medium]` | 5,160 | 382.800 μs | 460.533 μs | 415.300 μs | 123.903 μs | 61.725 μs | 2,171 | 89.3 ns | 2599 |
| `test_encode_cl100k[large]` | 258,000 | 20.579 ms | 23.399 ms | 23.237 ms | 1.535 ms | 1.828 ms | 43 | 90.7 ns | 49 |
| `test_decode_cl100k[small]` | 10 | 158.000 ns | 173.485 ns | 165.999 ns | 46.970 ns | 4.000 ns | 5,764,194 | 17.3 ns | 62500 |
| `test_decode_cl100k[medium]` | 5,160 | 47.900 μs | 52.506 μs | 49.200 μs | 13.500 μs | 2.700 μs | 19,045 | 10.2 ns | 20877 |
| `test_decode_cl100k[large]` | 258,000 | 2.425 ms | 2.822 ms | 2.764 ms | 325.938 μs | 122.500 μs | 354 | 10.9 ns | 410 |
| `test_roundtrip_cl100k[small]` | 10 | 929.995 ns | 1.076 μs | 999.996 ns | 432.249 ns | 39.989 ns | 929,239 | 107.6 ns | 107527 |
| `test_roundtrip_cl100k[medium]` | 5,160 | 444.400 μs | 515.991 μs | 469.900 μs | 132.595 μs | 44.100 μs | 1,938 | 100.0 ns | 2248 |
| `test_roundtrip_cl100k[large]` | 258,000 | 24.228 ms | 26.745 ms | 26.066 ms | 2.106 ms | 2.257 ms | 37 | 103.7 ns | 42 |
| `test_count_cl100k[small]` | 10 | 579.993 ns | 667.349 ns | 610.005 ns | 302.966 ns | 10.012 ns | 1,498,466 | 66.7 ns | 169494 |
| `test_count_cl100k[medium]` | 5,160 | 329.600 μs | 396.240 μs | 351.900 μs | 118.711 μs | 47.075 μs | 2,524 | 76.8 ns | 3043 |
| `test_count_cl100k[large]` | 258,000 | 16.836 ms | 19.604 ms | 19.325 ms | 1.449 ms | 2.140 ms | 51 | 76.0 ns | 59 |
| `test_count_till_limit_cl100k[small]` | 10 | 1.230 μs | 1.475 μs | 1.280 μs | 711.012 ns | 50.000 ns | 678,051 | 147.5 ns | 80646 |
| `test_count_till_limit_cl100k[medium]` | 50 | 4.650 μs | 5.704 μs | 5.000 μs | 3.112 μs | 300.060 ns | 175,307 | 114.1 ns | 107528 |
| `test_count_till_limit_cl100k[large]` | 50 | 4.750 μs | 5.651 μs | 5.100 μs | 2.448 μs | 350.061 ns | 176,957 | 113.0 ns | 106384 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 67.000 μs | 75.128 μs | 69.300 μs | 19.160 μs | 4.400 μs | 13,311 | 75.1 ns | 15528 |
| `test_encode_batch_cl100k[medium_x10]` | 51,600 | 4.007 ms | 4.619 ms | 4.483 ms | 574.787 μs | 701.100 μs | 217 | 89.5 ns | 250 |
| `test_encode_batch_cl100k[large_x2]` | 516,000 | 41.687 ms | 46.382 ms | 46.233 ms | 2.659 ms | 3.553 ms | 22 | 89.9 ns | 24 |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 127.600 μs | 576.530 μs | 592.400 μs | 133.640 μs | 168.950 μs | 1,735 | 576.5 ns | 6641 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 51,600 | 1.104 ms | 1.531 ms | 1.484 ms | 212.159 μs | 240.150 μs | 653 | 29.7 ns | 937 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 516,000 | 24.848 ms | 27.009 ms | 26.603 ms | 1.740 ms | 2.370 ms | 37 | 52.3 ns | 40 |
| `test_decode_o200k[small]` | 10 | 138.000 ns | 156.675 ns | 144.000 ns | 81.876 ns | 5.000 ns | 6,382,637 | 15.7 ns | 71943 |
| `test_decode_o200k[medium]` | 5,215 | 48.600 μs | 54.162 μs | 51.300 μs | 12.894 μs | 3.700 μs | 18,463 | 10.4 ns | 20492 |
| `test_decode_o200k[large]` | 260,750 | 2.477 ms | 2.883 ms | 2.839 ms | 264.562 μs | 151.300 μs | 347 | 11.1 ns | 403 |
| `test_encode_o200k[small]` | 10 | 729.994 ns | 832.137 ns | 780.006 ns | 313.719 ns | 30.000 ns | 1,201,725 | 83.2 ns | 135137 |
| `test_encode_o200k[medium]` | 5,215 | 441.900 μs | 523.822 μs | 481.700 μs | 109.032 μs | 72.400 μs | 1,909 | 100.4 ns | 2273 |
| `test_encode_o200k[large]` | 260,750 | 24.436 ms | 27.563 ms | 27.360 ms | 1.933 ms | 2.752 ms | 36 | 105.7 ns | 41 |
| `test_roundtrip_o200k[small]` | 10 | 909.995 ns | 1.038 μs | 970.007 ns | 426.025 ns | 40.000 ns | 963,550 | 103.8 ns | 108697 |
| `test_roundtrip_o200k[medium]` | 5,215 | 507.000 μs | 594.940 μs | 548.150 μs | 124.172 μs | 76.900 μs | 1,681 | 114.1 ns | 1966 |
| `test_roundtrip_o200k[large]` | 260,750 | 26.997 ms | 30.373 ms | 30.810 ms | 1.587 ms | 2.459 ms | 33 | 116.5 ns | 38 |

## Comparison (median, rs_bpe vs c_bpe)

| Benchmark | rs_bpe | c_bpe | ratio |
| --- | --- | --- | --- |
| `test_count_cl100k[large]` | 38.105 ms | 19.325 ms | 1.97× |
| `test_count_cl100k[medium]` | 715.600 μs | 351.900 μs | 2.03× |
| `test_count_cl100k[small]` | 1.230 μs | 610.005 ns | 2.02× |
| `test_count_till_limit_cl100k[large]` | 5.600 μs | 5.100 μs | 1.10× |
| `test_count_till_limit_cl100k[medium]` | 5.600 μs | 5.000 μs | 1.12× |
| `test_count_till_limit_cl100k[small]` | 1.230 μs | 1.280 μs | **0.96×** |
| `test_decode_cl100k[large]` | 3.837 ms | 2.764 ms | 1.39× |
| `test_decode_cl100k[medium]` | 67.600 μs | 49.200 μs | 1.37× |
| `test_decode_cl100k[small]` | 284.213 ns | 165.999 ns | 1.71× |
| `test_decode_o200k[large]` | 4.003 ms | 2.839 ms | 1.41× |
| `test_decode_o200k[medium]` | 69.700 μs | 51.300 μs | 1.36× |
| `test_decode_o200k[small]` | 262.000 ns | 144.000 ns | 1.82× |
| `test_encode_batch_cl100k[large_x2]` | 83.095 ms | 46.233 ms | 1.80× |
| `test_encode_batch_cl100k[medium_x10]` | 8.068 ms | 4.483 ms | 1.80× |
| `test_encode_batch_cl100k[small_x100]` | 149.400 μs | 69.300 μs | 2.16× |
| `test_encode_batch_parallel_cl100k[large_x2]` | 74.114 ms | 26.603 ms | 2.79× |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 7.384 ms | 1.484 ms | 4.98× |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1.616 ms | 592.400 μs | 2.73× |
| `test_encode_cl100k[large]` | 41.689 ms | 23.237 ms | 1.79× |
| `test_encode_cl100k[medium]` | 788.600 μs | 415.300 μs | 1.90× |
| `test_encode_cl100k[small]` | 1.490 μs | 760.006 ns | 1.96× |
| `test_encode_o200k[large]` | 47.029 ms | 27.360 ms | 1.72× |
| `test_encode_o200k[medium]` | 845.900 μs | 481.700 μs | 1.76× |
| `test_encode_o200k[small]` | 1.470 μs | 780.006 ns | 1.88× |
| `test_roundtrip_cl100k[large]` | 46.648 ms | 26.066 ms | 1.79× |
| `test_roundtrip_cl100k[medium]` | 843.600 μs | 469.900 μs | 1.80× |
| `test_roundtrip_cl100k[small]` | 1.820 μs | 999.996 ns | 1.82× |
| `test_roundtrip_o200k[large]` | 56.336 ms | 30.810 ms | 1.83× |
| `test_roundtrip_o200k[medium]` | 946.700 μs | 548.150 μs | 1.73× |
| `test_roundtrip_o200k[small]` | 1.750 μs | 970.007 ns | 1.80× |
