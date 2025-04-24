#!/bin/sh

g++ main.cpp random.cpp Scene.cpp utils.cpp Sphere.cpp Metallic.cpp Crystalline.cpp -fopenmp -o programa -O2 && echo Compilado &&  ./programa && xdg-open imgCompleta.bmp
