/* lru_cache.h — LRU cache backed by a doubly-linked list + hash map.
 *
 * Two specialised caches matching the Rust BPE implementation:
 *   TokenCache  — bytes key → uint32_t token_id   (capacity 4096)
 *   MergeCache  — bytes key → uint32_t* tokens    (capacity 1024)
 *
 * For thread-safety at the Python layer the caches are declared
 * thread-local; in C11 use _Thread_local, on MSVC __declspec(thread).
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Generic LRU node (intrusive doubly-linked list)
 * Keys are byte-arrays; values are union-typed per cache kind.
 * ------------------------------------------------------------------------- */

/* Forward declaration of cache types used internally */
typedef struct LruNode LruNode;

struct LruNode {
    LruNode *prev, *next;   /* LRU list links */
    uint8_t *key;
    uint32_t key_len;
    uint32_t hash;
    /* slot index in the hash table */
    uint32_t slot;
    /* value storage — used differently per cache */
    uint32_t  val_u32;      /* token_id for TokenCache */
    uint32_t *val_arr;      /* token array for MergeCache */
    uint32_t  val_arr_len;
};

typedef struct {
    LruNode  **buckets;     /* hash table: bucket → node or NULL */
    uint32_t   capacity;    /* max entries */
    uint32_t   count;
    LruNode   *head;        /* most-recently used */
    LruNode   *tail;        /* least-recently used */
    uint32_t   nbuckets;    /* power-of-2 */
} LruCache;

static inline LruCache lru_cache_new(uint32_t capacity) {
    LruCache c;
    c.capacity = capacity;
    c.count    = 0;
    c.head     = NULL;
    c.tail     = NULL;
    uint32_t nb = 16;
    while (nb < capacity * 2) nb <<= 1;
    c.nbuckets = nb;
    c.buckets  = (LruNode **)calloc(nb, sizeof(LruNode *));
    return c;
}

static inline void lru_cache_free(LruCache *c) {
    LruNode *n = c->head;
    while (n) {
        LruNode *nx = n->next;
        free(n->key);
        free(n->val_arr);
        free(n);
        n = nx;
    }
    free(c->buckets);
    c->buckets = NULL;
}

static inline uint32_t lru_key_hash(const uint8_t *key, uint32_t len) {
    uint64_t h = UINT64_C(0xcbf29ce484222325);
    for (uint32_t i = 0; i < len; i++) {
        h ^= (uint64_t)key[i];
        h *= UINT64_C(0x100000001b3);
    }
    return (uint32_t)(h >> 32) ^ (uint32_t)h;
}

/* Unlink node from LRU list */
static inline void lru_unlink(LruCache *c, LruNode *n) {
    if (n->prev) n->prev->next = n->next; else c->head = n->next;
    if (n->next) n->next->prev = n->prev; else c->tail = n->prev;
    n->prev = n->next = NULL;
}

/* Push node to head (most recently used) */
static inline void lru_push_head(LruCache *c, LruNode *n) {
    n->prev = NULL;
    n->next = c->head;
    if (c->head) c->head->prev = n;
    c->head = n;
    if (!c->tail) c->tail = n;
}

/* Evict least-recently-used node */
static inline void lru_evict(LruCache *c) {
    LruNode *n = c->tail;
    if (!n) return;
    lru_unlink(c, n);
    /* Remove from hash table */
    uint32_t mask = c->nbuckets - 1;
    uint32_t bi   = n->hash & mask;
    while (c->buckets[bi] != n) bi = (bi + 1) & mask;
    /* Robin-Hood backward shift deletion */
    for (;;) {
        uint32_t next = (bi + 1) & mask;
        if (!c->buckets[next] || (c->buckets[next]->hash & mask) == next) {
            c->buckets[bi] = NULL;
            break;
        }
        c->buckets[bi] = c->buckets[next];
        bi = next;
    }
    free(n->key);
    free(n->val_arr);
    free(n);
    c->count--;
}

/* Insert or update.  val_arr is adopted (ownership transferred). */
static inline void lru_put_u32(LruCache *c, const uint8_t *key, uint32_t klen, uint32_t val) {
    uint32_t h    = lru_key_hash(key, klen);
    uint32_t mask = c->nbuckets - 1;
    uint32_t bi   = h & mask;
    while (c->buckets[bi]) {
        LruNode *node = c->buckets[bi];
        if (node->hash == h && node->key_len == klen &&
            memcmp(node->key, key, klen) == 0) {
            node->val_u32 = val;
            lru_unlink(c, node);
            lru_push_head(c, node);
            return;
        }
        bi = (bi + 1) & mask;
    }
    if (c->count >= c->capacity) lru_evict(c);
    LruNode *n  = (LruNode *)calloc(1, sizeof(LruNode));
    n->key      = (uint8_t *)malloc(klen);
    memcpy(n->key, key, klen);
    n->key_len  = klen;
    n->hash     = h;
    n->val_u32  = val;
    /* Re-probe (eviction may have changed things) */
    bi = h & mask;
    while (c->buckets[bi]) bi = (bi + 1) & mask;
    c->buckets[bi] = n;
    lru_push_head(c, n);
    c->count++;
}

/* val_arr is adopted.  Pass NULL to store a u32-only entry. */
static inline void lru_put_arr(LruCache *c, const uint8_t *key, uint32_t klen,
                               uint32_t *arr, uint32_t arr_len) {
    uint32_t h    = lru_key_hash(key, klen);
    uint32_t mask = c->nbuckets - 1;
    uint32_t bi   = h & mask;
    while (c->buckets[bi]) {
        LruNode *node = c->buckets[bi];
        if (node->hash == h && node->key_len == klen &&
            memcmp(node->key, key, klen) == 0) {
            free(node->val_arr);
            node->val_arr     = arr;
            node->val_arr_len = arr_len;
            lru_unlink(c, node);
            lru_push_head(c, node);
            return;
        }
        bi = (bi + 1) & mask;
    }
    if (c->count >= c->capacity) lru_evict(c);
    LruNode *n  = (LruNode *)calloc(1, sizeof(LruNode));
    n->key      = (uint8_t *)malloc(klen);
    memcpy(n->key, key, klen);
    n->key_len    = klen;
    n->hash       = h;
    n->val_arr    = arr;
    n->val_arr_len = arr_len;
    bi = h & mask;
    while (c->buckets[bi]) bi = (bi + 1) & mask;
    c->buckets[bi] = n;
    lru_push_head(c, n);
    c->count++;
}

/* Returns UINT32_MAX if miss, otherwise the cached token_id. */
static inline uint32_t lru_get_u32(LruCache *c, const uint8_t *key, uint32_t klen) {
    uint32_t h    = lru_key_hash(key, klen);
    uint32_t mask = c->nbuckets - 1;
    uint32_t bi   = h & mask;
    while (c->buckets[bi]) {
        LruNode *node = c->buckets[bi];
        if (node->hash == h && node->key_len == klen &&
            memcmp(node->key, key, klen) == 0) {
            lru_unlink(c, node);
            lru_push_head(c, node);
            return node->val_u32;
        }
        bi = (bi + 1) & mask;
    }
    return UINT32_MAX;
}

/* Returns NULL on miss; sets *out_len. */
static inline const uint32_t *lru_get_arr(LruCache *c, const uint8_t *key, uint32_t klen,
                                           uint32_t *out_len) {
    uint32_t h    = lru_key_hash(key, klen);
    uint32_t mask = c->nbuckets - 1;
    uint32_t bi   = h & mask;
    while (c->buckets[bi]) {
        LruNode *node = c->buckets[bi];
        if (node->hash == h && node->key_len == klen &&
            memcmp(node->key, key, klen) == 0) {
            if (out_len) *out_len = node->val_arr_len;
            lru_unlink(c, node);
            lru_push_head(c, node);
            return node->val_arr;
        }
        bi = (bi + 1) & mask;
    }
    return NULL;
}
