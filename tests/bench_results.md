# Benchmark Results

## rs_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_import_cl100k` | n/a | 107.875 μs | 131.551 μs | 117.573 μs | 95.271 μs | 17.373 μs | 7,602 | n/a | 9378 |
| `test_import_o200k` | n/a | 107.931 μs | 133.016 μs | 118.624 μs | 50.455 μs | 18.807 μs | 7,518 | n/a | 9326 |

## c_bpe

| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `test_import_cl100k` | n/a | 111.350 μs | 134.176 μs | 119.430 μs | 46.609 μs | 19.378 μs | 7,453 | n/a | 9099 |
| `test_import_o200k` | n/a | 107.027 μs | 126.641 μs | 113.014 μs | 50.679 μs | 13.800 μs | 7,896 | n/a | 9454 |

## Comparison (median, rs_bpe vs c_bpe)

| Benchmark | rs_bpe | c_bpe | ratio |
| --- | --- | --- | --- |
| `test_import_cl100k` | 117.573 μs | 119.430 μs | **0.98×** |
| `test_import_o200k` | 118.624 μs | 113.014 μs | 1.05× |
