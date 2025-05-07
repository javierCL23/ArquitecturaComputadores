#include <cstdlib>
#include <iostream>
#include <vector>
#include <limits>
#include <chrono>  //Para cronometrar cuánto tarda el algoritmo
#include <fstream> //Para guardar los tiempos
#include <algorithm> //Para función find de arrayIndex
#include <mpi.h>
#include "omp.h"
using namespace std;

class matriz
{
private:
    int nRows, nCols;
    vector<float> data;
public:

    matriz(int rows, int cols) : nRows(rows), nCols(cols), data(rows * cols) {}
    matriz(char *fileName){
        FILE *readFile;
        readFile = fopen(fileName, "rb");
        if (!readFile){
            fprintf(stderr, "ERROR: Couldn't open the file\n");
            exit(EXIT_FAILURE);
        }
        if (fread(&nRows, sizeof(int), 1, readFile) != 1) {
            fprintf(stderr, "ERROR: Failed to read number of rows\n");
            exit(EXIT_FAILURE);
        }
        
        if (fread(&nCols, sizeof(int), 1, readFile) != 1) {
            fprintf(stderr, "ERROR: Failed to read number of columns\n");
            exit(EXIT_FAILURE);
        }
        
        data = vector<float>(nRows * nCols);
        if (fread(data.data(), sizeof(float), nRows * nCols, readFile) != size_t(nRows * nCols)) {
            fprintf(stderr, "ERROR: Failed to read matrix data\n");
            exit(EXIT_FAILURE);
        }        fclose(readFile);
    }
    // Para lectura en objetos const
    float operator()(int row, int col) const {
        return data[row * nCols + col];
    }

    // Para escritura en objetos no const
    float& operator()(int row, int col) {
        return data[row * nCols + col];
    }

    int getRows() const { return nRows; }
    int getCols() const { return nCols; }

    const vector<float>& getData() const {
        return data;
    }
    
    vector<float>& getData() {
        return data;
    }
    
    void printData(){
        cout << endl
             << "Data:" << endl;
        for (int fila = 0; fila < nRows; fila++){
            printf("|");
            for (int col = 0; col < nCols; col++){
                printf(" %f ", (*this)(fila, col));
            }
            printf("|\n");
        }
        fflush(stdout);
        cout << endl;
    }
    void resetToZero(){
        fill(data.begin(), data.end(), 0.0f);
    }
};

matriz getLocalData(matriz datos, int nRows, int nCols, int pid, int np){
    int baseRows = nRows / np;
    int resto = nRows % np;
    int myRows = baseRows;
    //Creación de matriz de datos locales:
    if (pid == np-1){ //En caso del último nodo procesa unos pocos más de puntos
        myRows = baseRows+resto; 
    }

    matriz datosLocales(myRows,nCols);
    //Compartir los datos
    vector<int> cantidades(np,baseRows*nCols);
    vector<int> desplazamientos(np);

    cantidades[np-1] += nCols*resto; //El último procesa unos pocos más de puntos
    
    desplazamientos[0] = 0;
    for (int i = 1; i < np; i++) {
        desplazamientos[i] = desplazamientos[i - 1] + cantidades[i - 1];
    }
    MPI_Scatterv(datos.getData().data(), cantidades.data(),desplazamientos.data(),MPI_FLOAT,datosLocales.getData().data(),cantidades[pid],MPI_FLOAT,0,MPI_COMM_WORLD);

    return datosLocales;
}

//Cálculo de asignación a centroide
float calcDistanceToCentroid(const float* dato, const float* centroid, int dims){
    float suma = 0.0f;
    for (int i = 0; i < dims; i++){
        float diff = centroid[i] - dato[i];
        suma += diff * diff;
    }
    return suma;
}

int getClosestCentroid(const float* dato, const matriz& centroids, const vector<int>& nPointsPerCluster){
    float minDist = numeric_limits<float>::infinity();
    int centroideMin = 0;
    int dims = centroids.getCols();
    int nCentroids = centroids.getRows();

    float localMinDist = numeric_limits<float>::infinity();
    int localCentroide = 0;

    for (int j = 0; j < nCentroids; j++){
        const float *actualCentroid = centroids.getData().data() + (j * dims);
        float dist = calcDistanceToCentroid(dato, actualCentroid, dims);
        if (dist < localMinDist){
            localMinDist = dist;
            localCentroide = j;
        }
    }
    if (localMinDist < minDist){
        minDist = localMinDist;
        centroideMin = localCentroide;
    }
    return centroideMin;
}

void reCalcCentroids(const matriz& datos, matriz& centroidesLocalSum, vector<int>& pointsPerCentroidLocal, vector<int>& asignaciones, const matriz& centroidesGlobal) {
    int n = datos.getRows();       // Número de puntos
    int m = datos.getCols();       // Dimensión de cada punto
    int k = centroidesLocalSum.getRows();  // Número de centroides

    int numThreads = omp_get_max_threads();

    // Acumuladores locales por hilo
    vector<vector<float>> localSums(numThreads, vector<float>(k * m, 0.0f));
    vector<vector<int>> localCounts(numThreads, vector<int>(k, 0));

    int tid = omp_get_thread_num();
    float* localSum = localSums[tid].data();
    int* localCount = localCounts[tid].data();

    #pragma omp for
    for (int i = 0; i < n; ++i) {
        const float* dato = datos.getData().data() + i * m;

        int oldAsignacion = asignaciones[i];
        int asignacion = getClosestCentroid(dato, centroidesGlobal, pointsPerCentroidLocal);  // Usa los centroides globales para asignar

        if (asignacion != oldAsignacion) {
            asignaciones[i] = asignacion;

            // Resta del centroide antiguo
            for (int j = 0; j < m; ++j) {
                localSum[oldAsignacion * m + j] -= dato[j];
            }
            localCount[oldAsignacion]--;

            // Suma al nuevo centroide
            for (int j = 0; j < m; ++j) {
                localSum[asignacion * m + j] += dato[j];
            }
            localCount[asignacion]++;
        }
    }
    // Reducción final de acumuladores locales
    for (int t = 0; t < numThreads; ++t) {
        for (int c = 0; c < k; ++c) {
            for (int j = 0; j < m; ++j) {
                centroidesLocalSum(c, j) += localSums[t][c * m + j];
            }
            pointsPerCentroidLocal[c] += localCounts[t][c];
        }
    }
}

void FirstCalcCentroids(const matriz& datos, matriz& centroidesLocalSum, vector<int>& pointsPerCentroidLocal, vector<int>& asignaciones, const matriz& centroidesGlobal) {
    int n = datos.getRows();       // Número de puntos
    int m = datos.getCols();       // Dimensión de cada punto
    int k = centroidesLocalSum.getRows();  // Número de centroides

    int numThreads = omp_get_max_threads();

    // Acumuladores locales por hilo
    vector<vector<float>> localSums(numThreads, vector<float>(k * m, 0.0f));
    vector<vector<int>> localCounts(numThreads, vector<int>(k, 0));

    int tid = omp_get_thread_num();
    float* localSum = localSums[tid].data();
    int* localCount = localCounts[tid].data();

    #pragma omp for
    for (int i = 0; i < n; ++i) {
        const float* dato = datos.getData().data() + i * m;
        int asignacion = getClosestCentroid(dato, centroidesGlobal, pointsPerCentroidLocal);  // Usa los centroides globales para asignar        
        asignaciones[i] = asignacion;
        // Suma al nuevo centroide
        for (int j = 0; j < m; ++j) {
            localSum[asignacion * m + j] += dato[j];
        }
        localCount[asignacion]++;
    
    }

    // Reducción final de acumuladores locales
    for (int t = 0; t < numThreads; ++t) {
        for (int c = 0; c < k; ++c) {
            for (int j = 0; j < m; ++j) {
                centroidesLocalSum(c, j) += localSums[t][c * m + j];
            }
            pointsPerCentroidLocal[c] += localCounts[t][c];
        }
    }
}

void normalizeCentroids(matriz& centroides, const vector<int>& nPointsPerCluster) {
    int n = centroides.getRows();
    int m = centroides.getCols();

    #pragma omp for
    for (int i = 0; i < n; i++) {
        if (nPointsPerCluster[i] == 0) continue;  // evitar dividir por 0
        float factor = 1.0f / nPointsPerCluster[i];
        for (int j = 0; j < m; j++) {
            centroides(i, j) *= factor;
        }
    }
}

bool stopCalculation(
    const vector<int>& oldPointsPerCentroidGlobal,
    const vector<int>& pointsPerCentroidGlobal,
    int totalPoints
) {
    int n = pointsPerCentroidGlobal.size();
    int different = 0;
    for (int i = 0; i < n; i++) {
        int diff = abs(oldPointsPerCentroidGlobal[i] - pointsPerCentroidGlobal[i]);
        different += diff;
    }
    if (static_cast<float>(different) / totalPoints < 0.05) {
        return false;
    }
    return true;
}

void printVector(const vector<int> &vector, int pid){
    printf("Nodo%d: [",pid);
    for (int i=0;i<vector.size();i++){
        printf(" %d ",vector[i]);
    }
    printf("]\n");
}

//----------------------------------------------------------    MAIN    ------------------------------------------------
int main(int argc, char **argv)
{
    if (argc <= 1){
        fprintf(stderr, "ERROR: Insert a dataFile\n");
        exit(EXIT_FAILURE);
    }

    MPI_Init(&argc, &argv);
    int np, pid;
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    auto t_Inicial = std::chrono::high_resolution_clock::now();
    matriz datos(1,1);
    vector<float> localData;
    int nRows, nCols;
    if (pid == 0){
        datos = matriz(argv[1]); 
        nRows = datos.getRows();
        nCols = datos.getCols();
        // printf("Data Dimensions: [%d , %d]\n",nRows, nCols);
        if (nRows<40) datos.printData();
    }
    
    //Mandar a cada nodo nRows/np datos 
    MPI_Bcast(&nRows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&nCols, 1, MPI_INT, 0, MPI_COMM_WORLD);

    matriz datosLocales = getLocalData(datos,nRows,nCols,pid,np);
    
    //Inicializar centroides con un punto aleatorio de sus datos:
    matriz centroidesLocalSum(np,nCols);
    vector<int> pointsPerCentroidLocal(np,0);

    matriz centroidesGlobal(np,nCols);
    vector<int> pointsPerCentroidGlobal(np,1);
    vector<int> oldPointsPerCentroidGlobal(np,-1);

    vector<int> asignaciones(datosLocales.getRows(),-1);

    for(int i=0; i<nCols;i++){ // Inicializamos los centroides como un punto concreto de los datos
        centroidesLocalSum(pid,i) = datosLocales(rand()%5,i);
    }
    //Comunicar centroides a los demás
    MPI_Allreduce(centroidesLocalSum.getData().data(), centroidesGlobal.getData().data(), np*nCols, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);

    auto t0 = std::chrono::high_resolution_clock::now();
    int iter = 0;
    bool seguir = true;
#pragma omp parallel
{
    //Primera iteración
    FirstCalcCentroids(datosLocales,centroidesLocalSum,pointsPerCentroidLocal,asignaciones,centroidesGlobal);



    #pragma omp single
    MPI_Allreduce(centroidesLocalSum.getData().data(), centroidesGlobal.getData().data(), np*nCols, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    #pragma omp single
    MPI_Allreduce(pointsPerCentroidLocal.data(),pointsPerCentroidGlobal.data(),np,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
    #pragma omp barrier

    normalizeCentroids(centroidesGlobal,pointsPerCentroidGlobal);
    
    
    #pragma omp barrier

    while (seguir && iter<200){
        reCalcCentroids(datosLocales,centroidesLocalSum,pointsPerCentroidLocal,asignaciones,centroidesGlobal);
        #pragma omp barrier

        #pragma omp single
        MPI_Allreduce(centroidesLocalSum.getData().data(), centroidesGlobal.getData().data(), np*nCols, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
        #pragma omp single
        MPI_Allreduce(pointsPerCentroidLocal.data(),pointsPerCentroidGlobal.data(),np,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
        #pragma omp barrier
        normalizeCentroids(centroidesGlobal,pointsPerCentroidGlobal);
        //printVector(asignaciones,pid);
        #pragma omp single
        {
        if (pid == 0) centroidesGlobal.printData();
        iter++;
        printf("IterNº:%d\n",iter);
        }
        #pragma omp single
        seguir = stopCalculation(oldPointsPerCentroidGlobal,pointsPerCentroidGlobal,nRows);
        #pragma omp barrier
        #pragma omp single
        oldPointsPerCentroidGlobal = vector<int>(pointsPerCentroidGlobal);
        #pragma omp barrier
    }
}
    if (pid == 0){
        auto t1 = std::chrono::high_resolution_clock::now();
        auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t_Inicial).count();
        std::cout << "Tiempo total de ejecución: " << float(ms2)/1000 << " s\n";
        std::cout << "Tiempo de iteraciones: " << float(ms1)/1000 << " s\n";
        std::cout << "Número de iteraciones hasta converger: " << iter << "\n";
        std::ofstream tiempos("tiempos.txt", std::ios::app);
        if (tiempos.is_open()) {
            tiempos << float(ms1)/1000 << endl;
            tiempos.close();
        }
    }
    //if (pid==0){centroidesGlobal.printData();printf("ITERS:%d\n",iter);}
    MPI_Finalize();
    return 0;
}