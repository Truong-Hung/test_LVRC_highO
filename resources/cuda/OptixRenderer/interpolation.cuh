#include "renderer/cuda/utils/CUDAMath.h"
#include <math_constants.h>



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
static __forceinline__ __device__ float serendipityInterpolationHex20(
    float* M_plus,              // 10x20 matrix used for world→ref mapping
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