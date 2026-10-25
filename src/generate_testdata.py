#!/usr/bin/env python3
"""
Generate deterministic text datasets for A1 Task 2 (fauxgrep) and Task 3 (fhistogram).

Examples:
  python3 generate_testdata.py --output data_report --preset report --match-every 500
  python3 generate_testdata.py --output data_nomatch --preset report --match-every 0
  python3 generate_testdata.py --output data_dense --preset report --match-every 10

The generated files are ordinary text files, so the SAME dataset can be used by:
  ./fauxgrep ...
  ./fauxgrep-mt ...
  ./fhistogram ...
  ./fhistogram-mt ...
"""

import argparse
from pathlib import Path
import shutil

DEFAULT_NEEDLE = "THREAD_NEEDLE_42"

PRESETS = {
    # Useful for checking correctness quickly.
    "quick":  {"files": 32,  "mib_per_file": 0.25},
    # About 512 MiB total. Good starting point for a report benchmark.
    "report": {"files": 256, "mib_per_file": 2},
    # About 1 GiB total if your machine is too fast on the report preset.
    "large":  {"files": 256, "mib_per_file": 4},
}

def make_line(file_index: int, line_index: int, line_bytes: int, needle: str, is_match: bool) -> bytes:
    # Vary the payload deterministically while keeping lines fixed-width.
    alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    shift = (file_index * 17 + line_index * 31) % len(alphabet)
    rotated = alphabet[shift:] + alphabet[:shift]

    prefix = f"file={file_index:04d} line={line_index:08d} "
    marker = f"{needle} " if is_match else "ordinary_data "
    body_seed = prefix + marker + rotated + " "

    # Reserve one byte for '\n'.
    target = line_bytes - 1
    repeats = (target // len(body_seed)) + 2
    text = (body_seed * repeats)[:target]
    return text.encode("ascii") + b"\n"

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--output", default="benchmark_data")
    p.add_argument("--preset", choices=PRESETS, default="report")
    p.add_argument("--files", type=int, default=None,
                   help="Override number of files.")
    p.add_argument("--mib-per-file", type=float, default=None,
                   help="Override approximate MiB per file.")
    p.add_argument("--line-bytes", type=int, default=256)
    p.add_argument("--needle", default=DEFAULT_NEEDLE)
    p.add_argument("--match-every", type=int, default=500,
                   help="Put needle in every Nth line. 0 means no matches.")
    p.add_argument("--force", action="store_true",
                   help="Delete output directory first if it already exists.")
    args = p.parse_args()

    if args.line_bytes < 64:
        p.error("--line-bytes must be at least 64")
    if args.match_every < 0:
        p.error("--match-every must be >= 0")

    cfg = PRESETS[args.preset]
    file_count = args.files if args.files is not None else cfg["files"]
    mib_per_file = args.mib_per_file if args.mib_per_file is not None else cfg["mib_per_file"]

    out = Path(args.output)
    if out.exists():
        if not args.force:
            raise SystemExit(f"{out} already exists. Use --force to replace it.")
        shutil.rmtree(out)
    out.mkdir(parents=True)

    bytes_per_file_target = int(mib_per_file * 1024 * 1024)
    lines_per_file = max(1, bytes_per_file_target // args.line_bytes)
    actual_bytes_per_file = lines_per_file * args.line_bytes

    total_matches = 0

    for fi in range(file_count):
        path = out / f"data_{fi:04d}.txt"
        with path.open("wb") as f:
            for li in range(lines_per_file):
                is_match = args.match_every > 0 and ((li + 1) % args.match_every == 0)
                if is_match:
                    total_matches += 1
                f.write(make_line(fi, li + 1, args.line_bytes, args.needle, is_match))

    total_bytes = file_count * actual_bytes_per_file
    metadata = f"""A1 benchmark dataset
====================
files={file_count}
bytes_per_file={actual_bytes_per_file}
total_bytes={total_bytes}
total_mib={total_bytes / 1024 / 1024:.2f}
line_bytes={args.line_bytes}
lines_per_file={lines_per_file}
needle={args.needle}
match_every={args.match_every}
expected_matches={total_matches}

Suggested correctness check:
  ./fauxgrep {args.needle} {out} | sort > single.txt
  ./fauxgrep-mt -n 4 {args.needle} {out} | sort > multi.txt
  diff single.txt multi.txt

Suggested timing:
  /usr/bin/time -p ./fauxgrep {args.needle} {out} > /dev/null
  /usr/bin/time -p ./fauxgrep-mt -n 1 {args.needle} {out} > /dev/null
  /usr/bin/time -p ./fauxgrep-mt -n 2 {args.needle} {out} > /dev/null
  /usr/bin/time -p ./fauxgrep-mt -n 4 {args.needle} {out} > /dev/null
  /usr/bin/time -p ./fauxgrep-mt -n 8 {args.needle} {out} > /dev/null

Histogram timing:
  /usr/bin/time -p ./fhistogram {out} > /dev/null
  /usr/bin/time -p ./fhistogram-mt -n 1 {out} > /dev/null
  /usr/bin/time -p ./fhistogram-mt -n 2 {out} > /dev/null
  /usr/bin/time -p ./fhistogram-mt -n 4 {out} > /dev/null
  /usr/bin/time -p ./fhistogram-mt -n 8 {out} > /dev/null
"""
    (out / "METADATA.txt").write_text(metadata)

    print(metadata)

if __name__ == "__main__":
    main()
