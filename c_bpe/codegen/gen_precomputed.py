#!/usr/bin/env python3
"""
codegen/gen_precomputed.py

Two-mode script for precomputed BPE data:

  generate  — Compile and run gen_precomputed.c to produce gzip-compressed
              binary blobs (data/precomputed_*.bin.gz).  Requires a C compiler.
              Run manually by developers; the .bin.gz files are committed.

  emit      — Decompress committed .bin.gz blobs into raw .bin files that
              are installed as package data and loaded from disk at runtime.
              No compiler needed.
              Called automatically by setup.py at build time.

See README_codegen.md for details.
"""
import argparse
import gzip
import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).parent.resolve()
PROJECT = HERE.parent

MODELS = [
    ("cl100k", "precomputed_cl100k"),
    ("o200k",  "precomputed_o200k"),
]



def find_compiler():
    """Return (compiler_cmd, is_msvc) for the platform."""
    if sys.platform == "win32":
        # Try cl.exe (MSVC)
        try:
            r = subprocess.run(["cl"], capture_output=True)
            return "cl", True
        except FileNotFoundError:
            pass
    # GCC / Clang
    for cc in ("cc", "gcc", "clang"):
        try:
            subprocess.run([cc, "--version"], capture_output=True)
            return cc, False
        except FileNotFoundError:
            continue
    raise RuntimeError("No C compiler found")


def compile_serializer(src_dir: Path, out_exe: Path):
    """Compile gen_precomputed.c into an executable."""
    cc, is_msvc = find_compiler()

    c_src = HERE / "gen_precomputed.c"
    bpe_sources = [
        src_dir / "ac_bpe.c",
        src_dir / "bpe_core.c",
    ]
    include_dirs = [PROJECT / "include", src_dir]

    if is_msvc:
        cmd = [
            cc, "/nologo", "/O2", "/std:c11", "/experimental:c11atomics",
            "/DNDEBUG",
            str(c_src),
        ]
        for s in bpe_sources:
            cmd.append(str(s))
        for d in include_dirs:
            cmd.extend(["/I", str(d)])
        cmd.extend(["/Fe:" + str(out_exe), "/link"])
    else:
        cmd = [
            cc, "-O2", "-std=c11", "-DNDEBUG",
            str(c_src),
        ]
        for s in bpe_sources:
            cmd.append(str(s))
        for d in include_dirs:
            cmd.extend(["-I", str(d)])
        cmd.extend(["-o", str(out_exe), "-lm"])

    print(f"  compiling: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        raise RuntimeError("Compilation failed")
    # Clean up .obj files on MSVC
    for f in Path(".").glob("*.obj"):
        f.unlink(missing_ok=True)


def run_serializer(exe: Path, model: str) -> bytes:
    """Run the serializer and return the binary blob."""
    print(f"  running: {exe} {model}")
    result = subprocess.run([str(exe), model], capture_output=True)
    if result.returncode != 0:
        print(result.stderr.decode("utf-8", errors="replace"), file=sys.stderr)
        raise RuntimeError(f"Serializer failed for {model}")
    print(f"  stderr: {result.stderr.decode('utf-8', errors='replace').strip()}")
    return result.stdout





def cmd_generate(args):
    """Compile gen_precomputed.c, run it, write .bin.gz blobs."""
    src_dir = Path(args.src_dir).resolve()
    data_dir = Path(args.data_dir).resolve()
    data_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmpdir:
        exe_name = "gen_precomputed.exe" if sys.platform == "win32" else "gen_precomputed"
        exe_path = Path(tmpdir) / exe_name

        print("Compiling precomputed data serializer...")
        compile_serializer(src_dir, exe_path)

        for model, stem in MODELS:
            print(f"Generating {stem}.bin.gz ...")
            blob = run_serializer(exe_path, model)
            out_path = data_dir / f"{stem}.bin.gz"
            with gzip.open(out_path, "wb", compresslevel=9) as f:
                f.write(blob)
            raw_mb = len(blob) / (1024 * 1024)
            gz_mb = out_path.stat().st_size / (1024 * 1024)
            print(f"  {stem}: {raw_mb:.1f} MB raw → {gz_mb:.1f} MB gzipped")

    print("Done. Commit the .bin.gz files in data/.")


def cmd_emit(args):
    """Decompress .bin.gz blobs into raw .bin files for runtime loading."""
    data_dir = Path(args.data_dir).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    for model, stem in MODELS:
        gz_path = data_dir / f"{stem}.bin.gz"
        if not gz_path.exists():
            print(f"  {gz_path} not found — skipping")
            return False

        out_path = out_dir / f"{stem}.bin"
        print(f"Decompressing {gz_path.name} → {out_path.name} ...")
        with gzip.open(gz_path, "rb") as f:
            blob = f.read()
        out_path.write_bytes(blob)
        print(f"  {out_path.name}: {len(blob) / (1024*1024):.1f} MB")

    print("Precomputed blob decompression done.")


def main():
    parser = argparse.ArgumentParser(
        description="Generate / emit precomputed BPE C headers")
    sub = parser.add_subparsers(dest="command", required=True)

    p_gen = sub.add_parser("generate",
        help="Compile+run C serializer → write .bin.gz blobs (requires C compiler)")
    p_gen.add_argument("--src-dir", required=True,
        help="Directory containing BPE .c source files and dict headers")
    p_gen.add_argument("--data-dir", required=True,
        help="Directory to write .bin.gz blobs into")

    p_emit = sub.add_parser("emit",
        help="Decompress .bin.gz blobs → .bin files for runtime loading")
    p_emit.add_argument("--data-dir", required=True,
        help="Directory containing .bin.gz blobs")
    p_emit.add_argument("--out-dir", required=True,
        help="Directory to write .bin files into (typically python/c_bpe/)")

    args = parser.parse_args()
    if args.command == "generate":
        cmd_generate(args)
    else:
        success = cmd_emit(args)
        sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
