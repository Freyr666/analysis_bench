#/bin/env bash

for size in $(seq 1 15); do
    for kind in cpu gpu; do
        echo "Running $kind $size"
        ./run.py $kind $size >> "data/$kind$size"
    done;
done;

for size in 1 5 15; do
    ./plot_hist.py $size "data/cpu$size" "data/gpu$size"
done;

./plot_degrad.py cpu 1 15
./plot_degrad.py gpu 1 15
