# Benchmark Results

## rs_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_import_cl100k` | n/a | 105.862 μs | 128.041 μs | 113.218 μs | 64.920 μs | 9.565 μs | 7,810 | n/a | 9457 |
| `test_import_o200k` | n/a | 102.244 μs | 121.669 μs | 108.543 μs | 55.058 μs | 9.145 μs | 8,219 | n/a | 9787 |
| `test_encode_cl100k[small]` | 10 | 581.200 ns | 663.336 ns | 611.600 ns | 343.091 ns | 43.700 ns | 1,507,532 | 66.3 ns | 169866 |
| `test_encode_cl100k[medium]` | 5,157 | 342.841 μs | 397.111 μs | 369.918 μs | 117.906 μs | 25.939 μs | 2,518 | 77.0 ns | 2947 |
| `test_encode_cl100k[large]` | 257,850 | 19.596 ms | 22.186 ms | 21.520 ms | 1.730 ms | 2.815 ms | 45 | 86.0 ns | 51 |
| `test_decode_cl100k[small]` | 10 | 141.180 ns | 159.536 ns | 151.460 ns | 48.994 ns | 8.320 ns | 6,268,158 | 16.0 ns | 71261 |
| `test_decode_cl100k[medium]` | 5,157 | 35.422 μs | 40.839 μs | 37.609 μs | 14.912 μs | 2.658 μs | 24,487 | 7.9 ns | 27811 |
| `test_decode_cl100k[large]` | 257,850 | 1.863 ms | 2.113 ms | 1.972 ms | 434.553 μs | 151.393 μs | 473 | 8.2 ns | 533 |
| `test_roundtrip_cl100k[small]` | 10 | 760.400 ns | 882.231 ns | 830.800 ns | 456.570 ns | 36.400 ns | 1,133,490 | 88.2 ns | 129266 |
| `test_roundtrip_cl100k[medium]` | 5,157 | 402.914 μs | 470.109 μs | 430.555 μs | 153.006 μs | 29.325 μs | 2,127 | 91.2 ns | 2441 |
| `test_roundtrip_cl100k[large]` | 257,850 | 21.510 ms | 23.607 ms | 23.171 ms | 1.606 ms | 1.906 ms | 42 | 91.6 ns | 46 |
| `test_count_cl100k[small]` | 10 | 473.909 ns | 538.549 ns | 500.000 ns | 378.217 ns | 13.091 ns | 1,856,840 | 53.9 ns | 190913 |
| `test_count_cl100k[medium]` | 5,157 | 292.177 μs | 336.639 μs | 310.541 μs | 102.712 μs | 20.107 μs | 2,971 | 65.3 ns | 3438 |
| `test_count_cl100k[large]` | 257,850 | 15.392 ms | 17.191 ms | 16.830 ms | 1.440 ms | 1.598 ms | 58 | 66.7 ns | 66 |
| `test_count_till_limit_cl100k[small]` | 10 | 518.500 ns | 573.522 ns | 542.300 ns | 360.269 ns | 9.701 ns | 1,743,612 | 57.4 ns | 198256 |
| `test_count_till_limit_cl100k[medium]` | 50 | 2.243 μs | 2.477 μs | 2.348 μs | 959.429 ns | 61.400 ns | 403,768 | 49.5 ns | 43158 |
| `test_count_till_limit_cl100k[large]` | 50 | 2.176 μs | 2.451 μs | 2.275 μs | 1.024 μs | 58.101 ns | 408,028 | 49.0 ns | 45999 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 62.329 μs | 69.310 μs | 65.641 μs | 18.713 μs | 2.060 μs | 14,428 | 69.3 ns | 16493 |
| `test_encode_batch_cl100k[medium_x10]` | 51,570 | 3.604 ms | 3.996 ms | 3.754 ms | 715.517 μs | 164.904 μs | 250 | 77.5 ns | 283 |
| `test_encode_batch_cl100k[large_x2]` | 515,700 | 39.400 ms | 42.650 ms | 42.902 ms | 1.570 ms | 1.799 ms | 23 | 82.7 ns | 26 |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 1.746 ms | 3.055 ms | 2.297 ms | 2.107 ms | 1.564 ms | 327 | 3055.2 ns | 5 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 51,570 | 3.518 ms | 3.814 ms | 3.581 ms | 402.452 μs | 553.667 μs | 262 | 74.0 ns | 5 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 515,700 | 38.623 ms | 40.271 ms | 40.003 ms | 1.394 ms | 2.428 ms | 25 | 78.1 ns | 5 |
| `test_decode_o200k[small]` | 10 | 143.410 ns | 163.996 ns | 151.790 ns | 79.186 ns | 7.030 ns | 6,097,720 | 16.4 ns | 69344 |
| `test_decode_o200k[medium]` | 5,212 | 37.789 μs | 42.550 μs | 39.214 μs | 17.311 μs | 1.933 μs | 23,502 | 8.2 ns | 26350 |
| `test_decode_o200k[large]` | 260,600 | 1.905 ms | 2.224 ms | 2.065 ms | 446.667 μs | 217.635 μs | 450 | 8.5 ns | 525 |
| `test_encode_o200k[small]` | 10 | 637.900 ns | 740.698 ns | 681.600 ns | 613.119 ns | 24.901 ns | 1,350,078 | 74.1 ns | 154226 |
| `test_encode_o200k[medium]` | 5,212 | 358.321 μs | 419.890 μs | 383.955 μs | 147.973 μs | 21.366 μs | 2,382 | 80.6 ns | 2708 |
| `test_encode_o200k[large]` | 260,600 | 19.670 ms | 22.768 ms | 22.038 ms | 2.261 ms | 3.200 ms | 44 | 87.4 ns | 51 |
| `test_roundtrip_o200k[small]` | 10 | 763.600 ns | 907.880 ns | 843.600 ns | 580.328 ns | 33.700 ns | 1,101,467 | 90.8 ns | 131028 |
| `test_roundtrip_o200k[medium]` | 5,212 | 422.841 μs | 488.176 μs | 447.844 μs | 156.306 μs | 27.865 μs | 2,048 | 93.7 ns | 2333 |
| `test_roundtrip_o200k[large]` | 260,600 | 22.942 ms | 25.065 ms | 24.898 ms | 1.374 ms | 1.744 ms | 40 | 96.2 ns | 45 |

## c_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_import_cl100k` | n/a | 97.812 μs | 113.048 μs | 102.416 μs | 48.861 μs | 5.448 μs | 8,846 | n/a | 10205 |
| `test_import_o200k` | n/a | 98.350 μs | 112.949 μs | 103.139 μs | 45.826 μs | 6.268 μs | 8,854 | n/a | 10426 |
| `test_encode_cl100k[small]` | 10 | 509.300 ns | 571.708 ns | 539.900 ns | 313.349 ns | 17.100 ns | 1,749,144 | 57.2 ns | 191535 |
| `test_encode_cl100k[medium]` | 5,160 | 372.636 μs | 451.157 μs | 405.927 μs | 151.416 μs | 37.180 μs | 2,217 | 87.4 ns | 2644 |
| `test_encode_cl100k[large]` | 258,000 | 20.504 ms | 22.574 ms | 22.100 ms | 1.666 ms | 1.922 ms | 44 | 87.5 ns | 49 |
| `test_decode_cl100k[small]` | 10 | 76.640 ns | 86.427 ns | 80.570 ns | 54.271 ns | 3.620 ns | 11,570,481 | 8.6 ns | 131979 |
| `test_decode_cl100k[medium]` | 5,160 | 21.602 μs | 24.585 μs | 22.821 μs | 9.535 μs | 1.119 μs | 40,675 | 4.8 ns | 45288 |
| `test_decode_cl100k[large]` | 258,000 | 1.113 ms | 1.302 ms | 1.188 ms | 351.132 μs | 102.035 μs | 768 | 5.0 ns | 897 |
| `test_roundtrip_cl100k[small]` | 10 | 611.000 ns | 719.313 ns | 658.400 ns | 496.204 ns | 27.200 ns | 1,390,216 | 71.9 ns | 156618 |
| `test_roundtrip_cl100k[medium]` | 5,160 | 419.525 μs | 499.618 μs | 449.158 μs | 181.914 μs | 37.429 μs | 2,002 | 96.8 ns | 2371 |
| `test_roundtrip_cl100k[large]` | 258,000 | 22.551 ms | 25.254 ms | 25.015 ms | 1.692 ms | 2.582 ms | 40 | 97.9 ns | 44 |
| `test_count_cl100k[small]` | 10 | 420.917 ns | 481.435 ns | 448.000 ns | 348.054 ns | 16.167 ns | 2,077,122 | 48.1 ns | 197512 |
| `test_count_cl100k[medium]` | 5,160 | 320.785 μs | 424.745 μs | 373.756 μs | 162.277 μs | 88.403 μs | 2,354 | 82.3 ns | 3096 |
| `test_count_cl100k[large]` | 258,000 | 16.668 ms | 19.054 ms | 19.075 ms | 1.815 ms | 2.884 ms | 52 | 73.9 ns | 61 |
| `test_count_till_limit_cl100k[small]` | 10 | 470.364 ns | 537.634 ns | 505.637 ns | 454.118 ns | 13.181 ns | 1,860,002 | 53.8 ns | 191535 |
| `test_count_till_limit_cl100k[medium]` | 50 | 2.078 μs | 2.337 μs | 2.179 μs | 980.264 ns | 54.200 ns | 427,901 | 46.7 ns | 47251 |
| `test_count_till_limit_cl100k[large]` | 50 | 2.073 μs | 2.286 μs | 2.148 μs | 977.806 ns | 62.300 ns | 437,360 | 45.7 ns | 46233 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 54.354 μs | 61.088 μs | 56.981 μs | 20.528 μs | 1.854 μs | 16,370 | 61.1 ns | 18424 |
| `test_encode_batch_cl100k[medium_x10]` | 51,600 | 3.922 ms | 4.763 ms | 4.571 ms | 698.205 μs | 1.018 ms | 210 | 92.3 ns | 255 |
| `test_encode_batch_cl100k[large_x2]` | 516,000 | 45.652 ms | 49.682 ms | 49.465 ms | 2.949 ms | 2.850 ms | 20 | 96.3 ns | 22 |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 181.204 μs | 581.817 μs | 323.945 μs | 475.019 μs | 733.778 μs | 1,719 | 581.8 ns | 5 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 51,600 | 1.284 ms | 1.787 ms | 1.523 ms | 637.251 μs | 823.490 μs | 560 | 34.6 ns | 5 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 516,000 | 26.400 ms | 27.889 ms | 27.763 ms | 1.089 ms | 1.609 ms | 36 | 54.0 ns | 5 |
| `test_decode_o200k[small]` | 10 | 76.050 ns | 86.402 ns | 81.040 ns | 45.545 ns | 1.330 ns | 11,573,814 | 8.6 ns | 131011 |
| `test_decode_o200k[medium]` | 5,215 | 21.191 μs | 24.571 μs | 22.670 μs | 12.089 μs | 740.998 ns | 40,699 | 4.7 ns | 47062 |
| `test_decode_o200k[large]` | 260,750 | 1.095 ms | 1.325 ms | 1.178 ms | 416.630 μs | 149.134 μs | 755 | 5.1 ns | 910 |
| `test_encode_o200k[small]` | 10 | 511.100 ns | 581.785 ns | 539.900 ns | 399.107 ns | 16.100 ns | 1,718,849 | 58.2 ns | 198413 |
| `test_encode_o200k[medium]` | 5,215 | 424.086 μs | 508.770 μs | 458.049 μs | 153.815 μs | 61.258 μs | 1,966 | 97.6 ns | 2344 |
| `test_encode_o200k[large]` | 260,750 | 24.438 ms | 31.926 ms | 30.896 ms | 4.642 ms | 5.492 ms | 31 | 122.4 ns | 39 |
| `test_roundtrip_o200k[small]` | 10 | 598.800 ns | 695.702 ns | 636.400 ns | 560.635 ns | 42.300 ns | 1,437,398 | 69.6 ns | 165454 |
| `test_roundtrip_o200k[medium]` | 5,215 | 474.754 μs | 576.261 μs | 518.196 μs | 188.193 μs | 60.601 μs | 1,735 | 110.5 ns | 2063 |
| `test_roundtrip_o200k[large]` | 260,750 | 25.798 ms | 29.606 ms | 29.233 ms | 3.292 ms | 2.133 ms | 34 | 113.5 ns | 39 |

## Comparison (median, rs_bpe vs c_bpe)

| Benchmark | rs_bpe | c_bpe | ratio |
| --- | --- | --- | --- |
| `test_count_cl100k[large]` | 16.830 ms | 19.075 ms | **0.88×** |
| `test_count_cl100k[medium]` | 310.541 μs | 373.756 μs | **0.83×** |
| `test_count_cl100k[small]` | 500.000 ns | 448.000 ns | 1.12× |
| `test_count_till_limit_cl100k[large]` | 2.275 μs | 2.148 μs | 1.06× |
| `test_count_till_limit_cl100k[medium]` | 2.348 μs | 2.179 μs | 1.08× |
| `test_count_till_limit_cl100k[small]` | 542.300 ns | 505.637 ns | 1.07× |
| `test_decode_cl100k[large]` | 1.972 ms | 1.188 ms | 1.66× |
| `test_decode_cl100k[medium]` | 37.609 μs | 22.821 μs | 1.65× |
| `test_decode_cl100k[small]` | 151.460 ns | 80.570 ns | 1.88× |
| `test_decode_o200k[large]` | 2.065 ms | 1.178 ms | 1.75× |
| `test_decode_o200k[medium]` | 39.214 μs | 22.670 μs | 1.73× |
| `test_decode_o200k[small]` | 151.790 ns | 81.040 ns | 1.87× |
| `test_encode_batch_cl100k[large_x2]` | 42.902 ms | 49.465 ms | **0.87×** |
| `test_encode_batch_cl100k[medium_x10]` | 3.754 ms | 4.571 ms | **0.82×** |
| `test_encode_batch_cl100k[small_x100]` | 65.641 μs | 56.981 μs | 1.15× |
| `test_encode_batch_parallel_cl100k[large_x2]` | 40.003 ms | 27.763 ms | 1.44× |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 3.581 ms | 1.523 ms | 2.35× |
| `test_encode_batch_parallel_cl100k[small_x100]` | 2.297 ms | 323.945 μs | 7.09× |
| `test_encode_cl100k[large]` | 21.520 ms | 22.100 ms | **0.97×** |
| `test_encode_cl100k[medium]` | 369.918 μs | 405.927 μs | **0.91×** |
| `test_encode_cl100k[small]` | 611.600 ns | 539.900 ns | 1.13× |
| `test_encode_o200k[large]` | 22.038 ms | 30.896 ms | **0.71×** |
| `test_encode_o200k[medium]` | 383.955 μs | 458.049 μs | **0.84×** |
| `test_encode_o200k[small]` | 681.600 ns | 539.900 ns | 1.26× |
| `test_import_cl100k` | 113.218 μs | 102.416 μs | 1.11× |
| `test_import_o200k` | 108.543 μs | 103.139 μs | 1.05× |
| `test_roundtrip_cl100k[large]` | 23.171 ms | 25.015 ms | **0.93×** |
| `test_roundtrip_cl100k[medium]` | 430.555 μs | 449.158 μs | **0.96×** |
| `test_roundtrip_cl100k[small]` | 830.800 ns | 658.400 ns | 1.26× |
| `test_roundtrip_o200k[large]` | 24.898 ms | 29.233 ms | **0.85×** |
| `test_roundtrip_o200k[medium]` | 447.844 μs | 518.196 μs | **0.86×** |
| `test_roundtrip_o200k[small]` | 843.600 ns | 636.400 ns | 1.33× |
