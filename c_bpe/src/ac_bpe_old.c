/* ac_bpe.c — Byte-level Double Array Aho-Corasick for BPE.
 *
 * Algorithm overview
 * ------------------
 *  Phase 1 (build):   Insert all patterns into a compact linked-edge trie.
 *  Phase 2 (build):   BFS/XCHECK to assign each trie node a "base" in the
 *                     flat Double Array.
 *  Phase 3 (build):   BFS again to compute AC failure links and output chains.
 *  Runtime:           ac_step() follows one byte in O(1) amortised time.
 *
 * Double Array encoding (TRIE_ROOT == 1):
 *   - transition(s, c) = t  iff  base[s]+c == t  &&  check[t] == s
 *   - unused cells have check[t] == -1
 *
 * MIT licence – adapted from ac_dat (https://github.com/Izolex/ac_dat).
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>
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
    int32_t   dat_id; /* assigned DAT cell index, -1 until Phase 2 */
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
    /* calloc zeros memory, but token must be AC_INVALID_TOKEN (UINT32_MAX)
     * and dat_id must be -1. Initialise ALL pre-allocated nodes. */
    for (int32_t i = 0; i < tb->cap; i++) {
        tb->nodes[i].token  = AC_INVALID_TOKEN;
        tb->nodes[i].dat_id = -1;
    }
    return tb;
}

static int32_t tb_new_node(TrieBuilder *tb) {
    if (tb->count == tb->cap) {
        int32_t old_cap = tb->cap;
        tb->cap *= 2;
        tb->nodes = (TrieNode *)realloc(tb->nodes, tb->cap * sizeof(TrieNode));
        memset(tb->nodes + old_cap, 0, (tb->cap - old_cap) * sizeof(TrieNode));
        for (int32_t i = old_cap; i < tb->cap; i++) {
            tb->nodes[i].token  = AC_INVALID_TOKEN;
            tb->nodes[i].dat_id = -1;
        }
    }
    int32_t id = tb->count++;
    /* Belt-and-suspenders: ensure the node is properly initialised
     * regardless of whether it came from calloc or realloc. */
    tb->nodes[id].edges  = NULL;
    tb->nodes[id].nedges = 0;
    tb->nodes[id].cap    = 0;
    tb->nodes[id].token  = AC_INVALID_TOKEN;
    tb->nodes[id].dat_id = -1;
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
    /* Pointer may have been reallocated; re-fetch */
    n = &tb->nodes[node];
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
 * Phase 2: XCHECK-based DAT construction
 * ========================================================================= */

typedef struct {
    int32_t *base;
    int32_t *check; /* -1 = free */
    int32_t  cap;
    int32_t  first_free; /* lowest index with check == -1 (scan hint) */
} DatArray;

static void dat_ensure(DatArray *dat, int32_t need) {
    if (need < dat->cap) return;
    int32_t nc = dat->cap;
    while (nc <= need) nc *= 2;
    dat->base  = (int32_t *)realloc(dat->base,  nc * sizeof(int32_t));
    dat->check = (int32_t *)realloc(dat->check, nc * sizeof(int32_t));
    for (int32_t i = dat->cap; i < nc; i++) {
        dat->base[i]  = 0;
        dat->check[i] = -1;
    }
    dat->cap = nc;
}

/* Find smallest base b >= 1 such that base+c is free for all children c.
 * Uses a free-cell bitset for fast scanning. */
static int32_t dat_xcheck(DatArray *dat, const TrieEdge *edges,
                           uint32_t nedges) {
    if (nedges == 0) return 1;
    /* Scan free positions for the first child byte */
    int32_t first_byte = (int32_t)edges[0].byte;
    int32_t pos = dat->first_free;
    if (pos < first_byte + 1) pos = first_byte + 1;
    for (; ; pos++) {
        if (pos >= dat->cap) dat_ensure(dat, pos + 256);
        if (dat->check[pos] >= 0) continue; /* occupied */
        int32_t b = pos - first_byte;
        /* Now check remaining children */
        int32_t ok = 1;
        for (uint32_t i = 1; i < nedges; i++) {
            int32_t idx = b + (int32_t)edges[i].byte;
            if (idx >= dat->cap) break; /* will grow, assume free */
            if (dat->check[idx] >= 0) { ok = 0; break; }
        }
        if (ok) return b;
    }
}

static DatArray *build_dat(TrieBuilder *tb) {
    DatArray *dat = (DatArray *)calloc(1, sizeof(DatArray));
    /* Pre-allocate moderately — just enough to avoid excessive resizes */
    int32_t init_cap = tb->count + 512;
    if (init_cap < 8192) init_cap = 8192;
    dat->cap   = init_cap;
    dat->base  = (int32_t *)calloc(dat->cap, sizeof(int32_t));
    dat->check = (int32_t *)malloc(dat->cap * sizeof(int32_t));
    for (int32_t i = 0; i < dat->cap; i++) dat->check[i] = -1;
    dat->first_free = 0;

    /* BFS queue over trie node indices */
    int32_t *q = (int32_t *)malloc(tb->cap * 2 * sizeof(int32_t));
    int32_t  qh = 0, qt = 0;

    /* Root is assigned DAT index 1 */
    dat_ensure(dat, 1);
    dat->check[1] = 1;   /* root's check = itself */
    dat->base[1]  = 0;   /* set after xcheck */
    tb->nodes[0].dat_id = 1;
    q[qt++] = 0;

    while (qh < qt) {
        int32_t tn     = q[qh++];
        int32_t dat_id = tb->nodes[tn].dat_id;
        TrieNode *n    = &tb->nodes[tn];

        if (n->nedges == 0) { dat->base[dat_id] = 1; continue; }

        int32_t b = dat_xcheck(dat, n->edges, n->nedges);
        dat->base[dat_id] = b;

        for (uint32_t i = 0; i < n->nedges; i++) {
            int32_t ci = b + (int32_t)n->edges[i].byte;
            dat_ensure(dat, ci + 257);
            dat->check[ci] = dat_id;
            dat->base[ci]  = 0;  /* will be filled when this child is processed */
            tb->nodes[n->edges[i].child].dat_id = ci;
            q[qt++] = n->edges[i].child;
        }
        /* Advance first_free past any newly-occupied cells */
        while (dat->first_free < dat->cap &&
               dat->check[dat->first_free] >= 0)
            dat->first_free++;
    }
    free(q);
    return dat;
}

/* =========================================================================
 * Reverse-lookup: given a DAT cell id, find the corresponding trie node.
 * (Used only during build for the BFS in Phase 3.)
 * ========================================================================= */
static int32_t find_trie_node(const TrieBuilder *tb, int32_t dat_id) {
    for (int32_t i = 0; i < tb->count; i++)
        if (tb->nodes[i].dat_id == dat_id) return i;
    return -1;
}

/* =========================================================================
 * AcBuilder / ac_builder_build
 * ========================================================================= */

struct AcBuilder {
    void *priv; /* TrieBuilder* */
};

AcBuilder *ac_builder_new(AcKind kind) {
    AcBuilder *b  = (AcBuilder *)calloc(1, sizeof(AcBuilder));
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
    free(a);
}

/* Build an AcAutomaton from a TrieBuilder whose nodes already have dat_id set,
 * and the corresponding DatArray. This runs Phase 3 (fail + output BFS). */
static AcAutomaton *build_automaton_from_dat(TrieBuilder *tb, DatArray *dat,
                                              AcKind kind) {
    int32_t ncells = dat->cap;
    AcAutomaton *a = (AcAutomaton *)calloc(1, sizeof(AcAutomaton));
    a->kind    = kind;
    a->ncells  = ncells;
    a->cells   = (AcCell *)calloc(ncells, sizeof(AcCell));
    for (int32_t i = 0; i < ncells; i++) {
        a->cells[i].token  = AC_INVALID_TOKEN;
        a->cells[i].fail   = 1;
        a->cells[i].output = 0;
        a->cells[i].check  = dat->check[i];
        a->cells[i].base   = dat->base[i];
    }

    /* Copy token IDs from trie nodes */
    for (int32_t tn = 0; tn < tb->count; tn++) {
        int32_t di = tb->nodes[tn].dat_id;
        if (di > 0 && di < ncells)
            a->cells[di].token = tb->nodes[tn].token;
    }

    /* Root: fail = self, check = self */
    a->cells[1].fail  = 1;
    a->cells[1].check = 1;

    /* Output pool; 0 = sentinel */
    int32_t out_cap = 1024;
    a->noutputs = 1;
    a->outputs  = (AcOutput *)calloc(out_cap, sizeof(AcOutput));

    /* BFS to compute failure links + output chains.
     * Queue holds trie-node indices (NOT DAT indices) so we can access
     * child edge lists in O(1) without a linear trie scan. */
    int32_t *queue = (int32_t *)malloc(tb->count * 2 * sizeof(int32_t));
    int32_t  qh = 0, qt = 0;

    /* Seed: root's direct children (trie node indices) */
    {
        TrieNode *root = &tb->nodes[0];
        for (uint32_t i = 0; i < root->nedges; i++) {
            int32_t child_tn = root->edges[i].child;
            int32_t ci = tb->nodes[child_tn].dat_id;
            if (ci > 0 && ci < ncells) {
                a->cells[ci].fail = 1;
                queue[qt++] = child_tn;
            }
        }
    }

    while (qh < qt) {
        int32_t tn   = queue[qh++];   /* trie node index */
        int32_t curr = tb->nodes[tn].dat_id;  /* DAT cell index */

        /* Output for curr */
        if (a->cells[curr].token != AC_INVALID_TOKEN) {
            if (a->noutputs >= out_cap) {
                out_cap *= 2;
                a->outputs = (AcOutput *)realloc(a->outputs,
                                                  out_cap * sizeof(AcOutput));
            }
            /* Compute depth (pattern length) by walking check links to root */
            uint32_t depth = 0;
            for (int32_t s = curr; s != 1 && depth < 200000; depth++)
                s = a->cells[s].check;

            int32_t oidx = a->noutputs++;
            a->outputs[oidx].token        = a->cells[curr].token;
            a->outputs[oidx].start_offset = depth;
            a->outputs[oidx].next =
                a->cells[a->cells[curr].fail].output;
            a->cells[curr].output = oidx;
        } else {
            a->cells[curr].output = a->cells[a->cells[curr].fail].output;
        }

        /* Process children (use trie edges — no linear scan) */
        TrieNode *n = &tb->nodes[tn];
        for (uint32_t i = 0; i < n->nedges; i++) {
            uint8_t  c        = n->edges[i].byte;
            int32_t  child_tn = n->edges[i].child;
            int32_t  ci       = a->cells[curr].base + (int32_t)c;
            if (ci <= 0 || ci >= ncells || a->cells[ci].check != curr) continue;

            /* Walk failure chain to find fail for ci */
            int32_t fs = a->cells[curr].fail;
            for (;;) {
                int32_t nxt = a->cells[fs].base + (int32_t)c;
                if (nxt > 0 && nxt < ncells && a->cells[nxt].check == fs &&
                    nxt != ci) {
                    fs = nxt;
                    break;
                }
                if (fs == 1) break; /* root with no match */
                fs = a->cells[fs].fail;
            }
            a->cells[ci].fail = fs;
            queue[qt++] = child_tn;  /* enqueue trie node index */
        }
    }
    free(queue);
    return a;
}

AcAutomaton *ac_builder_build(AcBuilder *b) {
    TrieBuilder *tb  = (TrieBuilder *)b->priv;
    clock_t _p2a = clock();
    DatArray    *dat = build_dat(tb);
    clock_t _p2b = clock();
    fprintf(stderr, "  [AC] Phase2 (DAT): %.3fs  nodes=%d cells=%d\n",
            (double)(_p2b - _p2a) / CLOCKS_PER_SEC, tb->count, dat->cap);

    AcAutomaton *a = build_automaton_from_dat(tb, dat, tb->kind);
    clock_t _p3b = clock();
    fprintf(stderr, "  [AC] Phase3 (BFS): %.3fs\n",
            (double)(_p3b - _p2b) / CLOCKS_PER_SEC);
    free(dat->base);
    free(dat->check);
    free(dat);
    return a;
}

/* Build two automatons sharing the same trie and DAT (avoids duplicate
 * Phase 2 construction). kind1/kind2 may differ (output chain logic). */
void ac_builder_build_two(AcBuilder *b, AcKind kind1, AcKind kind2,
                           AcAutomaton **out1, AcAutomaton **out2) {
    TrieBuilder *tb  = (TrieBuilder *)b->priv;
    clock_t _p2a = clock();
    DatArray    *dat = build_dat(tb);
    clock_t _p2b = clock();
    fprintf(stderr, "  [AC] Phase2 (DAT): %.3fs  nodes=%d cells=%d (shared)\n",
            (double)(_p2b - _p2a) / CLOCKS_PER_SEC, tb->count, dat->cap);
    *out1 = build_automaton_from_dat(tb, dat, kind1);
    *out2 = build_automaton_from_dat(tb, dat, kind2);
    clock_t _p3b = clock();
    fprintf(stderr, "  [AC] Phase3x2 (BFS): %.3fs\n",
            (double)(_p3b - _p2b) / CLOCKS_PER_SEC);
    free(dat->base);
    free(dat->check);
    free(dat);
}

/* =========================================================================
 * Runtime step
 * ========================================================================= */

static inline int32_t ac_step(const AcAutomaton *a, int32_t state,
                               uint8_t byte) {
    int32_t c = (int32_t)(unsigned char)byte;
    for (;;) {
        int32_t base = a->cells[state].base;
        int32_t t    = base + c;
        if (t > 0 && t < a->ncells && a->cells[t].check == state) return t;
        if (state == 1) return 1;
        state = a->cells[state].fail;
    }
}

/* =========================================================================
 * Leftmost-longest search
 * ========================================================================= */

bool ac_leftmost_longest(const AcAutomaton *a, const uint8_t *text, size_t len,
                         uint32_t *out_token, size_t *out_end) {
    int32_t  state          = 1;
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
        /* Once state falls back to root after committing a match we can stop */
        if (best_token != AC_INVALID_TOKEN && state == 1) break;
    }

    if (best_token == AC_INVALID_TOKEN) return false;
    *out_token = best_token;
    *out_end   = best_end;
    return true;
}

/* =========================================================================
 * Overlapping iterator
 * ========================================================================= */

int32_t ac_start_state(void) { return 1; }

AcMatchIter ac_iter_new(const AcAutomaton *a) {
    return (AcMatchIter){ .automaton=a, .state=1, .pos=0,
                          .pending_out=0, .pending_end=0 };
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
