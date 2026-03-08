/* ac_bpe.c — Byte-level Aho-Corasick automaton for BPE (CSR trie variant).
 *
 * Algorithm overview
 * ------------------
 *  Phase 1 (build):   Insert all patterns into a compact linked-edge trie.
 *  Phase 2 (build):   Convert trie edges to CSR (Compressed Sparse Row) layout.
 *  Phase 3 (build):   BFS to compute AC failure links and output chains.
 *  Runtime:           ac_step() follows one byte in O(log K) amortised time
 *                     where K ≤ 256 is the fanout.
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
    free(a->edge_off);
    free(a->edge_bytes);
    free(a->edge_tgt);
    free(a);
}

/* Build an AcAutomaton from a TrieBuilder with the given kind.
 * Converts trie edges to CSR format and runs BFS for fail/output links. */
static AcAutomaton *build_automaton_from_trie(TrieBuilder *tb, AcKind kind) {
    int32_t nnodes = tb->count;
    AcAutomaton *a = (AcAutomaton *)calloc(1, sizeof(AcAutomaton));
    a->kind   = kind;
    a->ncells = nnodes;

    /* Allocate cells (one per trie node) */
    a->cells = (AcCell *)calloc(nnodes, sizeof(AcCell));
    for (int32_t i = 0; i < nnodes; i++) {
        a->cells[i].fail   = 0; /* root = 0 */
        a->cells[i].output = 0;
        a->cells[i].token  = tb->nodes[i].token;
        a->cells[i].depth  = 0;
    }

    /* Build CSR edge arrays from trie */
    int32_t total_edges = 0;
    a->edge_off = (int32_t *)malloc((nnodes + 1) * sizeof(int32_t));
    for (int32_t i = 0; i < nnodes; i++) {
        a->edge_off[i] = total_edges;
        total_edges += (int32_t)tb->nodes[i].nedges;
    }
    a->edge_off[nnodes] = total_edges;

    a->edge_bytes = (uint8_t *)malloc(total_edges * sizeof(uint8_t));
    a->edge_tgt   = (int32_t *)malloc(total_edges * sizeof(int32_t));

    for (int32_t i = 0; i < nnodes; i++) {
        int32_t off = a->edge_off[i];
        TrieNode *n = &tb->nodes[i];
        for (uint32_t j = 0; j < n->nedges; j++) {
            a->edge_bytes[off + (int32_t)j] = n->edges[j].byte;
            a->edge_tgt[off + (int32_t)j]   = n->edges[j].child;
        }
    }

    /* Output pool; 0 = sentinel */
    int32_t out_cap = 1024;
    a->noutputs = 1;
    a->outputs = (AcOutput *)calloc(out_cap, sizeof(AcOutput));

    /* BFS to compute depth, failure links, and output chains. */
    int32_t *queue = (int32_t *)malloc(nnodes * sizeof(int32_t));
    int32_t  qh = 0, qt = 0;

    /* Seed: root's direct children */
    {
        int32_t off = a->edge_off[0];
        int32_t end = a->edge_off[1];
        for (int32_t j = off; j < end; j++) {
            int32_t ci = a->edge_tgt[j];
            a->cells[ci].fail  = 0;
            a->cells[ci].depth = 1;
            queue[qt++] = ci;
        }
    }

    while (qh < qt) {
        int32_t nd = queue[qh++];
        uint32_t depth = a->cells[nd].depth;

        /* Output for nd */
        if (a->cells[nd].token != AC_INVALID_TOKEN) {
            if (a->noutputs >= out_cap) {
                out_cap *= 2;
                a->outputs = (AcOutput *)realloc(a->outputs,
                                                  out_cap * sizeof(AcOutput));
            }
            int32_t oidx = a->noutputs++;
            a->outputs[oidx].token        = a->cells[nd].token;
            a->outputs[oidx].start_offset = depth;
            a->outputs[oidx].next         = a->cells[a->cells[nd].fail].output;
            a->cells[nd].output = oidx;
        } else {
            a->cells[nd].output = a->cells[a->cells[nd].fail].output;
        }

        /* Process children */
        int32_t eoff = a->edge_off[nd];
        int32_t eend = a->edge_off[nd + 1];
        for (int32_t j = eoff; j < eend; j++) {
            uint8_t c  = a->edge_bytes[j];
            int32_t ci = a->edge_tgt[j];
            a->cells[ci].depth = depth + 1;

            /* Walk failure chain of nd to find fail for ci */
            int32_t fs = a->cells[nd].fail;
            for (;;) {
                /* Check if fs has a child on byte c */
                int32_t child = -1;
                int32_t foff = a->edge_off[fs];
                int32_t fend = a->edge_off[fs + 1];
                /* Binary search */
                int32_t lo2 = foff, hi2 = fend;
                while (lo2 < hi2) {
                    int32_t mid = (lo2 + hi2) / 2;
                    if (a->edge_bytes[mid] == c) { child = a->edge_tgt[mid]; break; }
                    if (a->edge_bytes[mid] < c) lo2 = mid + 1;
                    else hi2 = mid;
                }
                if (child >= 0 && child != ci) {
                    fs = child;
                    break;
                }
                if (fs == 0) break; /* root */
                fs = a->cells[fs].fail;
            }
            a->cells[ci].fail = fs;
            queue[qt++] = ci;
        }
    }
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
 * Runtime step: binary search on sorted CSR edges
 * ========================================================================= */

static inline int32_t ac_step(const AcAutomaton *a, int32_t state,
                               uint8_t byte) {
    for (;;) {
        int32_t lo = a->edge_off[state];
        int32_t hi = a->edge_off[state + 1];
        /* Binary search for byte */
        while (lo < hi) {
            int32_t mid = (lo + hi) / 2;
            uint8_t mb  = a->edge_bytes[mid];
            if (mb == byte) return a->edge_tgt[mid];
            if (mb < byte) lo = mid + 1;
            else           hi = mid;
        }
        if (state == 0) return 0; /* root: stay at root */
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
