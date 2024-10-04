#ifndef CUDA_CPP
#define CUDA_CPP

#if defined(USE_CUDA)

#include "alt_bn128.hpp"
#include "logging.hpp"


#include "../depends/ffiasm/depends/cryptography_cuda/src/lib.h"

using namespace AltBn128;

void g1MultiMulByScalarGpu(AltBn128::Engine::G1Point &r, AltBn128::Engine::G1PointAffine *bases, uint8_t *scalars, unsigned int sizePoint, unsigned int n)
{
    LOG_DEBUG("use gpu msm on g1");
    // AltBn128::Engine::engine.g1.multiMulByScalar(r, bases, scalars, sizePoint, n);

    point_t *gpu_result = new point_t{};
    // double start_g1 = omp_get_wtime();
    mult_pippenger(gpu_result, (affine_t *)bases, n, (fr_t *)scalars, sizePoint);
    // g1_msm += ((double)(omp_get_wtime() - start_g1));
    LOG_DEBUG("wait gpu result");
    F1Element *gpu_x = (F1Element *)(&gpu_result->X);
    F1Element *gpu_y = (F1Element *)(&gpu_result->Y);
    F1Element *gpu_z = (F1Element *)(&gpu_result->Z);

    F1.copy(r.x, *gpu_x);
    F1.copy(r.y, *gpu_y);
    F1.square(r.zz, *gpu_z);
    F1.mul(r.zzz, r.zz, *gpu_z);
}

void g2MultiMulByScalarGpu(AltBn128::Engine::G2Point &r, AltBn128::Engine::G2PointAffine *bases, scalar_field_t *scalars, unsigned int n)
{
    LOG_DEBUG("use gpu msm on g2");

    G2PointAffine zero = G2.zeroAffine();
    // g2_affine_t *points = (g2_affine_t *)malloc(sizeof(g2_affine_t) * n);
    g2_affine_t *points = (g2_affine_t *)bases; // takes more time operating on original data
    LOG_DEBUG("start allocate scalras");
    scalar_field_t *scalars_gpu = (scalar_field_t *)malloc(sizeof(scalar_field_t) * n);
    LOG_DEBUG("end allocate scalars");
    for (int i = 0; i < n; i++)
    {

        if (G2.eq(*(bases + i), zero))
        {
            // printf("memset zero %d\n", i);
            memset(scalars_gpu + i, 0, 32);
        }
        else
        {
            // printf("copy scalars %d\n", i);
            // *(scalars_gpu + i) = *(scalars + i);
            memcpy(scalars_gpu + i, scalars + i, 32);
        }

        F1.toRprLE((bases + i)->x.a, (uint8_t *)((points + i)->x.real.export_limbs()), 32);
        F1.toRprLE((bases + i)->x.b, (uint8_t *)((points + i)->x.imaginary.export_limbs()), 32);
        F1.toRprLE((bases + i)->y.a, (uint8_t *)((points + i)->y.real.export_limbs()), 32);
        F1.toRprLE((bases + i)->y.b, (uint8_t *)((points + i)->y.imaginary.export_limbs()), 32);
    }
    LOG_DEBUG("end allocate points");

    size_t msm_size = n;

    g2_projective_t *gpu_result_projective = (g2_projective_t *)malloc(sizeof(g2_projective_t));
    // double start_g2 = omp_get_wtime();
    LOG_DEBUG("start invoking g2");
    mult_pippenger_g2(gpu_result_projective, points, msm_size, scalars_gpu, 10, false, false);
    LOG_DEBUG("end invoking g2");
    // g2_msm += ((double)(omp_get_wtime() - start_g2));
    g2_affine_t gpu_result_affine = g2_projective_t::to_affine(*gpu_result_projective);
    G2PointAffine gpu_result_affine_in_host_format;
    F1.fromRprLE(gpu_result_affine_in_host_format.x.a, (uint8_t *)(gpu_result_affine.x.real.export_limbs()), 32);
    F1.fromRprLE(gpu_result_affine_in_host_format.x.b, (uint8_t *)(gpu_result_affine.x.imaginary.export_limbs()), 32);
    F1.fromRprLE(gpu_result_affine_in_host_format.y.a, (uint8_t *)(gpu_result_affine.y.real.export_limbs()), 32);
    F1.fromRprLE(gpu_result_affine_in_host_format.y.b, (uint8_t *)(gpu_result_affine.y.imaginary.export_limbs()), 32);

    G2.copy(r, gpu_result_affine_in_host_format);
}
#endif
#endif