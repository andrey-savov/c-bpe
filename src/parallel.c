/* parallel.c — Thin wrapper around the persistent BpePool (threadpool.c).
 *
 * Workers spin on an atomic epoch counter between calls so there is no
 * OS-scheduler round-trip penalty between consecutive encode_batch calls.
 * The pool is created once on first use via call_once.
 */
#include "c11_threads.h"  /* C11 threads (pthread shim on macOS) */
#include <stddef.h>
#include "tokenizer.h"
#include "threadpool.h"

/* ---- Global pool (lazy init) -------------------------------------------- */

static BpePool  *g_pool           = NULL;
static once_flag g_pool_init_flag = ONCE_FLAG_INIT;

static void init_pool(void) { g_pool = bpe_pool_create(0); }

static BpePool *get_pool(void) {
    call_once(&g_pool_init_flag, init_pool);
    return g_pool;
}

/* ---- Public API ---------------------------------------------------------- */

int parallel_num_threads(void) {
    return bpe_pool_nthreads(get_pool());
}

void parallel_encode_batch(const Tokenizer *tok,
                            const uint8_t **texts, const size_t *text_lens,
                            size_t n, uint32_t **out_tokens, size_t *out_ns) {
    BpePool *p = get_pool();
    if (p) {
        bpe_pool_encode_batch(p, tok, texts, text_lens, n, out_tokens, out_ns);
    } else {
        for (size_t i = 0; i < n; i++)
            out_tokens[i] = tokenizer_encode(tok, texts[i], text_lens[i],
                                             &out_ns[i]);
    }
}

void parallel_count_batch(const Tokenizer *tok,
                           const uint8_t **texts, const size_t *text_lens,
                           size_t n, size_t *out_counts) {
    BpePool *p = get_pool();
    if (p) {
        bpe_pool_count_batch(p, tok, texts, text_lens, n, out_counts);
    } else {
        for (size_t i = 0; i < n; i++)
            out_counts[i] = tokenizer_count(tok, texts[i], text_lens[i]);
    }
}

void parallel_count_till_limit_batch(const Tokenizer *tok,
                                      const uint8_t **texts,
                                      const size_t *text_lens,
                                      size_t n, size_t token_limit,
                                      size_t *out_counts) {
    BpePool *p = get_pool();
    if (p) {
        bpe_pool_count_till_limit_batch(p, tok, texts, text_lens, n,
                                        token_limit, out_counts);
    } else {
        for (size_t i = 0; i < n; i++)
            out_counts[i] = tokenizer_count_till_limit(tok, texts[i],
                                                       text_lens[i],
                                                       token_limit);
    }
}
