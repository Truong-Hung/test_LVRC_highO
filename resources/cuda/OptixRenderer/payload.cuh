#include "renderer/cuda/utils/CUDAMath.h"

#include <cstdint>
#include <cuda.h>
#include <optix_device.h>

#define float_as_args(u) reinterpret_cast<unsigned int&>((u))
#define float2_as_args(u) reinterpret_cast<unsigned int&>((u).x), reinterpret_cast<unsigned int&>((u).y)
#define float3_as_args(u) reinterpret_cast<unsigned int&>((u).x), reinterpret_cast<unsigned int&>((u).y), reinterpret_cast<unsigned int&>((u).z)
#define float4_as_args(u) reinterpret_cast<unsigned int&>((u).x), reinterpret_cast<unsigned int&>((u).y), reinterpret_cast<unsigned int&>((u).z), reinterpret_cast<unsigned int&>((u).w)



static __forceinline__ __device__ void setPayload_0(float p)
{
    optixSetPayload_0(__float_as_uint(p));
}

static __forceinline__ __device__ void setPayload_0(float2 p)
{
    optixSetPayload_0(__float_as_uint(p.x));
    optixSetPayload_1(__float_as_uint(p.y));
}

static __forceinline__ __device__ void setPayload_0(float3 p)
{
    optixSetPayload_0(__float_as_uint(p.x));
    optixSetPayload_1(__float_as_uint(p.y));
    optixSetPayload_2(__float_as_uint(p.z));
}

static __forceinline__ __device__ void setPayload_0(float4 p)
{
    optixSetPayload_0(__float_as_uint(p.x));
    optixSetPayload_1(__float_as_uint(p.y));
    optixSetPayload_2(__float_as_uint(p.z));
    optixSetPayload_3(__float_as_uint(p.w));
}

static __forceinline__ __device__ void setPayload_4(float p)
{
    optixSetPayload_4(__float_as_uint(p));
}

static __forceinline__ __device__ float getPayload_0AsFloat()
{
    return __uint_as_float(optixGetPayload_0());
}

static __forceinline__ __device__ float2 getPayload_0AsFloat2()
{
    return make_float2(
        __uint_as_float(optixGetPayload_0()),
        __uint_as_float(optixGetPayload_1()));
}

static __forceinline__ __device__ float3 getPayload_0AsFloat3()
{
    return make_float3(
        __uint_as_float(optixGetPayload_0()),
        __uint_as_float(optixGetPayload_1()),
        __uint_as_float(optixGetPayload_2()));
}

static __forceinline__ __device__ float4 getPayload_0AsFloat4()
{
    return make_float4(
        __uint_as_float(optixGetPayload_0()),
        __uint_as_float(optixGetPayload_1()),
        __uint_as_float(optixGetPayload_2()), 
        __uint_as_float(optixGetPayload_3()));
}

static __forceinline__ __device__ float getPayload_4AsFloat()
{
    return __uint_as_float(optixGetPayload_4());
}