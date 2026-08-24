#include "renderer/cuda/utils/CUDAStructs.h"
#include "renderer/cuda/utils/CUDAMath.h"
#include "../resources/cuda/OptixRenderer/payload.cuh"
#include "../resources/cuda/OptixRenderer/sampling.cuh"
#include "../resources/cuda/OptixRenderer/meshTraversal.cu"

#include <cstdint>
#include <cuda.h>
#include <optix_device.h>

#define EPSILON 1.e-6f



extern "C" __constant__ LaunchData launchData;

extern "C" __global__ void __raygen__launch()
{    
    // Current 2D pixel
    const uint ix = optixGetLaunchIndex().x;
    const uint iy = optixGetLaunchIndex().y;

    // 2D position in [-1;+1]^2 screen space
    const float2 pixel = 2.f*(make_float2(ix, iy) + .5f)/make_float2(launchData.imageWidth, launchData.imageHeight) - 1.f;

    // Pixel color
    float4 color = make_float4(0.f);

    // Primary ray direction
    const float3 rayOrigin = launchData.cameraEye;
    const float3 rayDirection = normalize(pixel.x*launchData.cameraU + pixel.y*launchData.cameraV + launchData.cameraW);

    // Retrieve ray generation data from SBT
    RayGenData* rayGenData = reinterpret_cast<RayGenData*>(optixGetSbtDataPointer());

    // Traversal informations
    float4 sampledColor;

    float segmentEnd;
    float cellEnd;
    float maxOpacity;
    float dataVariance;
    float sampledValue;
    float t = 0.f;
    float step;

    uint floatPayload;
    uint hitBoxID;
    uint hitTriangleID;
    uint hitCellID;
    uint hitCellType;
    uint backFaceHit;

    // Traversal loop
    // While alpha threshold is not reached
    while(color.w < launchData.alphaThreshold){
        // Ray against boxes to find current box
        optixTrace(
            launchData.boxes,
            rayOrigin,
            rayDirection,
            t,                          // tMin
            1.e16f,                     // tMax
            0.f,                        // rayTime
            OptixVisibilityMask(1),
            OPTIX_RAY_FLAG_CULL_FRONT_FACING_TRIANGLES|OPTIX_RAY_FLAG_DISABLE_ANYHIT,
            1,                          // offset 1
            0,                          // no SBT Stride
            0,                          // miss shader
            floatPayload,               // Hit t              
            hitBoxID);                  // ID of hit box

        // Retrieve hit distance
        segmentEnd = __uint_as_float(floatPayload);

        // Ray missed the geometry
        if(segmentEnd < 0.f){ 
            launchData.image[ix + iy*launchData.imageWidth] = color;
            return;
        }

        // Retrieve box informations
        maxOpacity = rayGenData->boxes.maxOpacities[hitBoxID];
        dataVariance = rayGenData->boxes.dataVariances[hitBoxID];

        // If box is transparent, skip it
        if(maxOpacity < launchData.opacityThreshold + EPSILON){
            t = segmentEnd + EPSILON;
        // Query along the ray inside current box
        }else{
            // Update sampling step, formula from RTX article
            step = max(
                launchData.minSamplePeriod + (launchData.maxSamplePeriod - launchData.minSamplePeriod)*pow(abs(min(sqrt(dataVariance), 1.f) - 1.f), launchData.samplingPower), 
                launchData.minSamplePeriod);

            while((t < segmentEnd)){
                // Ray against mesh to find current cell
                optixTrace(
                    launchData.mesh,
                    rayOrigin,
                    rayDirection,
                    t,                          // tMin
                    1.e16f,                     // tMax
                    0.f,                        // rayTime
                    OptixVisibilityMask(1),
                    OPTIX_RAY_FLAG_DISABLE_ANYHIT,
                    0,                          // offset 0
                    0,                          // no SBT Stride
                    0,                          // miss shader
                    floatPayload,               // Hit t        
                    hitTriangleID,              // HID of hit triangle
                    backFaceHit);               // Back hit ?

                // Retrieve hit distance
                cellEnd = __uint_as_float(floatPayload);

                // Ray missed the geometry
                if(cellEnd < 0.f){ 
                    launchData.image[ix + iy*launchData.imageWidth] = color;
                    return;
                }

                // Retrieve hit cell
                if(backFaceHit){
                    hitCellID = rayGenData->mesh.triangles[hitTriangleID].backCellID;
                    hitCellType = rayGenData->mesh.triangles[hitTriangleID].backCellType;
                }else{
                    hitCellID = rayGenData->mesh.triangles[hitTriangleID].frontCellID;
                    hitCellType = rayGenData->mesh.triangles[hitTriangleID].frontCellType;
                }

                // If ray is outside the mesh, advance it to next entry point
                if(hitCellID == UINT32_MAX){
                    t = cellEnd + EPSILON;
                // Else sample inside the cell
                }else{
                    while(t < cellEnd){
                        // // Sample the cell
                        // sampledValue = dataVariance;
                        sampledValue = sampleCell(
                            &rayGenData->mesh, 
                            hitCellID, 
                            hitCellType, 
                            rayOrigin + t*rayDirection,
                            launchData.leastSquaresMatrix,
                            launchData.interpolationStrategy);

                        // Sample the transfer function (apply data window [tfMin, tfMax])
                        float tfRange = launchData.tfMax - launchData.tfMin;
                        float tfCoord = (tfRange > 1e-6f)
                            ? (sampledValue - launchData.tfMin) / tfRange
                            : 0.f;
                        tfCoord = fmaxf(0.f, fminf(1.f, tfCoord));
                        sampledColor = tex1D<float4>(launchData.transferFunction, tfCoord);

                        // Color accumulation
                        sampledColor.w = 1.f - pow(1.f - sampledColor.w, step);
                        sampledColor.x *= sampledColor.w;
                        sampledColor.y *= sampledColor.w;
                        sampledColor.z *= sampledColor.w;

                        color += sampledColor*(1.f - color.w);

                        // Opacity threshold reached, early termination
                        if(color.w > launchData.alphaThreshold){  
                            launchData.image[ix + iy*launchData.imageWidth] = color;
                            return;
                        }

                        // Advance ray
                        t += step;
                    }
                }
            }
        }
    }
}

extern "C" __global__ void __raygen__difference()
{
    const uint ix = optixGetLaunchIndex().x;
    const uint iy = optixGetLaunchIndex().y;
    const uint index = ix + iy * launchData.imageWidth;

    float4 color1 = launchData.image1[index];
    float4 color2 = launchData.image2[index];

    const float scale = 25.f;
    const float4 diff = make_float4(
        fabsf(color1.x - color2.x) * scale,
        fabsf(color1.y - color2.y) * scale,
        fabsf(color1.z - color2.z) * scale,
        1.f
    );

    launchData.image[index] = diff;
}
