/* ac_bpe.h — Byte-level Double Array Aho-Corasick automaton for BPE.
 *
 * Adapted from the ac_dat project (https://github.com/Izolex/ac_dat, MIT license)
 * with the following simplifications / changes:
 *   - Byte alphabet (0-255) instead of Unicode code points
 *   - No tail compression (simplifies correctness for BPE)
 *   - uint32_t token IDs stored at accepting states
 *   - Three automaton kinds: LeftmostLongest, OverlapFwd, OverlapRev
 *   - Iterator-style incremental search API (mirrors daachorse)
 *
 * Build flow:
 *   AcBuilder *b = ac_builder_new(kind);
 *   for each token:  ac_builder_add(b, bytes, len, token_id);
 *   AcAutomaton *a = ac_builder_build(b);
 *   ac_builder_free(b);
 *
 * Leftmost-longest search:
 *   uint32_t token;
 *   size_t   end;
 *   if (ac_leftmost_longest(a, text, len, &token, &end)) { ... }
 *
 * Overlapping incremental search (forward):
 *   AcMatchIter it = ac_iter_new(a);
 *   for (size_t i = 0; i < len; i++) {
 *       ac_iter_advance(&it, text[i], i + 1);
 *       size_t start; uint32_t tok;
 *       while (ac_iter_next_match(&it, &tok, &start)) { ... }
 *   }
 *
 * The reverse automaton is built on byte-reversed patterns and
 * processed with the same iterator API.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------
 * Public constants
 * ------------------------------------------------------------------------- */
#define AC_INVALID_TOKEN UINT32_MAX

/* -------------------------------------------------------------------------
 * Automaton kinds
 * ------------------------------------------------------------------------- */
typedef enum {
    AC_KIND_LEFTMOST_LONGEST, /* leftmost-first, longest match at each pos    */
    AC_KIND_OVERLAPPING_FWD,  /* all overlapping matches, forward byte order   */
    AC_KIND_OVERLAPPING_REV,  /* all overlapping matches, reverse byte order   */
} AcKind;

/* -------------------------------------------------------------------------
 * Internal automaton node
 *   fail:   AC failure link (node index)
 *   output: chain of output-match node indices (0 = none)
 *   token:  token_id if this state is an accepting state; AC_INVALID_TOKEN otherwise
 *   depth:  distance from root (= pattern length for accepting nodes)
 * ------------------------------------------------------------------------- */
typedef struct {
    int32_t  fail;
    int32_t  output;   /* index of output-chain head, 0 = end */
    uint32_t token;    /* accepting token id (AC_INVALID_TOKEN if not accepting) */
    uint32_t depth;    /* distance from root */
} AcCell;

/* -------------------------------------------------------------------------
 * Output chain node: stored in a separate array so multiple states can
 * share output information via the 'output' field in AcCell.
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t token;
    uint32_t start_offset; /* byte offset from match end (= pattern length) */
    int32_t  next;         /* index of next node in chain, 0 = end          */
} AcOutput;

/* -------------------------------------------------------------------------
 * Automaton — uses Double-Array edge storage for O(1) transitions.
 *   Transition from state s on byte c:
 *     t = da_base[s] + c;  valid iff da_check[t] == s.
 * ------------------------------------------------------------------------- */
typedef struct AcAutomaton {
    AcCell  *cells;
    int32_t  ncells;       /* number of DA slots allocated */
    AcOutput *outputs;
    int32_t  noutputs;
    AcKind   kind;
    /* Double-array edge storage */
    int32_t  *da_base;     /* da_base[state] + byte → candidate next state */
    int32_t  *da_check;    /* da_check[t] == state confirms the transition */
    int32_t   da_size;     /* total DA array size                          */
} AcAutomaton;

/* -------------------------------------------------------------------------
 * Iterator state for incremental overlapping search
 * ------------------------------------------------------------------------- */
typedef struct {
    const AcAutomaton *automaton;
    int32_t  state;         /* current automaton state */
    size_t   pos;           /* byte position in text (next byte to process) */
    /* Pending matches to yield (from output chain traversal) */
    int32_t  pending_out;   /* index into outputs[] chain, 0 = none */
    size_t   pending_end;   /* byte position where pending matches end */
} AcMatchIter;

/* -------------------------------------------------------------------------
 * Builder
 * ------------------------------------------------------------------------- */
typedef struct AcBuilder AcBuilder;

AcBuilder    *ac_builder_new(AcKind kind);
void          ac_builder_add(AcBuilder *b, const uint8_t *bytes, size_t len, uint32_t token_id);
AcAutomaton  *ac_builder_build(AcBuilder *b);
void          ac_builder_build_two(AcBuilder *b, AcKind kind1, AcKind kind2,
                                   AcAutomaton **out1, AcAutomaton **out2);
void          ac_builder_free(AcBuilder *b);

/* -------------------------------------------------------------------------
 * Automaton lifecycle
 * ------------------------------------------------------------------------- */
void ac_automaton_free(AcAutomaton *a);

/* -------------------------------------------------------------------------
 * Leftmost-longest search: find the leftmost-first longest match in [text, text+len).
 * Returns true on match, sets *out_token and *out_end (exclusive, relative to text).
 * ------------------------------------------------------------------------- */
bool ac_leftmost_longest(const AcAutomaton *a, const uint8_t *text, size_t len,
                         uint32_t *out_token, size_t *out_end);

/* -------------------------------------------------------------------------
 * Overlapping iterator API
 * ------------------------------------------------------------------------- */
AcMatchIter ac_iter_new(const AcAutomaton *a);

/* Advance by one byte and set up pending matches ending at `end_pos`. */
void ac_iter_advance(AcMatchIter *it, uint8_t byte, size_t end_pos);

/* Yield the next pending match.
 * Sets *out_token and *out_start (match start position, relative to original text).
 * Returns true while matches remain; false when the current position has no more. */
bool ac_iter_next_match(AcMatchIter *it, uint32_t *out_token, size_t *out_start);

/* -------------------------------------------------------------------------
 * Root state index (0 in our trie-based implementation)
 * ------------------------------------------------------------------------- */
int32_t ac_start_state(void);
