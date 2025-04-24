#!/bin/sh

if test "$#" -eq 0
then
	echo "ERROR: introduce Program name"
	return 1
fi

if test "$#" -eq 1
then
	c++ $1.cpp -o $1 -fopenmp && ./$1
	return 0
fi

mpic++ $1.cpp -o $1 -fopenmp && mpirun -np 4 ./$1 $2

#c++ $1.cpp -o $1 -fopenmp && ./$1 $2


