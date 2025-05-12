#!/bin/sh

mpic++ main.cpp random.cpp Scene.cpp utils.cpp Sphere.cpp Metallic.cpp Crystalline.cpp -fopenmp -o programa -O2 && echo Compilado &&  mpirun -np 4 ./programa
