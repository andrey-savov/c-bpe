/* ac_bpe.c — Byte-level Aho-Corasick automaton for BPE (double-array variant).
 *
 * Algorithm overview
 * ------------------
 *  Phase 1 (build):   Insert all patterns into a compact linked-edge trie.
 *  Phase 2 (build):   BFS to compute AC failure links and output chains.
 *  Phase 3 (build):   Pack trie into double-array for O(1) transitions.
 *  Runtime:           ac_step() follows one byte in O(1) amortised time.
 *
 * MIT licence – adapted from ac_dat (https://github.com/Izolex/ac_dat).
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "ac_bpe.h"

/* =========================================================================
 * Build-phase: compact trie with sorted edge lists
 * ========================================================================= */

typedef struct TrieEdge {
    uint8_t  byte;
    int32_t  child;   /* index into nodes[] */
} TrieEdge;

typedef struct TrieNode {
    TrieEdge *edges;
    uint32_t  nedges;
    uint32_t  cap;
    uint32_t  token;  /* token id if accepting, else AC_INVALID_TOKEN */
} TrieNode;

typedef struct {
    TrieNode *nodes;
    int32_t   count;
    int32_t   cap;
    AcKind    kind;
} TrieBuilder;

static TrieBuilder *tb_new(AcKind kind) {
    TrieBuilder *tb = (TrieBuilder *)calloc(1, sizeof(TrieBuilder));
    tb->kind   = kind;
    tb->cap    = 4096;
    tb->nodes  = (TrieNode *)calloc(tb->cap, sizeof(TrieNode));
    tb->count  = 1; /* node 0 = root */
    for (int32_t i = 0; i < tb->cap; i++)
        tb->nodes[i].token = AC_INVALID_TOKEN;
    return tb;
}

static int32_t tb_new_node(TrieBuilder *tb) {
    if (tb->count == tb->cap) {
        int32_t old_cap = tb->cap;
        tb->cap *= 2;
        tb->nodes = (TrieNode *)realloc(tb->nodes, tb->cap * sizeof(TrieNode));
        memset(tb->nodes + old_cap, 0, (tb->cap - old_cap) * sizeof(TrieNode));
        for (int32_t i = old_cap; i < tb->cap; i++)
            tb->nodes[i].token = AC_INVALID_TOKEN;
    }
    int32_t id = tb->count++;
    tb->nodes[id].edges  = NULL;
    tb->nodes[id].nedges = 0;
    tb->nodes[id].cap    = 0;
    tb->nodes[id].token  = AC_INVALID_TOKEN;
    return id;
}

static int32_t tb_child(TrieBuilder *tb, int32_t node, uint8_t byte) {
    TrieNode *n = &tb->nodes[node];
    /* Binary search */
    int32_t lo = 0, hi = (int32_t)n->nedges;
    while (lo < hi) {
        int32_t mid = (lo + hi) / 2;
        if (n->edges[mid].byte == byte) return n->edges[mid].child;
        if (n->edges[mid].byte < byte)  lo = mid + 1;
        else                             hi = mid;
    }
    /* Insert sorted at position lo */
    if (n->nedges == n->cap) {
        n->cap = n->cap ? n->cap * 2 : 4;
        n->edges = (TrieEdge *)realloc(n->edges, n->cap * sizeof(TrieEdge));
    }
    int32_t child = tb_new_node(tb);
    n = &tb->nodes[node]; /* re-fetch after possible realloc */
    memmove(&n->edges[lo + 1], &n->edges[lo],
            (n->nedges - (uint32_t)lo) * sizeof(TrieEdge));
    n->edges[lo] = (TrieEdge){ byte, child };
    n->nedges++;
    return child;
}

static void tb_insert(TrieBuilder *tb, const uint8_t *bytes, uint32_t len,
                      uint32_t token_id) {
    int32_t node = 0;
    for (uint32_t i = 0; i < len; i++) node = tb_child(tb, node, bytes[i]);
    if (tb->nodes[node].token == AC_INVALID_TOKEN)
        tb->nodes[node].token = token_id;
}

/* =========================================================================
 * AcBuilder / ac_builder_build
 * ========================================================================= */

struct AcBuilder {
    void *priv; /* TrieBuilder* */
};

AcBuilder *ac_builder_new(AcKind kind) {
    AcBuilder *b = (AcBuilder *)calloc(1, sizeof(AcBuilder));
    b->priv = tb_new(kind);
    return b;
}

void ac_builder_add(AcBuilder *b, const uint8_t *bytes, size_t len,
                    uint32_t token_id) {
    TrieBuilder *tb = (TrieBuilder *)b->priv;
    if (tb->kind == AC_KIND_OVERLAPPING_REV) {
        uint8_t *rev = (uint8_t *)malloc(len);
        for (size_t i = 0; i < len; i++) rev[i] = bytes[len - 1 - i];
        tb_insert(tb, rev, (uint32_t)len, token_id);
        free(rev);
    } else {
        tb_insert(tb, bytes, (uint32_t)len, token_id);
    }
}

void ac_builder_free(AcBuilder *b) {
    if (!b) return;
    TrieBuilder *tb = (TrieBuilder *)b->priv;
    for (int32_t i = 0; i < tb->count; i++) free(tb->nodes[i].edges);
    free(tb->nodes);
    free(tb);
    free(b);
}

void ac_automaton_free(AcAutomaton *a) {
    if (!a) return;
    free(a->cells);
    free(a->outputs);
    free(a->da_base);
    free(a->da_check);
    free(a);
}

/* =========================================================================
 * Trie child lookup (binary search on sorted edges)
 * ========================================================================= */

static int32_t trie_find_child(TrieBuilder *tb, int32_t node, uint8_t byte) {
    TrieNode *n = &tb->nodes[node];
    int32_t lo = 0, hi = (int32_t)n->nedges;
    while (lo < hi) {
        int32_t mid = (lo + hi) / 2;
        if (n->edges[mid].byte == byte) return n->edges[mid].child;
        if (n->edges[mid].byte < byte) lo = mid + 1;
        else                            hi = mid;
    }
    return -1;
}

/* =========================================================================
 * Double-array growth helper
 * ========================================================================= */

static void da_grow(int32_t **base, int32_t **check, int32_t *cap,
                    int32_t needed) {
    if (needed <= *cap) return;
    int32_t new_cap = *cap;
    while (new_cap < needed) new_cap *= 2;
    *base  = (int32_t *)realloc(*base,  new_cap * sizeof(int32_t));
    *check = (int32_t *)realloc(*check, new_cap * sizeof(int32_t));
    memset(*base  + *cap, 0,    (size_t)(new_cap - *cap) * sizeof(int32_t));
    memset(*check + *cap, 0xFF, (size_t)(new_cap - *cap) * sizeof(int32_t));
    *cap = new_cap;
}

/* =========================================================================
 * Build: BFS on trie for fail/output, then pack into double-array
 * ========================================================================= */

static AcAutomaton *build_automaton_from_trie(TrieBuilder *tb, AcKind kind) {
    int32_t nnodes = tb->count;

    /* ---- Phase 1: BFS on trie for failure links and output chains ---- */

    int32_t  *trie_fail   = (int32_t  *)calloc(nnodes, sizeof(int32_t));
    int32_t  *trie_output = (int32_t  *)calloc(nnodes, sizeof(int32_t));
    uint32_t *trie_depth  = (uint32_t *)calloc(nnodes, sizeof(uint32_t));

    int32_t  out_cap  = 1024;
    int32_t  noutputs = 1; /* index 0 = sentinel (end of chain) */
    AcOutput *outputs = (AcOutput *)calloc(out_cap, sizeof(AcOutput));

    int32_t *queue = (int32_t *)malloc(nnodes * sizeof(int32_t));
    int32_t  qh = 0, qt = 0;

    /* Seed: root's children (fail → root, depth = 1) */
    {
        TrieNode *root = &tb->nodes[0];
        for (uint32_t j = 0; j < root->nedges; j++) {
            int32_t ci = root->edges[j].child;
            trie_fail[ci]  = 0;
            trie_depth[ci] = 1;
            queue[qt++]    = ci;
        }
    }

    while (qh < qt) {
        int32_t  nd    = queue[qh++];
        uint32_t depth = trie_depth[nd];

        /* Compute output chain entry for nd */
        if (tb->nodes[nd].token != AC_INVALID_TOKEN) {
            if (noutputs >= out_cap) {
                out_cap *= 2;
                outputs = (AcOutput *)realloc(outputs,
                                              out_cap * sizeof(AcOutput));
            }
            int32_t oidx = noutputs++;
            outputs[oidx].token        = tb->nodes[nd].token;
            outputs[oidx].start_offset = depth;
            outputs[oidx].next         = trie_output[trie_fail[nd]];
            trie_output[nd] = oidx;
        } else {
            trie_output[nd] = trie_output[trie_fail[nd]];
        }

        /* Process children of nd */
        TrieNode *n = &tb->nodes[nd];
        for (uint32_t j = 0; j < n->nedges; j++) {
            uint8_t c  = n->edges[j].byte;
            int32_t ci = n->edges[j].child;
            trie_depth[ci] = depth + 1;

            /* Walk failure chain of nd to find fail link for ci */
            int32_t fs = trie_fail[nd];
            for (;;) {
                int32_t child = trie_find_child(tb, fs, c);
                if (child >= 0 && child != ci) { fs = child; break; }
                if (fs == 0) break;
                fs = trie_fail[fs];
            }
            trie_fail[ci] = fs;
            queue[qt++]   = ci;
        }
    }

    /* ---- Phase 2: Pack trie into double-array ---- */

    int32_t da_cap = nnodes * 2;
    if (da_cap < 512) da_cap = 512;

    int32_t *da_base  = (int32_t *)calloc(da_cap, sizeof(int32_t));
    int32_t *da_check = (int32_t *)malloc(da_cap * sizeof(int32_t));
    memset(da_check, 0xFF, da_cap * sizeof(int32_t)); /* -1 = empty */

    int32_t *trie_to_da = (int32_t *)malloc(nnodes * sizeof(int32_t));
    for (int32_t i = 0; i < nnodes; i++) trie_to_da[i] = -1;

    /* Root → DA slot 0 */
    trie_to_da[0] = 0;
    da_check[0]   = 0; /* mark occupied */

    int32_t next_base = 1; /* scanning hint for base assignment */

    /* BFS: assign DA slots to all children */
    qh = qt = 0;
    queue[qt++] = 0;

    while (qh < qt) {
        int32_t   tnd = queue[qh++];
        TrieNode *n   = &tb->nodes[tnd];
        if (n->nedges == 0) continue; /* leaf, no children */

        int32_t parent_da = trie_to_da[tnd];

        /* Find smallest base b >= next_base s.t. all b + child_byte
           positions are empty in da_check. */
        int32_t b;
        for (b = next_base; ; b++) {
            da_grow(&da_base, &da_check, &da_cap, b + 256);
            bool ok = true;
            for (uint32_t j = 0; j < n->nedges; j++) {
                if (da_check[b + (int32_t)n->edges[j].byte] != -1) {
                    ok = false;
                    break;
                }
            }
            if (ok) break;
        }

        da_base[parent_da] = b;

        /* Place each child */
        for (uint32_t j = 0; j < n->nedges; j++) {
            int32_t slot       = b + (int32_t)n->edges[j].byte;
            int32_t child_trie = n->edges[j].child;
            da_check[slot]         = parent_da;
            trie_to_da[child_trie] = slot;
            queue[qt++]            = child_trie;
        }

        /* Advance hint past first occupied slot */
        while (next_base < da_cap && da_check[next_base] != -1)
            next_base++;
    }

    int32_t da_size = da_cap;

    /* ---- Phase 3: Build cells array (indexed by DA slot) ---- */

    AcCell *cells = (AcCell *)calloc(da_size, sizeof(AcCell));
    for (int32_t i = 0; i < da_size; i++) {
        cells[i].fail   = 0;
        cells[i].output = 0;
        cells[i].token  = AC_INVALID_TOKEN;
        cells[i].depth  = 0;
    }

    for (int32_t i = 0; i < nnodes; i++) {
        int32_t da = trie_to_da[i];
        assert(da >= 0);
        cells[da].fail   = trie_to_da[trie_fail[i]];
        cells[da].output = trie_output[i];
        cells[da].token  = tb->nodes[i].token;
        cells[da].depth  = trie_depth[i];
    }

    /* ---- Assemble automaton ---- */

    AcAutomaton *a = (AcAutomaton *)calloc(1, sizeof(AcAutomaton));
    a->kind     = kind;
    a->ncells   = da_size;
    a->cells    = cells;
    a->outputs  = outputs;
    a->noutputs = noutputs;
    a->da_base  = da_base;
    a->da_check = da_check;
    a->da_size  = da_size;

    free(trie_to_da);
    free(trie_fail);
    free(trie_output);
    free(trie_depth);
    free(queue);
    return a;
}

AcAutomaton *ac_builder_build(AcBuilder *b) {
    TrieBuilder *tb = (TrieBuilder *)b->priv;
    return build_automaton_from_trie(tb, tb->kind);
}

void ac_builder_build_two(AcBuilder *b, AcKind kind1, AcKind kind2,
                           AcAutomaton **out1, AcAutomaton **out2) {
    TrieBuilder *tb = (TrieBuilder *)b->priv;
    *out1 = build_automaton_from_trie(tb, kind1);
    *out2 = build_automaton_from_trie(tb, kind2);
}

/* =========================================================================
 * Runtime step: O(1) double-array lookup
 * ========================================================================= */

static inline int32_t ac_step(const AcAutomaton *a, int32_t state,
                               uint8_t byte) {
    for (;;) {
        int32_t t = a->da_base[state] + (int32_t)byte;
        if (a->da_check[t] == state) return t;
        if (state == 0) return 0;
        state = a->cells[state].fail;
    }
}

/* =========================================================================
 * Leftmost-longest search
 * ========================================================================= */

bool ac_leftmost_longest(const AcAutomaton *a, const uint8_t *text, size_t len,
                         uint32_t *out_token, size_t *out_end) {
    int32_t  state          = 0;
    uint32_t best_token     = AC_INVALID_TOKEN;
    size_t   best_end       = 0;
    size_t   leftmost_start = (size_t)-1;

    for (size_t i = 0; i < len; i++) {
        state = ac_step(a, state, text[i]);
        int32_t out = a->cells[state].output;
        while (out) {
            const AcOutput *o = &a->outputs[out];
            size_t mstart = (i + 1) - o->start_offset;
            size_t mend   = i + 1;
            if (leftmost_start == (size_t)-1 ||
                mstart < leftmost_start ||
                (mstart == leftmost_start && mend > best_end)) {
                leftmost_start = mstart;
                best_token     = o->token;
                best_end       = mend;
            }
            out = o->next;
        }
        if (best_token != AC_INVALID_TOKEN && state == 0) break;
    }

    if (best_token == AC_INVALID_TOKEN) return false;
    *out_token = best_token;
    *out_end   = best_end;
    return true;
}

/* =========================================================================
 * Overlapping iterator
 * ========================================================================= */

int32_t ac_start_state(void) { return 0; }

AcMatchIter ac_iter_new(const AcAutomaton *a) {
    return (AcMatchIter){ .automaton = a, .state = 0, .pos = 0,
                          .pending_out = 0, .pending_end = 0 };
}

void ac_iter_advance(AcMatchIter *it, uint8_t byte, size_t end_pos) {
    it->state       = ac_step(it->automaton, it->state, byte);
    it->pos         = end_pos;
    it->pending_out = it->automaton->cells[it->state].output;
    it->pending_end = end_pos;
}

bool ac_iter_next_match(AcMatchIter *it, uint32_t *out_token,
                        size_t *out_start) {
    if (!it->pending_out) return false;
    const AcOutput *o = &it->automaton->outputs[it->pending_out];
    *out_token  = o->token;
    *out_start  = it->pending_end - o->start_offset;
    it->pending_out = o->next;
    return true;
}
