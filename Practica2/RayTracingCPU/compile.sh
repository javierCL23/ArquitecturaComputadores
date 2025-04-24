#!/bin/sh

g++ main_basico.cpp random.cpp Scene.cpp utils.cpp Sphere.cpp Metallic.cpp Crystalline.cpp -o programa_basico -O2 && echo Compilado &&  ./programa_basico && open imgCompleta.bmp
