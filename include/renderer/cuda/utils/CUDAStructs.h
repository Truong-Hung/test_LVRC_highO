#pragma once

#include <cuda_runtime.h>
#include <optix_types.h>

// Order 1
#define TETRAHEDRON4    0
#define HEXAHEDRON8     1
#define PRISM6          2
#define PYRAMID5        3

// Order 2
#define TETRAHEDRON10   4
#define HEXAHEDRON20    5
#define PRISM15         6
#define PYRAMID13       7



// Triangle connectivity
struct Triangle
{
    unsigned int frontCellID;
    unsigned int backCellID;
    unsigned int frontCellType;
    unsigned int backCellType;
};

// Tetrahedron4 vertex indices
struct Tetrahedron4
{
    unsigned int vertices[4];
};

// Hexahedron8 vertex indices
struct Hexahedron8
{
    unsigned int vertices[8];
};

// Prism6 vertex indices
struct Prism6
{
    unsigned int vertices[6];
};

// Pyramid5 vertex indices
struct Pyramid5
{
    unsigned int vertices[5];
};

// Hexahedron20 vertex indices
struct Hexahedron20
{
    unsigned int vertices[20];
};

// Tetrahedron10 vertex indices
struct Tetrahedron10
{
    unsigned int vertices[10];
};

// Cells
struct Cells
{
    Tetrahedron4* tetrahedrons4;
    Hexahedron8* hexahedrons8;
    Prism6* prisms6;
    Pyramid5* pyramids5;
    Tetrahedron10* tetrahedrons10;
    Hexahedron20* hexahedrons20;
    // Prism15* prisms15;
    // Pyramid13* pyramids13;  
};

// Mesh data
struct MeshData
{
    float3* vertices;
    float* datas;
    Triangle* triangles;
    Cells cells;
};

// Boxes
struct BoxesData
{
    float* maxOpacities;
    float* dataVariances;
};

// Ray generation data
struct RayGenData
{
    MeshData mesh;
    BoxesData boxes;
};

// Miss data
struct MissData
{
};

// Hit Group data
struct HitGroupData
{
};

// Launch data
struct LaunchData
{
    // Rendered image
    float4 *image;
    float4 *image1;
    float4 *image2;
    unsigned int imageWidth;
    unsigned int imageHeight;

    // Camera parameters
    float3 cameraEye;
    float3 cameraU;
    float3 cameraV;
    float3 cameraW;

    // Transfer function
    cudaTextureObject_t transferFunction;

    // Renderer parameters
    float alphaThreshold;

    // ESS and AS
    float opacityThreshold;
    float samplingPower;
    float minSamplePeriod;
    float maxSamplePeriod;

    // Interpolation
    float *leastSquaresMatrix;   // Hex20: 10x20 left pseudo-inverse
    float *tet4Matrix;           // Tet4:  10x4  right pseudo-inverse
    float *hex8Matrix;           // Hex8:  10x8  right pseudo-inverse
    float *tet10Matrix;          // Tet10: 10x10 pseudo-inverse
    int interpolationStrategy; // 0: Linear, 1: High-Order, 2: Difference

    // Traversable handles
    OptixTraversableHandle boxes;
    OptixTraversableHandle mesh;
};