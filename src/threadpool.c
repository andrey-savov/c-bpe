/* threadpool.c — Work-queue thread pool for batch BPE operations.
 *
 * Workers block on a condition variable when the queue is empty and wake
 * only when items are pushed.  Each batch tracks completion via an atomic
 * counter + condvar so the caller blocks until every item is done.
 * Idle workers consume zero CPU.
 *
 * Portability: C11 <threads.h> + <stdatomic.h>.
 * Platform #ifdefs limited to hw_concurrency().
 */

/* ---- Platform: hardware-thread count ------------------------------------ */
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
static int hw_concurrency(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
}
#else
#  include <unistd.h>
static int hw_concurrency(void) {
    int n = (int)sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? n : 1;
}
#endif

#include "threadpool.h"
#include "tokenizer.h"
#include <threads.h>      /* C11: thrd_create, mtx_t, cnd_t */
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POOL_THREADS 256
#define QUEUE_CAPACITY   4096

/* ---- Work item types ---------------------------------------------------- */
typedef enum { WORK_ENCODE, WORK_COUNT, WORK_COUNT_LIMIT } WorkType;

/* ---- Batch completion tracker ------------------------------------------- */
typedef struct {
    atomic_int remaining;   /* items left to complete */
    mtx_t      mtx;
    cnd_t      cnd;         /* signalled when remaining hits 0 */
} BatchTracker;

/* ---- Single work item --------------------------------------------------- */
typedef struct {
    const Tokenizer *tok;
    const uint8_t   *text;
    size_t           text_len;
    WorkType         type;
    size_t           token_limit;
    /* Output — pointer directly into caller's output slot */
    uint32_t       **out_tokens_ptr;   /* WORK_ENCODE: &out_tokens[i] */
    size_t          *out_n_ptr;        /* WORK_ENCODE: &out_ns[i]     */
    size_t          *out_count_ptr;    /* WORK_COUNT*: &out_counts[i]  */
    BatchTracker    *tracker;
} WorkItem;

/* ---- Bounded ring-buffer work queue ------------------------------------- */
typedef struct {
    WorkItem items[QUEUE_CAPACITY];
    int      head, tail, count;
    mtx_t    mtx;
    cnd_t    not_empty;     /* workers wait here  */
    cnd_t    not_full;      /* producer waits here */
    int      shutdown;
} WorkQueue;

/* ---- Thread pool -------------------------------------------------------- */
struct BpePool {
    thrd_t    threads[MAX_POOL_THREADS];
    int       nworkers;
    WorkQueue queue;
};

/* ---- Execute one work item ---------------------------------------------- */
static void execute_work_item(const WorkItem *wi) {
    switch (wi->type) {
    case WORK_ENCODE:
        *wi->out_tokens_ptr = tokenizer_encode(
            wi->tok, wi->text, wi->text_len, wi->out_n_ptr);
        break;
    case WORK_COUNT:
        *wi->out_count_ptr = tokenizer_count(
            wi->tok, wi->text, wi->text_len);
        break;
    case WORK_COUNT_LIMIT:
        *wi->out_count_ptr = tokenizer_count_till_limit(
            wi->tok, wi->text, wi->text_len, wi->token_limit);
        break;
    }
}

/* ---- Worker thread ------------------------------------------------------ */
static int worker_fn(void *arg) {
    WorkQueue *q = (WorkQueue *)arg;

    for (;;) {
        mtx_lock(&q->mtx);
        while (q->count == 0 && !q->shutdown)
            cnd_wait(&q->not_empty, &q->mtx);

        if (q->shutdown && q->count == 0) {
            mtx_unlock(&q->mtx);
            break;
        }

        /* Dequeue one item */
        WorkItem wi = q->items[q->head];
        q->head = (q->head + 1) % QUEUE_CAPACITY;
        q->count--;
        cnd_signal(&q->not_full);
        mtx_unlock(&q->mtx);

        /* Execute outside the queue lock */
        execute_work_item(&wi);

        /* Signal batch completion if this was the last item */
        if (atomic_fetch_sub_explicit(&wi.tracker->remaining, 1,
                                      memory_order_acq_rel) == 1) {
            mtx_lock(&wi.tracker->mtx);
            cnd_signal(&wi.tracker->cnd);
            mtx_unlock(&wi.tracker->mtx);
        }
    }
    return 0;
}

/* ---- Enqueue one item (blocks if queue is full) ------------------------- */
static void queue_push(WorkQueue *q, const WorkItem *wi) {
    mtx_lock(&q->mtx);
    while (q->count == QUEUE_CAPACITY)
        cnd_wait(&q->not_full, &q->mtx);

    q->items[q->tail] = *wi;
    q->tail = (q->tail + 1) % QUEUE_CAPACITY;
    q->count++;
    cnd_signal(&q->not_empty);
    mtx_unlock(&q->mtx);
}

/* ---- Public API --------------------------------------------------------- */

BpePool *bpe_pool_create(int nthreads) {
    if (nthreads <= 0) nthreads = hw_concurrency();
    if (nthreads > MAX_POOL_THREADS) nthreads = MAX_POOL_THREADS;

    BpePool *pool = (BpePool *)calloc(1, sizeof(BpePool));
    if (!pool) return NULL;

    WorkQueue *q = &pool->queue;
    q->head = q->tail = q->count = 0;
    q->shutdown = 0;
    if (mtx_init(&q->mtx, mtx_plain) != thrd_success) { free(pool); return NULL; }
    if (cnd_init(&q->not_empty) != thrd_success) { mtx_destroy(&q->mtx); free(pool); return NULL; }
    if (cnd_init(&q->not_full)  != thrd_success) { cnd_destroy(&q->not_empty); mtx_destroy(&q->mtx); free(pool); return NULL; }

    pool->nworkers = 0;
    for (int i = 0; i < nthreads; i++) {
        if (thrd_create(&pool->threads[pool->nworkers], worker_fn, q)
                != thrd_success)
            break;
        pool->nworkers++;
    }
    return pool;
}

void bpe_pool_destroy(BpePool *pool) {
    if (!pool) return;
    WorkQueue *q = &pool->queue;

    mtx_lock(&q->mtx);
    q->shutdown = 1;
    cnd_broadcast(&q->not_empty);   /* wake all workers so they exit */
    mtx_unlock(&q->mtx);

    for (int i = 0; i < pool->nworkers; i++)
        thrd_join(pool->threads[i], NULL);

    cnd_destroy(&q->not_full);
    cnd_destroy(&q->not_empty);
    mtx_destroy(&q->mtx);
    free(pool);
}

int bpe_pool_nthreads(const BpePool *pool) {
    return pool ? pool->nworkers : 1;
}

/* ---- Internal: push a batch and wait for completion --------------------- */
static void pool_run(BpePool *pool,
                     const Tokenizer *tok,
                     const uint8_t **texts, const size_t *text_lens,
                     int n, WorkType type,
                     uint32_t **out_tokens, size_t *out_ns,
                     size_t *out_counts, size_t token_limit) {
    BatchTracker tracker;
    atomic_init(&tracker.remaining, n);
    mtx_init(&tracker.mtx, mtx_plain);
    cnd_init(&tracker.cnd);

    /* Push n work items onto the queue */
    for (int i = 0; i < n; i++) {
        WorkItem wi;
        wi.tok        = tok;
        wi.text       = texts[i];
        wi.text_len   = text_lens[i];
        wi.type       = type;
        wi.token_limit = token_limit;
        wi.tracker    = &tracker;

        if (type == WORK_ENCODE) {
            wi.out_tokens_ptr = &out_tokens[i];
            wi.out_n_ptr      = &out_ns[i];
            wi.out_count_ptr  = NULL;
        } else {
            wi.out_tokens_ptr = NULL;
            wi.out_n_ptr      = NULL;
            wi.out_count_ptr  = &out_counts[i];
        }

        queue_push(&pool->queue, &wi);
    }

    /* Wait for all items in this batch to complete */
    mtx_lock(&tracker.mtx);
    while (atomic_load_explicit(&tracker.remaining, memory_order_acquire) > 0)
        cnd_wait(&tracker.cnd, &tracker.mtx);
    mtx_unlock(&tracker.mtx);

    cnd_destroy(&tracker.cnd);
    mtx_destroy(&tracker.mtx);
}

/* ---- Public batch functions --------------------------------------------- */

void bpe_pool_encode_batch(BpePool *pool, const Tokenizer *tok,
                            const uint8_t **texts, const size_t *text_lens,
                            size_t n, uint32_t **out_tokens, size_t *out_ns) {
    pool_run(pool, tok, texts, text_lens, (int)n,
             WORK_ENCODE, out_tokens, out_ns, NULL, 0);
}

void bpe_pool_count_batch(BpePool *pool, const Tokenizer *tok,
                           const uint8_t **texts, const size_t *text_lens,
                           size_t n, size_t *out_counts) {
    pool_run(pool, tok, texts, text_lens, (int)n,
             WORK_COUNT, NULL, NULL, out_counts, 0);
}

void bpe_pool_count_till_limit_batch(BpePool *pool, const Tokenizer *tok,
                                      const uint8_t **texts,
                                      const size_t *text_lens,
                                      size_t n, size_t token_limit,
                                      size_t *out_counts) {
    pool_run(pool, tok, texts, text_lens, (int)n,
             WORK_COUNT_LIMIT, NULL, NULL, out_counts, token_limit);
}
