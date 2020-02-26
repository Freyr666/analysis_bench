#/bin/env bash

PREFIX=intel
#PREFIX=data

for size in $(seq 1 15); do
    for kind in cpu gpu; do
        echo "Running $kind $size"
        ./run.py $kind $size >> "$PREFIX/$kind$size"
    done;
done;

for size in 1 5 15; do
    ./plot_hist.py $PREFIX $size "$PREFIX/cpu$size" "$PREFIX/gpu$size"
done;

./plot_degrad.py $PREFIX 1 15
