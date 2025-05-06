#!/bin/sh

# Comprobar si el archivo existe
if [ $# -ne 1 ]; then
	echo "Usage: sacarMuestras.sh 'fileName'"
	exit 1
fi

if [ ! -f "$1" ]; then
    echo "El archivo no existe"
    exit 1
fi

# Comprobar si existe el fichero tiempos.txt
if [ -f "tiempos.txt" ]; then
    rm tiempos.txt
fi

# Leer el archivo línea por línea
while IFS="	" read -r var1 var2 var3; do
	echo "VARIABLES: $var1 $var2 $var3"
	./repetidor.sh $var1 $var2 $var3 </dev/null
done < $1
