/* pymodule.c — CPython extension for c_bpe.
 *
 * Module hierarchy (matches rs_bpe):
 *   c_bpe.bpe          <- this extension module
 *   c_bpe.bpe.openai   <- submodule defined below
 *
 * Types exposed:
 *   BytePairEncoding   (raw BPE without pretokenisation)
 *   Tokenizer          (BPE + native pretokeniser)
 *   ParallelOptions
 *
 * The generated dict headers are included here.
 */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* Portable monotonic timestamp (seconds) */
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
static double monotonic_secs(void) {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / (double)freq.QuadPart;
}
#else
static double monotonic_secs(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}
#endif

#include "bpe.h"
#include "tokenizer.h"
#include "pretok.h"

/* Generated at build time by codegen/gen_dict.py */
#include "dict_cl100k.h"
#include "dict_o200k.h"

/* ---------------------------------------------------------------------------
 * Load precomputed binary blob from disk.
 * Returns malloc'd buffer on success (caller must free), NULL on failure.
 * Sets *out_len to the file size.
 * --------------------------------------------------------------------------- */
static unsigned char *load_blob_file(const char *filename, size_t *out_len) {
    /* Get this module's __file__ to find the package directory */
    PyObject *mod = PyImport_ImportModule("c_bpe");
    if (!mod) { PyErr_Clear(); return NULL; }
    PyObject *file_attr = PyObject_GetAttrString(mod, "__file__");
    Py_DECREF(mod);
    if (!file_attr) { PyErr_Clear(); return NULL; }

    const char *init_path = PyUnicode_AsUTF8(file_attr);
    if (!init_path) { Py_DECREF(file_attr); PyErr_Clear(); return NULL; }

    /* Build path: dirname(__file__) / filename */
    char path[4096];
    size_t plen = strlen(init_path);
    /* Find last separator */
    size_t sep = plen;
    while (sep > 0 && init_path[sep - 1] != '/' && init_path[sep - 1] != '\\') sep--;
    if (sep + strlen(filename) + 1 >= sizeof(path)) {
        Py_DECREF(file_attr);
        return NULL;
    }
    memcpy(path, init_path, sep);
    strcpy(path + sep, filename);
    Py_DECREF(file_attr);

    /* Read the file */
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    if (fsize <= 0) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);

    unsigned char *buf = (unsigned char *)malloc((size_t)fsize);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)fsize, f);
    fclose(f);
    if (rd != (size_t)fsize) { free(buf); return NULL; }

    *out_len = (size_t)fsize;
    return buf;
}

/* Parallel batch functions from parallel.c */
extern int  parallel_num_threads(void);
extern void parallel_encode_batch(const Tokenizer *tok,
                                   const uint8_t **texts, const size_t *text_lens,
                                   size_t n, uint32_t **out_tokens, size_t *out_ns);
extern void parallel_count_batch(const Tokenizer *tok,
                                  const uint8_t **texts, const size_t *text_lens,
                                  size_t n, size_t *out_counts);

/* =========================================================================
 * Global singleton tokenizers (initialised once, never freed)
 * ========================================================================= */

static Tokenizer *g_cl100k = NULL;
static Tokenizer *g_o200k  = NULL;

/* cl100k pretokenizer patterns (from bpe-openai/src/lib.rs) */
static const char *CL100K_PATTERNS[] = {
    "(?i:'s|'t|'re|'ve|'m|'ll|'d)|[^\\r\\n\\p{L}\\p{N}]?\\p{L}+|\\p{N}{1,3}"
    "| ?[^\\s\\p{L}\\p{N}]+[\\r\\n]*|\\s*[\\r\\n]+|\\s+$",
    "\\s+\\s",
    "\\s+"
};
static const bool CL100K_LOOKAHEADS[] = { false, true, false };

/* o200k pretokenizer patterns */
static const char *O200K_PATTERNS[] = {
    "[^\\r\\n\\p{L}\\p{N}]?[\\p{Lu}\\p{Lt}\\p{Lm}\\p{Lo}\\p{M}]*"
    "[\\p{Ll}\\p{Lm}\\p{Lo}\\p{M}]+(?i:'s|'t|'re|'ve|'m|'ll|'d)?"
    "|[^\\r\\n\\p{L}\\p{N}]?[\\p{Lu}\\p{Lt}\\p{Lm}\\p{Lo}\\p{M}]+"
    "[\\p{Ll}\\p{Lm}\\p{Lo}\\p{M}]*(?i:'s|'t|'re|'ve|'m|'ll|'d)?"
    "|\\p{N}{1,3}| ?[^\\s\\p{L}\\p{N}]+[\\r\\n/]*|\\s*[\\r\\n]+|\\s+$",
    "\\s+\\s",
    "\\s+"
};
static const bool O200K_LOOKAHEADS[] = { false, true, false };

static Tokenizer *build_cl100k(void) {
    size_t blob_len = 0;
    unsigned char *blob = load_blob_file("precomputed_cl100k.bin", &blob_len);
    BytePairEncoding *bpe;
    if (blob) {
        bpe = bpe_from_blob(CL100K_ALL_TOKENS, CL100K_TOKEN_STARTS,
                            blob, blob_len);
        free(blob);
    } else {
        bpe = bpe_from_dictionary(CL100K_ALL_TOKENS, CL100K_TOKEN_STARTS,
                                  CL100K_NUM_TOKENS, CL100K_HASH_FACTOR);
    }
    Tokenizer *tok = tokenizer_new_native(bpe, pretok_cl100k);
    return tok;
}

static Tokenizer *build_o200k(void) {
    size_t blob_len = 0;
    unsigned char *blob = load_blob_file("precomputed_o200k.bin", &blob_len);
    BytePairEncoding *bpe;
    if (blob) {
        bpe = bpe_from_blob(O200K_ALL_TOKENS, O200K_TOKEN_STARTS,
                            blob, blob_len);
        free(blob);
    } else {
        bpe = bpe_from_dictionary(O200K_ALL_TOKENS, O200K_TOKEN_STARTS,
                                  O200K_NUM_TOKENS, O200K_HASH_FACTOR);
    }
    Tokenizer *tok = tokenizer_new_native(bpe, pretok_o200k);
    return tok;
}

/* =========================================================================
 * Utility: convert uint32_t[] to Python list of ints
 * ========================================================================= */

static PyObject *tokens_to_pylist(const uint32_t *toks, size_t n) {
    PyObject *lst = PyList_New((Py_ssize_t)n);
    if (!lst) return NULL;
    for (size_t i = 0; i < n; i++)
        PyList_SET_ITEM(lst, (Py_ssize_t)i, PyLong_FromUnsignedLong(toks[i]));
    return lst;
}

static int pylist_to_tokens(PyObject *lst, uint32_t **out, size_t *out_n) {
    if (!PyList_Check(lst)) {
        PyErr_SetString(PyExc_TypeError, "expected list of ints");
        return -1;
    }
    Py_ssize_t n = PyList_GET_SIZE(lst);
    uint32_t *arr = (uint32_t *)malloc((size_t)n * sizeof(uint32_t));
    if (!arr) { PyErr_NoMemory(); return -1; }
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = PyList_GET_ITEM(lst, i);
        long v = PyLong_AsLong(item);
        if (v < 0) { free(arr); return -1; }
        arr[i] = (uint32_t)v;
    }
    *out   = arr;
    *out_n = (size_t)n;
    return 0;
}

/* =========================================================================
 * ParallelOptions type
 * ========================================================================= */

typedef struct {
    PyObject_HEAD
    size_t min_batch_size;
    size_t chunk_size;
    size_t max_threads;
} PyParallelOptions;

static int PyParOpts_init(PyParallelOptions *self, PyObject *args, PyObject *kw) {
    self->min_batch_size = 20;
    self->chunk_size     = 100;
    self->max_threads    = 0;
    static char *kwlist[] = { "min_batch_size", "chunk_size", "max_threads", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|nnn", kwlist,
                                     &self->min_batch_size,
                                     &self->chunk_size,
                                     &self->max_threads))
        return -1;
    return 0;
}

static PyObject *PyParOpts_get_mbs(PyParallelOptions *s, void *_) {
    return PyLong_FromSize_t(s->min_batch_size);
}
static PyObject *PyParOpts_get_cs(PyParallelOptions *s, void *_) {
    return PyLong_FromSize_t(s->chunk_size);
}
static PyObject *PyParOpts_get_mt(PyParallelOptions *s, void *_) {
    return PyLong_FromSize_t(s->max_threads);
}

static PyGetSetDef PyParOpts_getset[] = {
    { "min_batch_size", (getter)PyParOpts_get_mbs, NULL, NULL, NULL },
    { "chunk_size",     (getter)PyParOpts_get_cs,  NULL, NULL, NULL },
    { "max_threads",    (getter)PyParOpts_get_mt,  NULL, NULL, NULL },
    { NULL }
};

static PyTypeObject PyParallelOptionsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "c_bpe.bpe.openai.ParallelOptions",
    .tp_basicsize = sizeof(PyParallelOptions),
    .tp_init      = (initproc)PyParOpts_init,
    .tp_new       = PyType_GenericNew,
    .tp_getset    = PyParOpts_getset,
    .tp_flags     = Py_TPFLAGS_DEFAULT,
};

/* =========================================================================
 * BytePairEncoding type
 * ========================================================================= */

typedef struct {
    PyObject_HEAD
    BytePairEncoding *bpe;
    int               owned; /* 1 = we own it (must bpe_free) */
} PyBPE;

static void PyBPE_dealloc(PyBPE *self) {
    if (self->owned && self->bpe) bpe_free(self->bpe);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyBPE_count(PyBPE *self, PyObject *args) {
    Py_buffer view;
    if (!PyArg_ParseTuple(args, "y*", &view)) return NULL;
    size_t c = bpe_count(self->bpe, (const uint8_t*)view.buf, (size_t)view.len);
    PyBuffer_Release(&view);
    return PyLong_FromSize_t(c);
}

static PyObject *PyBPE_encode_via_backtracking(PyBPE *self, PyObject *args) {
    Py_buffer view;
    if (!PyArg_ParseTuple(args, "y*", &view)) return NULL;
    size_t    n;
    uint32_t *toks = bpe_encode_via_backtracking(self->bpe,
                                                  (const uint8_t*)view.buf,
                                                  (size_t)view.len, &n);
    PyBuffer_Release(&view);
    PyObject *lst = tokens_to_pylist(toks, n);
    free(toks);
    return lst;
}

static PyObject *PyBPE_decode_tokens(PyBPE *self, PyObject *args) {
    PyObject *lst;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &lst)) return NULL;
    uint32_t *toks;
    size_t    n;
    if (pylist_to_tokens(lst, &toks, &n) < 0) return NULL;
    size_t   out_len;
    uint8_t *buf = bpe_decode_tokens(self->bpe, toks, n, &out_len);
    free(toks);
    PyObject *result = PyBytes_FromStringAndSize((char*)buf, (Py_ssize_t)out_len);
    free(buf);
    return result;
}

static PyMethodDef PyBPE_methods[] = {
    { "count",                    (PyCFunction)PyBPE_count,                    METH_VARARGS, NULL },
    { "encode_via_backtracking",  (PyCFunction)PyBPE_encode_via_backtracking,  METH_VARARGS, NULL },
    { "decode_tokens",            (PyCFunction)PyBPE_decode_tokens,            METH_VARARGS, NULL },
    { NULL }
};

static PyTypeObject PyBPEType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "c_bpe.bpe.BytePairEncoding",
    .tp_basicsize = sizeof(PyBPE),
    .tp_dealloc   = (destructor)PyBPE_dealloc,
    .tp_methods   = PyBPE_methods,
    .tp_new       = PyType_GenericNew,
    .tp_flags     = Py_TPFLAGS_DEFAULT,
};

/* =========================================================================
 * Tokenizer type
 * ========================================================================= */

typedef struct {
    PyObject_HEAD
    Tokenizer *tok;
    int        owned;
} PyTokenizer;

static void PyTokenizer_dealloc(PyTokenizer *self) {
    if (self->owned && self->tok) {
        /* owned=1 only for user-constructed tokenizers (not singletons) */
        tokenizer_free(self->tok);
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyTokenizer_count(PyTokenizer *self, PyObject *args) {
    const char *text;
    Py_ssize_t  text_len;
    if (!PyArg_ParseTuple(args, "s#", &text, &text_len)) return NULL;
    size_t c = tokenizer_count(self->tok,
                                (const uint8_t*)text, (size_t)text_len);
    return PyLong_FromSize_t(c);
}

static PyObject *PyTokenizer_count_till_limit(PyTokenizer *self, PyObject *args) {
    const char *text;
    Py_ssize_t  text_len;
    size_t      limit;
    if (!PyArg_ParseTuple(args, "s#n", &text, &text_len, &limit)) return NULL;
    size_t c = tokenizer_count_till_limit(self->tok,
                                           (const uint8_t*)text,
                                           (size_t)text_len, limit);
    if (c == SIZE_MAX) Py_RETURN_NONE;
    return PyLong_FromSize_t(c);
}

static PyObject *PyTokenizer_encode(PyTokenizer *self, PyObject *args) {
    const char *text;
    Py_ssize_t  text_len;
    if (!PyArg_ParseTuple(args, "s#", &text, &text_len)) return NULL;
    size_t    n;
    uint32_t *toks = tokenizer_encode(self->tok, (const uint8_t*)text,
                                       (size_t)text_len, &n);
    PyObject *lst = tokens_to_pylist(toks, n);
    free(toks);
    return lst;
}

static PyObject *PyTokenizer_encode_batch(PyTokenizer *self, PyObject *args) {
    PyObject *lst;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &lst)) return NULL;

    Py_ssize_t   nbatch = PyList_GET_SIZE(lst);
    const uint8_t **texts     = (const uint8_t **)malloc((size_t)nbatch * sizeof(void*));
    size_t        *text_lens  = (size_t *)malloc((size_t)nbatch * sizeof(size_t));
    uint32_t     **out_toks   = (uint32_t **)calloc((size_t)nbatch, sizeof(void*));
    size_t        *out_ns     = (size_t *)calloc((size_t)nbatch, sizeof(size_t));

    /* Resolve Python str objects to UTF-8 buffers (borrow; valid for GIL hold) */
    for (Py_ssize_t i = 0; i < nbatch; i++) {
        PyObject *item = PyList_GET_ITEM(lst, i);
        const char *buf = PyUnicode_AsUTF8AndSize(item, (Py_ssize_t*)&text_lens[i]);
        if (!buf) {
            free(texts); free(text_lens); free(out_toks); free(out_ns);
            return NULL;
        }
        texts[i] = (const uint8_t*)buf;
    }

    /* Sequential encode — matches rs_bpe's encode_batch behaviour */
    double t0 = monotonic_secs();
    for (Py_ssize_t i = 0; i < nbatch; i++) {
        out_toks[i] = tokenizer_encode(self->tok, texts[i], text_lens[i],
                                       &out_ns[i]);
    }
    double elapsed = monotonic_secs() - t0;

    /* Build result */
    PyObject *result_list = PyList_New(nbatch);
    size_t total = 0;
    for (Py_ssize_t i = 0; i < nbatch; i++) {
        PyObject *sub = tokens_to_pylist(out_toks[i], out_ns[i]);
        free(out_toks[i]);
        total += out_ns[i];
        PyList_SET_ITEM(result_list, i, sub);
    }
    free(texts); free(text_lens); free(out_toks); free(out_ns);

    return Py_BuildValue("(Ond)", result_list, (Py_ssize_t)total, elapsed);
}

static PyObject *PyTokenizer_encode_batch_parallel(PyTokenizer *self, PyObject *args) {
    /* Same as encode_batch but also returns thread_count */
    PyObject *lst;
    PyObject *opts_obj = Py_None;
    if (!PyArg_ParseTuple(args, "O!|O", &PyList_Type, &lst, &opts_obj)) return NULL;

    Py_ssize_t   nbatch = PyList_GET_SIZE(lst);
    const uint8_t **texts    = (const uint8_t **)malloc((size_t)nbatch * sizeof(void*));
    size_t        *text_lens = (size_t *)malloc((size_t)nbatch * sizeof(size_t));
    uint32_t     **out_toks  = (uint32_t **)calloc((size_t)nbatch, sizeof(void*));
    size_t        *out_ns    = (size_t *)calloc((size_t)nbatch, sizeof(size_t));

    for (Py_ssize_t i = 0; i < nbatch; i++) {
        PyObject *item = PyList_GET_ITEM(lst, i);
        const char *buf = PyUnicode_AsUTF8AndSize(item, (Py_ssize_t*)&text_lens[i]);
        if (!buf) {
            free(texts); free(text_lens); free(out_toks); free(out_ns);
            return NULL;
        }
        texts[i] = (const uint8_t*)buf;
    }

    double t0 = monotonic_secs();
    Py_BEGIN_ALLOW_THREADS
    parallel_encode_batch(self->tok, texts, text_lens, (size_t)nbatch,
                          out_toks, out_ns);
    Py_END_ALLOW_THREADS
    double elapsed = monotonic_secs() - t0;

    PyObject *result_list = PyList_New(nbatch);
    size_t total = 0;
    for (Py_ssize_t i = 0; i < nbatch; i++) {
        PyObject *sub = tokens_to_pylist(out_toks[i], out_ns[i]);
        free(out_toks[i]);
        total += out_ns[i];
        PyList_SET_ITEM(result_list, i, sub);
    }
    free(texts); free(text_lens); free(out_toks); free(out_ns);

    return Py_BuildValue("(Ondl)", result_list, (Py_ssize_t)total, elapsed,
                         (long)parallel_num_threads());
}

static PyObject *PyTokenizer_decode(PyTokenizer *self, PyObject *args) {
    PyObject *lst;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &lst)) return NULL;
    uint32_t *toks;
    size_t    n;
    if (pylist_to_tokens(lst, &toks, &n) < 0) return NULL;
    size_t   out_len;
    uint8_t *buf = bpe_decode_tokens(self->tok->bpe, toks, n, &out_len);
    free(toks);
    if (!buf) Py_RETURN_NONE;
    /* Attempt UTF-8 decode */
    PyObject *result = PyUnicode_DecodeUTF8((char*)buf, (Py_ssize_t)out_len,
                                             "strict");
    free(buf);
    if (!result) {
        PyErr_Clear();
        Py_RETURN_NONE;
    }
    return result;
}

static PyObject *PyTokenizer_decode_batch(PyTokenizer *self, PyObject *args) {
    PyObject *batch;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &batch)) return NULL;
    Py_ssize_t m = PyList_GET_SIZE(batch);
    PyObject  *result = PyList_New(m);
    for (Py_ssize_t i = 0; i < m; i++) {
        PyObject *sub = PyList_GET_ITEM(batch, i);
        uint32_t *toks;
        size_t    n;
        if (pylist_to_tokens(sub, &toks, &n) < 0) {
            Py_DECREF(result);
            return NULL;
        }
        size_t   out_len;
        uint8_t *buf = bpe_decode_tokens(self->tok->bpe, toks, n, &out_len);
        free(toks);
        PyObject *s;
        if (!buf) {
            s = Py_NewRef(Py_None);
        } else {
            s = PyUnicode_DecodeUTF8((char*)buf, (Py_ssize_t)out_len,
                                      "strict");
            free(buf);
            if (!s) { PyErr_Clear(); s = Py_NewRef(Py_None); }
        }
        PyList_SET_ITEM(result, i, s);
    }
    return result;
}

static PyObject *PyTokenizer_decode_batch_parallel(PyTokenizer *self,
                                                    PyObject *args) {
    /* Identical to decode_batch; parallel optimisation omitted for brevity */
    PyObject *batch;
    PyObject *opts = Py_None;
    if (!PyArg_ParseTuple(args, "O!|O", &PyList_Type, &batch, &opts)) return NULL;
    PyObject *fwd_args = Py_BuildValue("(O)", batch);
    PyObject *result   = PyTokenizer_decode_batch(self, fwd_args);
    Py_DECREF(fwd_args);
    return result;
}

static PyObject *PyTokenizer_bpe(PyTokenizer *self, PyObject *_args) {
    PyBPE *obj = PyObject_New(PyBPE, &PyBPEType);
    if (!obj) return NULL;
    obj->bpe   = self->tok->bpe;
    obj->owned = 0;
    return (PyObject*)obj;
}

static PyMethodDef PyTokenizer_methods[] = {
    { "count",                   (PyCFunction)PyTokenizer_count,                   METH_VARARGS, NULL },
    { "count_till_limit",        (PyCFunction)PyTokenizer_count_till_limit,        METH_VARARGS, NULL },
    { "encode",                  (PyCFunction)PyTokenizer_encode,                  METH_VARARGS, NULL },
    { "encode_batch",            (PyCFunction)PyTokenizer_encode_batch,            METH_VARARGS, NULL },
    { "encode_batch_parallel",   (PyCFunction)PyTokenizer_encode_batch_parallel,   METH_VARARGS, NULL },
    { "decode",                  (PyCFunction)PyTokenizer_decode,                  METH_VARARGS, NULL },
    { "decode_batch",            (PyCFunction)PyTokenizer_decode_batch,            METH_VARARGS, NULL },
    { "decode_batch_parallel",   (PyCFunction)PyTokenizer_decode_batch_parallel,   METH_VARARGS, NULL },
    { "bpe",                     (PyCFunction)PyTokenizer_bpe,                     METH_NOARGS,  NULL },
    { NULL }
};

static PyTypeObject PyTokenizerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "c_bpe.bpe.openai.Tokenizer",
    .tp_basicsize = sizeof(PyTokenizer),
    .tp_dealloc   = (destructor)PyTokenizer_dealloc,
    .tp_methods   = PyTokenizer_methods,
    .tp_new       = PyType_GenericNew,
    .tp_flags     = Py_TPFLAGS_DEFAULT,
};

/* =========================================================================
 * openai submodule functions
 * ========================================================================= */

static PyObject *py_cl100k_base(PyObject *_mod, PyObject *_args) {
    if (!g_cl100k) {
        g_cl100k = build_cl100k();
        if (!g_cl100k) return NULL;
    }
    PyTokenizer *obj = PyObject_New(PyTokenizer, &PyTokenizerType);
    if (!obj) return NULL;
    obj->tok   = g_cl100k;
    obj->owned = 0;
    return (PyObject*)obj;
}

static PyObject *py_o200k_base(PyObject *_mod, PyObject *_args) {
    if (!g_o200k) {
        g_o200k = build_o200k();
        if (!g_o200k) return NULL;
    }
    PyTokenizer *obj = PyObject_New(PyTokenizer, &PyTokenizerType);
    if (!obj) return NULL;
    obj->tok   = g_o200k;
    obj->owned = 0;
    return (PyObject*)obj;
}

static PyObject *py_is_cached_cl100k(PyObject *_m, PyObject *_a) {
    return PyBool_FromLong(g_cl100k != NULL);
}

static PyObject *py_is_cached_o200k(PyObject *_m, PyObject *_a) {
    return PyBool_FromLong(g_o200k != NULL);
}

static PyObject *py_get_num_threads(PyObject *_m, PyObject *_a) {
    return PyLong_FromLong(parallel_num_threads());
}

static PyMethodDef openai_methods[] = {
    { "cl100k_base",      py_cl100k_base,      METH_NOARGS,  NULL },
    { "o200k_base",       py_o200k_base,       METH_NOARGS,  NULL },
    { "is_cached_cl100k", py_is_cached_cl100k, METH_NOARGS,  NULL },
    { "is_cached_o200k",  py_is_cached_o200k,  METH_NOARGS,  NULL },
    { "get_num_threads",  py_get_num_threads,  METH_NOARGS,  NULL },
    { NULL }
};

static struct PyModuleDef openai_moduledef = {
    PyModuleDef_HEAD_INIT, "c_bpe.bpe.openai", NULL, -1, openai_methods
};

/* =========================================================================
 * bpe module (top-level extension)
 * ========================================================================= */

static PyMethodDef bpe_methods[] = { { NULL } };

static struct PyModuleDef bpe_moduledef = {
    PyModuleDef_HEAD_INIT, "bpe", NULL, -1, bpe_methods
};

PyMODINIT_FUNC PyInit_bpe(void) {
    /* Register types */
    if (PyType_Ready(&PyBPEType) < 0)           return NULL;
    if (PyType_Ready(&PyTokenizerType) < 0)     return NULL;
    if (PyType_Ready(&PyParallelOptionsType) < 0) return NULL;

    /* Create openai submodule */
    PyObject *openai_mod = PyModule_Create(&openai_moduledef);
    if (!openai_mod) return NULL;
    Py_INCREF(&PyTokenizerType);
    PyModule_AddObject(openai_mod, "Tokenizer", (PyObject*)&PyTokenizerType);
    Py_INCREF(&PyParallelOptionsType);
    PyModule_AddObject(openai_mod, "ParallelOptions",
                       (PyObject*)&PyParallelOptionsType);

    /* Create bpe module */
    PyObject *bpe_mod = PyModule_Create(&bpe_moduledef);
    if (!bpe_mod) { Py_DECREF(openai_mod); return NULL; }

    Py_INCREF(&PyBPEType);
    PyModule_AddObject(bpe_mod, "BytePairEncoding", (PyObject*)&PyBPEType);
    Py_INCREF(openai_mod);
    PyModule_AddObject(bpe_mod, "openai", openai_mod);

    /* Register submodule so "from c_bpe.bpe.openai import ..." works */
    PyObject *sys_modules = PyImport_GetModuleDict();
    PyDict_SetItemString(sys_modules, "c_bpe.bpe.openai", openai_mod);
    Py_DECREF(openai_mod);

    return bpe_mod;
}
