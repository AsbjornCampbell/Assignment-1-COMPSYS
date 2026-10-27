## Reproducing the `fauxgrep` benchmarks

Run all commands from the `src` directory.

### Compile

make all

## Correctness
Confirm correctness by running following bash commands. If output is empty they are equal
./fhistogram benchmark_data > base.txt
./fhistogram-mt -n 4 benchmark_data > mt.txt
 tail -n 10 base.txt > base_final.txt
 tail -n 10 mt.txt > mt_final.txt
 diff -u base_final.txt mt_final.txt

### Generate the benchmark datasets
The benchmarks use three datasets. Each dataset contains a total of 512 MiB.

- Small
2048 files of 0.25 MiB each:

python3 generate_testdata.py --output data_small --files 2048 --mib-per-file 0.25 --line-bytes 256 --needle THREAD_NEEDLE_42 --match-every 500 --force

#### Balanced
256 files of 2 MiB each:


python3 generate_testdata.py --output data_balanced --files 256 --mib-per-file 2 --line-bytes 256 --needle THREAD_NEEDLE_42 --match-every 500 --force


#### Large
16 files of 32 MiB each:

python3 generate_testdata.py --output data_large --files 16 --mib-per-file 32 --line-bytes 256 --needle THREAD_NEEDLE_42 --match-every 500 --force


### Run the benchmarks
For the small dataset:

/usr/bin/time -p ./fauxgrep THREAD_NEEDLE_42 data_small > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 1 THREAD_NEEDLE_42 data_small > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 2 THREAD_NEEDLE_42 data_small > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 4 THREAD_NEEDLE_42 data_small > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 8 THREAD_NEEDLE_42 data_small > /dev/null

For the balanced dataset:

/usr/bin/time -p ./fauxgrep THREAD_NEEDLE_42 data_balanced > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 1 THREAD_NEEDLE_42 data_balanced > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 2 THREAD_NEEDLE_42 data_balanced > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 4 THREAD_NEEDLE_42 data_balanced > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 8 THREAD_NEEDLE_42 data_balanced > /dev/null


For the large dataset:

/usr/bin/time -p ./fauxgrep THREAD_NEEDLE_42 data_large > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 1 THREAD_NEEDLE_42 data_large > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 2 THREAD_NEEDLE_42 data_large > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 4 THREAD_NEEDLE_42 data_large > /dev/null
/usr/bin/time -p ./fauxgrep-mt -n 8 THREAD_NEEDLE_42 data_large > /dev/null