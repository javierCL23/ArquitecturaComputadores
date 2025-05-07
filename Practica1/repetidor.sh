#!/bin/sh

if [ "$#" -ne 2 ] && [ "$#" -ne 3 ];
then
	echo "Sintax ERROR: repetidor.sh nRows nCols (nLoops)"
	exit 1
fi

#Configuracion nº de ejecuciones (por defecto 5)
if [ "$#" -eq 3 ]
then
	loops=$3
else
	echo gols
	loops=5
fi

#Ejecucion:
echo "Times for [$1 | $2]" >> tiempos.txt

for i in $(seq 1 $loops);
do

	g++ OwnPointGenerator.cpp -o OwnPointGenerator
	./OwnPointGenerator $1 $2 Datos
	./ejecutar.sh Practica1Final Datos.bin
done

rm "Datos.bin" "Datos.txt"

cat tiempos.txt
