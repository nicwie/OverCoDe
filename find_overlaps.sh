#!/bin/bash

# rm -f sum_results/result_50_25_12
# touch sum_results/result_50_25_12

for i in {1..50}
do
	for j in {1..25}
	do
		for k in {1..12}
		do
			make run FILENAME="test_auto_$START_VAL" GRAPHS_RUNS="3 3" OVERLAPS="$i $j $k"
			python3 count_error.py clusters/test_auto 3 3 5000 $i $j $k \-m | tee -a sum_results/result_50_25_12
		done
	done
done
