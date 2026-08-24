#include <optix_device.h>



extern "C" __global__ void __closesthit__mesh()
{ 
    // Get index of the intersected triangle
    uint hitTriangleID = optixGetPrimitiveIndex();

    // Return hit distance, triangle index and hit side
    optixSetPayload_0(__float_as_uint(optixGetRayTmax()));
    optixSetPayload_1(hitTriangleID);
    optixSetPayload_2(optixIsTriangleBackFaceHit() ? 1 : 0);
}

extern "C" __global__ void __closesthit__boxes()
{
    // Get index of the intersected triangle
    uint hitTriangleID = optixGetPrimitiveIndex();

    // Return hit distance and box index
    optixSetPayload_0(__float_as_uint(optixGetRayTmax()));
    optixSetPayload_1(hitTriangleID/12);
}

extern "C" __global__ void __miss__any()
{
    // Return miss marker
    optixSetPayload_0(__float_as_uint(-1.f));
}