#include <cstdlib>
#include <iostream>
#include <vector>
#include <limits>
//#include "mpi.h"
#include "omp.h"
using namespace std;

//Estructuras para manejo de datos
struct arrayIndex
{
    vector<int> values;
    int n;
    int lastValueNotEmpty; //Maximo en el que se debe de buscar indices
    int firstValueEmpty; //Primer valor por el que se tiene que buscar huecos. Solo se actualiza
    arrayIndex(vector <int> datosIniciales, int elementos) : lastValueNotEmpty(datosIniciales.size()-1),firstValueEmpty(datosIniciales.size()), values(elementos,-1),n(datosIniciales.size()) {
        for (int i=0; i<datosIniciales.size();i++){
            values[i] = datosIniciales[i];
        }
    }
    int removeIndex(int index){
        if (index >= 0 && index <= lastValueNotEmpty){
            int retorno = values[index];
            values[index] = -1;
            if (index < firstValueEmpty){
                firstValueEmpty = index;
            }
            n--;
            return retorno;
        }
        fprintf(stderr,"ERROR trying to removeValue in arrayIndex\n");
        exit(EXIT_FAILURE);
    }
    void addValue(int value){
        bool parar = false;
        int i = firstValueEmpty;
        while(!parar){
            if (values[i] == -1){
                parar = true;
                values[i] = value;
                if(i > lastValueNotEmpty){
                    lastValueNotEmpty = i;
                }
                firstValueEmpty = i+1; //Como minimo el siguiente hueco estara de aqui para adelante
            }
            i++;
            if (i>values.size()){
                fprintf(stderr,"ERROR trying to addValue in arrayIndex\n");
                exit(EXIT_FAILURE);
            }
        }
        n++;
    }
    void printValues(){ //Funcion de debuggeo
        printf("LastValueNotEmpty: %d\nFirstValueEmpty: %d\n", lastValueNotEmpty, firstValueEmpty);
        printf("[");
        for (int i=0;i<values.size();i++){
            printf(" %d ",values[i]);
        }
        printf("]\n");
    }
};

struct matriz {
    int nRows, nCols;
    vector<float> data;

    matriz(int rows, int cols) : nRows(rows), nCols(cols), data(rows * cols) {}

    float& operator()(int row, int col) {
        return data[row * nCols + col];  
    }

    void printData(){
        cout << endl;
        for (int fila=0;fila<nRows;fila++){
            printf("|");
            for (int col = 0; col<nCols; col++){
                printf(" %f ",(*this)(fila,col));
            }
            printf("|\n");
        }
        fflush(stdout);
        cout<< endl;
    }
    
};

matriz read_data(char* fileName){
    FILE* readFile;
    readFile = fopen(fileName, "rb");
    if (!readFile) {
        fprintf(stderr,"ERROR: Couldn't open the file\n");
        exit(EXIT_FAILURE);
    }
    int nRows, nCols;
    fread(&nRows,sizeof(int),1,readFile);
    fread(&nCols,sizeof(int),1,readFile);

    if (nRows <= 0 || nCols <= 0){
        fprintf(stderr,"ERROR: can't use those dimensions for your data\n");
        exit(EXIT_FAILURE);
    }
    matriz datos(nRows,nCols);
    fread(datos.data.data(),sizeof(float),nRows*nCols,readFile);
    fclose(readFile);
    return datos;
}

//Funciones OMP (granularidad fina --> Cálculo de estadísticos de toda la matriz de datos)
float meanCalcRow(matriz data,int col){
    int nt;
    float suma = 0;
    #pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        if (tid < data.nRows){ //En caso de que los datos tengan menos observaciones que hilos, no debe ejecutar.
            float sumaInterna = data(tid,col);
            for (int i=tid+nt;i<data.nRows;i+=nt){
                sumaInterna+=data(i,col);
            }
            #pragma omp critical
            suma+=sumaInterna;
        }
    }
    return suma/data.nRows;
}

float minCalcRow(matriz data, int col){
    int nt;
    float min = numeric_limits<float>::infinity();
    #pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        if (tid < data.nRows){ //En caso de que los datos tengan menos observaciones que hilos, no debe ejecutar.
            float minimoActual = data(tid,col);
            for (int i=tid+nt; i<data.nRows; i+=nt){
                if (data(i,col) < minimoActual){
                    minimoActual = data(i,col);
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

float maxCalcRow(matriz data, int col){
    int nt;
    float max = -numeric_limits<float>::infinity();
    #pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        if (tid < data.nRows){ //En caso de que los datos tengan menos observaciones que hilos, no debe ejecutar.
            float maxActual = data(tid,col);
            for (int i=tid+nt; i<data.nRows; i+=nt){
                if (data(i,col) < maxActual){
                    maxActual = data(i,col);
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

float varCalcRow(matriz data, int col){
    int nt;
    float media = meanCalcRow(data,col);
    float suma = 0;
    #pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        if (tid < data.nRows){ //En caso de que los datos tengan menos observaciones que hilos, no debe ejecutar.
            float sumaInterna = 0;
            for (int i=tid;i<data.nRows;i+=nt){
                sumaInterna+=(data(i,col) - media)*(data(i,col) - media);
            }
            #pragma omp critical
            suma+=sumaInterna;
        }
    }
    return suma/(data.nRows-1); //Cuasi-Varianza porque es insesgado
}

vector<float> statisticCalc(matriz data, string option){
    /*Para aprovechar codigo y no tener multiples 
    funciones innecesariamente, mientras que se desicciona 
    el codigo un poco en varias funciones (funciones que 
    calculan para una sola columna).*/
    vector<float> stats(data.nCols);

    for(int col = 0; col < data.nCols; col++){
        if (option == "min"){
            stats[col] = minCalcRow(data,col);
        }
        else if(option == "max"){
            stats[col] = maxCalcRow(data,col);
        }
        else if(option == "mean"){
            stats[col] = meanCalcRow(data,col);
        }
        else if(option == "var"){
            stats[col] = varCalcRow(data,col);
        }
        else{
            fprintf(stderr,"ERROR: Statistic option not valid (Use min, max, mean or var)");
            exit(EXIT_FAILURE);
        }
    }
    return stats;
}

//Visualizacion de las estadisticas
void printStatistic(vector<float> stats, string stat, int n){
    std::cout << stat << ": [";
    for (int i = 0; i<n; i++){
        printf("%f",stats[i]);
        if (i!=n-1){
            printf(",");
        }
        printf(" ");
    }
    printf("]\n");

}

void printAllStatistics(matriz data){
    vector<float> medias = statisticCalc(data,"mean");
    vector<float> mins = statisticCalc(data,"min");
    vector<float> maxs = statisticCalc(data,"max");
    vector<float> vars = statisticCalc(data,"var");

    printStatistic(medias,"Mean",data.nCols);    
    printStatistic(vars,"Var",data.nCols); 
    printStatistic(mins,"Min",data.nCols);    
    printStatistic(maxs,"Max",data.nCols);    
}

//Funciones parte MPI (granularidad fina)
float sumCalRowMPI(matriz data, vector<int> indices, int col){
    float suma = 0;
    int nt;
    #pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
        float sumaInterna = 0;
        for (int i=tid; i<indices.size(); i+=nt){
            if (indices[i] != -1){
                sumaInterna += data(indices[i],col);
            }
        }
        #pragma omp critical
        suma+= sumaInterna;
    }
    return suma;
}


void reCalcCentroidPosition(matriz data,vector<float> &centroidPosition, vector<float>& centroidPositionSum, int nActual, vector<int> newPoints, int nNews, vector<int> oldPoints, int nOlds){
    for (int i= 0; i<data.nCols; i++){
        centroidPositionSum[i]+=sumCalRowMPI(data,newPoints,i);
        centroidPositionSum[i]-=sumCalRowMPI(data,oldPoints,i);
        
        if ((nActual+nNews-nOlds) == 0){
            centroidPosition[i] = 0;
        }
        else{
            centroidPosition[i] = centroidPositionSum[i]/(nActual+nNews-nOlds);
        }
    }
}


int main(int argc, char** argv){
    if (argc <= 1){
        fprintf(stderr,"ERROR: Insert a dataFile\n");
        exit(EXIT_FAILURE);
    }
    matriz data = read_data(argv[1]);
    data.printData();

    vector<float> centroidSum(data.nCols,0);
    vector<float> centroids(data.nCols,0);
    
    vector<int> entran = {1,2,5};
    vector<int> salen = {};
    reCalcCentroidPosition(data,centroids,centroidSum,0,entran,entran.size(),salen,salen.size());

    for(int i=0; i<data.nCols;i++){
        printf(" %f ",centroids[i]);
    }
    printf("\n");
    for(int i=0; i<data.nCols;i++){
        printf(" %f ",centroidSum[i]);
    }
    printf("\n");
    return 0;
}