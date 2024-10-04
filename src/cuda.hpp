
#ifndef CUDA_HPP
#define CUDA_HPP


#include "../depends/ffiasm/depends/cryptography_cuda/src/lib.h"

void g1MultiMulByScalarGpu(AltBn128::Engine::G1Point &r, AltBn128::Engine::G1PointAffine *bases, uint8_t *scalars, unsigned int sizePoint, unsigned int n);
void g2MultiMulByScalarGpu(AltBn128::Engine::G2Point &r, AltBn128::Engine::G2PointAffine *bases, scalar_field_t *scalars, unsigned int n);
#endif