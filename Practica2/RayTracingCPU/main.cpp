//==================================================================================================
// Written in 2016 by Peter Shirley <ptrshrl@gmail.com>
//
// To the extent possible under law, the author(s) have dedicated all copyright and related and
// neighboring rights to this software to the public domain worldwide. This software is distributed
// without any warranty.
//
// You should have received a copy (see file COPYING.txt) of the CC0 Public Domain Dedication along
// with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//==================================================================================================
#include "omp.h"

#include <float.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <sstream>
#include <fstream>
#include <chrono>

#include "Camera.h"
#include "Object.h"
#include "Scene.h"
#include "Sphere.h"
#include "Diffuse.h"
#include "Metallic.h"
#include "Crystalline.h"

#include "random.h"
#include "utils.h"

Scene loadObjectsFromFile(const std::string& filename) {
	std::ifstream file(filename);
	std::string line;

	Scene list;

	if (file.is_open()) {
		while (std::getline(file, line)) {
			std::stringstream ss(line);
			std::string token;
			std::vector<std::string> tokens;

			while (ss >> token) {
				tokens.push_back(token);
			}

			if (tokens.empty()) continue; // L�nea vac�a

			// Esperamos al menos la palabra clave "Object"
			if (tokens[0] == "Object" && tokens.size() >= 12) { // M�nimo para Sphere y un material con 1 float
				// Parsear la esfera
				if (tokens[1] == "Sphere" && tokens[2] == "(" && tokens[7] == ")") {
					try {
						float sx = std::stof(tokens[3].substr(tokens[3].find('(') + 1, tokens[3].find(',') - tokens[3].find('(') - 1));
						float sy = std::stof(tokens[4].substr(0, tokens[4].find(',')));
						float sz = std::stof(tokens[5].substr(0, tokens[5].find(',')));
						float sr = std::stof(tokens[6]);

						// Parsear el material del �ltimo objeto creado

						if (tokens[8] == "Crystalline" && tokens[9] == "(" && tokens[11].back() == ')') {
							float ma = std::stof(tokens[10]);
							list.add(new Object(
								new Sphere(Vec3(sx, sy, sz), sr),
								new Crystalline(ma)
							));
							std::cout << "Crystaline" << sx << " " << sy << " " << sz << " " << sr << " " << ma << "\n";
						}
						else if (tokens[8] == "Metallic" && tokens.size() == 15 && tokens[9] == "(" && tokens[14] == ")") {
							float ma = std::stof(tokens[10].substr(tokens[10].find('(') + 1, tokens[10].find(',') - tokens[10].find('(') - 1));
							float mb = std::stof(tokens[11].substr(0, tokens[11].find(',')));
							float mc = std::stof(tokens[12].substr(0, tokens[12].find(',')));
							float mf = std::stof(tokens[13].substr(0, tokens[13].length() - 1));
							list.add(new Object(
								new Sphere(Vec3(sx, sy, sz), sr),
								new Metallic(Vec3(ma, mb, mc), mf)
							));
							std::cout << "Metallic" << sx << " " << sy << " " << sz << " " << sr << " " << ma << " " << mb << " " << mc << " " << mf << "\n";
						}
						else if (tokens[8] == "Diffuse" && tokens.size() == 14 && tokens[9] == "(" && tokens[13].back() == ')') {
							float ma = std::stof(tokens[10].substr(tokens[10].find('(') + 1, tokens[10].find(',') - tokens[10].find('(') - 1));
							float mb = std::stof(tokens[11].substr(0, tokens[11].find(',')));
							float mc = std::stof(tokens[12].substr(0, tokens[12].find(',')));
							list.add(new Object(
								new Sphere(Vec3(sx, sy, sz), sr),
								new Diffuse(Vec3(ma, mb, mc))
							));
							std::cout << "Diffuse" << sx << " " << sy << " " << sz << " " << sr << " " << ma << " " << mb << " " << mc << "\n";
						}
						else {
							std::cerr << "Error: Material desconocido o formato incorrecto en la l�nea: " << line << std::endl;
						}
					}
					catch (const std::invalid_argument& e) {
						std::cerr << "Error: Conversi�n inv�lida en la l�nea: " << line << " - " << e.what() << std::endl;
					}
					catch (const std::out_of_range& e) {
						std::cerr << "Error: Valor fuera de rango en la l�nea: " << line << " - " << e.what() << std::endl;
					}
				}
				else {
					std::cerr << "Error: Formato de esfera incorrecto en la l�nea: " << line << std::endl;
				}
			}
			else {
				std::cerr << "Error: Formato de objeto incorrecto en la l�nea: " << line << std::endl;
			}
		}
		file.close();
	}
	else {
		std::cerr << "Error: No se pudo abrir el archivo: " << filename << std::endl;
	}
	return list;
}


Scene randomScene() {
	int n = 500;
	Scene list;
	list.add(new Object(
		new Sphere(Vec3(0, -1000, 0), 1000),
		new Diffuse(Vec3(0.5, 0.5, 0.5))
	));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			float choose_mat = Random();
			Vec3 center(a + 0.9f * Random(), 0.2f, b + 0.9f * Random());
			if ((center - Vec3(4, 0.2f, 0)).length() > 0.9f) {
				if (choose_mat < 0.8f) {  // diffuse
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Diffuse(Vec3(Random() * Random(),
							Random() * Random(),
							Random() * Random()))
					));
				}
				else if (choose_mat < 0.95f) { // metal
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Metallic(Vec3(0.5f * (1 + Random()),
							0.5f * (1 + Random()),
							0.5f * (1 + Random())),
							0.5f * Random())
					));
				}
				else {  // glass
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Crystalline(1.5f)
					));
				}
			}
		}
	}

	list.add(new Object(
		new Sphere(Vec3(0, 1, 0), 1.0),
		new Crystalline(1.5f)
	));
	list.add(new Object(
		new Sphere(Vec3(-4, 1, 0), 1.0f),
		new Diffuse(Vec3(0.4f, 0.2f, 0.1f))
	));
	list.add(new Object(
		new Sphere(Vec3(4, 1, 0), 1.0f),
		new Metallic(Vec3(0.7f, 0.6f, 0.5f), 0.0f)
	));

	return list;
}

float calcDistanceToCentroid(float *dato, float *centroid, int dims){
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

void rayTracingCPU(
    unsigned char* img,
    int w, int h,
    int ns = 10,
    int px = 0,      // offset en X del parche
    int py = 0,      // offset en Y del parche
    int pw = -1,     // tamaño X absoluto (o -1 para usar w)
    int ph = -1,      // tamaño Y absoluto (o -1 para usar h)
	Scene world = loadObjectsFromFile("Scene1.txt")
) {
    // Si no se especifica pw/ph, cubrir toda la imagen
    if (pw == -1) pw = w;
    if (ph == -1) ph = h;

    int patch_w = pw - px;
    int patch_h = ph - py;

    Vec3 lookfrom(13, 2, 3);
    Vec3 lookat(0, 0, 0);
    float dist_to_focus = 10.0f;
    float aperture = 0.1f;
    Camera cam(
        lookfrom, lookat, Vec3(0, 1, 0),
        20.0f,
        float(w) / float(h),
        aperture,
        dist_to_focus
    );

	int nt;
	#pragma omp parallel
    {
        int tid;
        nt = omp_get_num_threads();
        tid = omp_get_thread_num();
		// Iterar sobre cada píxel del parche
		for (int j = tid; j < patch_h; j+=nt) {
			for (int i = 0; i < patch_w; ++i) {
		// for(int j=0;j<patch_h;++j){
		// 	for (int i=tid;i<patch_w;i+=nt) {
				Vec3 col(0.0f, 0.0f, 0.0f);
				// Muestras para anti-aliasing
				for (int s = 0; s < ns; ++s) {
					float u = float(i + px + Random()) / float(w);
					float v = float(j + py + Random()) / float(h);
					Ray r = cam.get_ray(u, v);
					col += world.getSceneColor(r);
				}
				// Promediar y corrección gamma (sqrt)
				col /= float(ns);
				col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

				// Coordenadas globales en el buffer
				int x = i + px;
				int y = j + py;
				int idx = (y * w + x) * 3;

				img[idx + 0] = char(255.99f * col[2]); // B
				img[idx + 1] = char(255.99f * col[1]); // G
				img[idx + 2] = char(255.99f * col[0]); // R
			}
    	}
	}
}

int main() {
	srand(time(0));
    int w = 600;
	int h = 600;
	int ns = 50;
    const int patches_x = 1;
    const int patches_y = 1;

    int patch_x_size = w / patches_x;
    int patch_y_size = h / patches_y;

	// Creación de la escena
	Scene world = randomScene();
    world.setSkyColor(Vec3(0.5f, 0.7f, 1.0f));
    world.setInfColor(Vec3(1.0f, 1.0f, 1.0f));


    // Buffer para toda la imagen
    size_t total_size = sizeof(unsigned char) * w * h * 3;
    unsigned char* data = (unsigned char*) calloc(total_size, 1);

	auto t0 = std::chrono::high_resolution_clock::now();
    // Renderizar cada parche
    for (int py = 0; py < patches_y; ++py) {
        for (int px = 0; px < patches_x; ++px) {
            int x0 = px * patch_x_size;
            int x1 = (px == patches_x - 1) ? w : x0 + patch_x_size;
            int y0 = py * patch_y_size;
            int y1 = (py == patches_y - 1) ? h : y0 + patch_y_size;
            rayTracingCPU(data, w, h, ns, x0, y0, x1, y1,world);
        }
    }
	auto t1 = std::chrono::high_resolution_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
	std::cout << "Tiempo total de ejecución: " << float(ms)/1000 << " s\n";
    // Guardar la imagen completa
    writeBMP("imgCompleta.bmp", data, w, h);
    printf("Imagen generada.\n");

    free(data);
	//getchar();
	return 0;
}
