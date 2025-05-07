#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iomanip>  // Para controlar la precisión

float randomFloat(float min, float max) {
    return min + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void generarDatosPorClusters(const std::string& archivoBin, const std::string& archivoTxt, int filas, int columnas) {
    std::ofstream binario(archivoBin, std::ios::binary);
    std::ofstream texto(archivoTxt);

    if (!binario || !texto) {
        std::cerr << "Error al abrir los archivos de salida.\n";
        return;
    }

    // Escribimos número de filas y columnas en el binario
    binario.write(reinterpret_cast<const char*>(&filas), sizeof(int));
    binario.write(reinterpret_cast<const char*>(&columnas), sizeof(int));

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    const int numClusters = 4;
    std::vector<std::vector<float>> clusters(numClusters, std::vector<float>(columnas));

    // Generar centros de clúster
    for (int c = 0; c < numClusters; ++c) {
        for (int j = 0; j < columnas; ++j) {
            clusters[c][j] = randomFloat(-10.0f, 10.0f);
        }
    }

    // Establecer precisión para el archivo de texto
    texto << std::fixed << std::setprecision(6);

    // Generar puntos
    for (int i = 0; i < filas; ++i) {
        int clusterElegido = std::rand() % numClusters;
        std::vector<float> punto(columnas);

        for (int j = 0; j < columnas; ++j) {
            float ruido = randomFloat(-1.0f, 1.0f);
            float valor = clusters[clusterElegido][j] + ruido;
            punto[j] = valor;

            // Escribir en binario
            binario.write(reinterpret_cast<const char*>(&valor), sizeof(float));
        }

        // Escribir en texto
        for (int j = 0; j < columnas; ++j) {
            texto << punto[j];
            if (j < columnas - 1)
                texto << "|";
        }
        texto << "\n";
    }

    binario.close();
    texto.close();
    std::cout << "Archivos generados:\n - Binario: " << archivoBin << "\n - Texto: " << archivoTxt << "\n";
}

int main(int argc, char* argv[]) {
    argc--;
    argv++;

    int filas, columnas;
    std::string nombreArchivoBase;

    if (argc == 3){
        filas = atoi(argv[0]);
        columnas = atoi(argv[1]);
        nombreArchivoBase = argv[2];
    }
    else{
        std::cout << "Introduce el nombre base para los archivos (sin extensión): ";
        std::cin >> nombreArchivoBase;

        std::cout << "Introduce el número de filas (número de muestras): ";
        std::cin >> filas;

        std::cout << "Introduce el número de columnas (dimensión de cada punto): ";
        std::cin >> columnas;
    }

    std::string archivoBin = nombreArchivoBase + ".bin";
    std::string archivoTxt = nombreArchivoBase + ".txt";

    generarDatosPorClusters(archivoBin, archivoTxt, filas, columnas);
    return 0;
}
