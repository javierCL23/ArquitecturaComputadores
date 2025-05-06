// #include <cstdlib>
#include <iostream>
#include <vector>
#include <tuple>
#include <limits>
#include <chrono>  //Para cronometrar cuánto tarda el algoritmo
#include <fstream> //Para guardar los tiempos
#include <mpi.h>
#include "omp.h"

using namespace std;

// Estructuras para manejo de datos
class arrayIndex
{
private:
    int n;
    vector<int> values;
    int lastValueNotEmpty; // Maximo en el que se debe de buscar indices
    int firstValueEmpty;   // Primer valor por el que se tiene que buscar huecos. Solo se actualiza

public:    
    arrayIndex(vector<int> datosIniciales, int elementos) : lastValueNotEmpty(datosIniciales.size() - 1), firstValueEmpty(datosIniciales.size()), values(elementos, -1), n(datosIniciales.size()){
        for (int i = 0; i < datosIniciales.size(); i++){
            values[i] = datosIniciales[i];
        }
    }
    int getN() const{
        return n;
    }
    const vector<int>& getValues() const{ //Devuelve una referencia no modificable
        return values;
    }
    vector<int>& getValuesRef(){ //Devuelve una versión modificable
        return values;
    }
    void removeValue(int value){
        for (int i = 0; i <= lastValueNotEmpty; i++){
            if (values[i] == value){
                values[i] = -1;
                n--;
                if (i < firstValueEmpty){
                    firstValueEmpty = i;
                }
            }
        }
    }
    void addValue(int value){
        bool seguir = true;
        int i = firstValueEmpty;

        while (seguir){
            if (values[i] == -1){
                seguir = false;
                values[i] = value;
                if (i > lastValueNotEmpty){
                    lastValueNotEmpty = i;
                }
                firstValueEmpty = i + 1; // Como minimo ahora va a estar desde el que se ha anadido en adelante
            }
            i++;
            if (i >= values.size()){
                fprintf(stderr, "ERROR trying to addValue in arrayIndex\n");
                exit(EXIT_FAILURE);
            }
        }
        n++;
    }

    void addVector(vector<int> nuevosPuntos){
        int n = nuevosPuntos.size();
        for (int i = 0; i < n; i++){
            addValue(nuevosPuntos[i]);
        }
    }

    void removeVector(vector<int> viejosPuntos){
        int n = viejosPuntos.size();
        for (int i = 0; i < n; i++){
            removeValue(viejosPuntos[i]);
        }
    }

    vector<int> getNonZeroValues(){
        vector<int> retorno(n);
        int i=0;
        int puestos=0;
        while (puestos<n){
            if (values[i] != -1){
                retorno[puestos] = values[i];
                puestos++;
            }
            i++;
        }
        return retorno;
    }

    void printValues(){ // Funcion de debuggeo
        printf("LastValueNotEmpty: %d\nFirstValueEmpty: %d\n", lastValueNotEmpty, firstValueEmpty);
        printf("n:%d\n",n);
        printf("[");
        for (int i = 0; i < values.size(); i++){
            printf(" %d ", values[i]);
        }
        printf("]\n");
    }
};

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


// Funciones OMP (granularidad fina --> Cálculo de estadísticos de toda la matriz de datos)
float meanCalcRow(const matriz& data, int col){
    int nt;
    float suma = 0;
#pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        if (tid < data.getRows()){ // En caso de que los datos tengan menos observaciones que hilos, no debe ejecutar.
            float sumaInterna = data(tid, col);
            for (int i = tid + nt; i < data.getRows(); i += nt){
                sumaInterna += data(i, col);
            }
#pragma omp critical
            suma += sumaInterna;
        }
    }
    return suma / data.getRows();
}

float minCalcRow(const matriz& data, int col){
    int nt;
    float min = numeric_limits<float>::infinity();
#pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        if (tid < data.getRows()){ // En caso de que los datos tengan menos observaciones que hilos, no debe ejecutar.
            float minimoActual = data(tid, col);
            for (int i = tid + nt; i < data.getRows(); i += nt){
                if (data(i, col) < minimoActual){
                    minimoActual = data(i, col);
                }
            }
#pragma omp critical
            {
                if (minimoActual < min){
                    min = minimoActual;
                }
            }
        }
    }
    return min;
}

float maxCalcRow(const matriz& data, int col){
    int nt;
    float max = -numeric_limits<float>::infinity();
#pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        if (tid < data.getRows()){ // En caso de que los datos tengan menos observaciones que hilos, no debe ejecutar.
            float maxActual = data(tid, col);
            for (int i = tid + nt; i < data.getRows(); i += nt){
                if (data(i, col) > maxActual){
                    maxActual = data(i, col);
                }
            }
#pragma omp critical
            {
                if (maxActual > max){
                    max = maxActual;
                }
            }
        }
    }
    return max;
}

float varCalcRow(const matriz& data, int col){
    int nt;
    float media = meanCalcRow(data, col);
    float suma = 0;
#pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        if (tid < data.getRows()){ // En caso de que los datos tengan menos observaciones que hilos, no debe ejecutar.
            float sumaInterna = 0;
            for (int i = tid; i < data.getRows(); i += nt)
            {
                sumaInterna += (data(i, col) - media) * (data(i, col) - media);
            }
#pragma omp critical
            suma += sumaInterna;
        }
    }
    return suma / (data.getRows() - 1); // Cuasi-Varianza porque es insesgado
}

vector<float> statisticCalc(const matriz& data, const string& option){
    /*Para aprovechar codigo y no tener multiples
    funciones innecesariamente, mientras que se desicciona
    el codigo un poco en varias funciones (funciones que
    calculan para una sola columna).*/
    vector<float> stats(data.getCols());

    for (int col = 0; col < data.getCols(); col++){
        if (option == "min"){
            stats[col] = minCalcRow(data, col);
        }
        else if (option == "max"){
            stats[col] = maxCalcRow(data, col);
        }
        else if (option == "mean"){
            stats[col] = meanCalcRow(data, col);
        }
        else if (option == "var"){
            stats[col] = varCalcRow(data, col);
        }
        else{
            fprintf(stderr, "ERROR: Statistic option not valid (Use min, max, mean or var)");
            exit(EXIT_FAILURE);
        }
    }
    return stats;
}

// Visualizacion de las estadisticas
void printStatistic(const vector<float>& stats, const string& stat, int n){
    std::cout << stat << ": [";
    for (int i = 0; i < n; i++){
        printf("%f", stats[i]);
        if (i != n - 1){
            printf(",");
        }
        printf(" ");
    }
    printf("]\n");
}

void printAllStatistics(const matriz& data){
    vector<float> medias = statisticCalc(data, "mean");
    vector<float> mins = statisticCalc(data, "min");
    vector<float> maxs = statisticCalc(data, "max");
    vector<float> vars = statisticCalc(data, "var");
    printf("Statistics:\n");
    printStatistic(medias, "Mean", data.getCols());
    printStatistic(vars, "Var", data.getCols());
    printStatistic(mins, "Min", data.getCols());
    printStatistic(maxs, "Max", data.getCols());
}

// Funciones parte MPI (granularidad fina)
vector<int> partirDatos(int nodo, int n, int nNodos){
//Devuelve los índices de los datos, no los datos en sí
    int tamanio;
    int inicio;
    // Caso especial: más nodos que observaciones:		3obs, 4nodos --> rank:3 (nodo4) no coge datos.
    if (nNodos > n){
        if (nodo >= n){
            return {};
        }
        return {nodo}; // Cada cluster consiste de 1 dato
    }

    // Caso raro: ultimo nodo, se queda con lo que le toca y lo que sobra en caso de que la particion no sea perfecta
    if (nodo == nNodos - 1){
        inicio = nodo * (n / nNodos);
        tamanio = n - inicio;
    }
    // Caso general:
    else{
        tamanio = n / nNodos;
        inicio = tamanio * nodo;
    }
    vector<int> vectorRetorno(tamanio);
    int nt;
#pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        if (tid < tamanio){ // En caso de que los datos tengan menos observaciones que hilos, no debe ejecutar.
            for (int i = tid; i < tamanio; i += nt){
                vectorRetorno[i] = i + inicio;
            }
        }
    }
    return vectorRetorno;
}

float sumCalRowMPI(const matriz& data, const vector<int>& indices, int col){
    float suma = 0;
    int nt;
#pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        float sumaInterna = 0;
        for (int i = tid; i < indices.size(); i += nt){
            if (indices[i] != -1){
                sumaInterna += data(indices[i], col);
            }
        }
#pragma omp critical
        suma += sumaInterna;
    }
    return suma;
}

void reCalcCentroidPosition(
    const matriz& data,
    matriz& centroidPosition,
    matriz& centroidPositionSum,
    int nActual,
    const vector<int>& newPoints,
    const vector<int>& oldPoints
){
    int nNews = newPoints.size();
    int nOlds = oldPoints.size();
    float sumado;
    float restado;
    
    if ((nActual + nNews - nOlds) == 0){     
        for (int i = 0; i<data.getCols();i++)
        centroidPosition(0, i) = 0;
    }
    else{
        for (int i = 0; i<data.getCols();i++){
            centroidPositionSum(0,i)+=sumCalRowMPI(data,newPoints,i);
            centroidPositionSum(0,i)-=sumCalRowMPI(data,oldPoints,i);
            centroidPosition(0, i) = centroidPositionSum(0, i) / (nActual + nNews - nOlds);
        }
    }
}


float calcDistanceToCentroid(const float* dato, const float* centroid, int dims){
    float suma = 0;
    int nt;
#pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        float sumaInterna = 0;
        for (int i = tid; i < dims; i += nt){
            sumaInterna += (centroid[i] - dato[i]) * (centroid[i] - dato[i]); // ||c - x||²
        }
#pragma omp critical
        suma += sumaInterna;
    }
    return suma;
}

int getClosestCentroid(const float* dato, const matriz& centroids){
    float minDist = numeric_limits<float>::infinity();
    int centroideMin = 0;
    int dims = centroids.getCols();
    int nCentroids = centroids.getRows();
    for (int j = 0; j < nCentroids; j++){ // Calculo de la distancia a cada centroide
        const float *actualCentroid = centroids.getData().data() + (j * dims);
        float dist = calcDistanceToCentroid(dato, actualCentroid, dims);
        if (dist < minDist){
            minDist = dist;
            centroideMin = j;
        }
    }
    // printf("Dato: [%f,%f] -> %d\n", dato[0],dato[1],centroideMin);
    return centroideMin;
}

vector<tuple<int, int>> newPointsAssignation(
    const matriz& data,
    const arrayIndex& indices,
    const matriz& centroids
){
    vector<tuple<int, int>> newPointsAssignation(indices.getN());
    int dims = data.getCols();
    int i = 0;
    int addedPoints = 0;
    int indiceActual;
    while (addedPoints < indices.getN()){
        indiceActual = indices.getValues()[i];
        if (indiceActual != -1){
            const float *dato = data.getData().data() + (indiceActual * dims); // Puntero al elemento (indiceActual,0) de la matriz de datos;
            newPointsAssignation[addedPoints] = make_tuple(indiceActual, getClosestCentroid(dato, centroids));
            addedPoints += 1;
        }
        i++;
    }
    return newPointsAssignation;
}

void exchange_indices(int rank, int size, const vector<vector<int>> &indices_to_send, vector<int> &indices_to_recv){ 
    // 1. Preparar los parámetros para MPI_Alltoallv
    vector<int> send_counts(size, 0); // Número de índices a enviar a cada proceso
    vector<int> sdispls(size, 0);    // Desplazamientos en el buffer de envío
    vector<int> recv_counts(size, 0); // Número de índices a recibir de cada proceso
    vector<int> rdispls(size, 0);    // Desplazamientos en el buffer de recepción

    // 2. Calcular send_counts y sdispls
    int total_send = 0;
    for (int i = 0; i < size; ++i){
        send_counts[i] = indices_to_send[i].size(); // Número de índices a enviar al proceso i
        sdispls[i] = total_send;                  // Desplazamiento en el buffer de envío
        total_send += send_counts[i];             // Acumular el total de índices a enviar
    }

    // 3. Crear el buffer de envío (todos los índices concatenados)
    vector<int> send_buffer(total_send);
    int offset = 0;
    for (int i = 0; i < size; ++i){
        for (int j = 0; j < indices_to_send[i].size(); ++j){
            send_buffer[offset++] = indices_to_send[i][j];
        }
    }

    // 4. Intercambiar el número de índices que cada proceso recibirá
    MPI_Alltoall(send_counts.data(), 1, MPI_INT, recv_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);

    // 5. Calcular rdispls y el tamaño total del buffer de recepción
    int total_recv = 0;
    for (int i = 0; i < size; ++i){
        rdispls[i] = total_recv;      // Desplazamiento en el buffer de recepción
        total_recv += recv_counts[i]; // Acumular el total de índices a recibir
    }

    // 6. Preparar el buffer de recepción
    indices_to_recv.resize(total_recv);

    // 7. Intercambiar los índices usando MPI_Alltoallv
    MPI_Alltoallv(send_buffer.data(), send_counts.data(), sdispls.data(), MPI_INT,
                  indices_to_recv.data(), recv_counts.data(), rdispls.data(), MPI_INT,
                  MPI_COMM_WORLD);
}

bool stopCalculation(float umbral, int nOldPoints, int n, int pid, int np)
{
    float percentageOut;
    if (n == 0){
        percentageOut = 0;
    }
    else{
        percentageOut = static_cast<float>(nOldPoints) / n;
    }
    vector<float> rec_buffer(np);
    MPI_Gather(&percentageOut, 1, MPI_FLOAT, rec_buffer.data(), 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

    int retorno = 0;
    if (pid == 0){
        for (int i = 0; i < np; ++i){
            if (rec_buffer[i] > umbral){
                retorno = 1;
                break;
            }
        }
    }
    MPI_Bcast(&retorno, 1, MPI_INT, 0, MPI_COMM_WORLD);
    return (retorno == 1);
}

bool stopCalculation2(float umbral, int nOldPoints, int n, int pid, int np){
    int hayTrabajo = 0;
    float percentageOut;
    if (n == 0){
        percentageOut = 0;
    }
    else{
        percentageOut = static_cast<float>(nOldPoints) / n;
    }
    if (percentageOut > umbral){
        hayTrabajo++;
    }
    MPI_Allreduce(&hayTrabajo,&hayTrabajo,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
    return (hayTrabajo >= 1);
}

void printVector(const vector<int> &vector, int pid){
    printf("Nodo%d: [",pid);
    for (int i=0;i<vector.size();i++){
        printf(" %d ",vector[i]);
    }
    printf("]\n");
}

void printCambios(const vector<tuple<int, int>> &nuevasAsignaciones, int pid){
    printf("Nodo%d;\n",pid);
    for (int i=0;i<nuevasAsignaciones.size();i++){
        int index = get<0>(nuevasAsignaciones[i]);
        int nuevaAsignacion = get<1>(nuevasAsignaciones[i]);
        if (nuevaAsignacion != pid)
        printf("%d ==> %d\n",index,nuevaAsignacion);
    }
}

void printForINode(const vector<vector<int>> &pointsForINode){
    for (int i = 0; i < pointsForINode.size();i++){
        vector <int> vec = pointsForINode[i];
        printf("For Node%d [",i);
        for (int j = 0;j<vec.size();j++){
            printf(" %d ",vec[j]);
        }
        printf("]\n");
    }
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

    //--------------------------------------CARGAR LOS DATOS A TODOS LOS NODOS--------------------------------------
    int dims[2];
    matriz datos(0,0);

    if (pid == 0){
        datos = matriz(argv[1]);
        dims[0] = datos.getRows();
        dims[1] = datos.getCols();
        datos.printData();
    }

    MPI_Bcast(&dims, 2, MPI_INT, 0, MPI_COMM_WORLD);

    if (pid != 0){
        datos = matriz(dims[0], dims[1]);
    }
    MPI_Bcast(datos.getData().data(), dims[0] * dims[1], MPI_FLOAT, 0, MPI_COMM_WORLD);

    auto t0 = std::chrono::high_resolution_clock::now();

    //--------------------------------------ASIGNACION DE LOS CLUSTERS INICIALES--------------------------------------
    vector<int> datosEntrantes = partirDatos(pid, dims[0], np);
    int nNuevos = datosEntrantes.size();
    vector<int> datosSalientes = {};
    int nViejos = 0;

    arrayIndex indexCluster({}, dims[0]);

    matriz centroideLocal(1, dims[1]);
    matriz centroideSum(1, dims[1]);
    matriz globalCentroides(np, dims[1]);

    //--------------------------------------    CALCULO Y ENVIO DE CENTROIDES     --------------------------------------
    reCalcCentroidPosition(datos, centroideLocal, centroideSum,indexCluster.getN(), datosEntrantes, datosSalientes);
    indexCluster.addVector(datosEntrantes);
    
    bool seguir = true;
    int iter = 1;
    while (seguir && iter <= 2000){
        if (pid==0){
            printf("-------------------------------------------------------------------------\n");
        }
        MPI_Allgather(centroideLocal.getData().data(), dims[1], MPI_FLOAT,
                    globalCentroides.getData().data(), dims[1], MPI_FLOAT, MPI_COMM_WORLD);
     
        
        if (pid ==0) globalCentroides.printData();
        
        vector<tuple<int, int>> nuevasAsignaciones = newPointsAssignation(datos, indexCluster, globalCentroides);
        vector<int> oldPoints;
        vector<vector<int>> pointsForINode(np);

        for (int i = 0; i < nuevasAsignaciones.size(); i++){
            int index = get<0>(nuevasAsignaciones[i]);
            int nuevaAsignacion = get<1>(nuevasAsignaciones[i]);
            if (nuevaAsignacion != pid){
                oldPoints.push_back(index);
                pointsForINode[nuevaAsignacion].push_back(index);
            }
        }
        
        //printCambios(nuevasAsignaciones,pid);

        // printForINode(pointsForINode);
        vector<int> newPoints = {};
        exchange_indices(pid, np, pointsForINode, newPoints);

        //printVector(newPoints,pid);
        int nActual = indexCluster.getN();
        nViejos = oldPoints.size();
        nNuevos = newPoints.size();

        if (nActual + nNuevos - nViejos < nNuevos + nViejos){ //Si se tienen al final menos datos que calcular los nuevos y viejos por separado, calcula desde 0 la media
            centroideLocal.resetToZero();
            centroideSum.resetToZero();
            indexCluster.addVector(newPoints);
            indexCluster.removeVector(oldPoints);
            //Los nuevos valores son los del indexCluster (todos)
            newPoints = indexCluster.getNonZeroValues();
            reCalcCentroidPosition(datos,centroideLocal,centroideSum,0,newPoints,{});
        }
        else{ //Caso base: calcula la media de los nuevos puntos, y modifica la anterior.
            reCalcCentroidPosition(datos,centroideLocal,centroideSum,nActual,newPoints,oldPoints);
            indexCluster.addVector(newPoints);
            indexCluster.removeVector(oldPoints);
        }
    
        // seguir = stopCalculation(0.05, nViejos, indexCluster.getN(), pid, np);      //Version maestro-esclavo
        seguir = stopCalculation2(0.05,nViejos,indexCluster.getN(),pid, np);      //Version Reduce
        iter += 1;
    }
    //--------------------------------------    RESULTADOS     --------------------------------------
    if (pid == 0){
        printf("-------------------------------RESULTADOS-------------------------------\n");
        if (seguir) printf("Resultados no convergieron\n");
        globalCentroides.printData();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    printVector(indexCluster.getNonZeroValues(),pid);
    MPI_Barrier(MPI_COMM_WORLD);
    if (pid == 0){
        auto t1 = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        std::cout << "Tiempo total de ejecución: " << float(ms)/1000 << " s\n";
        std::ofstream tiempos("tiempos.txt", std::ios::app);
        if (tiempos.is_open()) {
            tiempos << float(ms)/1000 << endl;
            tiempos.close();
        }
    }
    MPI_Finalize();
    return 0;
}
