#include <cstdlib>
#include <iostream>
#include <vector>
using namespace std;
int main()
{
    FILE* resultsFile;
    resultsFile = fopen("datos2", "wb");
    int nFilas = 2;
    int nCol = 2;

    vector <float> data = {3,3,-3,-4};

    fwrite(&nFilas, sizeof(int), 1, resultsFile);
    fwrite(&nCol, sizeof(int), 1, resultsFile);
    fwrite(data.data(), sizeof(float), data.size()*nCol, resultsFile);
    fclose(resultsFile);
}
