#include "renderer/cuda/utils/CUDAMath.h"
#include "renderer/cuda/utils/CUDAStructs.h"
#include "../resources/cuda/OptixRenderer/interpolation.cuh"



static __forceinline__ __device__ float sample(
    MeshData* mesh, 
    Tetrahedron10 cell, 
    float3 samplePoint,
    int strategy)
{
    if (strategy == 0) {
        return linearInterpolation(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            samplePoint);
    }
    else if (strategy == 1) {
        return lagrangeInterpolationTet10(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
            mesh->vertices[cell.vertices[8]], mesh->datas[cell.vertices[8]],
            mesh->vertices[cell.vertices[9]], mesh->datas[cell.vertices[9]],
            samplePoint);
    }
    else {
        float valLinear = linearInterpolation(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            samplePoint);

        float valHighOrder = lagrangeInterpolationTet10(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
            mesh->vertices[cell.vertices[8]], mesh->datas[cell.vertices[8]],
            mesh->vertices[cell.vertices[9]], mesh->datas[cell.vertices[9]],
            samplePoint);

        return fabsf(valLinear - valHighOrder) * 25.f;
    }
}

static __forceinline__ __device__ float sample(
    MeshData* mesh, 
    Tetrahedron20 cell, 
    float3 samplePoint,
    int strategy)
{
    if (strategy == 0) {
        return linearInterpolation(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            samplePoint);
    }
    else if (strategy == 1) {
        return lagrangeInterpolationTet20(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
            mesh->vertices[cell.vertices[8]], mesh->datas[cell.vertices[8]],
            mesh->vertices[cell.vertices[9]], mesh->datas[cell.vertices[9]],
            mesh->vertices[cell.vertices[10]], mesh->datas[cell.vertices[10]],
            mesh->vertices[cell.vertices[11]], mesh->datas[cell.vertices[11]],
            mesh->vertices[cell.vertices[12]], mesh->datas[cell.vertices[12]],
            mesh->vertices[cell.vertices[13]], mesh->datas[cell.vertices[13]],
            mesh->vertices[cell.vertices[14]], mesh->datas[cell.vertices[14]],
            mesh->vertices[cell.vertices[15]], mesh->datas[cell.vertices[15]],
            mesh->vertices[cell.vertices[16]], mesh->datas[cell.vertices[16]],
            mesh->vertices[cell.vertices[17]], mesh->datas[cell.vertices[17]],
            mesh->vertices[cell.vertices[18]], mesh->datas[cell.vertices[18]],
            mesh->vertices[cell.vertices[19]], mesh->datas[cell.vertices[19]],
            samplePoint);
    }
    else {
        float valLinear = linearInterpolation(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            samplePoint);

        float valHighOrder = lagrangeInterpolationTet20(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
            mesh->vertices[cell.vertices[8]], mesh->datas[cell.vertices[8]],
            mesh->vertices[cell.vertices[9]], mesh->datas[cell.vertices[9]],
            mesh->vertices[cell.vertices[10]], mesh->datas[cell.vertices[10]],
            mesh->vertices[cell.vertices[11]], mesh->datas[cell.vertices[11]],
            mesh->vertices[cell.vertices[12]], mesh->datas[cell.vertices[12]],
            mesh->vertices[cell.vertices[13]], mesh->datas[cell.vertices[13]],
            mesh->vertices[cell.vertices[14]], mesh->datas[cell.vertices[14]],
            mesh->vertices[cell.vertices[15]], mesh->datas[cell.vertices[15]],
            mesh->vertices[cell.vertices[16]], mesh->datas[cell.vertices[16]],
            mesh->vertices[cell.vertices[17]], mesh->datas[cell.vertices[17]],
            mesh->vertices[cell.vertices[18]], mesh->datas[cell.vertices[18]],
            mesh->vertices[cell.vertices[19]], mesh->datas[cell.vertices[19]],
            samplePoint);

        return fabsf(valLinear - valHighOrder) * 25.f;
    }
}

static __forceinline__ __device__ float sample(
    MeshData* mesh, 
    Tetrahedron4 cell, 
    float3 samplePoint,
    int strategy)
{
    return linearInterpolation(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            samplePoint);
}

static __forceinline__ __device__ float sample(
    MeshData* mesh, 
    Hexahedron8 cell, 
    float3 samplePoint)
{
    return linearInterpolation(
        mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
        mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
        mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
        mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
        mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
        mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
        mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
        mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
        samplePoint);
}

static __forceinline__ __device__ float sample(
    MeshData* mesh, 
    Prism6 cell, 
    float3 samplePoint)
{
    return linearInterpolation(
        mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
        mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
        mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
        mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
        mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
        mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
        samplePoint);
}

static __forceinline__ __device__ float sample(
    MeshData* mesh, 
    Pyramid5 cell, 
    float3 samplePoint)
{
    return linearInterpolation(
        mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
        mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
        mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
        mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
        mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
        samplePoint);
}

static __forceinline__ __device__ float sample(
    MeshData* mesh, 
    Hexahedron20 cell, 
    float3 samplePoint,
    float* M_plus,
    int strategy)
{
    // Linear
    if(strategy == 0){
        return linearInterpolation(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
            mesh->vertices[cell.vertices[8]], mesh->datas[cell.vertices[8]],
            mesh->vertices[cell.vertices[9]], mesh->datas[cell.vertices[9]],
            mesh->vertices[cell.vertices[10]], mesh->datas[cell.vertices[10]],
            mesh->vertices[cell.vertices[11]], mesh->datas[cell.vertices[11]],
            mesh->vertices[cell.vertices[12]], mesh->datas[cell.vertices[12]],
            mesh->vertices[cell.vertices[13]], mesh->datas[cell.vertices[13]],
            mesh->vertices[cell.vertices[14]], mesh->datas[cell.vertices[14]],
            mesh->vertices[cell.vertices[15]], mesh->datas[cell.vertices[15]],
            mesh->vertices[cell.vertices[16]], mesh->datas[cell.vertices[16]],
            mesh->vertices[cell.vertices[17]], mesh->datas[cell.vertices[17]],
            mesh->vertices[cell.vertices[18]], mesh->datas[cell.vertices[18]],
            mesh->vertices[cell.vertices[19]], mesh->datas[cell.vertices[19]],
            samplePoint);
    }
    // High-Order Serendipity — C0-continuous (Strategy == 1)
    else if(strategy == 1){
        return serendipityInterpolation(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
            mesh->vertices[cell.vertices[8]], mesh->datas[cell.vertices[8]],
            mesh->vertices[cell.vertices[9]], mesh->datas[cell.vertices[9]],
            mesh->vertices[cell.vertices[10]], mesh->datas[cell.vertices[10]],
            mesh->vertices[cell.vertices[11]], mesh->datas[cell.vertices[11]],
            mesh->vertices[cell.vertices[12]], mesh->datas[cell.vertices[12]],
            mesh->vertices[cell.vertices[13]], mesh->datas[cell.vertices[13]],
            mesh->vertices[cell.vertices[14]], mesh->datas[cell.vertices[14]],
            mesh->vertices[cell.vertices[15]], mesh->datas[cell.vertices[15]],
            mesh->vertices[cell.vertices[16]], mesh->datas[cell.vertices[16]],
            mesh->vertices[cell.vertices[17]], mesh->datas[cell.vertices[17]],
            mesh->vertices[cell.vertices[18]], mesh->datas[cell.vertices[18]],
            mesh->vertices[cell.vertices[19]], mesh->datas[cell.vertices[19]],
            samplePoint);
    }
    // High-Order Monomial (least-squares) — kept for comparison (Strategy == 3)
    else if(strategy == 3){
        return leastSquaresQuadraticInterpolation(
            M_plus,
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
            mesh->vertices[cell.vertices[8]], mesh->datas[cell.vertices[8]],
            mesh->vertices[cell.vertices[9]], mesh->datas[cell.vertices[9]],
            mesh->vertices[cell.vertices[10]], mesh->datas[cell.vertices[10]],
            mesh->vertices[cell.vertices[11]], mesh->datas[cell.vertices[11]],
            mesh->vertices[cell.vertices[12]], mesh->datas[cell.vertices[12]],
            mesh->vertices[cell.vertices[13]], mesh->datas[cell.vertices[13]],
            mesh->vertices[cell.vertices[14]], mesh->datas[cell.vertices[14]],
            mesh->vertices[cell.vertices[15]], mesh->datas[cell.vertices[15]],
            mesh->vertices[cell.vertices[16]], mesh->datas[cell.vertices[16]],
            mesh->vertices[cell.vertices[17]], mesh->datas[cell.vertices[17]],
            mesh->vertices[cell.vertices[18]], mesh->datas[cell.vertices[18]],
            mesh->vertices[cell.vertices[19]], mesh->datas[cell.vertices[19]],
            samplePoint);
    }
    // Difference (Strategy == 4)
    else if(strategy == 4){
        float valLinear = linearInterpolation(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
            mesh->vertices[cell.vertices[8]], mesh->datas[cell.vertices[8]],
            mesh->vertices[cell.vertices[9]], mesh->datas[cell.vertices[9]],
            mesh->vertices[cell.vertices[10]], mesh->datas[cell.vertices[10]],
            mesh->vertices[cell.vertices[11]], mesh->datas[cell.vertices[11]],
            mesh->vertices[cell.vertices[12]], mesh->datas[cell.vertices[12]],
            mesh->vertices[cell.vertices[13]], mesh->datas[cell.vertices[13]],
            mesh->vertices[cell.vertices[14]], mesh->datas[cell.vertices[14]],
            mesh->vertices[cell.vertices[15]], mesh->datas[cell.vertices[15]],
            mesh->vertices[cell.vertices[16]], mesh->datas[cell.vertices[16]],
            mesh->vertices[cell.vertices[17]], mesh->datas[cell.vertices[17]],
            mesh->vertices[cell.vertices[18]], mesh->datas[cell.vertices[18]],
            mesh->vertices[cell.vertices[19]], mesh->datas[cell.vertices[19]],
            samplePoint);

        float valHighOrder = leastSquaresQuadraticInterpolation(
            M_plus,
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
            mesh->vertices[cell.vertices[8]], mesh->datas[cell.vertices[8]],
            mesh->vertices[cell.vertices[9]], mesh->datas[cell.vertices[9]],
            mesh->vertices[cell.vertices[10]], mesh->datas[cell.vertices[10]],
            mesh->vertices[cell.vertices[11]], mesh->datas[cell.vertices[11]],
            mesh->vertices[cell.vertices[12]], mesh->datas[cell.vertices[12]],
            mesh->vertices[cell.vertices[13]], mesh->datas[cell.vertices[13]],
            mesh->vertices[cell.vertices[14]], mesh->datas[cell.vertices[14]],
            mesh->vertices[cell.vertices[15]], mesh->datas[cell.vertices[15]],
            mesh->vertices[cell.vertices[16]], mesh->datas[cell.vertices[16]],
            mesh->vertices[cell.vertices[17]], mesh->datas[cell.vertices[17]],
            mesh->vertices[cell.vertices[18]], mesh->datas[cell.vertices[18]],
            mesh->vertices[cell.vertices[19]], mesh->datas[cell.vertices[19]],
            samplePoint);

        return fabsf(valLinear - valHighOrder) * 25.f;
    }
}

static __forceinline__ __device__ float sample(
    MeshData* mesh,
    Hexahedron27 cell,
    float3 samplePoint,
    int strategy)
{
    // Linear
    if(strategy == 0){
        return linearInterpolation(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
            samplePoint);
    }
    // High-Order Serendipity / Tensorial (Strategy == 1)
    else if(strategy == 1){
        return lagrangeInterpolationHex27(
            mesh->vertices[cell.vertices[0]],  mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]],  mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]],  mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]],  mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]],  mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]],  mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]],  mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]],  mesh->datas[cell.vertices[7]],
            mesh->vertices[cell.vertices[8]],  mesh->datas[cell.vertices[8]],
            mesh->vertices[cell.vertices[9]],  mesh->datas[cell.vertices[9]],
            mesh->vertices[cell.vertices[10]], mesh->datas[cell.vertices[10]],
            mesh->vertices[cell.vertices[11]], mesh->datas[cell.vertices[11]],
            mesh->vertices[cell.vertices[12]], mesh->datas[cell.vertices[12]],
            mesh->vertices[cell.vertices[13]], mesh->datas[cell.vertices[13]],
            mesh->vertices[cell.vertices[14]], mesh->datas[cell.vertices[14]],
            mesh->vertices[cell.vertices[15]], mesh->datas[cell.vertices[15]],
            mesh->vertices[cell.vertices[16]], mesh->datas[cell.vertices[16]],
            mesh->vertices[cell.vertices[17]], mesh->datas[cell.vertices[17]],
            mesh->vertices[cell.vertices[18]], mesh->datas[cell.vertices[18]],
            mesh->vertices[cell.vertices[19]], mesh->datas[cell.vertices[19]],
            mesh->vertices[cell.vertices[20]], mesh->datas[cell.vertices[20]],
            mesh->vertices[cell.vertices[21]], mesh->datas[cell.vertices[21]],
            mesh->vertices[cell.vertices[22]], mesh->datas[cell.vertices[22]],
            mesh->vertices[cell.vertices[23]], mesh->datas[cell.vertices[23]],
            mesh->vertices[cell.vertices[24]], mesh->datas[cell.vertices[24]],
            mesh->vertices[cell.vertices[25]], mesh->datas[cell.vertices[25]],
            mesh->vertices[cell.vertices[26]], mesh->datas[cell.vertices[26]],
            samplePoint);
    }
    // Difference (Strategy == 4)
    else if(strategy == 4){
        float valLinear = linearInterpolation(
            mesh->vertices[cell.vertices[0]], mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]], mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]], mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]], mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]], mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]], mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]], mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]], mesh->datas[cell.vertices[7]],
            samplePoint);

        float valHighOrder = lagrangeInterpolationHex27(
            mesh->vertices[cell.vertices[0]],  mesh->datas[cell.vertices[0]],
            mesh->vertices[cell.vertices[1]],  mesh->datas[cell.vertices[1]],
            mesh->vertices[cell.vertices[2]],  mesh->datas[cell.vertices[2]],
            mesh->vertices[cell.vertices[3]],  mesh->datas[cell.vertices[3]],
            mesh->vertices[cell.vertices[4]],  mesh->datas[cell.vertices[4]],
            mesh->vertices[cell.vertices[5]],  mesh->datas[cell.vertices[5]],
            mesh->vertices[cell.vertices[6]],  mesh->datas[cell.vertices[6]],
            mesh->vertices[cell.vertices[7]],  mesh->datas[cell.vertices[7]],
            mesh->vertices[cell.vertices[8]],  mesh->datas[cell.vertices[8]],
            mesh->vertices[cell.vertices[9]],  mesh->datas[cell.vertices[9]],
            mesh->vertices[cell.vertices[10]], mesh->datas[cell.vertices[10]],
            mesh->vertices[cell.vertices[11]], mesh->datas[cell.vertices[11]],
            mesh->vertices[cell.vertices[12]], mesh->datas[cell.vertices[12]],
            mesh->vertices[cell.vertices[13]], mesh->datas[cell.vertices[13]],
            mesh->vertices[cell.vertices[14]], mesh->datas[cell.vertices[14]],
            mesh->vertices[cell.vertices[15]], mesh->datas[cell.vertices[15]],
            mesh->vertices[cell.vertices[16]], mesh->datas[cell.vertices[16]],
            mesh->vertices[cell.vertices[17]], mesh->datas[cell.vertices[17]],
            mesh->vertices[cell.vertices[18]], mesh->datas[cell.vertices[18]],
            mesh->vertices[cell.vertices[19]], mesh->datas[cell.vertices[19]],
            mesh->vertices[cell.vertices[20]], mesh->datas[cell.vertices[20]],
            mesh->vertices[cell.vertices[21]], mesh->datas[cell.vertices[21]],
            mesh->vertices[cell.vertices[22]], mesh->datas[cell.vertices[22]],
            mesh->vertices[cell.vertices[23]], mesh->datas[cell.vertices[23]],
            mesh->vertices[cell.vertices[24]], mesh->datas[cell.vertices[24]],
            mesh->vertices[cell.vertices[25]], mesh->datas[cell.vertices[25]],
            mesh->vertices[cell.vertices[26]], mesh->datas[cell.vertices[26]],
            samplePoint);

        return fabsf(valLinear - valHighOrder) * 25.f;
    }

    return 0.f;
}

static __forceinline__ __device__ float sampleCell(
    MeshData* mesh,
    const ElementFieldData& elementField,
    uint32_t cellID,
    uint32_t cellType,
    float3 samplePoint,
    float* M_plus,
    int strategy)
{

    // Add support for element-local scalar field sampling
    if(elementField.enabled)
    {
        if(cellType != elementField.cellType)
        {
            return 0.f;
        }

        if(cellType != TETRAHEDRON4
        && cellType != TETRAHEDRON10
        && cellType != TETRAHEDRON20)
        {
            return 0.f;
        }

        const float4 bary =
            getTetrahedronReferencePoint(
                mesh,
                cellID,
                cellType,
                samplePoint);

        return evaluateTetrahedralElementField(
            elementField,
            cellID,
            bary,
            strategy);
    }

    // Sample according to cell type
    switch(cellType){
        case TETRAHEDRON4:  return sample(mesh, mesh->cells.tetrahedrons4[cellID], samplePoint, strategy);
        case HEXAHEDRON8:   return sample(mesh, mesh->cells.hexahedrons8[cellID], samplePoint);
        case PRISM6:        return sample(mesh, mesh->cells.prisms6[cellID], samplePoint);
        case PYRAMID5:      return sample(mesh, mesh->cells.pyramids5[cellID], samplePoint);
        case TETRAHEDRON10: return sample(mesh, mesh->cells.tetrahedrons10[cellID], samplePoint, strategy);
        case TETRAHEDRON20: return sample(mesh, mesh->cells.tetrahedrons20[cellID], samplePoint, strategy);
        case HEXAHEDRON20:  return sample(mesh, mesh->cells.hexahedrons20[cellID], samplePoint, M_plus, strategy);
        case HEXAHEDRON27:  return sample(mesh, mesh->cells.hexahedrons27[cellID], samplePoint, strategy);
        default:            return 1.f;
    }
}



