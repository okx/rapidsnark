#ifndef GROTH16_HPP
#define GROTH16_HPP

#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "logging.hpp"
#include "fft.hpp"
#include "random_generator.hpp"
// #include "logging.hpp"
#include "alt_bn128.hpp"
#include <future>

using namespace AltBn128;

#if defined(USE_CUDA)

#include "../depends/ffiasm/depends/cryptography_cuda/src/lib.h"
#ifndef GPU_IMPL
#define GPU_IMPL
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

static double g1_msm = 0;
static double g2_msm = 0;

namespace Groth16
{

    template <typename Engine>
    class Proof
    {
        Engine &E;

    public:
        typename Engine::G1PointAffine A;
        typename Engine::G2PointAffine B;
        typename Engine::G1PointAffine C;

        Proof(Engine &_E) : E(_E) {}
        std::string toJsonStr();
        json toJson();
    };

#pragma pack(push, 1)
    template <typename Engine>
    struct Coef
    {
        u_int32_t m;
        u_int32_t c;
        u_int32_t s;
        typename Engine::FrElement coef;
    };
#pragma pack(pop)

    template <typename Engine>
    class Prover
    {

        Engine &E;
        u_int32_t nVars;
        u_int32_t nPublic;
        u_int32_t domainSize;
        u_int64_t nCoefs;
        typename Engine::G1PointAffine &vk_alpha1;
        typename Engine::G1PointAffine &vk_beta1;
        typename Engine::G2PointAffine &vk_beta2;
        typename Engine::G1PointAffine &vk_delta1;
        typename Engine::G2PointAffine &vk_delta2;
        Coef<Engine> *coefs;
        typename Engine::G1PointAffine *pointsA;
        typename Engine::G1PointAffine *pointsB1;
        typename Engine::G2PointAffine *pointsB2;
        typename Engine::G1PointAffine *pointsC;
        typename Engine::G1PointAffine *pointsH;

        FFT<typename Engine::Fr> *fft;

    public:
        Prover(
            Engine &_E,
            u_int32_t _nVars,
            u_int32_t _nPublic,
            u_int32_t _domainSize,
            u_int64_t _nCoefs,
            typename Engine::G1PointAffine &_vk_alpha1,
            typename Engine::G1PointAffine &_vk_beta1,
            typename Engine::G2PointAffine &_vk_beta2,
            typename Engine::G1PointAffine &_vk_delta1,
            typename Engine::G2PointAffine &_vk_delta2,
            Coef<Engine> *_coefs,
            typename Engine::G1PointAffine *_pointsA,
            typename Engine::G1PointAffine *_pointsB1,
            typename Engine::G2PointAffine *_pointsB2,
            typename Engine::G1PointAffine *_pointsC,
            typename Engine::G1PointAffine *_pointsH) : E(_E),
                                                        nVars(_nVars),
                                                        nPublic(_nPublic),
                                                        domainSize(_domainSize),
                                                        nCoefs(_nCoefs),
                                                        vk_alpha1(_vk_alpha1),
                                                        vk_beta1(_vk_beta1),
                                                        vk_beta2(_vk_beta2),
                                                        vk_delta1(_vk_delta1),
                                                        vk_delta2(_vk_delta2),
                                                        coefs(_coefs),
                                                        pointsA(_pointsA),
                                                        pointsB1(_pointsB1),
                                                        pointsB2(_pointsB2),
                                                        pointsC(_pointsC),
                                                        pointsH(_pointsH)
        {
            LOG_DEBUG(("new fft in prover constructor with domain size: " + std::to_string(domainSize)).c_str());
            fft = new FFT<typename Engine::Fr>(domainSize * 2);
        }

        ~Prover()
        {
            delete fft;
        }

        std::unique_ptr<Proof<Engine>> prove(typename Engine::FrElement *wtns);
    };

    template <typename Engine>
    std::unique_ptr<Prover<Engine>> makeProver(
        u_int32_t nVars,
        u_int32_t nPublic,
        u_int32_t domainSize,
        u_int64_t nCoefs,
        void *vk_alpha1,
        void *vk_beta1,
        void *vk_beta2,
        void *vk_delta1,
        void *vk_delta2,
        void *coefs,
        void *pointsA,
        void *pointsB1,
        void *pointsB2,
        void *pointsC,
        void *pointsH);

    template <typename Engine>
    std::unique_ptr<Prover<Engine>> makeProver(
        u_int32_t nVars,
        u_int32_t nPublic,
        u_int32_t domainSize,
        u_int64_t nCoeffs,
        void *vk_alpha1,
        void *vk_beta_1,
        void *vk_beta_2,
        void *vk_delta_1,
        void *vk_delta_2,
        void *coefs,
        void *pointsA,
        void *pointsB1,
        void *pointsB2,
        void *pointsC,
        void *pointsH)
    {
        Prover<Engine> *p = new Prover<Engine>(
            Engine::engine,
            nVars,
            nPublic,
            domainSize,
            nCoeffs,
            *(typename Engine::G1PointAffine *)vk_alpha1,
            *(typename Engine::G1PointAffine *)vk_beta_1,
            *(typename Engine::G2PointAffine *)vk_beta_2,
            *(typename Engine::G1PointAffine *)vk_delta_1,
            *(typename Engine::G2PointAffine *)vk_delta_2,
            (Coef<Engine> *)((uint64_t)coefs + 4),
            (typename Engine::G1PointAffine *)pointsA,
            (typename Engine::G1PointAffine *)pointsB1,
            (typename Engine::G2PointAffine *)pointsB2,
            (typename Engine::G1PointAffine *)pointsC,
            (typename Engine::G1PointAffine *)pointsH);
        return std::unique_ptr<Prover<Engine>>(p);
    }

    template <typename Engine>
    std::unique_ptr<Proof<Engine>> Prover<Engine>::prove(typename Engine::FrElement *wtns)
    {
        printf("mem size: %d\n", nVars);
#ifdef USE_OPENMP
        LOG_TRACE("Start Multiexp A");
        uint32_t sW = sizeof(wtns[0]);
        typename Engine::G1Point pi_a;
        uint8_t *scalars = (uint8_t *)wtns;

#if defined(USE_CUDA)
        g1MultiMulByScalarGpu(pi_a, pointsA, (uint8_t *)wtns, sizeof(affine_t), nVars);
#else

        // double start0 = omp_get_wtime();
        E.g1.multiMulByScalar(pi_a, pointsA, (uint8_t *)wtns, sW, nVars);
        // g1_msm += ((double)(omp_get_wtime() - start0));
#endif
        std::ostringstream ss2;
        ss2 << "pi_a: " << E.g1.toString(pi_a);
        LOG_DEBUG(ss2);

        LOG_TRACE("Start Multiexp B1");
        typename Engine::G1Point pib1;
#if defined(USE_CUDA)
        g1MultiMulByScalarGpu(pib1, pointsB1, (uint8_t *)wtns, sizeof(affine_t), nVars);
#else
        // double start1 = omp_get_wtime();
        E.g1.multiMulByScalar(pib1, pointsB1, (uint8_t *)wtns, sW, nVars);
        // g1_msm += ((double)(omp_get_wtime() - start1));
#endif

        std::ostringstream ss3;
        ss3 << "pib1: " << E.g1.toString(pib1);
        LOG_DEBUG(ss3);

        LOG_TRACE("Start Multiexp B2");
        typename Engine::G2Point pi_b;
#if defined(USE_CUDA)
        g2MultiMulByScalarGpu(pi_b, pointsB2, (scalar_field_t *)wtns, nVars);

#else
        // double start_g2_b = omp_get_wtime();
        E.g2.multiMulByScalar(pi_b, pointsB2, (uint8_t *)wtns, sW, nVars);
        // g2_msm += ((double)(omp_get_wtime() - start_g2_b));
#endif
        std::ostringstream ss4;
        ss4 << "pi_b: " << E.g2.toString(pi_b);
        LOG_DEBUG(ss4);

        LOG_TRACE("Start Multiexp C");
        typename Engine::G1Point pi_c;
#if defined(USE_CUDA)
        g1MultiMulByScalarGpu(pi_c, pointsC, (uint8_t *)((uint64_t)wtns + (nPublic + 1) * sW), sizeof(affine_t), nVars - nPublic - 1);
#else
        // double start_g1_c = omp_get_wtime();
        E.g1.multiMulByScalar(pi_c, pointsC, (uint8_t *)((uint64_t)wtns + (nPublic + 1) * sW), sW, nVars - nPublic - 1);
        // g1_msm += ((double)(omp_get_wtime() - start_g1_c));
#endif
        std::ostringstream ss5;
        ss5 << "pi_c: " << E.g1.toString(pi_c);
        LOG_DEBUG(ss5);
#else
        LOG_TRACE("Start Multiexp A");
        uint32_t sW = sizeof(wtns[0]);
        typename Engine::G1Point pi_a;
        auto pA_future = std::async(
            [&]()
            {
#if defined(USE_CUDA)
                g1MultiMulByScalarGpu(pi_a, pointsA, (uint8_t *)wtns, sizeof(affine_t), nVars);
#else
                E.g1.multiMulByScalar(pi_a, pointsA, (uint8_t *)wtns, sW, nVars);
#endif
            });

        LOG_TRACE("Start Multiexp B1");
        typename Engine::G1Point pib1;
        auto pB1_future = std::async(
            [&]()
            {
#if defined(USE_CUDA)
                g1MultiMulByScalarGpu(pib1, pointsB1, (uint8_t *)wtns, sizeof(affine_t), nVars);
#else
                E.g1.multiMulByScalar(pib1, pointsB1, (uint8_t *)wtns, sW, nVars);
#endif
            });

        LOG_TRACE("Start Multiexp B2");
        typename Engine::G2Point pi_b;
        auto pB2_future = std::async(
            [&]()
            {
                E.g2.multiMulByScalar(pi_b, pointsB2, (uint8_t *)wtns, sW, nVars);
            });

        LOG_TRACE("Start Multiexp C");
        typename Engine::G1Point pi_c;
        auto pC_future = std::async(
            [&]()
            {
#if defined(USE_CUDA)
                g1MultiMulByScalarGpu(pi_c, pointsC, (uint8_t *)((uint64_t)wtns + (nPublic + 1) * sW), sizeof(affine_t), nVars - nPublic - 1);
#else
                E.g1.multiMulByScalar(pi_c, pointsC, (uint8_t *)((uint64_t)wtns + (nPublic + 1) * sW), sW, nVars - nPublic - 1);
#endif
            });
#endif

        LOG_TRACE("Start Initializing a b c A");
        auto a = new typename Engine::FrElement[domainSize];
        auto b = new typename Engine::FrElement[domainSize];
        auto c = new typename Engine::FrElement[domainSize];

#pragma omp parallel for
        for (u_int32_t i = 0; i < domainSize; i++)
        {
            E.fr.copy(a[i], E.fr.zero());
            E.fr.copy(b[i], E.fr.zero());
        }

        LOG_TRACE("Processing coefs");
#ifdef _OPENMP
#define NLOCKS 1024
        omp_lock_t locks[NLOCKS];
        for (int i = 0; i < NLOCKS; i++)
            omp_init_lock(&locks[i]);
#pragma omp parallel for
#endif
        for (u_int64_t i = 0; i < nCoefs; i++)
        {
            typename Engine::FrElement *ab = (coefs[i].m == 0) ? a : b;
            typename Engine::FrElement aux;

            E.fr.mul(
                aux,
                wtns[coefs[i].s],
                coefs[i].coef);
#ifdef _OPENMP
            omp_set_lock(&locks[coefs[i].c % NLOCKS]);
#endif
            E.fr.add(
                ab[coefs[i].c],
                ab[coefs[i].c],
                aux);
#ifdef _OPENMP
            omp_unset_lock(&locks[coefs[i].c % NLOCKS]);
#endif
        }
#ifdef _OPENMP
        for (int i = 0; i < NLOCKS; i++)
            omp_destroy_lock(&locks[i]);
#endif

        LOG_TRACE("Calculating c");
#pragma omp parallel for
        for (u_int32_t i = 0; i < domainSize; i++)
        {
            E.fr.mul(
                c[i],
                a[i],
                b[i]);
        }

        LOG_TRACE("Initializing fft");
        u_int32_t domainPower = fft->log2(domainSize);

        LOG_TRACE("Start iFFT A");
        fft->ifft(a, domainSize);
        LOG_TRACE("End iFFT A");
        LOG_DEBUG(E.fr.toString(a[0]).c_str());
        LOG_DEBUG(E.fr.toString(a[1]).c_str());
        LOG_TRACE("Start Shift A");
#pragma omp parallel for
        for (u_int64_t i = 0; i < domainSize; i++)
        {
            E.fr.mul(a[i], a[i], fft->root(domainPower + 1, i));
        }
        LOG_TRACE("End Shift A");
        LOG_DEBUG(E.fr.toString(a[0]).c_str());
        LOG_DEBUG(E.fr.toString(a[1]).c_str());
        LOG_TRACE("Start FFT A");
        fft->fft(a, domainSize);
        LOG_TRACE("End FFT A");
        LOG_DEBUG(E.fr.toString(a[0]).c_str());
        LOG_DEBUG(E.fr.toString(a[1]).c_str());
        LOG_TRACE("Start iFFT B");
        fft->ifft(b, domainSize);
        LOG_TRACE("End iFFT B");
        LOG_DEBUG(E.fr.toString(b[0]).c_str());
        LOG_DEBUG(E.fr.toString(b[1]).c_str());
        LOG_TRACE("Start Shift B");
#pragma omp parallel for
        for (u_int64_t i = 0; i < domainSize; i++)
        {
            E.fr.mul(b[i], b[i], fft->root(domainPower + 1, i));
        }
        LOG_TRACE("End Shift B");
        LOG_DEBUG(E.fr.toString(b[0]).c_str());
        LOG_DEBUG(E.fr.toString(b[1]).c_str());
        LOG_TRACE("Start FFT B");
        fft->fft(b, domainSize);
        LOG_TRACE("End FFT B");
        LOG_DEBUG(E.fr.toString(b[0]).c_str());
        LOG_DEBUG(E.fr.toString(b[1]).c_str());

        LOG_TRACE("Start iFFT C");
        fft->ifft(c, domainSize);
        LOG_TRACE("End iFFT C");
        LOG_DEBUG(E.fr.toString(c[0]).c_str());
        LOG_DEBUG(E.fr.toString(c[1]).c_str());
        LOG_TRACE("Start Shift C");
#pragma omp parallel for
        for (u_int64_t i = 0; i < domainSize; i++)
        {
            E.fr.mul(c[i], c[i], fft->root(domainPower + 1, i));
        }
        LOG_TRACE("End Shift C");
        LOG_DEBUG(E.fr.toString(c[0]).c_str());
        LOG_DEBUG(E.fr.toString(c[1]).c_str());
        LOG_TRACE("Start FFT C");
        fft->fft(c, domainSize);
        LOG_TRACE("End FFT C");
        LOG_DEBUG(E.fr.toString(c[0]).c_str());
        LOG_DEBUG(E.fr.toString(c[1]).c_str());

        LOG_TRACE("Start ABC");
#pragma omp parallel for
        for (u_int64_t i = 0; i < domainSize; i++)
        {
            E.fr.mul(a[i], a[i], b[i]);
            E.fr.sub(a[i], a[i], c[i]);
            E.fr.fromMontgomery(a[i], a[i]);
        }
        LOG_TRACE("End ABC");
        LOG_DEBUG(E.fr.toString(a[0]).c_str());
        LOG_DEBUG(E.fr.toString(a[1]).c_str());

        delete[] b;
        delete[] c;
        // delete[] pointsB2;
        LOG_TRACE("Start Multiexp H");
        typename Engine::G1Point pih;
#if defined(USE_CUDA)
        g1MultiMulByScalarGpu(pih, pointsH, (uint8_t *)a, sizeof(affine_t), domainSize);
#else
        // double start_g1_h = omp_get_wtime();
        E.g1.multiMulByScalar(pih, pointsH, (uint8_t *)a, sizeof(a[0]), domainSize);
        // g1_msm += ((double)(omp_get_wtime() - start_g1_h));
#endif
        std::ostringstream ss1;
        ss1 << "pih: " << E.g1.toString(pih);
        LOG_DEBUG(ss1);

        delete[] a;

        typename Engine::FrElement r;
        typename Engine::FrElement s;
        typename Engine::FrElement rs;

        E.fr.copy(r, E.fr.zero());
        E.fr.copy(s, E.fr.zero());

        randombytes_buf((void *)&(r.v[0]), sizeof(r) - 1);
        randombytes_buf((void *)&(s.v[0]), sizeof(s) - 1);

#ifndef USE_OPENMP
        pA_future.get();
        pB1_future.get();
        pB2_future.get();
        pC_future.get();
#endif

        typename Engine::G1Point p1;
        typename Engine::G2Point p2;

        E.g1.add(pi_a, pi_a, vk_alpha1);
        E.g1.mulByScalar(p1, vk_delta1, (uint8_t *)&r, sizeof(r));
        E.g1.add(pi_a, pi_a, p1);

        E.g2.add(pi_b, pi_b, vk_beta2);
        E.g2.mulByScalar(p2, vk_delta2, (uint8_t *)&s, sizeof(s));
        E.g2.add(pi_b, pi_b, p2);

        E.g1.add(pib1, pib1, vk_beta1);
        E.g1.mulByScalar(p1, vk_delta1, (uint8_t *)&s, sizeof(s));
        E.g1.add(pib1, pib1, p1);

        E.g1.add(pi_c, pi_c, pih);

        E.g1.mulByScalar(p1, pi_a, (uint8_t *)&s, sizeof(s));
        E.g1.add(pi_c, pi_c, p1);

        E.g1.mulByScalar(p1, pib1, (uint8_t *)&r, sizeof(r));
        E.g1.add(pi_c, pi_c, p1);

        E.fr.mul(rs, r, s);
        E.fr.toMontgomery(rs, rs);

        E.g1.mulByScalar(p1, vk_delta1, (uint8_t *)&rs, sizeof(rs));
        E.g1.sub(pi_c, pi_c, p1);

        Proof<Engine> *p = new Proof<Engine>(Engine::engine);
        E.g1.copy(p->A, pi_a);
        E.g2.copy(p->B, pi_b);
        E.g1.copy(p->C, pi_c);
        // printf("time used msm g1 (ms): %.3lf\n", g1_msm * 1000);
        // printf("time used msm g2 (ms): %.3lf\n", g2_msm * 1000);
        return std::unique_ptr<Proof<Engine>>(p);
    }

    template <typename Engine>
    std::string Proof<Engine>::toJsonStr()
    {

        std::ostringstream ss;
        ss << "{ \"pi_a\":[\"" << E.f1.toString(A.x) << "\",\"" << E.f1.toString(A.y) << "\",\"1\"], ";
        ss << " \"pi_b\": [[\"" << E.f1.toString(B.x.a) << "\",\"" << E.f1.toString(B.x.b) << "\"],[\"" << E.f1.toString(B.y.a) << "\",\"" << E.f1.toString(B.y.b) << "\"], [\"1\",\"0\"]], ";
        ss << " \"pi_c\": [\"" << E.f1.toString(C.x) << "\",\"" << E.f1.toString(C.y) << "\",\"1\"], ";
        ss << " \"protocol\":\"groth16\" }";

        return ss.str();
    }

    template <typename Engine>
    json Proof<Engine>::toJson()
    {

        json p;

        p["pi_a"] = {};
        p["pi_a"].push_back(E.f1.toString(A.x));
        p["pi_a"].push_back(E.f1.toString(A.y));
        p["pi_a"].push_back("1");

        json x2;
        x2.push_back(E.f1.toString(B.x.a));
        x2.push_back(E.f1.toString(B.x.b));
        json y2;
        y2.push_back(E.f1.toString(B.y.a));
        y2.push_back(E.f1.toString(B.y.b));
        json z2;
        z2.push_back("1");
        z2.push_back("0");
        p["pi_b"] = {};
        p["pi_b"].push_back(x2);
        p["pi_b"].push_back(y2);
        p["pi_b"].push_back(z2);

        p["pi_c"] = {};
        p["pi_c"].push_back(E.f1.toString(C.x));
        p["pi_c"].push_back(E.f1.toString(C.y));
        p["pi_c"].push_back("1");

        p["protocol"] = "groth16";

        return p;
    }

}
#endif
