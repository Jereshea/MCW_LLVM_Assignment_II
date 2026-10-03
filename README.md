# Matrix multiplication - loop optimizations

C = A * B for square n x n matrices of doubles (row major). I wrote the same
multiplication in 5 ways and timed each one to see how much loop interchange,
loop tiling and loop unrolling help.

1. naive - normal i, j, k order. The inner loop goes down a column of B
   (stride n), so it keeps missing the cache.
2. interchange - swapped the j and k loops (i, k, j order). Now the inner loop
   goes along a row of B and a row of C, so memory is read in order.
3. tiling - same i, k, j order but done in blocks of size bs x bs, so the
   parts of A, B and C being used stay in cache.
4. unrolling - i, k, j order with the inner j loop unrolled by 4, plus a
   leftover loop when n is not a multiple of 4.
5. all combined - tiling + interchange + the k loop unrolled by 4.

## Files

- `matmul.c` - all 5 versions and the timing
- `Makefile` - builds two programs from the same `matmul.c`:
  - `matmul` with `-O2 -march=native`
  - `matmul_noopt` with `-O0` (no compiler optimization, so only my own loop
    changes make a difference)
- `run_benchmarks.sh` - prints the machine details, runs both programs with the
  same n, tile size and runs, and saves everything to `results.txt`

## How to run

```
make
./matmul                     # default n = 1024, tile = 64, runs = 3
./matmul 2048 64 3           # ./matmul [n] [tile] [runs]
./matmul_noopt 2048 64 3     # same arguments, no compiler optimization
./run_benchmarks.sh          # full run, output goes to results.txt
make clean
```

Each version is run `runs` times and the best time is kept. The full run takes
around 10 minutes, mostly because of the naive version at n = 2048.

## Machine

- CPU: 12th Gen Intel Core i5-1235U (10 cores, 12 threads, max 4.4 GHz)
- Cache: L1d 352 KiB, L2 6.5 MiB, L3 12 MiB
- RAM: 15 GiB
- OS: Ubuntu 22.04.5 LTS, kernel 6.8.0-138-generic
- Compiler: gcc 12.3.0

## Results

tile = 64, runs = 3 for both builds.

### matmul (-O2 -march=native)

Time in seconds (speedup over naive in brackets):

| version      | n = 512        | n = 1024       | n = 2048        |
|--------------|----------------|----------------|-----------------|
| naive        | 0.2531         | 2.6513         | 78.7570         |
| interchange  | 0.0386 (6.56)  | 0.3671 (7.22)  | 4.8578 (16.21)  |
| tiling       | 0.0390 (6.49)  | 0.3231 (8.21)  | 3.8332 (20.55)  |
| unrolling    | 0.0350 (7.23)  | 0.3347 (7.92)  | 4.7117 (16.72)  |
| all combined | 0.0249 (10.16) | 0.1950 (13.60) | 2.3521 (33.48)  |

### matmul_noopt (-O0)

| version      | n = 512       | n = 1024      | n = 2048       |
|--------------|---------------|---------------|----------------|
| naive        | 0.3740        | 3.9040        | 62.3487        |
| interchange  | 0.3204 (1.17) | 2.5207 (1.55) | 22.6912 (2.75) |
| tiling       | 0.3561 (1.05) | 2.5132 (1.55) | 22.2991 (2.80) |
| unrolling    | 0.2788 (1.34) | 2.2096 (1.77) | 20.1169 (3.10) |
| all combined | 0.1565 (2.39) | 1.2233 (3.19) | 10.6979 (5.83) |

## What I noticed

- Loop interchange gives the biggest jump on its own. With -O2 it is 6.5 to 16
  times faster, because B is now read row by row and gcc can vectorize the
  inner loop.
- The naive version gets much worse as n grows. At n = 2048 each matrix is
  32 MiB, bigger than the 12 MiB L3, so going down a column of B misses the
  cache almost every time.
- At n = 2048 the naive version takes about the same time, or even longer,
  with -O2 than with -O0 (79 s vs 62 s). It is limited by memory, so the
  compiler can't fix it. Only changing the loop order helps.
- Tiling does not help much at n = 512 because everything already fits in
  cache. At n = 2048 it is better than plain interchange (20.6x vs 16.2x).
- Unrolling helps more with -O0. With -O2 gcc already unrolls and vectorizes
  the loop, so doing it by hand adds less.
- Using all three together is the fastest every time: 33x with -O2 and about
  6x with -O0 at n = 2048.

