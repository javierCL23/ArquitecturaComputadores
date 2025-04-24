// clases.cpp
#include "clases.hpp"

// arrayIndex
arrayIndex::arrayIndex(vector<int> datosIniciales, int elementos)
    : lastValueNotEmpty(datosIniciales.size() - 1),
      firstValueEmpty(datosIniciales.size()),
      values(elementos, -1),
      n(datosIniciales.size()) {
    for (int i = 0; i < datosIniciales.size(); i++) {
        values[i] = datosIniciales[i];
    }
}

int arrayIndex::getN() {
    return n;
}

const vector<int>& arrayIndex::getValues() const {
    return values;
}

vector<int>& arrayIndex::getValuesRef() {
    return values;
}

void arrayIndex::removeValue(int value) {
    for (int i = 0; i <= lastValueNotEmpty; i++) {
        if (values[i] == value) {
            values[i] = -1;
            n--;
            if (i < firstValueEmpty) {
                firstValueEmpty = i;
            }
        }
    }
}

void arrayIndex::addValue(int value) {
    bool seguir = true;
    int i = firstValueEmpty;
    while (seguir) {
        if (values[i] == -1) {
            seguir = false;
            values[i] = value;
            if (i > lastValueNotEmpty) {
                lastValueNotEmpty = i;
            }
            firstValueEmpty = i + 1;
        }
        i++;
        if (i >= values.size()) {
            fprintf(stderr, "ERROR trying to addValue in arrayIndex\n");
            exit(EXIT_FAILURE);
        }
    }
    n++;
}

void arrayIndex::addVector(vector<int> nuevosPuntos) {
    int n = nuevosPuntos.size();
    for (int i = 0; i < n; i++) {
        addValue(nuevosPuntos[i]);
    }
}

void arrayIndex::removeVector(vector<int> viejosPuntos) {
    int n = viejosPuntos.size();
    for (int i = 0; i < n; i++) {
        removeValue(viejosPuntos[i]);
    }
}

vector<int> arrayIndex::getNonZeroValues() {
    vector<int> retorno(n);
    int i = 0;
    int puestos = 0;
    while (puestos < n) {
        if (values[i] != -1) {
            retorno[puestos] = values[i];
            puestos++;
        }
        i++;
    }
    return retorno;
}

void arrayIndex::printValues() {
    printf("LastValueNotEmpty: %d\nFirstValueEmpty: %d\n", lastValueNotEmpty, firstValueEmpty);
    printf("n:%d\n", n);
    printf("[");
    for (int i = 0; i < values.size(); i++) {
        printf(" %d ", values[i]);
    }
    printf("]\n");
}

// matriz
matriz::matriz(int rows, int cols)
    : nRows(rows), nCols(cols), data(rows * cols) {}

matriz::matriz(char *fileName) {
    FILE *readFile;
    readFile = fopen(fileName, "rb");
    if (!readFile) {
        fprintf(stderr, "ERROR: Couldn't open the file\n");
        exit(EXIT_FAILURE);
    }
    fread(&nRows, sizeof(int), 1, readFile);
    fread(&nCols, sizeof(int), 1, readFile);
    if (nRows <= 0 || nCols <= 0) {
        fprintf(stderr, "ERROR: can't use those dimensions for your data\n");
        exit(EXIT_FAILURE);
    }
    data = vector<float>(nRows * nCols);
    fread(data.data(), sizeof(float), nRows * nCols, readFile);
    fclose(readFile);
}

float &matriz::operator()(int row, int col) {
    return data[row * nCols + col];
}

int matriz::getRows() const {
    return nRows;
}

int matriz::getCols() const {
    return nCols;
}

vector<float> matriz::getData() const {
    return data;
}

vector<float>& matriz::getDataRef() {
    return data;
}

void matriz::printData() {
    cout << endl << "Data:" << endl;
    for (int fila = 0; fila < nRows; fila++) {
        printf("|");
        for (int col = 0; col < nCols; col++) {
            printf(" %f ", (*this)(fila, col));
        }
        printf("|\n");
    }
    fflush(stdout);
    cout << endl;
}

void matriz::resetToZero() {
    fill(data.begin(), data.end(), 0.0f);
}
