#!/bin/bash
# runs all the tests and saves output in results.txt
# both builds get the same sizes, tile and runs so the numbers can be compared directly

SIZES="512 1024 2048"
TILE=64
RUNS=3
OUT=results.txt

make || exit 1

{
    echo "==== machine details ===="
    echo "host      : $(hostname)"
    echo "cpu       : $(lscpu | grep 'Model name' | sed 's/.*: *//')"
    echo "cores     : $(lscpu | grep '^Core(s) per socket' | sed 's/.*: *//') cores, $(nproc) threads"
    echo "max freq  : $(lscpu | grep 'CPU max MHz' | sed 's/.*: *//') MHz"
    echo "L1d cache : $(lscpu | grep 'L1d' | sed 's/.*: *//')"
    echo "L2 cache  : $(lscpu | grep 'L2' | sed 's/.*: *//')"
    echo "L3 cache  : $(lscpu | grep 'L3' | sed 's/.*: *//')"
    echo "memory    : $(free -h | awk '/Mem:/ {print $2}')"
    echo "os        : $(grep PRETTY_NAME /etc/os-release | cut -d'"' -f2)"
    echo "kernel    : $(uname -r)"
    echo "compiler  : $(gcc --version | head -1)"
    echo
    echo "tile = $TILE, runs = $RUNS (best time is kept)"
    echo
} | tee $OUT

for prog in matmul matmul_noopt
do
    if [ $prog = matmul ]; then
        flags="-O2 -march=native"
    else
        flags="-O0"
    fi
    for n in $SIZES
    do
        echo "---- $prog ($flags), n = $n ----"
        ./$prog $n $TILE $RUNS
        echo
    done
done | tee -a $OUT
