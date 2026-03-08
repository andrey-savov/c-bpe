# Benchmark Results

## rs_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_encode_cl100k[small]` | 10 | 1.410 μs | 1.538 μs | 1.460 μs | 444.607 ns | 60.000 ns | 650,042 | 153.8 ns | 71429 |
| `test_encode_cl100k[medium]` | 5,157 | 714.400 μs | 820.952 μs | 756.400 μs | 156.522 μs | 109.275 μs | 1,218 | 159.2 ns | 1393 |
| `test_encode_cl100k[large]` | 257,850 | 39.839 ms | 42.091 ms | 42.151 ms | 1.103 ms | 1.716 ms | 24 | 163.2 ns | 26 |
| `test_decode_cl100k[small]` | 10 | 273.680 ns | 304.529 ns | 284.213 ns | 189.820 ns | 15.790 ns | 3,283,761 | 30.5 ns | 192311 |
| `test_decode_cl100k[medium]` | 5,157 | 66.500 μs | 75.946 μs | 72.300 μs | 17.306 μs | 5.000 μs | 13,167 | 14.7 ns | 14926 |
| `test_decode_cl100k[large]` | 257,850 | 3.359 ms | 3.934 ms | 3.817 ms | 458.375 μs | 365.500 μs | 254 | 15.3 ns | 295 |
| `test_roundtrip_cl100k[small]` | 10 | 1.740 μs | 1.910 μs | 1.800 μs | 636.012 ns | 100.012 ns | 523,672 | 191.0 ns | 57472 |
| `test_roundtrip_cl100k[medium]` | 5,157 | 792.400 μs | 905.488 μs | 838.300 μs | 179.208 μs | 91.100 μs | 1,104 | 175.6 ns | 1257 |
| `test_roundtrip_cl100k[large]` | 257,850 | 42.372 ms | 46.623 ms | 46.154 ms | 2.281 ms | 2.175 ms | 21 | 180.8 ns | 24 |
| `test_count_cl100k[small]` | 10 | 1.210 μs | 1.344 μs | 1.250 μs | 537.299 ns | 70.001 ns | 743,878 | 134.4 ns | 81968 |
| `test_count_cl100k[medium]` | 5,157 | 665.300 μs | 769.254 μs | 714.100 μs | 148.638 μs | 95.075 μs | 1,300 | 149.2 ns | 1513 |
| `test_count_cl100k[large]` | 257,850 | 34.438 ms | 39.606 ms | 39.041 ms | 3.318 ms | 3.768 ms | 25 | 153.6 ns | 28 |
| `test_count_till_limit_cl100k[small]` | 10 | 1.200 μs | 1.306 μs | 1.240 μs | 473.723 ns | 30.012 ns | 765,639 | 130.6 ns | 82645 |
| `test_count_till_limit_cl100k[medium]` | 50 | 5.200 μs | 5.735 μs | 5.500 μs | 2.804 μs | 200.002 ns | 174,363 | 114.7 ns | 192311 |
| `test_count_till_limit_cl100k[large]` | 50 | 5.200 μs | 5.941 μs | 5.500 μs | 9.334 μs | 300.002 ns | 168,332 | 118.8 ns | 192311 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 145.000 μs | 156.241 μs | 147.700 μs | 34.279 μs | 4.500 μs | 6,400 | 156.2 ns | 6907 |
| `test_encode_batch_cl100k[medium_x10]` | 51,570 | 7.329 ms | 8.748 ms | 8.308 ms | 1.699 ms | 1.074 ms | 114 | 169.6 ns | 138 |
| `test_encode_batch_cl100k[large_x2]` | 515,700 | 81.171 ms | 84.355 ms | 84.286 ms | 1.827 ms | 2.643 ms | 12 | 163.6 ns | 13 |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 1.296 ms | 1.761 ms | 1.724 ms | 255.937 μs | 327.150 μs | 568 | 1760.8 ns | 760 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 51,570 | 6.584 ms | 7.521 ms | 7.398 ms | 797.254 μs | 722.175 μs | 133 | 145.8 ns | 153 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 515,700 | 70.757 ms | 74.634 ms | 72.836 ms | 3.282 ms | 5.090 ms | 13 | 144.7 ns | 14 |
| `test_decode_o200k[small]` | 10 | 259.000 ns | 288.518 ns | 280.001 ns | 67.762 ns | 7.999 ns | 3,465,993 | 28.9 ns | 38611 |
| `test_decode_o200k[medium]` | 5,212 | 69.600 μs | 77.276 μs | 73.200 μs | 88.792 μs | 3.600 μs | 12,941 | 14.8 ns | 14045 |
| `test_decode_o200k[large]` | 260,600 | 3.647 ms | 4.091 ms | 4.006 ms | 319.783 μs | 224.900 μs | 244 | 15.7 ns | 277 |
| `test_encode_o200k[small]` | 10 | 1.350 μs | 1.688 μs | 1.500 μs | 617.593 ns | 419.992 ns | 592,514 | 168.8 ns | 69445 |
| `test_encode_o200k[medium]` | 5,212 | 751.500 μs | 894.292 μs | 817.250 μs | 206.163 μs | 138.000 μs | 1,118 | 171.6 ns | 1330 |
| `test_encode_o200k[large]` | 260,600 | 41.504 ms | 45.493 ms | 45.322 ms | 2.014 ms | 2.400 ms | 22 | 174.6 ns | 24 |
| `test_roundtrip_o200k[small]` | 10 | 1.600 μs | 1.963 μs | 1.780 μs | 741.101 ns | 419.992 ns | 509,344 | 196.3 ns | 62500 |
| `test_roundtrip_o200k[medium]` | 5,212 | 833.800 μs | 988.432 μs | 904.250 μs | 214.455 μs | 146.300 μs | 1,012 | 189.6 ns | 1202 |
| `test_roundtrip_o200k[large]` | 260,600 | 46.614 ms | 50.548 ms | 50.681 ms | 1.970 ms | 3.032 ms | 20 | 194.0 ns | 22 |

## c_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_encode_cl100k[small]` | 10 | 689.994 ns | 797.408 ns | 739.994 ns | 432.406 ns | 40.012 ns | 1,254,063 | 79.7 ns | 135137 |
| `test_encode_cl100k[medium]` | 5,160 | 395.700 μs | 475.954 μs | 426.900 μs | 130.846 μs | 60.425 μs | 2,101 | 92.2 ns | 2537 |
| `test_encode_cl100k[large]` | 258,000 | 20.792 ms | 23.270 ms | 23.061 ms | 1.474 ms | 2.022 ms | 43 | 90.2 ns | 47 |
| `test_decode_cl100k[small]` | 10 | 141.000 ns | 158.109 ns | 146.000 ns | 126.711 ns | 4.000 ns | 6,324,759 | 15.8 ns | 70423 |
| `test_decode_cl100k[medium]` | 5,160 | 49.000 μs | 54.599 μs | 50.800 μs | 15.664 μs | 3.500 μs | 18,315 | 10.6 ns | 20409 |
| `test_decode_cl100k[large]` | 258,000 | 2.511 ms | 2.890 ms | 2.798 ms | 412.639 μs | 169.600 μs | 346 | 11.2 ns | 398 |
| `test_roundtrip_cl100k[small]` | 10 | 919.995 ns | 1.069 μs | 1.010 μs | 468.048 ns | 40.000 ns | 935,067 | 106.9 ns | 103094 |
| `test_roundtrip_cl100k[medium]` | 5,160 | 458.500 μs | 538.998 μs | 490.900 μs | 140.414 μs | 58.650 μs | 1,855 | 104.5 ns | 2172 |
| `test_roundtrip_cl100k[large]` | 258,000 | 24.361 ms | 27.091 ms | 27.265 ms | 1.514 ms | 2.478 ms | 37 | 105.0 ns | 41 |
| `test_count_cl100k[small]` | 10 | 579.993 ns | 675.245 ns | 610.005 ns | 391.036 ns | 49.989 ns | 1,480,945 | 67.5 ns | 172416 |
| `test_count_cl100k[medium]` | 5,160 | 333.300 μs | 401.267 μs | 363.200 μs | 119.306 μs | 40.000 μs | 2,492 | 77.8 ns | 2959 |
| `test_count_cl100k[large]` | 258,000 | 17.622 ms | 20.251 ms | 20.179 ms | 1.685 ms | 2.408 ms | 49 | 78.5 ns | 58 |
| `test_count_till_limit_cl100k[small]` | 10 | 1.250 μs | 1.506 μs | 1.320 μs | 589.075 ns | 300.002 ns | 664,002 | 150.6 ns | 79366 |
| `test_count_till_limit_cl100k[medium]` | 50 | 5.000 μs | 6.064 μs | 5.350 μs | 2.932 μs | 600.063 ns | 164,895 | 121.3 ns | 100001 |
| `test_count_till_limit_cl100k[large]` | 50 | 5.000 μs | 6.042 μs | 5.400 μs | 2.606 μs | 650.063 ns | 165,506 | 120.8 ns | 100001 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 71.800 μs | 142.380 μs | 82.700 μs | 3.822 ms | 9.300 μs | 7,023 | 142.4 ns | 13813 |
| `test_encode_batch_cl100k[medium_x10]` | 51,600 | 4.367 ms | 5.133 ms | 4.809 ms | 2.828 ms | 650.000 μs | 195 | 99.5 ns | 230 |
| `test_encode_batch_cl100k[large_x2]` | 516,000 | 44.063 ms | 46.899 ms | 46.852 ms | 1.838 ms | 2.762 ms | 21 | 90.9 ns | 23 |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1,000 | 142.600 μs | 586.467 μs | 595.000 μs | 182.746 μs | 166.075 μs | 1,705 | 586.5 ns | 6601 |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 51,600 | 1.347 ms | 2.076 ms | 1.775 ms | 3.170 ms | 294.225 μs | 482 | 40.2 ns | 781 |
| `test_encode_batch_parallel_cl100k[large_x2]` | 516,000 | 25.897 ms | 29.248 ms | 28.137 ms | 2.890 ms | 3.780 ms | 34 | 56.7 ns | 39 |
| `test_decode_o200k[small]` | 10 | 131.000 ns | 145.634 ns | 136.000 ns | 71.761 ns | 8.000 ns | 6,866,525 | 14.6 ns | 76336 |
| `test_decode_o200k[medium]` | 5,215 | 48.200 μs | 54.665 μs | 50.300 μs | 18.206 μs | 4.200 μs | 18,293 | 10.5 ns | 20747 |
| `test_decode_o200k[large]` | 260,750 | 2.466 ms | 3.082 ms | 2.962 ms | 566.789 μs | 348.000 μs | 325 | 11.8 ns | 403 |
| `test_encode_o200k[small]` | 10 | 699.994 ns | 837.535 ns | 780.006 ns | 414.453 ns | 50.012 ns | 1,193,980 | 83.8 ns | 140847 |
| `test_encode_o200k[medium]` | 5,215 | 430.800 μs | 572.617 μs | 504.050 μs | 167.456 μs | 139.300 μs | 1,746 | 109.8 ns | 2246 |
| `test_encode_o200k[large]` | 260,750 | 26.580 ms | 30.699 ms | 30.356 ms | 2.283 ms | 3.324 ms | 33 | 117.7 ns | 39 |
| `test_roundtrip_o200k[small]` | 10 | 929.995 ns | 1.116 μs | 1.030 μs | 663.390 ns | 70.001 ns | 896,102 | 111.6 ns | 109891 |
| `test_roundtrip_o200k[medium]` | 5,215 | 491.900 μs | 654.873 μs | 583.100 μs | 189.525 μs | 146.550 μs | 1,527 | 125.6 ns | 1973 |
| `test_roundtrip_o200k[large]` | 260,750 | 29.202 ms | 33.633 ms | 32.985 ms | 2.161 ms | 2.386 ms | 30 | 129.0 ns | 34 |

## Comparison (median, rs_bpe vs c_bpe)

| Benchmark | rs_bpe | c_bpe | ratio |
| --- | --- | --- | --- |
| `test_count_cl100k[large]` | 39.041 ms | 20.179 ms | 1.93× |
| `test_count_cl100k[medium]` | 714.100 μs | 363.200 μs | 1.97× |
| `test_count_cl100k[small]` | 1.250 μs | 610.005 ns | 2.05× |
| `test_count_till_limit_cl100k[large]` | 5.500 μs | 5.400 μs | 1.02× |
| `test_count_till_limit_cl100k[medium]` | 5.500 μs | 5.350 μs | 1.03× |
| `test_count_till_limit_cl100k[small]` | 1.240 μs | 1.320 μs | **0.94×** |
| `test_decode_cl100k[large]` | 3.817 ms | 2.798 ms | 1.36× |
| `test_decode_cl100k[medium]` | 72.300 μs | 50.800 μs | 1.42× |
| `test_decode_cl100k[small]` | 284.213 ns | 146.000 ns | 1.95× |
| `test_decode_o200k[large]` | 4.006 ms | 2.962 ms | 1.35× |
| `test_decode_o200k[medium]` | 73.200 μs | 50.300 μs | 1.46× |
| `test_decode_o200k[small]` | 280.001 ns | 136.000 ns | 2.06× |
| `test_encode_batch_cl100k[large_x2]` | 84.286 ms | 46.852 ms | 1.80× |
| `test_encode_batch_cl100k[medium_x10]` | 8.308 ms | 4.809 ms | 1.73× |
| `test_encode_batch_cl100k[small_x100]` | 147.700 μs | 82.700 μs | 1.79× |
| `test_encode_batch_parallel_cl100k[large_x2]` | 72.836 ms | 28.137 ms | 2.59× |
| `test_encode_batch_parallel_cl100k[medium_x10]` | 7.398 ms | 1.775 ms | 4.17× |
| `test_encode_batch_parallel_cl100k[small_x100]` | 1.724 ms | 595.000 μs | 2.90× |
| `test_encode_cl100k[large]` | 42.151 ms | 23.061 ms | 1.83× |
| `test_encode_cl100k[medium]` | 756.400 μs | 426.900 μs | 1.77× |
| `test_encode_cl100k[small]` | 1.460 μs | 739.994 ns | 1.97× |
| `test_encode_o200k[large]` | 45.322 ms | 30.356 ms | 1.49× |
| `test_encode_o200k[medium]` | 817.250 μs | 504.050 μs | 1.62× |
| `test_encode_o200k[small]` | 1.500 μs | 780.006 ns | 1.92× |
| `test_roundtrip_cl100k[large]` | 46.154 ms | 27.265 ms | 1.69× |
| `test_roundtrip_cl100k[medium]` | 838.300 μs | 490.900 μs | 1.71× |
| `test_roundtrip_cl100k[small]` | 1.800 μs | 1.010 μs | 1.78× |
| `test_roundtrip_o200k[large]` | 50.681 ms | 32.985 ms | 1.54× |
| `test_roundtrip_o200k[medium]` | 904.250 μs | 583.100 μs | 1.55× |
| `test_roundtrip_o200k[small]` | 1.780 μs | 1.030 μs | 1.73× |
