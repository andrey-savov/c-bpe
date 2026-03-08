# Benchmark Results

## rs_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_import_cl100k` | n/a | 107.049 μs | 136.204 μs | 117.905 μs | 127.626 μs | 20.747 μs | 7,342 | n/a | 9438 |
| `test_import_o200k` | n/a | 106.615 μs | 136.628 μs | 118.486 μs | 64.521 μs | 20.798 μs | 7,319 | n/a | 9358 |
| `test_encode_cl100k[small]` | 10 | 620.800 ns | 711.897 ns | 673.300 ns | 267.670 ns | 36.199 ns | 1,404,697 | 71.2 ns | 162602 |
| `test_encode_cl100k[medium]` | 5,157 | 343.571 μs | 423.801 μs | 370.196 μs | 200.758 μs | 31.917 μs | 2,360 | 82.2 ns | 2802 |
| `test_encode_cl100k[large]` | 257,850 | 21.572 ms | 23.436 ms | 23.044 ms | 1.330 ms | 2.620 ms | 43 | 90.9 ns | 47 |
| `test_decode_cl100k[small]` | 10 | 137.630 ns | 160.640 ns | 152.120 ns | 44.279 ns | 13.960 ns | 6,225,112 | 16.1 ns | 72119 |
| `test_decode_cl100k[medium]` | 5,157 | 37.247 μs | 41.488 μs | 39.030 μs | 12.286 μs | 1.832 μs | 24,103 | 8.0 ns | 25948 |
| `test_decode_cl100k[large]` | 257,850 | 1.942 ms | 2.224 ms | 2.131 ms | 347.191 μs | 122.527 μs | 450 | 8.6 ns | 510 |
| `test_roundtrip_cl100k[small]` | 10 | 802.100 ns | 948.540 ns | 885.800 ns | 511.447 ns | 47.800 ns | 1,054,251 | 94.9 ns | 122295 |
| `test_roundtrip_cl100k[medium]` | 5,157 | 406.407 μs | 581.115 μs | 461.514 μs | 254.859 μs | 107.676 μs | 1,721 | 112.7 ns | 2356 |
| `test_roundtrip_cl100k[large]` | 257,850 | 24.235 ms | 26.330 ms | 26.092 ms | 1.409 ms | 1.172 ms | 38 | 102.1 ns | 43 |
| `test_count_cl100k[small]` | 10 | 528.100 ns | 591.830 ns | 576.400 ns | 219.829 ns | 30.600 ns | 1,689,673 | 59.2 ns | 189718 |
| `test_count_cl100k[medium]` | 5,157 | 310.070 μs | 342.036 μs | 331.052 μs | 74.340 μs | 13.430 μs | 2,924 | 66.3 ns | 3145 |
| `test_count_cl100k[large]` | 257,850 | 16.283 ms | 17.854 ms | 17.188 ms | 1.635 ms | 1.776 ms | 56 | 69.2 ns | 62 |
| `test_count_till_limit_cl100k[small]` | 10 | 513.700 ns | 566.940 ns | 542.800 ns | 206.851 ns | 25.801 ns | 1,763,854 | 56.7 ns | 194629 |
| `test_count_till_limit_cl100k[medium]` | 50 | 2.330 μs | 2.678 μs | 2.580 μs | 670.001 ns | 80.201 ns | 373,434 | 53.6 ns | 42623 |
| `test_count_till_limit_cl100k[large]` | 50 | 2.295 μs | 2.617 μs | 2.512 μs | 732.805 ns | 89.800 ns | 382,179 | 52.3 ns | 41579 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 65.501 μs | 72.971 μs | 70.849 μs | 14.397 μs | 2.455 μs | 13,704 | 73.0 ns | 15647 |
| `test_encode_batch_cl100k[medium_x10]` | 51,570 | 3.697 ms | 4.022 ms | 3.903 ms | 513.355 μs | 118.151 μs | 249 | 78.0 ns | 265 |
| `test_encode_batch_cl100k[large_x2]` | 515,700 | 41.807 ms | 45.434 ms | 45.799 ms | 2.962 ms | 4.423 ms | 22 | 88.1 ns | 24 |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 1.424 ms | 2.033 ms | 1.957 ms | 323.764 μs | 348.890 μs | 492 | 2033.4 ns | 682 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 51,570 | 3.659 ms | 4.102 ms | 3.946 ms | 574.233 μs | 224.880 μs | 244 | 79.5 ns | 275 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 515,700 | 41.275 ms | 44.468 ms | 44.494 ms | 2.224 ms | 3.174 ms | 22 | 86.2 ns | 25 |
| `test_decode_o200k[small]` | 10 | 142.590 ns | 162.718 ns | 153.120 ns | 55.428 ns | 16.280 ns | 6,145,597 | 16.3 ns | 69508 |
| `test_decode_o200k[medium]` | 5,212 | 39.263 μs | 42.969 μs | 40.641 μs | 11.589 μs | 1.727 μs | 23,273 | 8.2 ns | 27853 |
| `test_decode_o200k[large]` | 260,600 | 1.995 ms | 2.318 ms | 2.206 ms | 453.475 μs | 142.320 μs | 431 | 8.9 ns | 503 |
| `test_encode_o200k[small]` | 10 | 651.300 ns | 759.049 ns | 700.800 ns | 467.936 ns | 42.099 ns | 1,317,439 | 75.9 ns | 158303 |
| `test_encode_o200k[medium]` | 5,212 | 368.045 μs | 406.477 μs | 391.489 μs | 90.539 μs | 19.512 μs | 2,460 | 78.0 ns | 2654 |
| `test_encode_o200k[large]` | 260,600 | 21.324 ms | 23.103 ms | 22.768 ms | 1.618 ms | 2.123 ms | 43 | 88.7 ns | 48 |
| `test_roundtrip_o200k[small]` | 10 | 791.500 ns | 908.853 ns | 862.100 ns | 347.106 ns | 45.800 ns | 1,100,288 | 90.9 ns | 126423 |
| `test_roundtrip_o200k[medium]` | 5,212 | 426.421 μs | 488.937 μs | 456.788 μs | 136.339 μs | 33.573 μs | 2,045 | 93.8 ns | 2265 |
| `test_roundtrip_o200k[large]` | 260,600 | 23.716 ms | 25.269 ms | 24.554 ms | 1.631 ms | 1.996 ms | 40 | 97.0 ns | 43 |

## c_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_import_cl100k` | n/a | 109.871 μs | 133.963 μs | 118.915 μs | 49.662 μs | 20.742 μs | 7,465 | n/a | 9235 |
| `test_import_o200k` | n/a | 104.968 μs | 121.449 μs | 112.048 μs | 42.383 μs | 9.426 μs | 8,234 | n/a | 9610 |
| `test_encode_cl100k[small]` | 10 | 514.300 ns | 581.804 ns | 558.500 ns | 247.578 ns | 20.200 ns | 1,718,792 | 58.2 ns | 187759 |
| `test_encode_cl100k[medium]` | 5,160 | 370.595 μs | 425.361 μs | 400.219 μs | 116.989 μs | 25.359 μs | 2,351 | 82.4 ns | 2659 |
| `test_encode_cl100k[large]` | 258,000 | 21.663 ms | 23.602 ms | 22.822 ms | 2.013 ms | 1.801 ms | 42 | 91.5 ns | 47 |
| `test_decode_cl100k[small]` | 10 | 74.120 ns | 86.405 ns | 83.600 ns | 29.870 ns | 2.970 ns | 11,573,451 | 8.6 ns | 135630 |
| `test_decode_cl100k[medium]` | 5,160 | 21.220 μs | 24.096 μs | 22.625 μs | 9.724 μs | 1.395 μs | 41,501 | 4.7 ns | 44920 |
| `test_decode_cl100k[large]` | 258,000 | 1.085 ms | 1.698 ms | 1.640 ms | 514.682 μs | 552.264 μs | 589 | 6.6 ns | 920 |
| `test_roundtrip_cl100k[small]` | 10 | 610.700 ns | 788.355 ns | 659.300 ns | 494.881 ns | 299.500 ns | 1,268,464 | 78.8 ns | 162602 |
| `test_roundtrip_cl100k[medium]` | 5,160 | 429.288 μs | 489.459 μs | 460.793 μs | 137.725 μs | 26.215 μs | 2,043 | 94.9 ns | 2366 |
| `test_roundtrip_cl100k[large]` | 258,000 | 23.852 ms | 25.525 ms | 24.707 ms | 1.680 ms | 2.562 ms | 39 | 98.9 ns | 43 |
| `test_count_cl100k[small]` | 10 | 418.417 ns | 467.617 ns | 444.833 ns | 176.959 ns | 15.083 ns | 2,138,504 | 46.8 ns | 192865 |
| `test_count_cl100k[medium]` | 5,160 | 332.842 μs | 360.364 μs | 351.140 μs | 65.159 μs | 16.588 μs | 2,775 | 69.8 ns | 2994 |
| `test_count_cl100k[large]` | 258,000 | 16.470 ms | 18.337 ms | 17.340 ms | 2.000 ms | 2.838 ms | 55 | 71.1 ns | 61 |
| `test_count_till_limit_cl100k[small]` | 10 | 470.455 ns | 532.350 ns | 511.545 ns | 218.515 ns | 14.182 ns | 1,878,464 | 53.2 ns | 196194 |
| `test_count_till_limit_cl100k[medium]` | 50 | 2.106 μs | 2.381 μs | 2.293 μs | 665.736 ns | 136.501 ns | 419,906 | 47.6 ns | 47371 |
| `test_count_till_limit_cl100k[large]` | 50 | 2.112 μs | 2.307 μs | 2.225 μs | 708.753 ns | 73.400 ns | 433,449 | 46.1 ns | 46293 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 53.825 μs | 59.380 μs | 57.054 μs | 14.221 μs | 1.211 μs | 16,841 | 59.4 ns | 18187 |
| `test_encode_batch_cl100k[medium_x10]` | 51,600 | 4.053 ms | 4.372 ms | 4.239 ms | 488.709 μs | 222.303 μs | 229 | 84.7 ns | 254 |
| `test_encode_batch_cl100k[large_x2]` | 516,000 | 42.626 ms | 44.478 ms | 43.484 ms | 1.799 ms | 3.003 ms | 22 | 86.2 ns | 24 |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 75.980 μs | 371.075 μs | 277.539 μs | 3.417 ms | 126.855 μs | 2,695 | 371.1 ns | 10144 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 51,600 | 2.006 ms | 3.195 ms | 2.351 ms | 13.440 ms | 329.985 μs | 313 | 61.9 ns | 488 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 516,000 | 33.629 ms | 37.026 ms | 36.911 ms | 2.456 ms | 3.967 ms | 27 | 71.8 ns | 32 |
| `test_decode_o200k[small]` | 10 | 72.020 ns | 80.926 ns | 78.080 ns | 26.743 ns | 2.340 ns | 12,357,004 | 8.1 ns | 138909 |
| `test_decode_o200k[medium]` | 5,215 | 21.408 μs | 23.320 μs | 22.184 μs | 7.899 μs | 786.000 ns | 42,881 | 4.5 ns | 45732 |
| `test_decode_o200k[large]` | 260,750 | 1.269 ms | 1.532 ms | 1.484 ms | 271.137 μs | 114.595 μs | 653 | 5.9 ns | 741 |
| `test_encode_o200k[small]` | 10 | 503.200 ns | 549.735 ns | 527.599 ns | 278.716 ns | 23.100 ns | 1,819,057 | 55.0 ns | 197434 |
| `test_encode_o200k[medium]` | 5,215 | 421.825 μs | 474.275 μs | 446.563 μs | 112.295 μs | 32.025 μs | 2,108 | 90.9 ns | 2366 |
| `test_encode_o200k[large]` | 260,750 | 23.826 ms | 25.741 ms | 24.593 ms | 2.420 ms | 2.323 ms | 39 | 98.7 ns | 42 |
| `test_roundtrip_o200k[small]` | 10 | 621.500 ns | 724.286 ns | 702.200 ns | 313.210 ns | 37.499 ns | 1,380,671 | 72.4 ns | 161891 |
| `test_roundtrip_o200k[medium]` | 5,215 | 505.907 μs | 571.806 μs | 541.219 μs | 127.262 μs | 31.734 μs | 1,749 | 109.6 ns | 1979 |
| `test_roundtrip_o200k[large]` | 260,750 | 26.233 ms | 29.926 ms | 30.154 ms | 2.300 ms | 2.320 ms | 33 | 114.8 ns | 37 |

## Comparison (median, rs_bpe vs c_bpe)

| Benchmark | rs_bpe | c_bpe | ratio |
| --- | --- | --- | --- |
| `test_count_cl100k[large]` | 17.188 ms | 17.340 ms | **0.99×** |
| `test_count_cl100k[medium]` | 331.052 μs | 351.140 μs | **0.94×** |
| `test_count_cl100k[small]` | 576.400 ns | 444.833 ns | 1.30× |
| `test_count_till_limit_cl100k[large]` | 2.512 μs | 2.225 μs | 1.13× |
| `test_count_till_limit_cl100k[medium]` | 2.580 μs | 2.293 μs | 1.13× |
| `test_count_till_limit_cl100k[small]` | 542.800 ns | 511.545 ns | 1.06× |
| `test_decode_cl100k[large]` | 2.131 ms | 1.640 ms | 1.30× |
| `test_decode_cl100k[medium]` | 39.030 μs | 22.625 μs | 1.73× |
| `test_decode_cl100k[small]` | 152.120 ns | 83.600 ns | 1.82× |
| `test_decode_o200k[large]` | 2.206 ms | 1.484 ms | 1.49× |
| `test_decode_o200k[medium]` | 40.641 μs | 22.184 μs | 1.83× |
| `test_decode_o200k[small]` | 153.120 ns | 78.080 ns | 1.96× |
| `test_encode_batch_cl100k[large_x2]` | 45.799 ms | 43.484 ms | 1.05× |
| `test_encode_batch_cl100k[medium_x10]` | 3.903 ms | 4.239 ms | **0.92×** |
| `test_encode_batch_cl100k[small_x100]` | 70.849 μs | 57.054 μs | 1.24× |
| `test_encode_batch_parallel_cl100k[large_x2]` | 44.494 ms | 36.911 ms | 1.21× |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 3.946 ms | 2.351 ms | 1.68× |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1.957 ms | 277.539 μs | 7.05× |
| `test_encode_cl100k[large]` | 23.044 ms | 22.822 ms | 1.01× |
| `test_encode_cl100k[medium]` | 370.196 μs | 400.219 μs | **0.92×** |
| `test_encode_cl100k[small]` | 673.300 ns | 558.500 ns | 1.21× |
| `test_encode_o200k[large]` | 22.768 ms | 24.593 ms | **0.93×** |
| `test_encode_o200k[medium]` | 391.489 μs | 446.563 μs | **0.88×** |
| `test_encode_o200k[small]` | 700.800 ns | 527.599 ns | 1.33× |
| `test_import_cl100k` | 117.905 μs | 118.915 μs | **0.99×** |
| `test_import_o200k` | 118.486 μs | 112.048 μs | 1.06× |
| `test_roundtrip_cl100k[large]` | 26.092 ms | 24.707 ms | 1.06× |
| `test_roundtrip_cl100k[medium]` | 461.514 μs | 460.793 μs | 1.00× |
| `test_roundtrip_cl100k[small]` | 885.800 ns | 659.300 ns | 1.34× |
| `test_roundtrip_o200k[large]` | 24.554 ms | 30.154 ms | **0.81×** |
| `test_roundtrip_o200k[medium]` | 456.788 μs | 541.219 μs | **0.84×** |
| `test_roundtrip_o200k[small]` | 862.100 ns | 702.200 ns | 1.23× |
