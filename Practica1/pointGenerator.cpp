#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>  // Para std::shuffle
#include <random>     // Para std::default_random_engine

#define PI 3.141592f

struct point2D {
    float x;
    float y;
};

point2D getRandomPoint(float x0, float y0, float maxRadius, float minRadius = 0.0f) {
    point2D p;
    float r = minRadius + (maxRadius - minRadius) * (float)rand() / RAND_MAX;
    float alpha = 2.0f * PI * (float)rand() / RAND_MAX;
    p.x = x0 + r * cos(alpha);
    p.y = y0 + r * sin(alpha);
    return p;
};

int main() {
    int nClusters = 4;
    int nPointsPerCluster = 5;

    std::vector<point2D> data;
    for (int i = 0; i < nClusters; i++) {
        point2D centroid = getRandomPoint(0.0f, 0.0f, 20.0, 0.0);
        for (int j = 0; j < nPointsPerCluster; j++)
            data.push_back(getRandomPoint(centroid.x, centroid.y, 1.0f));
    }

    // Mezclar los puntos para que no estén ordenados por clusters
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(data.begin(), data.end(), g);

    // Escribir los datos en un archivo
    FILE* resultsFile;
    resultsFile = fopen("salida", "wb");
    int nFilas = nClusters * nPointsPerCluster;
    int nCol = 2;
    fwrite(&nFilas, sizeof(int), 1, resultsFile);
    fwrite(&nCol, sizeof(int), 1, resultsFile);
    fwrite(data.data(), sizeof(float), data.size() * nCol, resultsFile);
    fclose(resultsFile);

    // Imprimir los puntos
    for (int i = 0; i < data.size(); i++)
        std::cout << data[i].x << "\t" << data[i].y << "\n";

    return 0;
}