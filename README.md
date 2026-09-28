# Optimización de Computadores: Computación Paralela

Repositorio con dos prácticas centradas en acelerar aplicaciones mediante programación paralela y heterogénea con **OpenMP**, **MPI** y **CUDA**.

| Práctica | Tema | Tecnologías |
|----------|------|-------------|
| [Práctica 1](./Practica1) | Algoritmo K-Means | C/C++, OpenMP, MPI |
| [Práctica 2](./Practica2) | Generación de imágenes con Ray Tracing | C/C++, OpenMP, MPI, CUDA |

---

## Práctica 1: K-Means con OpenMP y MPI

### Descripción

Implementación del algoritmo de clustering **K-Means** y optimización de su rendimiento mediante paralelismo. El algoritmo agrupa `N` puntos en `K` clusters repitiendo dos fases hasta converger:

1. **Asignación:** cada punto se asigna al centroide más cercano.
2. **Actualización:** cada centroide se recalcula como la media de los puntos asignados.

---

## Práctica 2: Ray Tracing con OpenMP, MPI y CUDA

### Descripción

Aceleración de un generador de imágenes mediante **ray tracing**. Para cada píxel se lanza un rayo desde la cámara, se calculan sus intersecciones con los objetos de la escena y se obtiene el color final (iluminación, sombras, reflejos). Al ser un problema **embarazosamente paralelo** (cada píxel es independiente), es un candidato ideal para el paralelismo.

---

### Requisitos

- Compilador con soporte C/C++11 o superior
- OpenMP
- Implementación de MPI (OpenMPI o MPICH)
- CUDA Toolkit y una GPU NVIDIA compatible
