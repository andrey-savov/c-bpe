/* bpe_core.c — BytePairEncoding construction + encoding algorithms.
 *
 * C port of bpe/src/byte_pair_encoding.rs.
 * All five encoding variants are implemented here.
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "bpe.h"
#include "fnv_hash.h"
#include "bitfield.h"
#include "ac_bpe.h"

/* =========================================================================
 * Helpers
 * ========================================================================= */

static inline uint32_t token_len(const BytePairEncoding *bpe, uint32_t id) {
    return bpe->token_starts[id + 1] - bpe->token_starts[id];
}

static inline const uint8_t *tok_bytes(const BytePairEncoding *bpe, uint32_t id) {
    return bpe->all_tokens + bpe->token_starts[id];
}

uint32_t bpe_find_token_by_bytes(const BytePairEncoding *bpe,
                                 const uint8_t *bytes, uint32_t len) {
    uint32_t h = bpe_hash_bytes(bytes, len, bpe->hash_factor);
    uint32_t id = bytesmap_get(&bpe->hash_to_token, h);
    if (id == UINT32_MAX) return UINT32_MAX;
    /* Verify (collision safety) */
    uint32_t tlen = token_len(bpe, id);
    if (tlen != len) return UINT32_MAX;
    if (memcmp(bpe->all_tokens + bpe->token_starts[id], bytes, len) != 0)
        return UINT32_MAX;
    return id;
}

uint32_t bpe_next_match(const BytePairEncoding *bpe,
                        const uint8_t *text, size_t len) {
    uint32_t tok;
    size_t   end;
    if (!ac_leftmost_longest(bpe->longest_searcher, text, len, &tok, &end))
        return UINT32_MAX;
    return tok;
}

/* =========================================================================
 * is_valid_token_pair — exact port of the Rust function
 * ========================================================================= */

bool bpe_is_valid_token_pair(const BytePairEncoding *bpe,
                             uint32_t token1, uint32_t token2) {
    uint32_t limit = UINT32_MAX;
    for (;;) {
        uint32_t combined = pairmap_get(&bpe->pair_lookup, token1, token2);
        if (combined != UINT32_MAX && combined < limit) return false;

        if (token1 > token2) {
            limit  = token1;
            token1 = bpe->split_right[token1];
            if (token1 == limit) {
                limit  = token2 + 1;
                token2 = bpe->split_left[token2];
                if (token2 + 1 == limit) return true;
            }
        } else {
            limit  = token2 + 1;
            token2 = bpe->split_left[token2];
            if (token2 + 1 == limit) {
                limit  = token1;
                token1 = bpe->split_right[token1];
                if (token1 == limit) return true;
            }
        }
    }
}

/* =========================================================================
 * encode_all_prefixes
 * ========================================================================= */

uint32_t *bpe_encode_all_prefixes(const BytePairEncoding *bpe,
                                  const uint8_t *text, size_t text_len) {
    uint32_t    *last_token = (uint32_t *)malloc(text_len * sizeof(uint32_t));
    AcMatchIter  it         = ac_iter_new(bpe->overlapping_searcher);

    for (size_t pos = 0; pos < text_len; pos++) {
        ac_iter_advance(&it, text[pos], pos + 1);

        uint32_t out_tok;
        size_t   out_start;
        while (ac_iter_next_match(&it, &out_tok, &out_start)) {
            /* The match covers out_start..pos+1 */
            if (out_start == 0) {
                last_token[pos] = out_tok;
                break;
            } else {
                uint32_t prev = last_token[out_start - 1];
                if (bpe_is_valid_token_pair(bpe, prev, out_tok)) {
                    last_token[pos] = out_tok;
                    break;
                }
            }
        }
    }
    return last_token;
}

/* =========================================================================
 * Decode
 * ========================================================================= */

uint8_t *bpe_decode_tokens(const BytePairEncoding *bpe,
                           const uint32_t *tokens, size_t ntokens,
                           size_t *out_len) {
    /* Validate all token IDs before accessing arrays */
    for (size_t i = 0; i < ntokens; i++) {
        if (tokens[i] >= bpe->num_tokens) {
            if (out_len) *out_len = 0;
            return NULL;
        }
    }
    size_t total = 0;
    for (size_t i = 0; i < ntokens; i++) total += token_len(bpe, tokens[i]);
    uint8_t *buf = (uint8_t *)malloc(total + 1);
    size_t   off = 0;
    for (size_t i = 0; i < ntokens; i++) {
        uint32_t len = token_len(bpe, tokens[i]);
        memcpy(buf + off, tok_bytes(bpe, tokens[i]), len);
        off += len;
    }
    buf[total] = 0;
    if (out_len) *out_len = total;
    return buf;
}

/* =========================================================================
 * Greedy encode
 * ========================================================================= */

uint32_t *bpe_encode_greedy(const BytePairEncoding *bpe,
                             const uint8_t *text, size_t text_len,
                             size_t *out_n) {
    uint32_t *result = NULL;
    size_t    cap    = text_len / 2 + 16;
    size_t    count  = 0;
    result = (uint32_t *)malloc(cap * sizeof(uint32_t));

    size_t pos = 0;
    while (pos < text_len) {
        uint32_t tok = bpe_next_match(bpe, text + pos, text_len - pos);
        if (tok == UINT32_MAX) {
            /* Should not happen for a complete vocabulary */
            break;
        }
        if (count == cap) {
            cap *= 2;
            result = (uint32_t *)realloc(result, cap * sizeof(uint32_t));
        }
        result[count++] = tok;
        pos += token_len(bpe, tok);
    }
    *out_n = count;
    return result;
}

/* =========================================================================
 * Encode via prefix table
 * ========================================================================= */

uint32_t *bpe_encode_via_table(const BytePairEncoding *bpe,
                               const uint8_t *text, size_t text_len,
                               size_t *out_n) {
    uint32_t *lasto = bpe_encode_all_prefixes(bpe, text, text_len);

    /* Walk back from end */
    uint32_t *result = (uint32_t *)malloc((text_len + 1) * sizeof(uint32_t));
    size_t    count  = 0;
    size_t    pos    = text_len;
    while (pos > 0) {
        uint32_t tok = lasto[pos - 1];
        result[count++] = tok;
        pos -= token_len(bpe, tok);
    }
    free(lasto);
    /* Reverse */
    for (size_t i = 0, j = count - 1; i < j; i++, j--) {
        uint32_t tmp = result[i]; result[i] = result[j]; result[j] = tmp;
    }
    *out_n = count;
    return result;
}

/* =========================================================================
 * Encode via bitfield (priority-heap BPE)
 * ========================================================================= */

/* Simple binary min-heap of (priority, start) pairs. */
typedef struct { uint32_t priority; uint32_t start; } HeapItem;

typedef struct {
    HeapItem *data;
    size_t    size;
    size_t    cap;
} MinHeap;

static void heap_push(MinHeap *h, uint32_t pri, uint32_t start) {
    if (h->size == h->cap) {
        h->cap = h->cap ? h->cap * 2 : 64;
        h->data = (HeapItem *)realloc(h->data, h->cap * sizeof(HeapItem));
    }
    size_t i = h->size++;
    h->data[i] = (HeapItem){ pri, start };
    /* Sift up */
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (h->data[parent].priority <= h->data[i].priority) break;
        HeapItem tmp = h->data[parent];
        h->data[parent] = h->data[i];
        h->data[i] = tmp;
        i = parent;
    }
}

static HeapItem heap_pop(MinHeap *h) {
    HeapItem top = h->data[0];
    h->data[0] = h->data[--h->size];
    /* Sift down */
    size_t i = 0;
    for (;;) {
        size_t l = 2*i+1, r = 2*i+2, smallest = i;
        if (l < h->size && h->data[l].priority < h->data[smallest].priority)
            smallest = l;
        if (r < h->size && h->data[r].priority < h->data[smallest].priority)
            smallest = r;
        if (smallest == i) break;
        HeapItem tmp = h->data[i];
        h->data[i] = h->data[smallest];
        h->data[smallest] = tmp;
        i = smallest;
    }
    return top;
}

static void encode_into_bitfield(const BytePairEncoding *bpe,
                                  const uint8_t *bytes, size_t len,
                                  BitField *bf, size_t *out_count) {
    bitfield_init(bf, len + 1);
    MinHeap heap = { NULL, 0, 0 };

    /* Seed heap with all adjacent pairs */
    for (size_t i = 0; i + 1 < len; i++) {
        uint32_t tok = bpe_find_token_by_bytes(bpe, bytes + i, 2);
        if (tok != UINT32_MAX) heap_push(&heap, tok, (uint32_t)i);
    }

    size_t count = len;
    while (heap.size > 0) {
        HeapItem item = heap_pop(&heap);
        uint32_t token = item.priority; /* priority = token id = merge rank */
        size_t   start = item.start;

        if (!bitfield_is_set(bf, start)) continue;

        size_t mid = bitfield_successor(bf, start + 1);
        if (mid >= len) continue;

        size_t end = bitfield_successor(bf, mid + 1);
        if (token_len(bpe, token) != end - start) continue;

        bitfield_clear(bf, mid);
        count--;

        /* Try extending right */
        if (end < len) {
            size_t new_end = bitfield_successor(bf, end + 1);
            uint32_t ext = bpe_find_token_by_bytes(bpe, bytes + start,
                                                   (uint32_t)(new_end - start));
            if (ext != UINT32_MAX) heap_push(&heap, ext, (uint32_t)start);
        }
        /* Try extending left */
        if (start > 0) {
            size_t new_start = bitfield_predecessor(bf, start - 1);
            uint32_t ext = bpe_find_token_by_bytes(bpe, bytes + new_start,
                                                   (uint32_t)(end - new_start));
            if (ext != UINT32_MAX) heap_push(&heap, ext, (uint32_t)new_start);
        }
    }

    free(heap.data);
    *out_count = count;
}

uint32_t *bpe_encode_via_bitfield(const BytePairEncoding *bpe,
                                  const uint8_t *text, size_t text_len,
                                  size_t *out_n) {
    BitField bf;
    size_t   count;
    encode_into_bitfield(bpe, text, text_len, &bf, &count);

    uint32_t *result = (uint32_t *)malloc(count * sizeof(uint32_t));
    size_t    idx    = 0;
    size_t    start  = 0;
    while (start < text_len) {
        size_t   end = bitfield_successor(&bf, start + 1);
        uint32_t tok = bpe_find_token_by_bytes(bpe, text + start,
                                               (uint32_t)(end - start));
        result[idx++] = tok;
        start = end;
    }
    bitfield_free(&bf);
    *out_n = count;
    return result;
}

/* =========================================================================
 * Encode minimal (shortest token sequence)
 * ========================================================================= */

typedef struct { uint32_t token; uint32_t count_so_far; } MinState;

uint32_t *bpe_encode_minimal(const BytePairEncoding *bpe,
                              const uint8_t *text, size_t text_len,
                              size_t *out_n) {
    /* DP: last_token[pos] = (best_token, count_to_pos+1) */
    MinState *states = (MinState *)malloc(text_len * sizeof(MinState));
    for (size_t i = 0; i < text_len; i++) {
        states[i].token = UINT32_MAX;
        states[i].count_so_far = UINT32_MAX;
    }

    AcMatchIter it = ac_iter_new(bpe->overlapping_searcher);
    for (size_t pos = 0; pos < text_len; pos++) {
        ac_iter_advance(&it, text[pos], pos + 1);
        uint32_t out_tok;
        size_t   out_start;
        while (ac_iter_next_match(&it, &out_tok, &out_start)) {
            uint32_t prev_count = (out_start == 0) ? 0
                                : states[out_start - 1].count_so_far;
            if (prev_count == UINT32_MAX) continue;
            uint32_t new_count = prev_count + 1;
            if (new_count < states[pos].count_so_far) {
                states[pos].token         = out_tok;
                states[pos].count_so_far  = new_count;
            }
        }
    }

    /* Reconstruct */
    size_t    count  = (text_len > 0) ? states[text_len - 1].count_so_far : 0;
    uint32_t *result = (uint32_t *)malloc(count * sizeof(uint32_t));
    size_t    idx    = count;
    size_t    pos    = text_len;
    while (pos > 0 && idx > 0) {
        uint32_t tok = states[pos - 1].token;
        result[--idx] = tok;
        pos -= token_len(bpe, tok);
    }
    free(states);
    *out_n = count;
    return result;
}

/* =========================================================================
 * Count
 * ========================================================================= */

/* BacktrackEncoder state (internal) */
typedef struct {
    const BytePairEncoding *bpe;
    const uint8_t          *text;
    size_t                  text_len;
    uint32_t               *tokens;
    size_t                   token_cap;
    size_t                   token_count;
    uint32_t                 next_token;  /* UINT32_MAX = done */
    size_t                   pos;
    BitField                 bitfield;
} BtEnc;

static BtEnc btenc_new(const BytePairEncoding *bpe, const uint8_t *text,
                        size_t text_len, size_t cap) {
    BtEnc e;
    e.bpe         = bpe;
    e.text        = text;
    e.text_len    = text_len;
    e.token_cap   = cap > 0 ? cap : text_len / 3 + 16;
    e.tokens      = (uint32_t *)malloc(e.token_cap * sizeof(uint32_t));
    e.token_count = 0;
    e.pos         = 0;
    e.next_token  = bpe_next_match(bpe, text, text_len);
    bitfield_init(&e.bitfield, text_len + 1);
    return e;
}

static void btenc_free(BtEnc *e) {
    free(e->tokens);
    bitfield_free(&e->bitfield);
}

/* Returns e->next_token after the step, or UINT32_MAX if encoding is done. */
static uint32_t btenc_step(BtEnc *e) {
    if (e->next_token == UINT32_MAX) return UINT32_MAX;
    uint32_t token = e->next_token;
    uint32_t last  = (e->token_count > 0) ? e->tokens[e->token_count - 1]
                                           : UINT32_MAX;
    for (;;) {
        uint32_t tlen    = token_len(e->bpe, token);
        size_t   end_pos = e->pos + tlen;

        bool pair_ok = (last == UINT32_MAX) ||
                       bpe_is_valid_token_pair(e->bpe, last, token);

        if (bitfield_is_set(&e->bitfield, end_pos) && pair_ok) {
            /* Accept */
            if (e->token_count == e->token_cap) {
                e->token_cap *= 2;
                e->tokens = (uint32_t *)realloc(e->tokens,
                                                e->token_cap * sizeof(uint32_t));
            }
            e->tokens[e->token_count++] = token;
            e->pos = end_pos;
            e->next_token = bpe_next_match(e->bpe, e->text + end_pos,
                                           e->text_len - end_pos);
            break;
        } else {
            /* Try shorter prefix */
            uint32_t shorter = bpe_next_prefix(e->bpe, token);
            if (shorter != UINT32_MAX) {
                token = shorter;
            } else {
                /* Backtrack */
                bitfield_clear(&e->bitfield, e->pos);
                if (e->token_count == 0) {
                    e->next_token = UINT32_MAX;
                    break;
                }
                e->token_count--;
                uint32_t popped = e->tokens[e->token_count];
                e->pos -= token_len(e->bpe, popped);
                last  = (e->token_count > 0) ? e->tokens[e->token_count - 1]
                                              : UINT32_MAX;
                e->next_token = popped;
                break;
            }
        }
    }
    return e->next_token;
}

size_t bpe_count(const BytePairEncoding *bpe,
                 const uint8_t *text, size_t text_len) {
    BtEnc e = btenc_new(bpe, text, text_len, text_len / 3 + 16);
    while (btenc_step(&e) != UINT32_MAX) {}
    size_t c = e.token_count;
    btenc_free(&e);
    return c;
}

size_t bpe_count_till_limit(const BytePairEncoding *bpe,
                             const uint8_t *text, size_t text_len,
                             size_t token_limit) {
    BtEnc  e = btenc_new(bpe, text, text_len, token_limit + 16);
    size_t limit_buf = token_limit + 10;
    while (btenc_step(&e) != UINT32_MAX) {
        if (e.token_count > limit_buf) {
            btenc_free(&e);
            return SIZE_MAX;
        }
    }
    size_t c = e.token_count;
    btenc_free(&e);
    return (c <= token_limit) ? c : SIZE_MAX;
}

uint32_t *bpe_encode_via_backtracking(const BytePairEncoding *bpe,
                                      const uint8_t *text, size_t text_len,
                                      size_t *out_n) {
    BtEnc e = btenc_new(bpe, text, text_len, text_len / 3 + 16);
    while (btenc_step(&e) != UINT32_MAX) {}
    *out_n = e.token_count;
    /* Transfer ownership */
    uint32_t *result = (uint32_t *)realloc(e.tokens,
                                           e.token_count * sizeof(uint32_t));
    e.tokens = NULL;
    bitfield_free(&e.bitfield);
    return result;
}

/* =========================================================================
 * Scratch-buffer API (amortise allocations across many encode/count calls)
 * ========================================================================= */

struct BpeEncScratch {
    uint32_t *tokens;
    size_t    token_cap;
    BitField  bitfield;   /* words pointer reused across calls */
};

BpeEncScratch *bpe_scratch_new(void) {
    BpeEncScratch *s = (BpeEncScratch *)calloc(1, sizeof(BpeEncScratch));
    return s;
}

void bpe_scratch_free(BpeEncScratch *s) {
    if (!s) return;
    free(s->tokens);
    bitfield_free(&s->bitfield);
    free(s);
}

/* Initialise a BtEnc reusing scratch buffers; grows them if needed. */
static void btenc_init_reuse(BtEnc *e, const BytePairEncoding *bpe,
                             const uint8_t *text, size_t text_len,
                             BpeEncScratch *s) {
    e->bpe      = bpe;
    e->text     = text;
    e->text_len = text_len;
    /* Grow tokens array if needed */
    size_t need_cap = text_len / 3 + 16;
    if (need_cap > s->token_cap) {
        s->token_cap = need_cap;
        s->tokens = (uint32_t *)realloc(s->tokens,
                                        s->token_cap * sizeof(uint32_t));
    }
    e->tokens      = s->tokens;
    e->token_cap   = s->token_cap;
    e->token_count = 0;
    e->pos         = 0;
    e->next_token  = bpe_next_match(bpe, text, text_len);
    /* Grow and reset bitfield */
    bitfield_reset(&s->bitfield, text_len + 1);
    e->bitfield = s->bitfield;
}

/* Write back any grown buffers to scratch after encoding. */
static void btenc_sync_scratch(BtEnc *e, BpeEncScratch *s) {
    s->tokens    = e->tokens;
    s->token_cap = e->token_cap;
    s->bitfield  = e->bitfield;
}

const uint32_t *bpe_encode_piece(const BytePairEncoding *bpe,
                                  const uint8_t *text, size_t text_len,
                                  BpeEncScratch *s, size_t *out_n) {
    BtEnc e;
    btenc_init_reuse(&e, bpe, text, text_len, s);
    while (btenc_step(&e) != UINT32_MAX) {}
    *out_n = e.token_count;
    btenc_sync_scratch(&e, s);
    return s->tokens;
}

size_t bpe_count_piece(const BytePairEncoding *bpe,
                        const uint8_t *text, size_t text_len,
                        BpeEncScratch *s) {
    BtEnc e;
    btenc_init_reuse(&e, bpe, text, text_len, s);
    while (btenc_step(&e) != UINT32_MAX) {}
    size_t c = e.token_count;
    btenc_sync_scratch(&e, s);
    return c;
}

/* =========================================================================
 * Self-check (validate that every token encodes to itself)
 * ========================================================================= */

bool bpe_self_check(const BytePairEncoding *bpe) {
    for (uint32_t id = 0; id < bpe->num_tokens; id++) {
        uint32_t len = token_len(bpe, id);
        size_t   n;
        uint32_t *enc = bpe_encode_via_bitfield(bpe, tok_bytes(bpe, id), len, &n);
        bool ok = (n == 1 && enc[0] == id);
        free(enc);
        if (!ok) return false;
    }
    return true;
}

/* =========================================================================
 * Construction: from_dictionary
 * ========================================================================= */

BytePairEncoding *bpe_from_dictionary(const uint8_t *all_tok,
                                      const uint32_t *tok_starts,
                                      uint32_t        num_tok,
                                      uint64_t        hash_factor) {
    if (hash_factor == 0) hash_factor = 1;

    BytePairEncoding *bpe = (BytePairEncoding *)calloc(1, sizeof(BytePairEncoding));
    bpe->num_tokens  = num_tok;
    bpe->hash_factor = hash_factor;

    /* Copy token data */
    uint32_t total_bytes = tok_starts[num_tok];
    bpe->all_tokens   = (uint8_t  *)malloc(total_bytes);
    bpe->token_starts = (uint32_t *)malloc((num_tok + 1) * sizeof(uint32_t));
    memcpy(bpe->all_tokens,   all_tok,    total_bytes);
    memcpy(bpe->token_starts, tok_starts, (num_tok + 1) * sizeof(uint32_t));

    /* Build hash map */
    bytesmap_init(&bpe->hash_to_token, num_tok * 2);
    for (uint32_t i = 0; i < num_tok; i++) {
        uint32_t len = bpe->token_starts[i + 1] - bpe->token_starts[i];
        const uint8_t *b = bpe->all_tokens + bpe->token_starts[i];
        uint32_t h = bpe_hash_bytes(b, len, hash_factor);
        bytesmap_put(&bpe->hash_to_token, h, i);
    }

    /* Build AC automatons */
    {
        AcBuilder *bf = ac_builder_new(AC_KIND_OVERLAPPING_FWD);
        AcBuilder *br = ac_builder_new(AC_KIND_OVERLAPPING_REV);

        for (uint32_t i = 0; i < num_tok; i++) {
            uint32_t len = bpe->token_starts[i + 1] - bpe->token_starts[i];
            const uint8_t *b = bpe->all_tokens + bpe->token_starts[i];
            ac_builder_add(bf, b, len, i);
            ac_builder_add(br, b, len, i); /* reversal done inside builder */
        }

        ac_builder_build_two(bf, AC_KIND_LEFTMOST_LONGEST,
                             AC_KIND_OVERLAPPING_FWD,
                             &bpe->longest_searcher,
                             &bpe->overlapping_searcher);

        bpe->overlapping_searcher_rev = ac_builder_build(br);

        ac_builder_free(bf);
        ac_builder_free(br);
    }

    /* Compute next_prefix_match */
    bpe->next_prefix_match = (uint32_t *)malloc(num_tok * sizeof(uint32_t));
    for (uint32_t i = 0; i < num_tok; i++) {
        uint32_t len = token_len(bpe, i);
        /* Leftmost-longest match on the prefix of token i (without last byte) */
        uint32_t next = UINT32_MAX;
        if (len > 0) {
            next = bpe_next_match(bpe, tok_bytes(bpe, i), len > 1 ? len - 1 : 0);
        }
        bpe->next_prefix_match[i] = next;
    }
    /* Build split_table and pair_lookup by reverse-engineering merges
     * (exact port of the Rust logic) */
    bpe->split_left  = (uint32_t *)malloc(num_tok * sizeof(uint32_t));
    bpe->split_right = (uint32_t *)malloc(num_tok * sizeof(uint32_t));
    pairmap_init(&bpe->pair_lookup, num_tok * 2);

    for (uint32_t id = 0; id < num_tok; id++) {
        bpe->split_left[id]  = id;
        bpe->split_right[id] = id;
    }

    /* Temporary split table for is_valid_token_pair (needs pair_lookup too) */
    for (uint32_t id = 0; id < num_tok; id++) {
        const uint8_t *tok   = tok_bytes(bpe, id);
        uint32_t       tlen  = token_len(bpe, id);

        uint32_t token1 = bpe->next_prefix_match[id];
        while (token1 != UINT32_MAX) {
            uint32_t rest_len = tlen - token_len(bpe, token1);
            const uint8_t *rest = tok + token_len(bpe, token1);
            uint32_t token2 = bpe_find_token_by_bytes(bpe, rest, rest_len);

            if (token2 != UINT32_MAX && token1 < id && token2 < id &&
                bpe_is_valid_token_pair(bpe, token1, token2)) {
                pairmap_put(&bpe->pair_lookup, token1, token2, id);
                bpe->split_left[id]  = token1;
                bpe->split_right[id] = token2;
                break;
            }
            token1 = bpe->next_prefix_match[token1];
        }
        /* If no merge found, split_left/right stay == id (primitive token) */
    }
    return bpe;
}

void bpe_free(BytePairEncoding *bpe) {
    if (!bpe) return;
    free(bpe->all_tokens);
    free(bpe->token_starts);
    bytesmap_free(&bpe->hash_to_token);
    free(bpe->split_left);
    free(bpe->split_right);
    pairmap_free(&bpe->pair_lookup);
    ac_automaton_free(bpe->longest_searcher);
    ac_automaton_free(bpe->overlapping_searcher);
    ac_automaton_free(bpe->overlapping_searcher_rev);
    free(bpe->next_prefix_match);
    free(bpe);
}

/* =========================================================================
 * Construction: from pre-computed binary blob
 *
 * The blob layout (little-endian) matches gen_precomputed.c output:
 *   Header:  num_tokens(u32), hash_factor(u64),
 *            bm_cap(u32), bm_count(u32), pm_cap(u32), pm_count(u32),
 *            3x automaton headers: da_size(i32), noutputs(i32), kind(i32)
 *   Arrays:  split_left[nt], split_right[nt], next_prefix_match[nt],
 *            BytesMapSlots[bm_cap], PairMapSlots[pm_cap],
 *            3x automaton data: cells, outputs, da_base, da_check
 * ========================================================================= */

BytePairEncoding *bpe_from_blob(
    const uint8_t  *all_tokens,
    const uint32_t *token_starts,
    const uint8_t  *blob,
    size_t          blob_len)
{
    const uint8_t *p = blob;

    /* ---- Read header scalars ---- */
    uint32_t num_tokens;     memcpy(&num_tokens,  p, 4); p += 4;
    uint64_t hash_factor;    memcpy(&hash_factor,  p, 8); p += 8;
    uint32_t bm_cap;         memcpy(&bm_cap,       p, 4); p += 4;
    uint32_t bm_count;       memcpy(&bm_count,     p, 4); p += 4;
    uint32_t pm_cap;         memcpy(&pm_cap,       p, 4); p += 4;
    uint32_t pm_count;       memcpy(&pm_count,     p, 4); p += 4;

    int32_t ac_da_size[3], ac_noutputs[3], ac_kind[3];
    for (int i = 0; i < 3; i++) {
        memcpy(&ac_da_size[i],  p, 4); p += 4;
        memcpy(&ac_noutputs[i], p, 4); p += 4;
        memcpy(&ac_kind[i],     p, 4); p += 4;
    }

    /* ---- Allocate BPE ---- */
    BytePairEncoding *bpe = (BytePairEncoding *)calloc(1, sizeof(BytePairEncoding));
    bpe->num_tokens  = num_tokens;
    bpe->hash_factor = hash_factor;

    /* Token data — copy so bpe_free works uniformly */
    uint32_t total_bytes = token_starts[num_tokens];
    bpe->all_tokens   = (uint8_t *)malloc(total_bytes);
    bpe->token_starts = (uint32_t *)malloc((num_tokens + 1) * sizeof(uint32_t));
    memcpy(bpe->all_tokens,   all_tokens,   total_bytes);
    memcpy(bpe->token_starts, token_starts, (num_tokens + 1) * sizeof(uint32_t));

    /* ---- Bulk-copy arrays from blob ---- */
#define BLOB_COPY(dst, type, count) do {                          \
        size_t sz = (size_t)(count) * sizeof(type);               \
        (dst) = (type *)malloc(sz);                               \
        memcpy((dst), p, sz);                                     \
        p += sz;                                                  \
    } while (0)

    BLOB_COPY(bpe->split_left,        uint32_t, num_tokens);
    BLOB_COPY(bpe->split_right,       uint32_t, num_tokens);
    BLOB_COPY(bpe->next_prefix_match, uint32_t, num_tokens);

    /* BytesMap */
    bpe->hash_to_token.capacity = bm_cap;
    bpe->hash_to_token.count    = bm_count;
    BLOB_COPY(bpe->hash_to_token.slots, BytesMapSlot, bm_cap);

    /* PairMap */
    bpe->pair_lookup.capacity = pm_cap;
    bpe->pair_lookup.count    = pm_count;
    BLOB_COPY(bpe->pair_lookup.slots, PairMapSlot, pm_cap);

    /* ---- 3 AC automatons ---- */
    AcAutomaton **ac_ptrs[3] = {
        &bpe->longest_searcher,
        &bpe->overlapping_searcher,
        &bpe->overlapping_searcher_rev,
    };
    for (int i = 0; i < 3; i++) {
        AcAutomaton *a = (AcAutomaton *)calloc(1, sizeof(AcAutomaton));
        a->kind     = (AcKind)ac_kind[i];
        a->ncells   = ac_da_size[i];
        a->da_size  = ac_da_size[i];
        a->noutputs = ac_noutputs[i];

        BLOB_COPY(a->cells,    AcCell,   ac_da_size[i]);
        BLOB_COPY(a->outputs,  AcOutput, ac_noutputs[i]);
        BLOB_COPY(a->da_base,  int32_t,  ac_da_size[i]);
        BLOB_COPY(a->da_check, int32_t,  ac_da_size[i]);

        *ac_ptrs[i] = a;
    }
#undef BLOB_COPY

    (void)blob_len; /* could assert p - blob == blob_len */
    return bpe;
}
