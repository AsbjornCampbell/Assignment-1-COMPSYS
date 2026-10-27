#!/usr/bin/env python3

import argparse
from pathlib import Path
import shutil

DEFAULT_NEEDLE = "THREAD_NEEDLE_42"

PRESETS = {
    "quick": {"files": 32, "mib_per_file": 0.25},
    "report": {"files": 256, "mib_per_file": 2},
    "large": {"files": 256, "mib_per_file": 4},
}


def make_line(file_index: int, line_index: int, line_bytes: int, needle: str, is_match: bool) -> bytes:
    alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    shift = (file_index * 17 + line_index * 31) % len(alphabet)
    rotated = alphabet[shift:] + alphabet[:shift]

    prefix = f"file={file_index:04d} line={line_index:08d} "
    marker = f"{needle} " if is_match else "ordinary_data "
    body_seed = prefix + marker + rotated + " "

    target = line_bytes - 1
    repeats = (target // len(body_seed)) + 2
    text = (body_seed * repeats)[:target]

    return text.encode("ascii") + b"\n"


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--output", default="benchmark_data")
    p.add_argument("--preset", choices=PRESETS, default="report")
    p.add_argument("--files", type=int, default=None)
    p.add_argument("--mib-per-file", type=float, default=None)
    p.add_argument("--line-bytes", type=int, default=256)
    p.add_argument("--needle", default=DEFAULT_NEEDLE)
    p.add_argument("--match-every", type=int, default=500)
    p.add_argument("--force", action="store_true")
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

    for fi in range(file_count):
        path = out / f"data_{fi:04d}.txt"

        with path.open("wb") as f:
            for li in range(lines_per_file):
                is_match = args.match_every > 0 and ((li + 1) % args.match_every == 0)
                f.write(make_line(fi, li + 1, args.line_bytes, args.needle, is_match))


if __name__ == "__main__":
    main()