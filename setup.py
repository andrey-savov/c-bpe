"""
Build script for c_bpe: C implementation of BPE tokenizer.

Optimization flags:
  GCC/Clang:  -O3 -march=native -flto -DNDEBUG
  MSVC:       /O2 /Ox /GL /DNDEBUG  (/O3 does not exist in MSVC)

PCRE2 UCD tables are bundled from third_party/ for Unicode property lookups.
OpenMP is auto-detected; if unavailable, parallel APIs fall back to sequential.
codegen/gen_dict.py runs before compilation to produce dict header files.
"""
import os
import sys
import subprocess
import tempfile
import shutil
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


HERE = Path(__file__).parent.resolve()
SRC = HERE / "src"
INCLUDE = HERE / "include"
CODEGEN = HERE / "codegen"
THIRD_PARTY = HERE / "third_party"
DATA = HERE / "data"


# ---------------------------------------------------------------------------
# Compiler detection helpers
# ---------------------------------------------------------------------------

def is_msvc(compiler):
    return hasattr(compiler, "compiler_type") and compiler.compiler_type == "msvc"


def test_compile(compiler, flags, code="#include <omp.h>\nint main(){return omp_get_max_threads();}"):
    """Return True if test compilation with `flags` succeeds."""
    with tempfile.TemporaryDirectory() as tmpdir:
        src = os.path.join(tmpdir, "test.c")
        obj = os.path.join(tmpdir, "test.o")
        with open(src, "w") as f:
            f.write(code)
        try:
            compiler.compile([src], output_dir=tmpdir, extra_postargs=flags)
            return True
        except Exception:
            return False


# ---------------------------------------------------------------------------
# Codegen: .tiktoken.gz  ->  src/dict_*.h
# ---------------------------------------------------------------------------

def run_codegen():
    gen_script = str(CODEGEN / "gen_dict.py")
    if not Path(gen_script).exists():
        raise FileNotFoundError(f"Codegen script not found: {gen_script}")

    print("c_bpe: running codegen (tiktoken -> C headers) ...")
    result = subprocess.run(
        [sys.executable, gen_script, "--data-dir", str(DATA), "--out-dir", str(SRC)],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr)
        raise RuntimeError("codegen failed")
    print("c_bpe: codegen complete.")


def copy_precomputed_blobs():
    """Copy committed .bin.gz blobs into the Python package directory.

    Decompression happens at runtime in Python (see c_bpe/openai.py).
    """
    pkg_dir = HERE / "python" / "c_bpe"
    copied = 0
    for name in ("precomputed_cl100k.bin.gz", "precomputed_o200k.bin.gz"):
        src = DATA / name
        dst = pkg_dir / name
        if src.exists():
            shutil.copy2(src, dst)
            copied += 1
    if copied:
        print(f"c_bpe: copied {copied} precomputed blob(s) into package directory.")
    else:
        print("c_bpe: no precomputed blobs found in data/, will use runtime init.")
    return copied > 0


# ---------------------------------------------------------------------------
# PCRE2 UCD tables (Unicode property data only — no regex engine)
# ---------------------------------------------------------------------------

PCRE2_NEEDED_SOURCES = [
    "pcre2_tables.c",
    "pcre2_ucd.c",
]

def get_pcre2_sources():
    pcre2_src = THIRD_PARTY / "pcre2" / "src"
    sources = []
    for name in PCRE2_NEEDED_SOURCES:
        p = pcre2_src / name
        if p.exists():
            sources.append("third_party/pcre2/src/" + name)
    if not sources:
        print(
            "WARNING: PCRE2 UCD sources not found in third_party/pcre2/src/\n"
            "         Run:  python codegen/fetch_pcre2.py  to download them."
        )
    return sources


# ---------------------------------------------------------------------------
# Custom build_ext to inject flags
# ---------------------------------------------------------------------------

class BpeBuildExt(build_ext):
    def build_extension(self, ext):
        # Run codegen first (idempotent)
        run_codegen()

        # Copy precomputed .bin.gz blobs into package dir
        copy_precomputed_blobs()

        compiler = self.compiler
        msvc = is_msvc(compiler)

        # ---- Optimisation flags ----
        if msvc:
            opt_flags = ["/O2", "/Ox", "/GL", "/DNDEBUG", "/fp:fast", "/std:c11", "/experimental:c11atomics"]
        else:
            opt_flags = ["-O3", "-DNDEBUG", "-ffast-math"]
            # -march=native may fail on cross-arch CI (e.g. Apple Silicon clang targeting x86)
            if test_compile(compiler, ["-march=native"], "int main(){return 0;}"):
                opt_flags.append("-march=native")
            # Try LTO
            if test_compile(compiler, ["-flto"], "int main(){return 0;}"):
                opt_flags.append("-flto")

        # ---- Parallel batch APIs use threadpool.c (persistent spin-waiting
        #      thread pool).  No external library required. ----

        # Inject flags into extension
        ext.extra_compile_args = opt_flags
        if not msvc:
            ext.extra_compile_args += ["-std=c11", "-Wall", "-Wextra", "-Wno-unused-parameter"]
        ext.extra_link_args = []
        if msvc:
            ext.extra_link_args += ["/LTCG"]

        super().build_extension(ext)


# ---------------------------------------------------------------------------
# C sources
# ---------------------------------------------------------------------------

BPE_SOURCES = [
    # fnv_hash.h, bitfield.h, lru_cache.h are header-only (no .c file)
    "src/ac_bpe.c",
    "src/bpe_core.c",
    "src/appendable_encoder.c",
    "src/prependable_encoder.c",
    "src/interval_encoding.c",
    "src/tokenizer.c",
    "src/pretok_cl100k.c",
    "src/pretok_o200k.c",
    "src/parallel.c",
    "src/threadpool.c",
    "src/pymodule.c",
]

PCRE2_SOURCES = get_pcre2_sources()

INCLUDE_DIRS = ["include", "src"]  # src for generated dict headers

PCRE2_MACROS = []
if PCRE2_SOURCES:
    PCRE2_MACROS = [
        ("PCRE2_CODE_UNIT_WIDTH", "8"),
        ("PCRE2_STATIC", None),
        ("HAVE_CONFIG_H", None),
    ]
    INCLUDE_DIRS.append("third_party/pcre2/src")
    INCLUDE_DIRS.append("third_party/pcre2")

ext = Extension(
    name="c_bpe.bpe",
    sources=BPE_SOURCES + PCRE2_SOURCES,
    include_dirs=INCLUDE_DIRS,
    define_macros=PCRE2_MACROS,
    language="c",
)

setup(
    ext_modules=[ext],
    package_dir={"": "python"},
    cmdclass={"build_ext": BpeBuildExt},
)
