#!/bin/bash

for i in {91..93}
do
	./main false 0.$i 0.95 cltest_0-$i 1 1 10 0 1&
done

wait
