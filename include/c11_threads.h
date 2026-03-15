/* c11_threads.h — Portable C11 threads shim.
 *
 * macOS does not ship <threads.h>.  On Apple platforms we provide a thin
 * pthread-based implementation of the subset used by threadpool.c / parallel.c.
 * Everywhere else we just forward to the real <threads.h>.
 */
#ifndef C11_THREADS_H
#define C11_THREADS_H

#if defined(__APPLE__)

#include <pthread.h>
#include <stdlib.h>

/* ---- Return codes ---- */
enum { thrd_success = 0, thrd_error = 1 };

/* ---- Mutex ---- */
enum { mtx_plain = 0 };
typedef pthread_mutex_t mtx_t;

static inline int mtx_init(mtx_t *m, int type) {
    (void)type;
    return pthread_mutex_init(m, NULL) == 0 ? thrd_success : thrd_error;
}
static inline int  mtx_lock(mtx_t *m)    { return pthread_mutex_lock(m)    == 0 ? thrd_success : thrd_error; }
static inline int  mtx_unlock(mtx_t *m)  { return pthread_mutex_unlock(m)  == 0 ? thrd_success : thrd_error; }
static inline void mtx_destroy(mtx_t *m) { pthread_mutex_destroy(m); }

/* ---- Condition variable ---- */
typedef pthread_cond_t cnd_t;

static inline int  cnd_init(cnd_t *c)                  { return pthread_cond_init(c, NULL) == 0 ? thrd_success : thrd_error; }
static inline int  cnd_signal(cnd_t *c)                { return pthread_cond_signal(c)     == 0 ? thrd_success : thrd_error; }
static inline int  cnd_broadcast(cnd_t *c)             { return pthread_cond_broadcast(c)  == 0 ? thrd_success : thrd_error; }
static inline int  cnd_wait(cnd_t *c, mtx_t *m)        { return pthread_cond_wait(c, m)    == 0 ? thrd_success : thrd_error; }
static inline void cnd_destroy(cnd_t *c)               { pthread_cond_destroy(c); }

/* ---- Thread ---- */
typedef pthread_t thrd_t;
typedef int (*thrd_start_t)(void *);

/* pthreads wants void*(*)(void*), C11 wants int(*)(void*).  Trampoline. */
struct _thrd_wrapper_arg { thrd_start_t fn; void *arg; };

static void *_thrd_wrapper(void *a) {
    struct _thrd_wrapper_arg w = *(struct _thrd_wrapper_arg *)a;
    free(a);
    w.fn(w.arg);
    return NULL;
}

static inline int thrd_create(thrd_t *t, thrd_start_t fn, void *arg) {
    struct _thrd_wrapper_arg *w = (struct _thrd_wrapper_arg *)malloc(sizeof(*w));
    if (!w) return thrd_error;
    w->fn = fn;
    w->arg = arg;
    return pthread_create(t, NULL, _thrd_wrapper, w) == 0 ? thrd_success : thrd_error;
}

static inline int thrd_join(thrd_t t, int *res) {
    (void)res;
    return pthread_join(t, NULL) == 0 ? thrd_success : thrd_error;
}

/* ---- once_flag ---- */
typedef pthread_once_t once_flag;
#define ONCE_FLAG_INIT PTHREAD_ONCE_INIT
#define call_once(flag, fn) pthread_once(flag, fn)

#else
/* Non-Apple: use the real C11 <threads.h> */
#  include <threads.h>
#endif

#endif /* C11_THREADS_H */
