#pragma once

#include <curand_kernel.h>

#include "Vec3.h"

float MyRandom();
Vec3 randomNormalSphere();
Vec3 randomNormalDisk();

/* GPU */

//__device__ Vec3 randomNormalSphereGPU(curandState* local_rand_state);
//__device__ Vec3 randomNormalDiskGPU(curandState* local_rand_state);
