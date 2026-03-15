/* fnv_hash.h — FNV-1a hash map, mirroring Rust's fnv crate.
 *
 * Two map types are provided:
 *   BytesMap  — maps byte-slice keys to uint32_t token IDs.
 *   PairMap   — maps (uint32_t, uint32_t) pairs to uint32_t merged-token IDs.
 *
 * Both use open addressing with linear probing.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * FNV-1a helpers (identical to Rust's FnvHasher seeded with 0xcbf29ce484222325)
 * ------------------------------------------------------------------------- */
static inline uint64_t fnv1a_hash_bytes(const uint8_t *data, size_t len) {
    uint64_t h = UINT64_C(0xcbf29ce484222325);
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)data[i];
        h *= UINT64_C(0x100000001b3);
    }
    return h;
}

/* Mirrors bpe's hash_bytes(): (fnv1a(bytes) * hash_factor) >> 32 */
static inline uint32_t bpe_hash_bytes(const uint8_t *bytes, size_t len, uint64_t factor) {
    uint64_t h = fnv1a_hash_bytes(bytes, len);
    h = (h * factor) >> 32;
    return (uint32_t)h;
}

/* -------------------------------------------------------------------------
 * BytesMap: bytes key → uint32_t value, open addressing, power-of-2 capacity
 * ------------------------------------------------------------------------- */
#define BYTES_MAP_EMPTY UINT32_MAX

typedef struct {
    uint32_t hash;          /* stored hash to avoid recompute on probe */
    uint32_t token_id;      /* value; BYTES_MAP_EMPTY if empty slot */
} BytesMapSlot;

typedef struct BytesMap {
    BytesMapSlot *slots;
    uint32_t      capacity; /* power of 2 */
    uint32_t      count;
    /* We do not store the actual key bytes here — callers verify equality
       using the all_tokens / token_starts arrays in BytePairEncoding.      */
} BytesMap;

static inline BytesMap bytesmap_new(uint32_t initial_cap) {
    /* round up to next power of 2 */
    uint32_t cap = 16;
    while (cap < initial_cap * 2) cap <<= 1;
    BytesMap m;
    m.capacity = cap;
    m.count    = 0;
    m.slots    = (BytesMapSlot *)calloc(cap, sizeof(BytesMapSlot));
    for (uint32_t i = 0; i < cap; i++) m.slots[i].token_id = BYTES_MAP_EMPTY;
    return m;
}

static inline void bytesmap_free(BytesMap *m) {
    free(m->slots);
    m->slots = NULL;
}

static inline void bytesmap_insert(BytesMap *m, uint32_t hash, uint32_t token_id) {
    uint32_t mask = m->capacity - 1;
    uint32_t idx  = hash & mask;
    while (m->slots[idx].token_id != BYTES_MAP_EMPTY) {
        idx = (idx + 1) & mask;
    }
    m->slots[idx].hash     = hash;
    m->slots[idx].token_id = token_id;
    m->count++;
}

/* Returns BYTES_MAP_EMPTY if not found. */
static inline uint32_t bytesmap_get(const BytesMap *m, uint32_t hash) {
    uint32_t mask = m->capacity - 1;
    uint32_t idx  = hash & mask;
    while (m->slots[idx].token_id != BYTES_MAP_EMPTY) {
        if (m->slots[idx].hash == hash) return m->slots[idx].token_id;
        idx = (idx + 1) & mask;
    }
    return BYTES_MAP_EMPTY;
}

/* -------------------------------------------------------------------------
 * PairMap: (uint32_t token1, uint32_t token2) → uint32_t merged_token_id
 * Uses FNV-1a over the 8-byte key.
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t t1, t2;        /* key */
    uint32_t merged;        /* value; UINT32_MAX if empty */
} PairMapSlot;

typedef struct PairMap {
    PairMapSlot *slots;
    uint32_t     capacity;
    uint32_t     count;
} PairMap;

static inline uint32_t pairmap_hash(uint32_t t1, uint32_t t2) {
    /* splitmix64 finaliser — faster than byte-by-byte FNV-1a for fixed 8-byte keys */
    uint64_t key = ((uint64_t)t1 << 32) | t2;
    key ^= key >> 30;
    key *= UINT64_C(0xbf58476d1ce4e5b9);
    key ^= key >> 27;
    key *= UINT64_C(0x94d049bb133111eb);
    return (uint32_t)(key >> 32);
}

static inline PairMap pairmap_new(uint32_t initial_cap) {
    uint32_t cap = 16;
    while (cap < initial_cap * 2) cap <<= 1;
    PairMap m;
    m.capacity = cap;
    m.count    = 0;
    m.slots    = (PairMapSlot *)malloc(cap * sizeof(PairMapSlot));
    for (uint32_t i = 0; i < cap; i++) m.slots[i].merged = UINT32_MAX;
    return m;
}

static inline void pairmap_free(PairMap *m) {
    free(m->slots);
    m->slots = NULL;
}

static inline void pairmap_grow(PairMap *m) {
    uint32_t old_cap = m->capacity;
    PairMapSlot *old = m->slots;
    m->capacity <<= 1;
    m->slots = (PairMapSlot *)malloc(m->capacity * sizeof(PairMapSlot));
    for (uint32_t i = 0; i < m->capacity; i++) m->slots[i].merged = UINT32_MAX;
    uint32_t mask = m->capacity - 1;
    for (uint32_t i = 0; i < old_cap; i++) {
        if (old[i].merged == UINT32_MAX) continue;
        uint32_t h   = pairmap_hash(old[i].t1, old[i].t2);
        uint32_t idx = h & mask;
        while (m->slots[idx].merged != UINT32_MAX) idx = (idx + 1) & mask;
        m->slots[idx] = old[i];
    }
    free(old);
}

static inline void pairmap_insert(PairMap *m, uint32_t t1, uint32_t t2, uint32_t merged) {
    if (m->count * 2 >= m->capacity) pairmap_grow(m);
    uint32_t mask = m->capacity - 1;
    uint32_t h    = pairmap_hash(t1, t2);
    uint32_t idx  = h & mask;
    while (m->slots[idx].merged != UINT32_MAX) idx = (idx + 1) & mask;
    m->slots[idx] = (PairMapSlot){t1, t2, merged};
    m->count++;
}

/* Returns UINT32_MAX if not found. */
static inline uint32_t pairmap_get(const PairMap *m, uint32_t t1, uint32_t t2) {
    uint32_t mask = m->capacity - 1;
    uint32_t h    = pairmap_hash(t1, t2);
    uint32_t idx  = h & mask;
    while (m->slots[idx].merged != UINT32_MAX) {
        if (m->slots[idx].t1 == t1 && m->slots[idx].t2 == t2)
            return m->slots[idx].merged;
        idx = (idx + 1) & mask;
    }
    return UINT32_MAX;
}

/* --------------------------------------------------------------------------
 * Compatibility aliases: in-place initialisation (bpe_core.c API)
 * -------------------------------------------------------------------------- */
static inline void bytesmap_init(BytesMap *m, uint32_t initial_cap) {
    *m = bytesmap_new(initial_cap);
}
static inline void bytesmap_put(BytesMap *m, uint32_t hash, uint32_t token_id) {
    bytesmap_insert(m, hash, token_id);
}
static inline void pairmap_init(PairMap *m, uint32_t initial_cap) {
    *m = pairmap_new(initial_cap);
}
static inline void pairmap_put(PairMap *m, uint32_t t1, uint32_t t2, uint32_t merged) {
    pairmap_insert(m, t1, t2, merged);
}
