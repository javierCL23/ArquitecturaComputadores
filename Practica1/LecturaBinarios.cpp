#include <cstdlib>
#include <iostream>
#include <vector>
#include <cstdio>  // Necesario para FILE, fopen, fread, fclose

using namespace std;

int main(int argc, char* argv[]) {
    // Verificar que se proporcionó un argumento
    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <archivo.bin>" << endl;
        return EXIT_FAILURE;
    }

    FILE* readFile = fopen(argv[1], "rb");
    if (!readFile) {
        perror("Error al abrir el archivo");
        return EXIT_FAILURE;
    }

    int nFilas, nCols;

    // Leer dimensiones
    if (fread(&nFilas, sizeof(int), 1, readFile) != 1 ||
        fread(&nCols, sizeof(int), 1, readFile) != 1) {
        cerr << "Error al leer las dimensiones del archivo" << endl;
        fclose(readFile);
        return EXIT_FAILURE;
    }

    // Verificar dimensiones válidas
    if (nFilas <= 0 || nCols <= 0) {
        cerr << "Dimensiones inválidas en el archivo" << endl;
        fclose(readFile);
        return EXIT_FAILURE;
    }

    vector<float> datos(nFilas * nCols);
    
    // Leer datos correctamente
    if (fread(datos.data(), sizeof(float), nFilas * nCols, readFile) != static_cast<size_t>(nFilas * nCols)) {
        cerr << "Error al leer los datos del archivo" << endl;
        fclose(readFile);
        return EXIT_FAILURE;
    }

    fclose(readFile);

    // Opcional: Mostrar algunos datos para verificación
    cout << "Filas: " << nFilas << ", Columnas: " << nCols << endl;
    if (!datos.empty()) {
        cout << "Primer elemento: " << datos[0] << endl;
        cout << "Último elemento: " << datos.back() << endl;
    }

    return EXIT_SUCCESS;
}
