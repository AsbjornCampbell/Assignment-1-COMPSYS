A1 benchmark test data
======================

This pack contains:

- generate_testdata.py
  Creates deterministic text data suitable for BOTH fauxgrep and fhistogram.


Recommended benchmark workflow
------------------------------


1. Generate a larger dataset:

   python3 generate_testdata.py --output data_report --preset report --match-every 500

   This creates about 512 MiB split across 256 files.

2. For a "no printing" search workload:

   python3 generate_testdata.py --output data_nomatch --preset report --match-every 0

3. For a print-heavy workload:

   python3 generate_testdata.py --output data_dense --preset report --match-every 10

Needle used by default:
   THREAD_NEEDLE_42

