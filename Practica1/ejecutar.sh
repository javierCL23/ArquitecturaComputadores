#!/bin/bash

# Añadir las rutas directamente aquí para evitar problemas con entorno
CXX=mpic++
CXXFLAGS="-I/usr/lib/x86_64-linux-gnu/openmpi/include"

if [ "$#" -eq 0 ]; then
    echo "ERROR: introduce Program name"
    exit 1
fi

program="$1"
input="$2"

# Compilar con OpenMP y MPI (usando flags definidos arriba)
$CXX $CXXFLAGS "$program.cpp" -o "$program" -fopenmp -O2

if [ $? -ne 0 ]; then
    echo "ERROR: Fallo en la compilación"
    exit 1
fi

# Ejecutar con o sin mpirun dependiendo del número de argumentos
if [ -z "$input" ]; then
    ./"$program"
else
    mpirun -np 4 ./"$program" "$input"
fi
