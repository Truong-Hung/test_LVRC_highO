#pragma once

#include <iostream>
#include <vector>
#include <assert.h>
#include <stdexcept>

#include <cuda_runtime.h>
#include <optix.h>
#include <optix_stubs.h>

#define CUDA_CHECK(call) {cudaError_t rc = call; if(rc != cudaSuccess){ std::stringstream txt; cudaError_t err =  rc; txt << "CUDA Error " << #call << " at line " << __LINE__ << " " << cudaGetErrorName(err) << " (" << cudaGetErrorString(err) << ")"; throw std::runtime_error(txt.str()); }}
#define CUDA_CHECK_NOEXCEPT(call) {cuda##call;}
#define OPTIX_CHECK(call) {OptixResult res = call; if(res != OPTIX_SUCCESS){ fprintf( stderr, "Optix call (%s) failed with code %d (line %d): %s (%s)\n", #call, res, __LINE__, optixGetErrorName(res), optixGetErrorString(res)); exit(2); }}
#define CUDA_SYNC_CHECK() {cudaDeviceSynchronize(); cudaError_t error = cudaGetLastError(); if(error != cudaSuccess){ fprintf(stderr, "error (%s: line %d): %s\n", __FILE__, __LINE__, cudaGetErrorString(error) ); exit(2); }}



struct CUDABuffer
{
	size_t sizeInBytes {0};
	void* d_ptr {nullptr};

	inline CUdeviceptr d_pointer() const
	{ 
		return (CUdeviceptr) d_ptr; 
	}

	void resize(size_t size, std::string name)
	{
		if(d_ptr) 
			free();

		alloc(size, name);
	}
  
	void alloc(size_t size, std::string name)
	{
		assert(d_ptr == nullptr);
		this->sizeInBytes = size;
		std::cout << "-- " << sizeInBytes  << " B (" << name << ")" << std::endl;
		CUDA_CHECK(cudaMalloc((void**) &d_ptr, sizeInBytes));
	}

	void free()
	{
		CUDA_CHECK(cudaFree(d_ptr));
		d_ptr = nullptr;
		sizeInBytes = 0;
	}

	template<typename T>
	void alloc_and_upload(const std::vector<T> &vt, std::string name)
	{
		if(!vt.size()) return;
		alloc(vt.size()*sizeof(T), name);
		upload((const T*) vt.data(), vt.size());
	}
  
	template<typename T>
	void upload(const T* t, size_t count)
	{
		assert(d_ptr != nullptr);
		assert(sizeInBytes == count*sizeof(T));
		CUDA_CHECK(cudaMemcpy(d_ptr, (void*) t, count*sizeof(T), cudaMemcpyHostToDevice));
	}
  
	template<typename T>
	void download(T* t, size_t count)
	{
		assert(d_ptr != nullptr);
		assert(sizeInBytes == count*sizeof(T));
		CUDA_CHECK(cudaMemcpy((void*) t, d_ptr, count*sizeof(T), cudaMemcpyDeviceToHost));
	}
};

