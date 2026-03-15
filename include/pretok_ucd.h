/* pretok_ucd.h — Unicode property helpers using UCD tables.
 *
 * Self-contained header that provides Unicode category lookups (Letter,
 * Number, Mark, whitespace) using the UCD stage-1/stage-2 tables from
 * pcre2_ucd.c / pcre2_tables.c.
 */
#ifndef PRETOK_UCD_H
#define PRETOK_UCD_H

#include <stdint.h>
#include <stdbool.h>

/* ---- PCRE2 UCD definitions (extracted, width=8) ---- */

/* General character categories */
enum {
    ucp_C, ucp_L, ucp_M, ucp_N, ucp_P, ucp_S, ucp_Z
};

/* Particular character categories */
enum {
    ucp_Cc, ucp_Cf, ucp_Cn, ucp_Co, ucp_Cs,
    ucp_Ll, ucp_Lm, ucp_Lo, ucp_Lt, ucp_Lu,
    ucp_Mc, ucp_Me, ucp_Mn,
    ucp_Nd, ucp_Nl, ucp_No,
    ucp_Pc, ucp_Pd, ucp_Pe, ucp_Pf, ucp_Pi, ucp_Po, ucp_Ps,
    ucp_Sc, ucp_Sk, ucp_Sm, ucp_So,
    ucp_Zl, ucp_Zp, ucp_Zs
};

/* UCD record — matches PCRE2 10.45 layout exactly */
typedef struct {
    uint8_t  script;
    uint8_t  chartype;   /* particular category (ucp_Cc, ucp_Lu, etc.) */
    uint8_t  gbprop;
    uint8_t  caseset;
    int32_t  other_case;
    uint16_t scriptx_bidiclass;
    uint16_t bprops;
} ucd_record;

/* Two-level lookup: stage1[cp/128]*128 + cp%128 → index into ucd_records */
#define UCD_BLOCK_SIZE 128

/* Extern tables from pcre2_ucd.c / pcre2_tables.c (width-suffixed to _8) */
extern const ucd_record _pcre2_ucd_records_8[];
extern const uint16_t   _pcre2_ucd_stage1_8[];
extern const uint16_t   _pcre2_ucd_stage2_8[];
extern const uint32_t   _pcre2_ucp_gentype_8[];  /* chartype → general cat */

#define GET_UCD(ch) (&_pcre2_ucd_records_8[ \
    _pcre2_ucd_stage2_8[_pcre2_ucd_stage1_8[(int)(ch) / UCD_BLOCK_SIZE] * \
    UCD_BLOCK_SIZE + (int)(ch) % UCD_BLOCK_SIZE]])

/* ---- General category checks ---- */

static inline bool ucd_is_letter(uint32_t cp) {
    return _pcre2_ucp_gentype_8[GET_UCD(cp)->chartype] == ucp_L;
}

static inline bool ucd_is_number(uint32_t cp) {
    return _pcre2_ucp_gentype_8[GET_UCD(cp)->chartype] == ucp_N;
}

static inline bool ucd_is_mark(uint32_t cp) {
    return _pcre2_ucp_gentype_8[GET_UCD(cp)->chartype] == ucp_M;
}

/* ---- Subcategory checks (for o200k) ---- */

static inline uint8_t ucd_chartype(uint32_t cp) {
    return GET_UCD(cp)->chartype;
}

static inline bool ucd_is_Lu(uint32_t cp) { return ucd_chartype(cp) == ucp_Lu; }
static inline bool ucd_is_Ll(uint32_t cp) { return ucd_chartype(cp) == ucp_Ll; }
static inline bool ucd_is_Lt(uint32_t cp) { return ucd_chartype(cp) == ucp_Lt; }
static inline bool ucd_is_Lm(uint32_t cp) { return ucd_chartype(cp) == ucp_Lm; }
static inline bool ucd_is_Lo(uint32_t cp) { return ucd_chartype(cp) == ucp_Lo; }

/* Is codepoint in [Lu Lt Lm Lo M]? (uppercase-class for o200k) */
static inline bool ucd_is_upper_class(uint32_t cp) {
    uint8_t ct = ucd_chartype(cp);
    return ct == ucp_Lu || ct == ucp_Lt || ct == ucp_Lm || ct == ucp_Lo ||
           _pcre2_ucp_gentype_8[ct] == ucp_M;
}

/* Is codepoint in [Ll Lm Lo M]? (lowercase-class for o200k) */
static inline bool ucd_is_lower_class(uint32_t cp) {
    uint8_t ct = ucd_chartype(cp);
    return ct == ucp_Ll || ct == ucp_Lm || ct == ucp_Lo ||
           _pcre2_ucp_gentype_8[ct] == ucp_M;
}

/* ---- Whitespace check (\s in PCRE2 = Perl space) ----
 * Matches: ASCII whitespace + Unicode horizontal/vertical spaces.
 * PCRE2's \s (PT_SPACE) checks specific codepoints plus category Z. */

static inline bool ucd_is_whitespace(uint32_t cp) {
    /* Fast path: ASCII */
    if (cp <= 0x7F) {
        return cp == ' ' || cp == '\t' || cp == '\n' || cp == '\v' ||
               cp == '\f' || cp == '\r';
    }
    /* NEL */
    if (cp == 0x85) return true;
    /* NBSP */
    if (cp == 0xA0) return true;
    /* Horizontal spaces */
    if (cp == 0x1680 || (cp >= 0x2000 && cp <= 0x200A) ||
        cp == 0x180E || cp == 0x202F || cp == 0x205F || cp == 0x3000)
        return true;
    /* Vertical spaces (line/paragraph separator) */
    if (cp == 0x2028 || cp == 0x2029) return true;
    return false;
}

/* Is \r or \n? */
static inline bool ucd_is_newline(uint32_t cp) {
    return cp == '\r' || cp == '\n';
}

/* Is \r, \n, or /? (for o200k) */
static inline bool ucd_is_newline_or_slash(uint32_t cp) {
    return cp == '\r' || cp == '\n' || cp == '/';
}

#endif /* PRETOK_UCD_H */
