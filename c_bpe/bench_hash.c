/* bench_hash.c — Compare FNV-1a vs splitmix64 for PairMap hashing. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#define ITERS 100000000

static inline uint32_t hash_fnv(uint32_t t1, uint32_t t2) {
    uint64_t h = UINT64_C(0xcbf29ce484222325);
    const uint8_t *p = (const uint8_t *)&t1;
    for (int i = 0; i < 4; i++) { h ^= p[i]; h *= UINT64_C(0x100000001b3); }
    p = (const uint8_t *)&t2;
    for (int i = 0; i < 4; i++) { h ^= p[i]; h *= UINT64_C(0x100000001b3); }
    return (uint32_t)(h >> 32);
}

static inline uint32_t hash_splitmix(uint32_t t1, uint32_t t2) {
    uint64_t key = ((uint64_t)t1 << 32) | t2;
    key ^= key >> 30;
    key *= UINT64_C(0xbf58476d1ce4e5b9);
    key ^= key >> 27;
    key *= UINT64_C(0x94d049bb133111eb);
    return (uint32_t)(key >> 32);
}

int main(void) {
    volatile uint32_t sink = 0;
    clock_t t0, t1;

    /* Warm up */
    for (uint32_t i = 0; i < 1000000; i++)
        sink ^= hash_fnv(i, i + 1);

    /* FNV-1a */
    t0 = clock();
    for (uint32_t i = 0; i < ITERS; i++)
        sink ^= hash_fnv(i, i + 7919);
    t1 = clock();
    double fnv_ms = (double)(t1 - t0) / CLOCKS_PER_SEC * 1000.0;

    /* Splitmix64 */
    t0 = clock();
    for (uint32_t i = 0; i < ITERS; i++)
        sink ^= hash_splitmix(i, i + 7919);
    t1 = clock();
    double split_ms = (double)(t1 - t0) / CLOCKS_PER_SEC * 1000.0;

    printf("FNV-1a:     %.1f ms  (%.2f ns/hash)\n", fnv_ms, fnv_ms / ITERS * 1e6);
    printf("Splitmix64: %.1f ms  (%.2f ns/hash)\n", split_ms, split_ms / ITERS * 1e6);
    printf("Speedup:    %.2fx\n", fnv_ms / split_ms);
    printf("(sink=%u)\n", (unsigned)sink);
    return 0;
}
