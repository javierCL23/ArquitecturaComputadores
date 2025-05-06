#!/bin/sh

nvcc -o raytracing_executable \
     main.cu \
     Crystalline.cu \
     Metallic.cu \
     Scene.cu \
     Sphere.cu \
     random.cu \
     raytracing.cu \
     utils.cu
echo Compilado

./raytracing_executable
