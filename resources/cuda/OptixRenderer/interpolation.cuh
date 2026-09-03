#include "renderer/cuda/utils/CUDAMath.h"
#include <math_constants.h>


__device__ __constant__ int HEX27_NODE_IJK[27][3] = {
    {0,0,0},{2,0,0},{2,2,0},{0,2,0},{0,0,2},{2,0,2},{2,2,2},{0,2,2},
    {1,0,0},{0,1,0},{0,0,1},{2,1,0},{2,0,1},{1,2,0},{2,2,1},{0,2,1},
    {1,0,2},{0,1,2},{2,1,2},{1,2,2},{1,1,0},{1,0,1},{0,1,1},{2,1,1},
    {1,2,1},{1,1,2},{1,1,1}
};

__device__ __constant__ int HEX64_NODE_IJK[64][3] = {
    {0,0,0},{3,0,0},{3,3,0},{0,3,0},{0,0,3},{3,0,3},{3,3,3},{0,3,3},
    {1,0,0},{2,0,0},{0,1,0},{0,2,0},{0,0,1},{0,0,2},{3,1,0},{3,2,0},
    {3,0,1},{3,0,2},{2,3,0},{1,3,0},{3,3,1},{3,3,2},{0,3,1},{0,3,2},
    {1,0,3},{2,0,3},{0,1,3},{0,2,3},{3,1,3},{3,2,3},{2,3,3},{1,3,3},
    {1,1,0},{1,2,0},{2,2,0},{2,1,0},{1,0,1},{2,0,1},{2,0,2},{1,0,2},
    {0,1,1},{0,1,2},{0,2,2},{0,2,1},{3,1,1},{3,2,1},{3,2,2},{3,1,2},
    {2,3,1},{1,3,1},{1,3,2},{2,3,2},{1,1,3},{2,1,3},{2,2,3},{1,2,3},
    {1,1,1},{2,1,1},{2,2,1},{1,2,1},{1,1,2},{2,1,2},{2,2,2},{1,2,2}
};

__device__ __constant__ int TET4_NODE_IJKL[4][4] = {
    {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}
};

__device__ __constant__ int TET10_NODE_IJKL[10][4] = {
    {2,0,0,0},{0,2,0,0},{0,0,2,0},{0,0,0,2},
    {1,1,0,0},{0,1,1,0},{1,0,1,0},{1,0,0,1},{0,0,1,1},{0,1,0,1}
};

// Please see the comment in the Cell.cpp file for the node ordering of TET20.
__device__ __constant__ int TET20_NODE_IJKL[20][4] = {
    {3,0,0,0},{0,3,0,0},{0,0,3,0},{0,0,0,3},
    {2,1,0,0},{1,2,0,0},{0,2,1,0},{0,1,2,0},{2,0,1,0},{1,0,2,0},
    {2,0,0,1},{1,0,0,2},{0,0,2,1},{0,0,1,2},{0,2,0,1},{0,1,0,2},
    {1,1,1,0},{1,1,0,1},{1,0,1,1},{0,1,1,1}
};

__device__ __constant__ float BARY_NODES_ORDER1[2] = { 0.f, 1.f };
__device__ __constant__ float BARY_NODES_ORDER2[3] = { 0.f, 0.5f, 1.f };
__device__ __constant__ float BARY_NODES_ORDER3[4] = {
    0.f, 0.333333333333f, 0.666666666667f, 1.f
};

#define MAX_HEX_NODES_1D 4   // supports up to order 3 (n = p+1 = 4 nodes/direction)
#define MAX_HEX_NODES    64  // 4^3, upper bound for order 3
#define MAX_TET_NODES_1D 4   // supports up to order 3
#define MAX_TET_NODES    20  // order 3 tet has 20 nodes
// Order 2: n = 3 nodes per direction
__device__ __constant__ float NODES_1D_ORDER2[3] = { -1.f, 0.f, 1.f };

// Order 3: n = 4 nodes per direction
__device__ __constant__ float NODES_1D_ORDER3[4] = {
    -1.f, -0.333333333333f, 0.333333333333f, 1.f
};

// ============================================================================
// Generic 1D Lagrange basis and its derivative
// ============================================================================

static __forceinline__ __device__ float lagrangeBasis1D(
    float xi, int i, const float* nodes, int n)
{
    float result = 1.f;
    for (int j = 0; j < n; ++j) {
        if (j != i) {
            result *= (xi - nodes[j]) / (nodes[i] - nodes[j]);
        }
    }
    return result;
}

// d/dxi of lagrangeBasis1D, via the standard sum-of-products rule:
// l_i'(xi) = l_i(xi) * sum_{j != i} 1/(xi - nodes[j])   -- unstable at xi = nodes[j]
// Safer form used here: direct sum of partial products
static __forceinline__ __device__ float lagrangeBasis1DDerivative(
    float xi, int i, const float* nodes, int n)
{
    float sum = 0.f;
    for (int k = 0; k < n; ++k) {
        if (k == i) continue;
        float term = 1.f / (nodes[i] - nodes[k]);
        for (int j = 0; j < n; ++j) {
            if (j != i && j != k) {
                term *= (xi - nodes[j]) / (nodes[i] - nodes[j]);
            }
        }
        sum += term;
    }
    return sum;
}

// ============================================================================
// Generic Hex shape functions and derivatives (full tensorial, GMSH ordering)
// ============================================================================

// nodeIJK: per-node (i,j,k) tensor position, size n_nodes x 3
// nodes1D: 1D reference nodes, size n
static __forceinline__ __device__ void hexNShapeFunctions(
    float g, float h, float r,
    const int nodeIJK[][3], const float* nodes1D, int n, int n_nodes,
    float* N)
{
    for (int m = 0; m < n_nodes; ++m) {
        int i = nodeIJK[m][0], j = nodeIJK[m][1], k = nodeIJK[m][2];
        N[m] = lagrangeBasis1D(g, i, nodes1D, n) *
               lagrangeBasis1D(h, j, nodes1D, n) *
               lagrangeBasis1D(r, k, nodes1D, n);
    }
}

// Partial derivatives via product rule: derivative hits exactly one of the
// three 1D factors at a time, the other two stay as plain values
static __forceinline__ __device__ void hexNShapeFunctionsDerivatives(
    float g, float h, float r,
    const int nodeIJK[][3], const float* nodes1D, int n, int n_nodes,
    float* dNdg, float* dNdh, float* dNdr)
{
    for (int m = 0; m < n_nodes; ++m) {
        int i = nodeIJK[m][0], j = nodeIJK[m][1], k = nodeIJK[m][2];

        float Li  = lagrangeBasis1D(g, i, nodes1D, n);
        float Lj  = lagrangeBasis1D(h, j, nodes1D, n);
        float Lk  = lagrangeBasis1D(r, k, nodes1D, n);
        float dLi = lagrangeBasis1DDerivative(g, i, nodes1D, n);
        float dLj = lagrangeBasis1DDerivative(h, j, nodes1D, n);
        float dLk = lagrangeBasis1DDerivative(r, k, nodes1D, n);

        dNdg[m] = dLi * Lj  * Lk;
        dNdh[m] = Li  * dLj * Lk;
        dNdr[m] = Li  * Lj  * dLk;
    }
}


static __forceinline__ __device__ float3 getHexNReferencePoint(
    const float3* verts, const int nodeIJK[][3],
    const float* nodes1D, int n, int n_nodes,
    float3 samplePoint)
{
    float3 curRef = make_float3(0.f, 0.f, 0.f);
    const int maxIter = 10;
    const float tol = 1e-5f;

    float N[MAX_HEX_NODES];
    float dNdg[MAX_HEX_NODES];
    float dNdh[MAX_HEX_NODES];
    float dNdr[MAX_HEX_NODES];

    for (int iter = 0; iter < maxIter; ++iter) {
        hexNShapeFunctions(curRef.x, curRef.y, curRef.z, nodeIJK, nodes1D, n, n_nodes, N);
        hexNShapeFunctionsDerivatives(curRef.x, curRef.y, curRef.z, nodeIJK, nodes1D, n, n_nodes, dNdg, dNdh, dNdr);

        float3 curPos = make_float3(0.f);
        float3 dPdg = make_float3(0.f);
        float3 dPdh = make_float3(0.f);
        float3 dPdr = make_float3(0.f);

        for (int m = 0; m < n_nodes; ++m) {
            curPos += N[m] * verts[m];
            dPdg   += dNdg[m] * verts[m];
            dPdh   += dNdh[m] * verts[m];
            dPdr   += dNdr[m] * verts[m];
        }

        float3 R = samplePoint - curPos;
        float err = dot(R, R);
        if (err < tol * tol) break;

        float detJ = dot(dPdg, cross(dPdh, dPdr));
        if (fabsf(detJ) < 1e-8f) break;  // singularity protection

        float invDet = 1.f / detJ;
        float3 r1 = cross(dPdh, dPdr);
        float3 r2 = cross(dPdr, dPdg);
        float3 r3 = cross(dPdg, dPdh);

        float3 delta;
        delta.x = dot(r1, R) * invDet;
        delta.y = dot(r2, R) * invDet;
        delta.z = dot(r3, R) * invDet;

        curRef += delta;

        curRef.x = fminf(fmaxf(curRef.x, -1.001f), 1.001f);
        curRef.y = fminf(fmaxf(curRef.y, -1.001f), 1.001f);
        curRef.z = fminf(fmaxf(curRef.z, -1.001f), 1.001f);
    }

    return curRef;
}

// ---------------------------------------------------------------------------
// Hex27 Full-Tensorial Interpolation (order 2)
// ---------------------------------------------------------------------------
static __forceinline__ __device__ float lagrangeInterpolationHex27(
    float3 v0,  float d0,  float3 v1,  float d1,  float3 v2,  float d2,  float3 v3,  float d3,
    float3 v4,  float d4,  float3 v5,  float d5,  float3 v6,  float d6,  float3 v7,  float d7,
    float3 v8,  float d8,  float3 v9,  float d9,  float3 v10, float d10, float3 v11, float d11,
    float3 v12, float d12, float3 v13, float d13, float3 v14, float d14, float3 v15, float d15,
    float3 v16, float d16, float3 v17, float d17, float3 v18, float d18, float3 v19, float d19,
    float3 v20, float d20, float3 v21, float d21, float3 v22, float d22, float3 v23, float d23,
    float3 v24, float d24, float3 v25, float d25, float3 v26, float d26,
    float3 samplePoint)
{
    float3 verts[27] = {
        v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,
        v14,v15,v16,v17,v18,v19,v20,v21,v22,v23,v24,v25,v26
    };
    float data[27] = {
        d0,d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,
        d14,d15,d16,d17,d18,d19,d20,d21,d22,d23,d24,d25,d26
    };

    float3 ref = getHexNReferencePoint(
        verts, HEX27_NODE_IJK, NODES_1D_ORDER2, 3, 27, samplePoint);

    float N[27];
    hexNShapeFunctions(ref.x, ref.y, ref.z, HEX27_NODE_IJK, NODES_1D_ORDER2, 3, 27, N);

    float result = 0.f;
    for (int m = 0; m < 27; ++m) result += N[m] * data[m];
    return result;
}

// ---------------------------------------------------------------------------
// Hex64 Full-Tensorial Interpolation (order 3)
// ---------------------------------------------------------------------------
static __forceinline__ __device__ float lagrangeInterpolationHex64(
    float3* verts,  // size 64, GMSH-ordered
    float* data,    // size 64, GMSH-ordered
    float3 samplePoint)
{
    float3 ref = getHexNReferencePoint(
        verts, HEX64_NODE_IJK, NODES_1D_ORDER3, 4, 64, samplePoint);

    float N[64];
    hexNShapeFunctions(ref.x, ref.y, ref.z, HEX64_NODE_IJK, NODES_1D_ORDER3, 4, 64, N);

    float result = 0.f;
    for (int m = 0; m < 64; ++m) result += N[m] * data[m];
    return result;
}

// ============================================================================
// Generic Tet shape functions and derivatives (barycentric, GMSH/Vizir order)
// ============================================================================

static __forceinline__ __device__ float simplexBasis1D(float L, int m, int p)
{
    if (m == 0) return 1.0f;
    float prod = 1.0f;
    for (int q = 0; q < m; ++q) {
        prod *= ((float)p * L - (float)q) / (float)(q + 1);
    }
    return prod;
}

static __forceinline__ __device__ float simplexBasis1DDerivative(float L, int m, int p)
{
    if (m == 0) return 0.0f;
    float sum = 0.0f;
    for (int k = 0; k < m; ++k) {
        float term = (float)p / (float)(k + 1);
        for (int q = 0; q < m; ++q) {
            if (q != k) {
                term *= ((float)p * L - (float)q) / (float)(q + 1);
            }
        }
        sum += term;
    }
    return sum;
}

static __forceinline__ __device__ void tetNShapeFunctions(
    float L1, float L2, float L3, float L4,
    const int nodeIJKL[][4], const float* baryNodes, int n, int n_nodes,
    float* N)
{
    int p = n - 1;
    for (int m = 0; m < n_nodes; ++m) {
        int i = nodeIJKL[m][0], j = nodeIJKL[m][1], k = nodeIJKL[m][2], l = nodeIJKL[m][3];
        N[m] = simplexBasis1D(L1, i, p) *
               simplexBasis1D(L2, j, p) *
               simplexBasis1D(L3, k, p) *
               simplexBasis1D(L4, l, p);
    }
}

// Derivatives w.r.t. (L2, L3, L4), with L1 = 1 - L2 - L3 - L4 substituted via chain rule:
static __forceinline__ __device__ void tetNShapeFunctionsDerivatives(
    float L1, float L2, float L3, float L4,
    const int nodeIJKL[][4], const float* baryNodes, int n, int n_nodes,
    float* dNdL2, float* dNdL3, float* dNdL4)
{
    int p = n - 1;
    for (int m = 0; m < n_nodes; ++m) {
        int i = nodeIJKL[m][0], j = nodeIJKL[m][1], k = nodeIJKL[m][2], l = nodeIJKL[m][3];

        float Li = simplexBasis1D(L1, i, p);
        float Lj = simplexBasis1D(L2, j, p);
        float Lk = simplexBasis1D(L3, k, p);
        float Ll = simplexBasis1D(L4, l, p);

        float dLi = simplexBasis1DDerivative(L1, i, p);  // dl_i/dL1
        float dLj = simplexBasis1DDerivative(L2, j, p);
        float dLk = simplexBasis1DDerivative(L3, k, p);
        float dLl = simplexBasis1DDerivative(L4, l, p);

        // Chain rule: dL1/dL2 = -1, dL1/dL3 = -1, dL1/dL4 = -1
        dNdL2[m] = (-dLi * Lj + Li * dLj) * Lk * Ll;
        dNdL3[m] = (-dLi * Lk + Li * dLk) * Lj * Ll;
        dNdL4[m] = (-dLi * Ll + Li * dLl) * Lj * Lk;
    }
}

// ============================================================================
// Generic Tet world -> barycentric mapping (Newton-Raphson)
// ============================================================================

static __forceinline__ __device__ float4 getTetNReferencePoint(
    const float3* verts, const int nodeIJKL[][4],
    const float* baryNodes, int n, int n_nodes,
    float3 samplePoint)
{
    // Unknowns: (L2, L3, L4); L1 = 1 - L2 - L3 - L4
    
    // Initial guess from 4 corner vertices (exact solution for linear tet)
    float3 e1 = verts[1] - verts[0];
    float3 e2 = verts[2] - verts[0];
    float3 e3 = verts[3] - verts[0];
    float detA = dot(e1, cross(e2, e3));

    float L2, L3, L4;
    if (fabsf(detA) > 1e-10f) {
        float invDetA = 1.0f / detA;
        float3 dP = samplePoint - verts[0];
        L2 = dot(dP, cross(e2, e3)) * invDetA; // xi
        L3 = dot(e1, cross(dP, e3)) * invDetA; // eta
        L4 = dot(e1, cross(e2, dP)) * invDetA; // zeta
    } else {
        L2 = 0.25f;
        L3 = 0.25f;
        L4 = 0.25f;
    }

    const int maxIter = 20;
    const float tol = 1e-5f;

    float N[MAX_TET_NODES];
    float dNdL2[MAX_TET_NODES], dNdL3[MAX_TET_NODES], dNdL4[MAX_TET_NODES];

    for (int iter = 0; iter < maxIter; ++iter) {
        float L1 = 1.f - L2 - L3 - L4;

        tetNShapeFunctions(L1, L2, L3, L4, nodeIJKL, baryNodes, n, n_nodes, N);
        tetNShapeFunctionsDerivatives(L1, L2, L3, L4, nodeIJKL, baryNodes, n, n_nodes, dNdL2, dNdL3, dNdL4);

        float3 curPos = make_float3(0.f);
        float3 dPdL2 = make_float3(0.f);
        float3 dPdL3 = make_float3(0.f);
        float3 dPdL4 = make_float3(0.f);

        for (int m = 0; m < n_nodes; ++m) {
            curPos += N[m] * verts[m];
            dPdL2  += dNdL2[m] * verts[m];
            dPdL3  += dNdL3[m] * verts[m];
            dPdL4  += dNdL4[m] * verts[m];
        }

        float3 R = samplePoint - curPos;
        float err = dot(R, R);
        if (err < tol * tol) break;

        float detJ = dot(dPdL2, cross(dPdL3, dPdL4));
        if (fabsf(detJ) < 1e-8f) break;

        float invDet = 1.f / detJ;
        float3 r1 = cross(dPdL3, dPdL4);
        float3 r2 = cross(dPdL4, dPdL2);
        float3 r3 = cross(dPdL2, dPdL3);

        L2 += dot(r1, R) * invDet;
        L3 += dot(r2, R) * invDet;
        L4 += dot(r3, R) * invDet;

    }

    // Clamp to valid barycentric range
    L2 = fminf(fmaxf(L2, -0.001f), 1.001f);
    L3 = fminf(fmaxf(L3, -0.001f), 1.001f);
    L4 = fminf(fmaxf(L4, -0.001f), 1.001f);

    float L1 = 1.f - L2 - L3 - L4;
    return make_float4(L1, L2, L3, L4);
}

// ---------------------------------------------------------------------------
// Tet4 Linear Interpolation (order 1) — trivial closed-form, no NR needed
// ---------------------------------------------------------------------------
static __forceinline__ __device__ float lagrangeInterpolationTet4(
    float3* verts, float* data, float3 samplePoint)
{
    float4 bary = getTetNReferencePoint(verts, TET4_NODE_IJKL, BARY_NODES_ORDER1, 2, 4, samplePoint);
    float N[4];
    tetNShapeFunctions(bary.x, bary.y, bary.z, bary.w, TET4_NODE_IJKL, BARY_NODES_ORDER1, 2, 4, N);
    float result = 0.f;
    for (int m = 0; m < 4; ++m) result += N[m] * data[m];
    return result;
}

// ---------------------------------------------------------------------------
// Tet10 Interpolation (order 2), generic Lagrange basis
// ---------------------------------------------------------------------------
static __forceinline__ __device__ float lagrangeInterpolationTet10(
    float3 v0,  float d0,  float3 v1,  float d1,  float3 v2,  float d2,  float3 v3,  float d3,
    float3 v4,  float d4,  float3 v5,  float d5,  float3 v6,  float d6,  float3 v7,  float d7,
    float3 v8,  float d8,  float3 v9,  float d9,
    float3 samplePoint)
{
    float3 verts[10] = { v0,v1,v2,v3,v4,v5,v6,v7,v8,v9 };
    float data[10]   = { d0,d1,d2,d3,d4,d5,d6,d7,d8,d9 };

    float4 bary = getTetNReferencePoint(verts, TET10_NODE_IJKL, BARY_NODES_ORDER2, 3, 10, samplePoint);

    float N[10];
    tetNShapeFunctions(bary.x, bary.y, bary.z, bary.w, TET10_NODE_IJKL, BARY_NODES_ORDER2, 3, 10, N);

    float result = 0.f;
    for (int m = 0; m < 10; ++m) result += N[m] * data[m];
    return result;
}

// ---------------------------------------------------------------------------
// Tet20 Interpolation (order 3)
// ---------------------------------------------------------------------------
static __forceinline__ __device__ float lagrangeInterpolationTet20(
    float3 v0,  float d0,  float3 v1,  float d1,  float3 v2,  float d2,  float3 v3,  float d3,
    float3 v4,  float d4,  float3 v5,  float d5,  float3 v6,  float d6,  float3 v7,  float d7,
    float3 v8,  float d8,  float3 v9,  float d9,  float3 v10, float d10, float3 v11, float d11,
    float3 v12, float d12, float3 v13, float d13, float3 v14, float d14, float3 v15, float d15,
    float3 v16, float d16, float3 v17, float d17, float3 v18, float d18, float3 v19, float d19,
    float3 samplePoint)
{
    float3 verts[20] = { v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15,v16,v17,v18,v19 };
    float data[20]   = { d0,d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,d14,d15,d16,d17,d18,d19 };

    float4 bary = getTetNReferencePoint(verts, TET20_NODE_IJKL, BARY_NODES_ORDER3, 4, 20, samplePoint);

    float N[20];
    tetNShapeFunctions(bary.x, bary.y, bary.z, bary.w, TET20_NODE_IJKL, BARY_NODES_ORDER3, 4, 20, N);

    float result = 0.f;
    for (int m = 0; m < 20; ++m) result += N[m] * data[m];
    return result;
}

static __forceinline__ __device__ float tetrahedronVolume(
    float3 v0, 
    float3 v1, 
    float3 v2, 
    float3 v3)
{
    return 0.1666f*fabsf(dot(v3 - v0, cross(v3 - v1, v3 - v2)));
}

static __forceinline__ __device__ float angle(float3 v0, float3 v1)
{
    return acos(dot(v0, v1)/(length(v0)*length(v1)));
}

static __forceinline__ __device__ void triangularMVC(
    float3 samplePoint, 
    float3 v0, 
    float3 v1, 
    float3 v2,
    float* w0,
    float* w1,
    float* w2)
{
	float3 e0 = v0 - samplePoint;
	float3 e1 = v1 - samplePoint;
	float3 e2 = v2 - samplePoint;

	float3 n0 = normalize(cross(e1, e2));
	float3 n1 = normalize(cross(e2, e0));
    float3 n2 = normalize(cross(e0, e1));

	float theta0 = angle(e0, e1);
	float theta1 = angle(e1, e2);
	float theta2 = angle(e2, e0);

	float3 m = .5f*(theta0*n0 + theta1*n1 + theta2*n2);

	*w0 += dot(n0, m)/dot(n0, e0);
	*w1 += dot(n1, m)/dot(n1, e1);
	*w2 += dot(n2, m)/dot(n2, e2);
}

static __forceinline__ __device__ float linearInterpolation(
    float3 v0, float d0, 
    float3 v1, float d1, 
    float3 v2, float d2, 
    float3 v3, float d3,
    float3 samplePoint)
{
    float w0 = 0.f;
    float w1 = 0.f;
    float w2 = 0.f;
    float w3 = 0.f;
    float totalWeight = 0.f;

    // Compute weights using tetrahedron volumes
    w0 = tetrahedronVolume(samplePoint, v1, v2, v3);
    w1 = tetrahedronVolume(samplePoint, v3, v2, v0);
    w2 = tetrahedronVolume(samplePoint, v0, v1, v3);
    w3 = tetrahedronVolume(samplePoint, v0, v2, v1);

    // Compute total weight
    totalWeight =
        w0 +
        w1 +
        w2 +
        w3;

    // Return interpolated value
    return (
        w0*d0 + 
        w1*d1 + 
        w2*d2 + 
        w3*d3
        )/totalWeight;
}

static __forceinline__ __device__ float linearInterpolation(
    float3 v0, float d0, 
    float3 v1, float d1, 
    float3 v2, float d2, 
    float3 v3, float d3,
    float3 v4, float d4, 
    float3 v5, float d5, 
    float3 v6, float d6, 
    float3 v7, float d7, 
    float3 samplePoint)
{
    float w0 = 0.f;
    float w1 = 0.f;
    float w2 = 0.f;
    float w3 = 0.f;
    float w4 = 0.f;
    float w5 = 0.f;
    float w6 = 0.f;
    float w7 = 0.f;
    float totalWeight = 0.f;

    // Compute weights using mean value coordinates
    triangularMVC(samplePoint, v1, v0, v2, &w1, &w0, &w2);
    triangularMVC(samplePoint, v2, v0, v3, &w2, &w0, &w3);

    triangularMVC(samplePoint, v2, v3, v6, &w2, &w3, &w6);
    triangularMVC(samplePoint, v6, v3, v7, &w6, &w3, &w7);

    triangularMVC(samplePoint, v5, v1, v6, &w5, &w1, &w6);
    triangularMVC(samplePoint, v6, v1, v2, &w6, &w1, &w2);

    triangularMVC(samplePoint, v0, v4, v3, &w0, &w4, &w3);
    triangularMVC(samplePoint, v3, v4, v7, &w3, &w4, &w7);

    triangularMVC(samplePoint, v5, v4, v1, &w5, &w4, &w1);
    triangularMVC(samplePoint, v1, v4, v0, &w1, &w4, &w0);

    triangularMVC(samplePoint, v6, v7, v5, &w6, &w7, &w5);
    triangularMVC(samplePoint, v5, v7, v4, &w5, &w7, &w4);

    // Compute total weight
    totalWeight =
        w0 +
        w1 +
        w2 +
        w3 +
        w4 +
        w5 +
        w6 +
        w7;

    // Return interpolated value
    return (
        w0*d0 + 
        w1*d1 + 
        w2*d2 +
        w3*d3 + 
        w4*d4 + 
        w5*d5 +
        w6*d6 + 
        w7*d7
        )/totalWeight;
}

static __forceinline__ __device__ float linearInterpolation(
    float3 v0, float d0, 
    float3 v1, float d1, 
    float3 v2, float d2, 
    float3 v3, float d3,
    float3 v4, float d4, 
    float3 v5, float d5, 
    float3 samplePoint)
{
    float w0 = 0.f;
    float w1 = 0.f;
    float w2 = 0.f;
    float w3 = 0.f;
    float w4 = 0.f;
    float w5 = 0.f;
    float totalWeight = 0.f;

    // Compute weights using mean value coordinates
    triangularMVC(samplePoint, v0, v2, v1, &w0, &w2, &w1);

    triangularMVC(samplePoint, v3, v4, v5, &w3, &w4, &w5);

    triangularMVC(samplePoint, v1, v2, v5, &w1, &w2, &w5);
    triangularMVC(samplePoint, v1, v5, v4, &w1, &w5, &w4);

    triangularMVC(samplePoint, v0, v1, v4, &w0, &w1, &w4);
    triangularMVC(samplePoint, v0, v4, v3, &w0, &w4, &w3);

    triangularMVC(samplePoint, v0, v3, v5, &w0, &w3, &w5);
    triangularMVC(samplePoint, v0, v5, v2, &w0, &w5, &w2);

    // Compute total weight
    totalWeight =
        w0 +
        w1 +
        w2 +
        w3 +
        w4 +
        w5;

    // Return interpolated value
    return (
        w0*d0 + 
        w1*d1 + 
        w2*d2 +
        w3*d3 + 
        w4*d4 + 
        w5*d5
        )/totalWeight;
}

static __forceinline__ __device__ float linearInterpolation(
    float3 v0, float d0, 
    float3 v1, float d1, 
    float3 v2, float d2, 
    float3 v3, float d3,
    float3 v4, float d4, 
    float3 samplePoint)
{
    float w0 = 0.f;
    float w1 = 0.f;
    float w2 = 0.f;
    float w3 = 0.f;
    float w4 = 0.f;
    float totalWeight = 0.f;

    // Compute weights using mean value coordinates
    triangularMVC(samplePoint, v0, v1, v4, &w0, &w1, &w4);

    triangularMVC(samplePoint, v1, v2, v4, &w1, &w2, &w4);

    triangularMVC(samplePoint, v2, v3, v4, &w2, &w3, &w4);

    triangularMVC(samplePoint, v3, v0, v4, &w3, &w0, &w4);

    triangularMVC(samplePoint, v0, v2, v1, &w0, &w2, &w1);
    triangularMVC(samplePoint, v0, v3, v2, &w0, &w3, &w2);

    // Compute total weight
    totalWeight =
        w0 +
        w1 +
        w2 +
        w3 +
        w4;

    // Return interpolated value
    return (
        w0*d0 + 
        w1*d1 + 
        w2*d2 +
        w3*d3 + 
        w4*d4
        )/totalWeight;
}

static __forceinline__ __device__ float lagrangeInterpolation(
    float d0, float d1, float d2, float d3,
    float d4, float d5, float d6, float d7,
    float d8, float d9,
    float3 samplePoint)
{
    float g = samplePoint.x;
    float h = samplePoint.y;
    float r = samplePoint.z;

    float L1 = 1.f - g - h - r;
    float L2 = g;
    float L3 = h;
    float L4 = r;

    float N0 = L1*(2.f*L1 - 1.f);
    float N1 = L2*(2.f*L2 - 1.f);
    float N2 = L3*(2.f*L3 - 1.f);
    float N3 = L4*(2.f*L4 - 1.f);

    float N4 = 4.f*L1*L2;
    float N5 = 4.f*L2*L3;
    float N6 = 4.f*L3*L1;
    float N7 = 4.f*L1*L4;
    float N8 = 4.f*L3*L4;
    float N9 = 4.f*L2*L4;

    return
        d0*N0 + d1*N1 + d2*N2 + d3*N3 +
        d4*N4 + d5*N5 + d6*N6 + d7*N7 + d8*N8 + d9*N9;
}

static __forceinline__ __device__ float lagrangeInterpolation(
    float d0, 
    float d1, 
    float d2, 
    float d3,
    float d4, 
    float d5,
    float d6, 
    float d7, 
    float d8, 
    float d9, 
    float d10, 
    float d11, 
    float d12, 
    float d13, 
    float d14, 
    float d15, 
    float d16, 
    float d17, 
    float d18, 
    float d19, 
    float3 samplePoint)
{
    // Reference space [g, h, r]
    float g = samplePoint.x;
    float h = samplePoint.y;
    float r = samplePoint.z;

    // Lagrangian interpolation
    return
        d0*(-.125f*(1.f - g)*(1.f - h)*(1.f - r)*(2.f + g + h + r)) +
        d1*(-.125f*(1.f + g)*(1.f - h)*(1.f - r)*(2.f - g + h + r)) +
        d2*(-.125f*(1.f + g)*(1.f + h)*(1.f - r)*(2.f - g - h + r)) +
        d3*(-.125f*(1.f - g)*(1.f + h)*(1.f - r)*(2.f + g - h + r)) +
        d4*(-.125f*(1.f - g)*(1.f - h)*(1.f + r)*(2.f + g + h - r)) +
        d5*(-.125f*(1.f + g)*(1.f - h)*(1.f + r)*(2.f - g + h - r)) +
        d6*(-.125f*(1.f + g)*(1.f + h)*(1.f + r)*(2.f - g - h - r)) +
        d7*(-.125f*(1.f - g)*(1.f + h)*(1.f + r)*(2.f + g - h - r)) +
        d8*(0.25f*(1.f - g)*(1.f + g)*(1.f - h)*(1.f - r)) +
        d9*(0.25f*(1.f - h)*(1.f + h)*(1.f - g)*(1.f - r)) +
        d10*(0.25f*(1.f - r)*(1.f + r)*(1.f - g)*(1.f - h)) +
        d11*(0.25f*(1.f - h)*(1.f + h)*(1.f + g)*(1.f - r)) +
        d12*(0.25f*(1.f - r)*(1.f + r)*(1.f + g)*(1.f - h)) +
        d13*(0.25f*(1.f - g)*(1.f + g)*(1.f + h)*(1.f - r)) +
        d14*(0.25f*(1.f - r)*(1.f + r)*(1.f + g)*(1.f + h)) +
        d15*(0.25f*(1.f - r)*(1.f + r)*(1.f - g)*(1.f + h)) +
        d16*(0.25f*(1.f - g)*(1.f + g)*(1.f - h)*(1.f + r)) +
        d17*(0.25f*(1.f - h)*(1.f + h)*(1.f - g)*(1.f + r)) +
        d18*(0.25f*(1.f - h)*(1.f + h)*(1.f + g)*(1.f + r)) +
        d19*(0.25f*(1.f - g)*(1.f + g)*(1.f + h)*(1.f + r));
}

/* static __forceinline__ __device__ void tet10ShapeFunctions(
    float xi, float eta, float zeta,
    float* N)
{
    float L1 = 1.0f - xi - eta - zeta;
    float L2 = xi;
    float L3 = eta;
    float L4 = zeta;

    N[0] = L1 * (2.0f * L1 - 1.0f);
    N[1] = L2 * (2.0f * L2 - 1.0f);
    N[2] = L3 * (2.0f * L3 - 1.0f);
    N[3] = L4 * (2.0f * L4 - 1.0f);

    N[4] = 4.0f * L1 * L2;
    N[5] = 4.0f * L2 * L3;
    N[6] = 4.0f * L1 * L3;
    N[7] = 4.0f * L1 * L4;
    N[8] = 4.0f * L3 * L4;
    N[9] = 4.0f * L2 * L4;
}

static __forceinline__ __device__ void tet10ShapeFunctionDerivatives(
    float xi, float eta, float zeta,
    float* dNdxi, float* dNdeta, float* dNdzeta)
{
    float L1 = 1.0f - xi - eta - zeta;
    float L2 = xi;
    float L3 = eta;
    float L4 = zeta;

    dNdxi[0]   = -(4.0f * L1 - 1.0f);
    dNdeta[0]  = -(4.0f * L1 - 1.0f);
    dNdzeta[0] = -(4.0f * L1 - 1.0f);

    dNdxi[1]   =  4.0f * L2 - 1.0f;
    dNdeta[1]  =  0.0f;
    dNdzeta[1] =  0.0f;

    dNdxi[2]   =  0.0f;
    dNdeta[2]  =  4.0f * L3 - 1.0f;
    dNdzeta[2] =  0.0f;

    dNdxi[3]   =  0.0f;
    dNdeta[3]  =  0.0f;
    dNdzeta[3] =  4.0f * L4 - 1.0f;

    dNdxi[4]   =  4.0f * (L1 - L2);
    dNdeta[4]  = -4.0f * L2;
    dNdzeta[4] = -4.0f * L2;

    dNdxi[5]   =  4.0f * L3;
    dNdeta[5]  =  4.0f * L2;
    dNdzeta[5] =  0.0f;

    dNdxi[6]   = -4.0f * L3;
    dNdeta[6]  =  4.0f * (L1 - L3);
    dNdzeta[6] = -4.0f * L3;

    dNdxi[7]   = -4.0f * L4;
    dNdeta[7]  = -4.0f * L4;
    dNdzeta[7] =  4.0f * (L1 - L4);

    dNdxi[8]   =  4.0f * L4;
    dNdeta[8]  =  0.0f;
    dNdzeta[8] =  4.0f * L3;

    dNdxi[9]   =  0.0f;
    dNdeta[9]  =  4.0f * L4;
    dNdzeta[9] =  4.0f * L2;
}

static __forceinline__ __device__ float3 getTet10ReferencePoint(
    float3 v0, float3 v1, float3 v2, float3 v3,
    float3 v4, float3 v5, float3 v6, float3 v7, float3 v8, float3 v9,
    float3 samplePoint)
{
    float3 verts[10] = { v0,v1,v2,v3,v4,v5,v6,v7,v8,v9 };

    // Initial guess from 4 corner vertices (exact solution for linear tet)
    float3 e1 = v1 - v0;
    float3 e2 = v2 - v0;
    float3 e3 = v3 - v0;
    float detA = dot(e1, cross(e2, e3));

    float3 curRef;
    if (fabsf(detA) > 1e-10f) {
        float invDetA = 1.0f / detA;
        float3 dP = samplePoint - v0;
        curRef.x = dot(dP, cross(e2, e3)) * invDetA; // xi
        curRef.y = dot(e1, cross(dP, e3)) * invDetA; // eta
        curRef.z = dot(e1, cross(e2, dP)) * invDetA; // zeta
    } else {
        curRef = make_float3(0.25f, 0.25f, 0.25f);
    }

    const int maxIter = 10;
    const float tol = 1e-6f;

    float N[10], dNdxi[10], dNdeta[10], dNdzeta[10];

    for (int iter = 0; iter < maxIter; ++iter) {
        tet10ShapeFunctions(curRef.x, curRef.y, curRef.z, N);
        tet10ShapeFunctionDerivatives(curRef.x, curRef.y, curRef.z, dNdxi, dNdeta, dNdzeta);

        float3 curPos = make_float3(0.f, 0.f, 0.f);
        float3 dPdxi  = make_float3(0.f, 0.f, 0.f);
        float3 dPdeta = make_float3(0.f, 0.f, 0.f);
        float3 dPdzeta= make_float3(0.f, 0.f, 0.f);

        for (int i = 0; i < 10; ++i) {
            curPos  += N[i]      * verts[i];
            dPdxi   += dNdxi[i]  * verts[i];
            dPdeta  += dNdeta[i] * verts[i];
            dPdzeta += dNdzeta[i]* verts[i];
        }

        float3 R = samplePoint - curPos;
        if (dot(R, R) < tol * tol) break;

        float detJ = dot(dPdxi, cross(dPdeta, dPdzeta));
        if (fabsf(detJ) < 1e-10f) break;

        float invDet = 1.0f / detJ;
        float3 r1 = cross(dPdeta, dPdzeta);
        float3 r2 = cross(dPdzeta, dPdxi);
        float3 r3 = cross(dPdxi, dPdeta);

        float3 delta;
        delta.x = dot(r1, R) * invDet;
        delta.y = dot(r2, R) * invDet;
        delta.z = dot(r3, R) * invDet;

        curRef += delta;
    }

    // Clamp final reference point to valid barycentric range
    curRef.x = fmaxf(0.f, fminf(1.f, curRef.x));
    curRef.y = fmaxf(0.f, fminf(1.f, curRef.y));
    curRef.z = fmaxf(0.f, fminf(1.f, curRef.z));

    return curRef;
}

// ---------------------------------------------------------------------------
// Tet10 Serendipity Interpolation — C0-continuous across element boundaries
// ---------------------------------------------------------------------------

static __forceinline__ __device__ float serendipityInterpolation(
    float3 v0,  float d0,
    float3 v1,  float d1,
    float3 v2,  float d2,
    float3 v3,  float d3,
    float3 v4,  float d4,
    float3 v5,  float d5,
    float3 v6,  float d6,
    float3 v7,  float d7,
    float3 v8,  float d8,
    float3 v9,  float d9,
    float3 samplePoint)
{
    // -- 1. Map world → reference using the Newton-Raphson solver --------
    float3 ref = getTet10ReferencePoint(
        v0,v1,v2,v3,v4,v5,v6,v7,v8,v9, samplePoint);

    float g = ref.x;   // ξ
    float h = ref.y;   // η
    float r = ref.z;   // ζ

    float N[10];
    tet10ShapeFunctions(g, h, r, N);

    float data[10] = { d0,d1,d2,d3,d4,d5,d6,d7,d8,d9 };

    float val = 0.0f;
    #pragma unroll
    for (int i = 0; i < 10; ++i)
        val += N[i] * data[i];

    return val;
} */

static __forceinline__ __device__ void hex20ShapeFunctions(
    float g, float h, float r, 
    float* N)
{
    // Corner nodes (0-7)
    // N_i = 1/8 * (1 + g_i*g) * (1 + h_i*h) * (1 + r_i*r) * (g_i*g + h_i*h + r_i*r - 2)
    float g_i[] = {-1,  1,  1, -1, -1,  1,  1, -1};
    float h_i[] = {-1, -1,  1,  1, -1, -1,  1,  1};
    float r_i[] = {-1, -1, -1, -1,  1,  1,  1,  1};

    for(int i=0; i<8; ++i) {
        float gg = 1.f + g_i[i]*g;
        float hh = 1.f + h_i[i]*h;
        float rr = 1.f + r_i[i]*r;
        N[i] = 0.125f * gg * hh * rr * (g_i[i]*g + h_i[i]*h + r_i[i]*r - 2.f);
    }

    // Edge nodes (8-19)
    // 8: (0, -1, -1) -> 1/4 * (1-g^2)(1-h)(1-r)
    N[8]  = 0.25f * (1.f - g*g) * (1.f - h) * (1.f - r);
    // 9: (-1, 0, -1) -> 1/4 * (1-g)(1-h^2)(1-r)
    N[9]  = 0.25f * (1.f - g) * (1.f - h*h) * (1.f - r);
    // 10: (-1, -1, 0) -> 1/4 * (1-g)(1-h)(1-r^2)
    N[10] = 0.25f * (1.f - g) * (1.f - h) * (1.f - r*r);
    // 11: (1, 0, -1) -> 1/4 * (1+g)(1-h^2)(1-r)
    N[11] = 0.25f * (1.f + g) * (1.f - h*h) * (1.f - r);
    // 12: (1, -1, 0) -> 1/4 * (1+g)(1-h)(1-r^2)
    N[12] = 0.25f * (1.f + g) * (1.f - h) * (1.f - r*r);
    // 13: (0, 1, -1) -> 1/4 * (1-g^2)(1+h)(1-r)
    N[13] = 0.25f * (1.f - g*g) * (1.f + h) * (1.f - r);
    // 14: (1, 1, 0) -> 1/4 * (1+g)(1+h)(1-r^2)
    N[14] = 0.25f * (1.f + g) * (1.f + h) * (1.f - r*r);
    // 15: (-1, 1, 0) -> 1/4 * (1-g)(1+h)(1-r^2)
    N[15] = 0.25f * (1.f - g) * (1.f + h) * (1.f - r*r);
    // 16: (0, -1, 1) -> 1/4 * (1-g^2)(1-h)(1+r)
    N[16] = 0.25f * (1.f - g*g) * (1.f - h) * (1.f + r);
    // 17: (-1, 0, 1) -> 1/4 * (1-g)(1-h^2)(1+r)
    N[17] = 0.25f * (1.f - g) * (1.f - h*h) * (1.f + r);
    // 18: (1, 0, 1) -> 1/4 * (1+g)(1-h^2)(1+r)
    N[18] = 0.25f * (1.f + g) * (1.f - h*h) * (1.f + r);
    // 19: (0, 1, 1) -> 1/4 * (1-g^2)(1+h)(1+r)
    N[19] = 0.25f * (1.f - g*g) * (1.f + h) * (1.f + r);
}

static __forceinline__ __device__ void hex20ShapeFunctionsDerivatives(
    float g, float h, float r, 
    float* dNdg, float* dNdh, float* dNdr)
{
    float g_i[] = {-1,  1,  1, -1, -1,  1,  1, -1};
    float h_i[] = {-1, -1,  1,  1, -1, -1,  1,  1};
    float r_i[] = {-1, -1, -1, -1,  1,  1,  1,  1};

    // Corners
    for(int i=0; i<8; ++i) {
        float gg = 1.f + g_i[i]*g;
        float hh = 1.f + h_i[i]*h;
        float rr = 1.f + r_i[i]*r;
        
        // dN/dg = 1/8 * hh * rr * (g_i * (g_i*g + h_i*h + r_i*r - 2) + gg * g_i)
        //       = 1/8 * hh * rr * g_i * (2*g_i*g + h_i*h + r_i*r - 1)
        dNdg[i] = 0.125f * hh * rr * g_i[i] * (2.f*g_i[i]*g + h_i[i]*h + r_i[i]*r - 1.f);
        dNdh[i] = 0.125f * gg * rr * h_i[i] * (2.f*h_i[i]*h + g_i[i]*g + r_i[i]*r - 1.f);
        dNdr[i] = 0.125f * gg * hh * r_i[i] * (2.f*r_i[i]*r + g_i[i]*g + h_i[i]*h - 1.f);
    }

    // Edges
    // 8: (0, -1, -1) -> 1/4 * (1-g^2)(1-h)(1-r)
    dNdg[8] = -0.5f * g * (1.f - h) * (1.f - r);
    dNdh[8] = -0.25f * (1.f - g*g) * (1.f - r);
    dNdr[8] = -0.25f * (1.f - g*g) * (1.f - h);

    // 9: (-1, 0, -1) -> 1/4 * (1-g)(1-h^2)(1-r)
    dNdg[9] = -0.25f * (1.f - h*h) * (1.f - r);
    dNdh[9] = -0.5f * (1.f - g) * h * (1.f - r);
    dNdr[9] = -0.25f * (1.f - g) * (1.f - h*h);

    // 10: (-1, -1, 0) -> 1/4 * (1-g)(1-h)(1-r^2)
    dNdg[10] = -0.25f * (1.f - h) * (1.f - r*r);
    dNdh[10] = -0.25f * (1.f - g) * (1.f - r*r);
    dNdr[10] = -0.5f * (1.f - g) * (1.f - h) * r;

    // 11: (1, 0, -1) -> 1/4 * (1+g)(1-h^2)(1-r)
    dNdg[11] = 0.25f * (1.f - h*h) * (1.f - r);
    dNdh[11] = -0.5f * (1.f + g) * h * (1.f - r);
    dNdr[11] = -0.25f * (1.f + g) * (1.f - h*h);

    // 12: (1, -1, 0) -> 1/4 * (1+g)(1-h)(1-r^2)
    dNdg[12] = 0.25f * (1.f - h) * (1.f - r*r);
    dNdh[12] = -0.25f * (1.f + g) * (1.f - r*r);
    dNdr[12] = -0.5f * (1.f + g) * (1.f - h) * r;

    // 13: (0, 1, -1) -> 1/4 * (1-g^2)(1+h)(1-r)
    dNdg[13] = -0.5f * g * (1.f + h) * (1.f - r);
    dNdh[13] = 0.25f * (1.f - g*g) * (1.f - r);
    dNdr[13] = -0.25f * (1.f - g*g) * (1.f + h);

    // 14: (1, 1, 0) -> 1/4 * (1+g)(1+h)(1-r^2)
    dNdg[14] = 0.25f * (1.f + h) * (1.f - r*r);
    dNdh[14] = 0.25f * (1.f + g) * (1.f - r*r);
    dNdr[14] = -0.5f * (1.f + g) * (1.f + h) * r;

    // 15: (-1, 1, 0) -> 1/4 * (1-g)(1+h)(1-r^2)
    dNdg[15] = -0.25f * (1.f + h) * (1.f - r*r);
    dNdh[15] = 0.25f * (1.f - g) * (1.f - r*r);
    dNdr[15] = -0.5f * (1.f - g) * (1.f + h) * r;

    // 16: (0, -1, 1) -> 1/4 * (1-g^2)(1-h)(1+r)
    dNdg[16] = -0.5f * g * (1.f - h) * (1.f + r);
    dNdh[16] = -0.25f * (1.f - g*g) * (1.f + r);
    dNdr[16] = 0.25f * (1.f - g*g) * (1.f - h);

    // 17: (-1, 0, 1) -> 1/4 * (1-g)(1-h^2)(1+r)
    dNdg[17] = -0.25f * (1.f - h*h) * (1.f + r);
    dNdh[17] = -0.5f * (1.f - g) * h * (1.f + r);
    dNdr[17] = 0.25f * (1.f - g) * (1.f - h*h);

    // 18: (1, 0, 1) -> 1/4 * (1+g)(1-h^2)(1+r)
    dNdg[18] = 0.25f * (1.f - h*h) * (1.f + r);
    dNdh[18] = -0.5f * (1.f + g) * h * (1.f + r);
    dNdr[18] = 0.25f * (1.f + g) * (1.f - h*h);

    // 19: (0, 1, 1) -> 1/4 * (1-g^2)(1+h)(1+r)
    dNdg[19] = -0.5f * g * (1.f + h) * (1.f + r);
    dNdh[19] = 0.25f * (1.f - g*g) * (1.f + r);
    dNdr[19] = 0.25f * (1.f - g*g) * (1.f + h);
}

static __forceinline__ __device__ float3 getHex20ReferencePoint(
    float3 v0, 
    float3 v1, 
    float3 v2, 
    float3 v3,
    float3 v4, 
    float3 v5, 
    float3 v6, 
    float3 v7, 
    float3 v8, 
    float3 v9, 
    float3 v10, 
    float3 v11, 
    float3 v12, 
    float3 v13, 
    float3 v14, 
    float3 v15, 
    float3 v16, 
    float3 v17, 
    float3 v18, 
    float3 v19, 
    float3 samplePoint)
{
    // Newton-Raphson iteration
    float3 curRef = make_float3(0.f, 0.f, 0.f); 
    const int maxIter = 10;
    const float tol = 1e-5f;

    float3 verts[20] = {
        v0, v1, v2, v3, v4, v5, v6, v7, 
        v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19
    };

    float N[20];
    float dNdg[20];
    float dNdh[20];
    float dNdr[20];

    for(int iter=0; iter<maxIter; ++iter) {
        // Evaluate shape functions and derivatives
        hex20ShapeFunctions(curRef.x, curRef.y, curRef.z, N);
        hex20ShapeFunctionsDerivatives(curRef.x, curRef.y, curRef.z, dNdg, dNdh, dNdr);

        // Compute current world position and Jacobian
        float3 curPos = make_float3(0.f);
        float3 dPdg = make_float3(0.f);
        float3 dPdh = make_float3(0.f);
        float3 dPdr = make_float3(0.f);

        for(int i=0; i<20; ++i) {
            curPos += N[i] * verts[i];
            dPdg += dNdg[i] * verts[i];
            dPdh += dNdh[i] * verts[i];
            dPdr += dNdr[i] * verts[i];
        }

        // Residual
        float3 R = samplePoint - curPos;
        float err = dot(R, R);
        if(err < tol*tol) break;

        // Solve J * delta = R => delta = J^-1 * R
        // J = [dPdg, dPdh, dPdr] (columns)
        
        // Cramer's rule for 3x3 inverse (or linear solve)
        // det(J) = dot(dPdg, cross(dPdh, dPdr))
        float detJ = dot(dPdg, cross(dPdh, dPdr));
        if(fabsf(detJ) < 1e-8f) break; // Singularity protection

        float invDet = 1.f / detJ;
        
        // Columns of J^-1 are rows of J^(-T) ? No.
        // J = [c1 c2 c3]
        // J^-1 * R = (1/det) * adj(J) * R
        // adj(J) rows:
        // r1 = cross(c2, c3)
        // r2 = cross(c3, c1)
        // r3 = cross(c1, c2)
        
        float3 r1 = cross(dPdh, dPdr);
        float3 r2 = cross(dPdr, dPdg);
        float3 r3 = cross(dPdg, dPdh);

        float3 delta;
        delta.x = dot(r1, R) * invDet;
        delta.y = dot(r2, R) * invDet;
        delta.z = dot(r3, R) * invDet;

        curRef += delta;
        
        // Clamp to [-1, 1] to avoid divergence? 
        // Usually NR should converge if initial guess is good (0,0,0).
        // Clamping might help if we overshoot.
        curRef.x = fminf(fmaxf(curRef.x, -1.001f), 1.001f);
        curRef.y = fminf(fmaxf(curRef.y, -1.001f), 1.001f);
        curRef.z = fminf(fmaxf(curRef.z, -1.001f), 1.001f);
    }
    
    return curRef;
}

// Alternative Newton-Raphson using the same monomial basis as the M matrix:
//   phi = { 1, g, r, h, g^2, g*r, r^2, g*h, h*r, h^2 }
// The coordinate mapping is fitted as:
//   P(g,h,r) = sum_i c_i * phi_i(g,h,r)
// where c_x = M+ * vx,  c_y = M+ * vy,  c_z = M+ * vz.
// The Jacobian columns are: dP/dg, dP/dh, dP/dr, all computable analytically.
static __forceinline__ __device__ float3 getHex20ReferencePointMonomial(
    float3 v0, 
    float3 v1, 
    float3 v2, 
    float3 v3,
    float3 v4, 
    float3 v5, 
    float3 v6, 
    float3 v7, 
    float3 v8, 
    float3 v9, 
    float3 v10, 
    float3 v11, 
    float3 v12, 
    float3 v13, 
    float3 v14, 
    float3 v15, 
    float3 v16, 
    float3 v17, 
    float3 v18, 
    float3 v19, 
    float* M_plus,
    float3 samplePoint)
{
    // Build coordinate arrays from vertex positions
    float vx[20] = {
        v0.x,  v1.x,  v2.x,  v3.x,  v4.x,  v5.x,  v6.x,  v7.x,
        v8.x,  v9.x,  v10.x, v11.x, v12.x, v13.x, v14.x, v15.x,
        v16.x, v17.x, v18.x, v19.x
    };
    float vy[20] = {
        v0.y,  v1.y,  v2.y,  v3.y,  v4.y,  v5.y,  v6.y,  v7.y,
        v8.y,  v9.y,  v10.y, v11.y, v12.y, v13.y, v14.y, v15.y,
        v16.y, v17.y, v18.y, v19.y
    };
    float vz[20] = {
        v0.z,  v1.z,  v2.z,  v3.z,  v4.z,  v5.z,  v6.z,  v7.z,
        v8.z,  v9.z,  v10.z, v11.z, v12.z, v13.z, v14.z, v15.z,
        v16.z, v17.z, v18.z, v19.z
    };

    // Compute polynomial coefficients for each coordinate: c_i = M+ * v_coord
    // Monomial order: 1, g, r, h, g^2, g*r, r^2, g*h, h*r, h^2  (same as M matrix)
    float c_x[10] = {0.f};
    float c_y[10] = {0.f};
    float c_z[10] = {0.f};
    for(int i=0; i<10; ++i) {
        for(int k=0; k<20; ++k) {
            c_x[i] += M_plus[i*20 + k] * vx[k];
            c_y[i] += M_plus[i*20 + k] * vy[k];
            c_z[i] += M_plus[i*20 + k] * vz[k];
        }
    }

    // Newton-Raphson iteration
    // curRef = (g, h, r) -> (curRef.x, curRef.y, curRef.z)
    // Monomial basis (matches CPU M-matrix order): { 1, g, r, h, g^2, g*r, r^2, g*h, h*r, h^2 }
    // Derivatives:
    //   dphi/dg = { 0, 1, 0, 0, 2g, r,  0, h,  0, 0  }
    //   dphi/dh = { 0, 0, 0, 1, 0,  0,  0, g,  r, 2h }
    //   dphi/dr = { 0, 0, 1, 0, 0,  g, 2r, 0,  h, 0  }
    float3 curRef = make_float3(0.f, 0.f, 0.f);
    const int maxIter = 10;
    const float tol = 1e-6f;

    for(int iter=0; iter<maxIter; ++iter) {
        float g = curRef.x;
        float h = curRef.y;  // NOTE: axis order matches basis: 1,g,r,h,...
        float r = curRef.z;

        // Evaluate basis
        float phi[10] = { 1.f, g, r, h, g*g, g*r, r*r, g*h, h*r, h*h };

        // Evaluate derivatives of basis
        float dphidg[10] = { 0.f, 1.f, 0.f, 0.f, 2.f*g, r,    0.f,  h,    0.f,  0.f };
        float dphidh[10] = { 0.f, 0.f, 0.f, 1.f, 0.f,   0.f,  0.f,  g,    r,   2.f*h };
        float dphidr[10] = { 0.f, 0.f, 1.f, 0.f, 0.f,   g,   2.f*r, 0.f,  h,    0.f };
        

        // Compute current world position P = sum c_i * phi_i
        float3 curPos = make_float3(0.f);
        float3 dPdg   = make_float3(0.f);
        float3 dPdr   = make_float3(0.f);
        float3 dPdh   = make_float3(0.f);
        for(int i=0; i<10; ++i) {
            curPos.x += c_x[i] * phi[i];    curPos.y += c_y[i] * phi[i];    curPos.z += c_z[i] * phi[i];
            dPdg.x   += c_x[i] * dphidg[i]; dPdg.y   += c_y[i] * dphidg[i]; dPdg.z   += c_z[i] * dphidg[i];
            dPdr.x   += c_x[i] * dphidr[i]; dPdr.y   += c_y[i] * dphidr[i]; dPdr.z   += c_z[i] * dphidr[i];
            dPdh.x   += c_x[i] * dphidh[i]; dPdh.y   += c_y[i] * dphidh[i]; dPdh.z   += c_z[i] * dphidh[i];
        }

        // Residual
        float3 R = samplePoint - curPos;
        if(dot(R, R) < tol*tol) break;

        // Solve J * delta = R via analytic 3x3 inverse (Cramer's rule)
        // J = [dPdg | dPdh | dPdr] matching curRef = (g, h, r)
        float detJ = dot(dPdg, cross(dPdh, dPdr));
        if(fabsf(detJ) < 1e-10f) break;

        float invDet = 1.f / detJ;
        float3 delta;
        delta.x = dot(cross(dPdh, dPdr), R) * invDet;  // dg -> curRef.x
        delta.y = dot(cross(dPdr, dPdg), R) * invDet;  // dh -> curRef.y
        delta.z = dot(cross(dPdg, dPdh), R) * invDet;  // dr -> curRef.z

        curRef += delta;
        curRef.x = fminf(fmaxf(curRef.x, -1.001f), 1.001f);
        curRef.y = fminf(fmaxf(curRef.y, -1.001f), 1.001f);
        curRef.z = fminf(fmaxf(curRef.z, -1.001f), 1.001f);

        // if(iter > 20) {
        //    printf("Warning: Newton did not converge well, iterations = %d\n", iter);
        // }
    }

    return curRef;
}

static __forceinline__ __device__ float leastSquaresQuadraticInterpolation(
    float* M_plus,
    float3 v0, float d0, 
    float3 v1, float d1, 
    float3 v2, float d2, 
    float3 v3, float d3,
    float3 v4, float d4, 
    float3 v5, float d5, 
    float3 v6, float d6, 
    float3 v7, float d7, 
    float3 v8, float d8, 
    float3 v9, float d9, 
    float3 v10, float d10, 
    float3 v11, float d11, 
    float3 v12, float d12, 
    float3 v13, float d13, 
    float3 v14, float d14, 
    float3 v15, float d15, 
    float3 v16, float d16, 
    float3 v17, float d17, 
    float3 v18, float d18, 
    float3 v19, float d19,
    float3 samplePoint)
{

    float vx[20] = {
        v0.x,  v1.x,  v2.x,  v3.x,  v4.x,  v5.x,  v6.x,  v7.x,
        v8.x,  v9.x,  v10.x, v11.x, v12.x, v13.x, v14.x, v15.x,
        v16.x, v17.x, v18.x, v19.x
    };
    float vy[20] = {
        v0.y,  v1.y,  v2.y,  v3.y,  v4.y,  v5.y,  v6.y,  v7.y,
        v8.y,  v9.y,  v10.y, v11.y, v12.y, v13.y, v14.y, v15.y,
        v16.y, v17.y, v18.y, v19.y
    };
    float vz[20] = {
        v0.z,  v1.z,  v2.z,  v3.z,  v4.z,  v5.z,  v6.z,  v7.z,
        v8.z,  v9.z,  v10.z, v11.z, v12.z, v13.z, v14.z, v15.z,
        v16.z, v17.z, v18.z, v19.z
    };
    float data[20] = {
        d0, d1, d2, d3, d4, d5, d6, d7, d8, d9,
        d10, d11, d12, d13, d14, d15, d16, d17, d18, d19
    };

    // Compute polynomial coefficients: c_{coord} = M+ * v_{coord},  c = M+ * data
    float c[10]   = {0.f};
    float c_x[10] = {0.f};
    float c_y[10] = {0.f};
    float c_z[10] = {0.f};
    for(int i=0; i<10; ++i) {
        for(int k=0; k<20; ++k) {
            c[i]   += M_plus[i*20 + k] * data[k];
            c_x[i] += M_plus[i*20 + k] * vx[k];
            c_y[i] += M_plus[i*20 + k] * vy[k];
            c_z[i] += M_plus[i*20 + k] * vz[k];
        }
    }

    // Map world samplePoint -> reference coordinates using monomial NR solver
    float3 refPoint = getHex20ReferencePointMonomial(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19,
        M_plus, samplePoint);

    // Basis: { 1, g, r, h, g^2, g*r, r^2, g*h, h*r, h^2 }
    float g = refPoint.x;
    float h = refPoint.y;
    float r = refPoint.z;
    float basis[10] = { 1.f, g, r, h, g*g, g*r, r*r, g*h, h*r, h*h };

    // Evaluate polynomial at reference point
    float val = 0.f;
    for(int i=0; i<10; ++i) {
        val += c[i] * basis[i];
    }

    return val;
}

// ---------------------------------------------------------------------------
// Hex20 Serendipity Interpolation — C0-continuous across element boundaries
// ---------------------------------------------------------------------------
// Uses the 20 standard Serendipity shape functions N_i(g,h,r) evaluated at the
// reference coordinates.  The physical → reference map is done with the same
// monomial Newton-Raphson solver already present in the file.
//
// Node ordering (matches mesh file ordering):
//   Corners (0-7):
//      0:(-1,-1,-1)  1:(+1,-1,-1)  2:(+1,+1,-1)  3:(-1,+1,-1)   bottom face
//      4:(-1,-1,+1)  5:(+1,-1,+1)  6:(+1,+1,+1)  7:(-1,+1,+1)   top face
//   Mid-edge (8-19):
//      8:( 0,-1,-1)   9:(-1, 0,-1)  10:(-1,-1, 0)  11:(+1, 0,-1)
//     12:(+1,-1, 0)  13:( 0,+1,-1)  14:(+1,+1, 0)  15:(-1,+1, 0)
//     16:( 0,-1,+1)  17:(-1, 0,+1)  18:(+1, 0,+1)  19:( 0,+1,+1)
static __forceinline__ __device__ float serendipityInterpolation(
    float3 v0,  float d0,
    float3 v1,  float d1,
    float3 v2,  float d2,
    float3 v3,  float d3,
    float3 v4,  float d4,
    float3 v5,  float d5,
    float3 v6,  float d6,
    float3 v7,  float d7,
    float3 v8,  float d8,
    float3 v9,  float d9,
    float3 v10, float d10,
    float3 v11, float d11,
    float3 v12, float d12,
    float3 v13, float d13,
    float3 v14, float d14,
    float3 v15, float d15,
    float3 v16, float d16,
    float3 v17, float d17,
    float3 v18, float d18,
    float3 v19, float d19,
    float3 samplePoint)
{
    // -- 1. Map world → reference using the Newton-Raphson solver --------
    float3 ref = getHex20ReferencePoint(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19,
        samplePoint);

    float g = ref.x;   // ξ
    float h = ref.y;   // η
    float r = ref.z;   // ζ

    // -- 2. Compute the 20 Serendipity shape functions -----------------------

    // Precompute frequently used factors
    float gm = 1.f - g,  gp = 1.f + g;
    float hm = 1.f - h,  hp = 1.f + h;
    float rm = 1.f - r,  rp = 1.f + r;

    // Corner nodes:  N_i = (1/8)(1+g0*g)(1+h0*h)(1+r0*r)(g0*g+h0*h+r0*r-2)
    float N[20];
    N[0]  = 0.125f * gm * hm * rm * (-g - h - r - 2.f);
    N[1]  = 0.125f * gp * hm * rm * ( g - h - r - 2.f);
    N[2]  = 0.125f * gp * hp * rm * ( g + h - r - 2.f);
    N[3]  = 0.125f * gm * hp * rm * (-g + h - r - 2.f);
    N[4]  = 0.125f * gm * hm * rp * (-g - h + r - 2.f);
    N[5]  = 0.125f * gp * hm * rp * ( g - h + r - 2.f);
    N[6]  = 0.125f * gp * hp * rp * ( g + h + r - 2.f);
    N[7]  = 0.125f * gm * hp * rp * (-g + h + r - 2.f);

    // Mid-edge shape functions — corrected to match mesh node ordering:
    //  8: ( 0,-1,-1)  9: (-1, 0,-1) 10: (-1,-1, 0) 11: ( 1, 0,-1)
    // 12: ( 1,-1, 0) 13: ( 0, 1,-1) 14: ( 1, 1, 0) 15: (-1, 1, 0)
    // 16: ( 0,-1, 1) 17: (-1, 0, 1) 18: ( 1, 0, 1) 19: ( 0, 1, 1)
    //
    

    N[8]  = 0.25f * (1.f - g*g) * hm * rm;  // ( 0,-1,-1): g=0, h=-1, r=-1
    N[9]  = 0.25f * gm * (1.f - h*h) * rm;  // (-1, 0,-1): g=-1, h=0, r=-1
    N[10] = 0.25f * gm * hm * (1.f - r*r);  // (-1,-1, 0): g=-1, h=-1, r=0
    N[11] = 0.25f * gp * (1.f - h*h) * rm;  // ( 1, 0,-1): g=+1, h=0, r=-1
    N[12] = 0.25f * gp * hm * (1.f - r*r);  // ( 1,-1, 0): g=+1, h=-1, r=0
    N[13] = 0.25f * (1.f - g*g) * hp * rm;  // ( 0, 1,-1): g=0, h=+1, r=-1
    N[14] = 0.25f * gp * hp * (1.f - r*r);  // ( 1, 1, 0): g=+1, h=+1, r=0
    N[15] = 0.25f * gm * hp * (1.f - r*r);  // (-1, 1, 0): g=-1, h=+1, r=0
    N[16] = 0.25f * (1.f - g*g) * hm * rp;  // ( 0,-1, 1): g=0, h=-1, r=+1
    N[17] = 0.25f * gm * (1.f - h*h) * rp;  // (-1, 0, 1): g=-1, h=0, r=+1
    N[18] = 0.25f * gp * (1.f - h*h) * rp;  // ( 1, 0, 1): g=+1, h=0, r=+1
    N[19] = 0.25f * (1.f - g*g) * hp * rp;  // ( 0, 1, 1): g=0, h=+1, r=+1

    // -- 3. Evaluate the interpolant -----------------------------------------
    float data[20] = {
        d0,  d1,  d2,  d3,  d4,  d5,  d6,  d7,
        d8,  d9,  d10, d11, d12, d13, d14, d15,
        d16, d17, d18, d19
    };

    float val = 0.f;
    for(int i = 0; i < 20; ++i) {
        val += N[i] * data[i];
    }

    return val;
}

static __forceinline__ __device__ float linearInterpolation(
    float3 v0, float d0, float3 v1, float d1, float3 v2, float d2, float3 v3, float d3,
    float3 v4, float d4, float3 v5, float d5, float3 v6, float d6,
    float3 v7, float d7, float3 v8, float d8, float3 v9, float d9,
    float3 samplePoint)
{
    float w0=0.f, w1=0.f, w2=0.f, w3=0.f, w4=0.f;
    float w5=0.f, w6=0.f, w7=0.f, w8=0.f, w9=0.f;

    // Face (0,1,2)
    triangularMVC(samplePoint, v0, v4, v6, &w0, &w4, &w6);
    triangularMVC(samplePoint, v4, v1, v5, &w4, &w1, &w5);
    triangularMVC(samplePoint, v6, v5, v2, &w6, &w5, &w2);
    triangularMVC(samplePoint, v4, v5, v6, &w4, &w5, &w6);

    // Face (0,1,3)
    triangularMVC(samplePoint, v0, v4, v7, &w0, &w4, &w7);
    triangularMVC(samplePoint, v4, v1, v9, &w4, &w1, &w9);
    triangularMVC(samplePoint, v7, v9, v3, &w7, &w9, &w3);
    triangularMVC(samplePoint, v4, v9, v7, &w4, &w9, &w7);

    // Face (1,2,3)
    triangularMVC(samplePoint, v1, v5, v9, &w1, &w5, &w9);
    triangularMVC(samplePoint, v5, v2, v8, &w5, &w2, &w8);
    triangularMVC(samplePoint, v9, v8, v3, &w9, &w8, &w3);
    triangularMVC(samplePoint, v5, v8, v9, &w5, &w8, &w9);

    // Face (0,2,3)
    triangularMVC(samplePoint, v0, v6, v7, &w0, &w6, &w7);
    triangularMVC(samplePoint, v6, v2, v8, &w6, &w2, &w8);
    triangularMVC(samplePoint, v7, v8, v3, &w7, &w8, &w3);
    triangularMVC(samplePoint, v6, v8, v7, &w6, &w8, &w7);

    float totalWeight =
        w0+w1+w2+w3+w4+w5+w6+w7+w8+w9;

    float3 referencePoint = (
        w0*make_float3(0.f, 0.f, 0.f) +
        w1*make_float3(1.f, 0.f, 0.f) +
        w2*make_float3(0.f, 1.f, 0.f) +
        w3*make_float3(0.f, 0.f, 1.f) +
        w4*make_float3(0.5f, 0.f, 0.f) +
        w5*make_float3(0.5f, 0.5f, 0.f) +
        w6*make_float3(0.f, 0.5f, 0.f) +
        w7*make_float3(0.f, 0.f, 0.5f) +
        w8*make_float3(0.f, 0.5f, 0.5f) +
        w9*make_float3(0.5f, 0.f, 0.5f)
    ) / totalWeight;

    return lagrangeInterpolation(
        d0,d1,d2,d3,d4,d5,d6,d7,d8,d9, 
        referencePoint);
}

static __forceinline__ __device__ float linearInterpolation(
    float3 v0, float d0, 
    float3 v1, float d1, 
    float3 v2, float d2, 
    float3 v3, float d3,
    float3 v4, float d4, 
    float3 v5, float d5, 
    float3 v6, float d6, 
    float3 v7, float d7, 
    float3 v8, float d8, 
    float3 v9, float d9, 
    float3 v10, float d10, 
    float3 v11, float d11, 
    float3 v12, float d12, 
    float3 v13, float d13, 
    float3 v14, float d14, 
    float3 v15, float d15, 
    float3 v16, float d16, 
    float3 v17, float d17, 
    float3 v18, float d18, 
    float3 v19, float d19, 
    float3 samplePoint)
{
    float w0 = 0.f;
    float w1 = 0.f;
    float w2 = 0.f;
    float w3 = 0.f;
    float w4 = 0.f;
    float w5 = 0.f;
    float w6 = 0.f;
    float w7 = 0.f;
    float w8 = 0.f;
    float w9 = 0.f;
    float w10 = 0.f;
    float w11 = 0.f;
    float w12 = 0.f;
    float w13 = 0.f;
    float w14 = 0.f;
    float w15 = 0.f;
    float w16 = 0.f;
    float w17 = 0.f;
    float w18 = 0.f;
    float w19 = 0.f;
    float totalWeight = 0.f;

    // Compute weights using mean value coordinates
    triangularMVC(samplePoint, v1, v8, v11, &w1, &w8, &w11);
    triangularMVC(samplePoint, v0, v9, v8, &w0, &w9, &w8);
    triangularMVC(samplePoint, v3, v13, v9, &w3, &w13, &w9);
    triangularMVC(samplePoint, v2, v11, v13, &w2, &w11, &w13);
    triangularMVC(samplePoint, v9, v13, v11, &w9, &w13, &w11);
    triangularMVC(samplePoint, v8, v9, v11, &w8, &w9, &w11);

    triangularMVC(samplePoint, v2, v13, v14, &w2, &w13, &w14);
    triangularMVC(samplePoint, v3, v15, v13, &w3, &w15, &w13);
    triangularMVC(samplePoint, v7, v19, v15, &w7, &w19, &w15);
    triangularMVC(samplePoint, v6, v14, v19, &w6, &w14, &w19);
    triangularMVC(samplePoint, v14, v15, v19, &w14, &w15, &w19);
    triangularMVC(samplePoint, v13, v15, v14, &w13, &w15, &w14);

    triangularMVC(samplePoint, v0, v10, v9, &w0, &w10, &w9);
    triangularMVC(samplePoint, v4, v17, v10, &w4, &w17, &w10);
    triangularMVC(samplePoint, v7, v15, v17, &w7, &w15, &w17);
    triangularMVC(samplePoint, v3, v9, v15, &w3, &w9, &w15);
    triangularMVC(samplePoint, v9, v17, v15, &w9, &w17, &w15);
    triangularMVC(samplePoint, v9, v10, v17, &w9, &w10, &w17);

    triangularMVC(samplePoint, v5, v12, v18, &w5, &w12, &w18);
    triangularMVC(samplePoint, v1, v11, v12, &w1, &w11, &w12);
    triangularMVC(samplePoint, v2, v14, v11, &w2, &w14, &w11);
    triangularMVC(samplePoint, v6, v18, v14, &w6, &w18, &w14);
    triangularMVC(samplePoint, v11, v14, v18, &w11, &w14, &w18);
    triangularMVC(samplePoint, v11, v18, v12, &w11, &w18, &w12);

    triangularMVC(samplePoint, v5, v16, v12, &w5, &w16, &w12);
    triangularMVC(samplePoint, v4, v10, v16, &w4, &w10, &w16);
    triangularMVC(samplePoint, v0, v8, v10, &w0, &w8, &w10);
    triangularMVC(samplePoint, v1, v12, v8, &w1, &w12, &w8);
    triangularMVC(samplePoint, v8, v12, v10, &w8, &w12, &w10);
    triangularMVC(samplePoint, v10, v12, v16, &w10, &w12, &w16);

    triangularMVC(samplePoint, v6, v19, v18, &w6, &w19, &w18);
    triangularMVC(samplePoint, v7, v17, v19, &w7, &w17, &w19);
    triangularMVC(samplePoint, v4, v16, v17, &w4, &w16, &w17);
    triangularMVC(samplePoint, v5, v18, v16, &w5, &w18, &w16);
    triangularMVC(samplePoint, v16, v18, v17, &w16, &w18, &w17);
    triangularMVC(samplePoint, v17, v18, v19, &w17, &w18, &w19);

    // Compute total weight
    totalWeight =
        w0 +
        w1 +
        w2 +
        w3 +
        w4 +
        w5 +
        w6 +
        w7 +
        w8 +
        w9 +
        w10 +
        w11 +
        w12 +
        w13 +
        w14 +
        w15 +
        w16 +
        w17 +
        w18 +
        w19;

    // Point in reference space
    float3 referencePoint = (
        w0*make_float3(-1.f, -1.f, -1.f) +
        w1*make_float3(1.f, -1.f, -1.f) +
        w2*make_float3(1.f, 1.f, -1.f) +
        w3*make_float3(-1.f, 1.f, -1.f) +
        w4*make_float3(-1.f, -1.f, 1.f) +
        w5*make_float3(1.f, -1.f, 1.f) +
        w6*make_float3(1.f, 1.f, 1.f) +
        w7*make_float3(-1.f, 1.f, 1.f) +
        w8*make_float3(0.f, -1.f, -1.f) +
        w9*make_float3(-1.f, 0.f, -1.f) +
        w10*make_float3(-1.f, -1.f, 0.f) +
        w11*make_float3(1.f, 0.f, -1.f) +
        w12*make_float3(1.f, -1.f, 0.f) +
        w13*make_float3(0.f, 1.f, -1.f) +
        w14*make_float3(1.f, 1.f, 0.f) +
        w15*make_float3(-1.f, 1.f, 0.f) +
        w16*make_float3(0.f, -1.f, 1.f) +
        w17*make_float3(-1.f, 0.f, 1.f) +
        w18*make_float3(1.f, 0.f, 1.f) +
        w19*make_float3(0.f, 1.f, 1.f))
        /totalWeight;

    return lagrangeInterpolation(
        d0,
        d1,
        d2,
        d3,
        d4,
        d5,
        d6,
        d7,
        d8,
        d9,
        d10,
        d11,
        d12,
        d13,
        d14,
        d15,
        d16,
        d17,
        d18,
        d19,
        referencePoint);

    // Return interpolated value
    // return (
    //     w0*d0 + 
    //     w1*d1 + 
    //     w2*d2 +
    //     w3*d3 + 
    //     w4*d4 + 
    //     w5*d5 +
    //     w6*d6 + 
    //     w7*d7 + 
    //     w8*d8 +
    //     w9*d9 + 
    //     w10*d10 + 
    //     w11*d11 +
    //     w12*d12 + 
    //     w13*d13 + 
    //     w14*d14 +
    //     w15*d15 + 
    //     w16*d16 + 
    //     w17*d17 +
    //     w18*d18 + 
    //     w19*d19
    //     )/totalWeight;
}