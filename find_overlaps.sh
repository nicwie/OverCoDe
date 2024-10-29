#!/bin/bash

rm -f sum_results/result
touch sum_results/result

for i in {1..12}
do
	for j in {1..25}
	do
		for k in {1..50}
		do
			make run ARGS="$k $j $i"
			python3 count_error.py clusters/test_auto 5 5 5000 $k $j $i \-m >> sum_results/result
			"\n\n" >> sum_results/result
		done
	done
done
