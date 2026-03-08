# Benchmark Results

## rs_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_encode_cl100k[small]` | 10 | 1.390 μs | 1.777 μs | 1.450 μs | 1.069 μs | 170.001 ns | 562,899 | 177.7 ns | 71429 |
| `test_encode_cl100k[medium]` | 3,435 | 559.100 μs | 954.627 μs | 662.050 μs | 463.777 μs | 842.200 μs | 1,048 | 277.9 ns | 1788 |
| `test_encode_cl100k[large]` | 171,750 | 29.662 ms | 44.180 ms | 44.040 ms | 9.562 ms | 11.330 ms | 23 | 257.2 ns | 28 |
| `test_decode_cl100k[small]` | 10 | 270.587 ns | 602.760 ns | 376.473 ns | 519.220 ns | 547.063 ns | 1,659,034 | 60.3 ns | 188680 |
| `test_decode_cl100k[medium]` | 3,435 | 41.300 μs | 88.469 μs | 58.300 μs | 51.802 μs | 82.600 μs | 11,303 | 25.8 ns | 24331 |
| `test_decode_cl100k[large]` | 171,750 | 2.070 ms | 4.008 ms | 3.475 ms | 1.717 ms | 2.275 ms | 249 | 23.3 ns | 397 |
| `test_roundtrip_cl100k[small]` | 10 | 1.710 μs | 3.487 μs | 2.130 μs | 3.001 μs | 3.430 μs | 286,754 | 348.7 ns | 49262 |
| `test_roundtrip_cl100k[medium]` | 3,435 | 516.400 μs | 1.011 ms | 697.750 μs | 508.956 μs | 838.150 μs | 989 | 294.2 ns | 1640 |
| `test_roundtrip_cl100k[large]` | 171,750 | 35.660 ms | 56.954 ms | 56.532 ms | 13.133 ms | 20.427 ms | 18 | 331.6 ns | 25 |
| `test_count_cl100k[small]` | 10 | 1.170 μs | 1.834 μs | 1.380 μs | 1.553 μs | 489.992 ns | 545,149 | 183.4 ns | 74627 |
| `test_count_cl100k[medium]` | 3,435 | 432.200 μs | 749.857 μs | 545.250 μs | 382.080 μs | 335.650 μs | 1,334 | 218.3 ns | 1920 |
| `test_count_cl100k[large]` | 171,750 | 26.420 ms | 34.116 ms | 33.315 ms | 4.618 ms | 6.746 ms | 29 | 198.6 ns | 38 |
| `test_count_till_limit_cl100k[small]` | 10 | 1.350 μs | 2.747 μs | 2.780 μs | 1.530 μs | 2.350 μs | 364,047 | 274.7 ns | 74075 |
| `test_count_till_limit_cl100k[medium]` | 50 | 5.100 μs | 9.330 μs | 6.800 μs | 9.227 μs | 1.900 μs | 107,179 | 186.6 ns | 153847 |
| `test_count_till_limit_cl100k[large]` | 50 | 6.500 μs | 11.415 μs | 6.900 μs | 12.741 μs | 11.200 μs | 87,601 | 228.3 ns | 153847 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 144.500 μs | 257.351 μs | 169.800 μs | 145.065 μs | 181.600 μs | 3,886 | 257.4 ns | 6840 |
| `test_encode_batch_cl100k[medium_x10]` | 34,350 | 5.681 ms | 8.148 ms | 6.824 ms | 2.762 ms | 2.857 ms | 123 | 237.2 ns | 175 |
| `test_encode_batch_cl100k[large_x2]` | 343,500 | 72.699 ms | 95.750 ms | 93.607 ms | 17.965 ms | 19.801 ms | 10 | 278.7 ns | 17 |
| `test_decode_o200k[small]` | 10 | 251.000 ns | 312.207 ns | 272.000 ns | 157.155 ns | 16.000 ns | 3,202,999 | 31.2 ns | 39216 |
| `test_decode_o200k[medium]` | 3,472 | 43.500 μs | 53.663 μs | 47.500 μs | 28.193 μs | 4.000 μs | 18,635 | 15.5 ns | 22523 |
| `test_decode_o200k[large]` | 173,600 | 2.105 ms | 2.488 ms | 2.358 ms | 414.089 μs | 243.700 μs | 402 | 14.3 ns | 452 |
| `test_encode_o200k[small]` | 10 | 1.280 μs | 1.531 μs | 1.380 μs | 810.202 ns | 50.000 ns | 653,119 | 153.1 ns | 77520 |
| `test_encode_o200k[medium]` | 3,472 | 446.900 μs | 538.462 μs | 488.300 μs | 162.334 μs | 61.300 μs | 1,857 | 155.1 ns | 2305 |
| `test_encode_o200k[large]` | 173,600 | 24.481 ms | 27.774 ms | 27.011 ms | 2.928 ms | 2.377 ms | 36 | 160.0 ns | 41 |
| `test_roundtrip_o200k[small]` | 10 | 1.510 μs | 1.892 μs | 1.640 μs | 1.252 μs | 90.001 ns | 528,468 | 189.2 ns | 65790 |
| `test_roundtrip_o200k[medium]` | 3,472 | 483.900 μs | 596.236 μs | 529.000 μs | 197.736 μs | 92.700 μs | 1,677 | 171.7 ns | 2112 |
| `test_roundtrip_o200k[large]` | 173,600 | 27.812 ms | 30.228 ms | 30.184 ms | 1.558 ms | 2.345 ms | 33 | 174.1 ns | 38 |

## c_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_encode_cl100k[small]` | 10 | 720.000 ns | 1.370 μs | 1.030 μs | 2.593 μs | 209.996 ns | 729,724 | 137.0 ns | 136987 |
| `test_encode_cl100k[medium]` | 3,438 | 251.100 μs | 461.562 μs | 322.700 μs | 257.676 μs | 237.675 μs | 2,167 | 134.3 ns | 3893 |
| `test_encode_cl100k[large]` | 171,900 | 15.725 ms | 24.181 ms | 23.053 ms | 5.030 ms | 6.499 ms | 41 | 140.7 ns | 57 |
| `test_decode_cl100k[small]` | 10 | 139.000 ns | 236.705 ns | 174.000 ns | 202.498 ns | 114.000 ns | 4,224,664 | 23.7 ns | 71429 |
| `test_decode_cl100k[medium]` | 3,438 | 28.100 μs | 54.638 μs | 37.900 μs | 35.939 μs | 27.200 μs | 18,302 | 15.9 ns | 35588 |
| `test_decode_cl100k[large]` | 171,900 | 1.432 ms | 1.619 ms | 1.570 ms | 212.299 μs | 206.000 μs | 618 | 9.4 ns | 690 |
| `test_roundtrip_cl100k[small]` | 10 | 909.995 ns | 1.030 μs | 970.002 ns | 501.002 ns | 30.000 ns | 970,973 | 103.0 ns | 109890 |
| `test_roundtrip_cl100k[medium]` | 3,438 | 279.900 μs | 329.854 μs | 298.300 μs | 96.329 μs | 31.200 μs | 3,032 | 95.9 ns | 3567 |
| `test_roundtrip_cl100k[large]` | 171,900 | 15.149 ms | 17.229 ms | 17.085 ms | 1.171 ms | 1.523 ms | 58 | 100.2 ns | 66 |
| `test_count_cl100k[small]` | 10 | 589.999 ns | 671.457 ns | 619.999 ns | 334.354 ns | 20.006 ns | 1,489,298 | 67.1 ns | 169492 |
| `test_count_cl100k[medium]` | 3,438 | 215.400 μs | 253.587 μs | 229.200 μs | 83.385 μs | 18.025 μs | 3,943 | 73.8 ns | 4665 |
| `test_count_cl100k[large]` | 171,900 | 10.772 ms | 12.778 ms | 12.721 ms | 1.174 ms | 1.583 ms | 78 | 74.3 ns | 91 |
| `test_count_till_limit_cl100k[small]` | 10 | 1.200 μs | 1.473 μs | 1.260 μs | 2.573 μs | 130.001 ns | 679,082 | 147.3 ns | 83334 |
| `test_count_till_limit_cl100k[medium]` | 50 | 4.650 μs | 5.641 μs | 5.000 μs | 6.899 μs | 399.974 ns | 177,262 | 112.8 ns | 107528 |
| `test_count_till_limit_cl100k[large]` | 50 | 4.650 μs | 5.377 μs | 4.950 μs | 3.374 μs | 250.002 ns | 185,994 | 107.5 ns | 107528 |
| `test_encode_batch_cl100k[small_x100]` | 1,000 | 26.600 μs | 119.100 μs | 46.400 μs | 6.228 ms | 13.700 μs | 8,396 | 119.1 ns | 37038 |
| `test_encode_batch_cl100k[medium_x10]` | 34,380 | 1.581 ms | 2.471 ms | 2.087 ms | 2.354 ms | 360.100 μs | 405 | 71.9 ns | 630 |
| `test_encode_batch_cl100k[large_x2]` | 343,800 | 32.383 ms | 38.554 ms | 35.807 ms | 5.533 ms | 9.846 ms | 26 | 112.1 ns | 32 |
| `test_decode_o200k[small]` | 10 | 134.000 ns | 154.895 ns | 141.000 ns | 130.606 ns | 7.000 ns | 6,456,006 | 15.5 ns | 74627 |
| `test_decode_o200k[medium]` | 3,475 | 27.300 μs | 31.450 μs | 28.300 μs | 16.463 μs | 1.400 μs | 31,797 | 9.1 ns | 36497 |
| `test_decode_o200k[large]` | 173,750 | 1.398 ms | 1.748 ms | 1.662 ms | 449.224 μs | 295.900 μs | 572 | 10.1 ns | 714 |
| `test_encode_o200k[small]` | 10 | 700.000 ns | 882.699 ns | 760.006 ns | 833.352 ns | 50.000 ns | 1,132,889 | 88.3 ns | 142858 |
| `test_encode_o200k[medium]` | 3,475 | 269.100 μs | 339.619 μs | 291.300 μs | 155.821 μs | 39.975 μs | 2,944 | 97.7 ns | 3695 |
| `test_encode_o200k[large]` | 173,750 | 14.922 ms | 17.171 ms | 17.104 ms | 1.195 ms | 1.592 ms | 58 | 98.8 ns | 66 |
| `test_roundtrip_o200k[small]` | 10 | 880.001 ns | 1.086 μs | 950.001 ns | 890.869 ns | 50.000 ns | 921,044 | 108.6 ns | 111112 |
| `test_roundtrip_o200k[medium]` | 3,475 | 304.000 μs | 385.741 μs | 336.200 μs | 126.538 μs | 76.500 μs | 2,592 | 111.0 ns | 3263 |
| `test_roundtrip_o200k[large]` | 173,750 | 17.273 ms | 19.610 ms | 19.353 ms | 1.251 ms | 1.281 ms | 51 | 112.9 ns | 57 |

## Comparison (median, rs_bpe vs c_bpe)

| Benchmark | rs_bpe | c_bpe | ratio |
| --- | --- | --- | --- |
| `test_count_cl100k[large]` | 33.315 ms | 12.721 ms | 2.62× |
| `test_count_cl100k[medium]` | 545.250 μs | 229.200 μs | 2.38× |
| `test_count_cl100k[small]` | 1.380 μs | 619.999 ns | 2.23× |
| `test_count_till_limit_cl100k[large]` | 6.900 μs | 4.950 μs | 1.39× |
| `test_count_till_limit_cl100k[medium]` | 6.800 μs | 5.000 μs | 1.36× |
| `test_count_till_limit_cl100k[small]` | 2.780 μs | 1.260 μs | 2.21× |
| `test_decode_cl100k[large]` | 3.475 ms | 1.570 ms | 2.21× |
| `test_decode_cl100k[medium]` | 58.300 μs | 37.900 μs | 1.54× |
| `test_decode_cl100k[small]` | 376.473 ns | 174.000 ns | 2.16× |
| `test_decode_o200k[large]` | 2.358 ms | 1.662 ms | 1.42× |
| `test_decode_o200k[medium]` | 47.500 μs | 28.300 μs | 1.68× |
| `test_decode_o200k[small]` | 272.000 ns | 141.000 ns | 1.93× |
| `test_encode_batch_cl100k[large_x2]` | 93.607 ms | 35.807 ms | 2.61× |
| `test_encode_batch_cl100k[medium_x10]` | 6.824 ms | 2.087 ms | 3.27× |
| `test_encode_batch_cl100k[small_x100]` | 169.800 μs | 46.400 μs | 3.66× |
| `test_encode_cl100k[large]` | 44.040 ms | 23.053 ms | 1.91× |
| `test_encode_cl100k[medium]` | 662.050 μs | 322.700 μs | 2.05× |
| `test_encode_cl100k[small]` | 1.450 μs | 1.030 μs | 1.41× |
| `test_encode_o200k[large]` | 27.011 ms | 17.104 ms | 1.58× |
| `test_encode_o200k[medium]` | 488.300 μs | 291.300 μs | 1.68× |
| `test_encode_o200k[small]` | 1.380 μs | 760.006 ns | 1.82× |
| `test_roundtrip_cl100k[large]` | 56.532 ms | 17.085 ms | 3.31× |
| `test_roundtrip_cl100k[medium]` | 697.750 μs | 298.300 μs | 2.34× |
| `test_roundtrip_cl100k[small]` | 2.130 μs | 970.002 ns | 2.20× |
| `test_roundtrip_o200k[large]` | 30.184 ms | 19.353 ms | 1.56× |
| `test_roundtrip_o200k[medium]` | 529.000 μs | 336.200 μs | 1.57× |
| `test_roundtrip_o200k[small]` | 1.640 μs | 950.001 ns | 1.73× |
