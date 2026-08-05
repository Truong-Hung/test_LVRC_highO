#ifndef LVRC_OPTIXRENDERER_HPP
#define LVRC_OPTIXRENDERER_HPP

#include <memory>
#include <fstream>

#include "renderer/MeshBasicRenderer.hpp"
#include "renderer/cuda/utils/CUDAStructs.h"
#include "renderer/cuda/utils/CUDABuffer.h"
#include "renderer/cuda/utils/CUDAMath.h"
#include "renderer/opengl/utils/Texture.hpp"
#include "renderer/opengl/utils/ShaderProgram.hpp"
#include "mesh/preprocessing/boxes/Boxes.hpp"

#include <vector_types.h>
#include <optix_types.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <GL/gl.h>



class OptixRenderer : public MeshBasicRenderer
{
public:

private:
    // Animation
    float animationRate;
    float timeSinceUpdate;
    bool animated;
    int varianceType;
    int interpolationStrategy;

    // ESS and AS
    float alphaThreshold;
    float opacityThreshold;
    float samplingPower;
    float minSamplePeriod;
    float maxSamplePeriod;
    
    // Mesh/boxes
    Boxes boxes;

    std::vector<uint32_t> meshTrianglesVertexIDs;
    std::vector<Triangle> meshTrianglesVertexData;

    std::vector<glm::vec3> boxesTrianglesVertexCoords;
    std::vector<uint32_t> boxesTrianglesVertexIDs;

    // OpenGL
    GLuint renderPBO = 0;
    GLuint renderVAO = 0;

    // CUDA/OptiX
    CUcontext cudaContext;
    CUstream stream;
    cudaDeviceProp deviceProps;

    struct cudaGraphicsResource* cudaPBODestResource;

    OptixDeviceContext optixContext;

    OptixPipeline pipeline;
    OptixPipelineCompileOptions pipelineCompileOptions = {};
    OptixPipelineLinkOptions pipelineLinkOptions    = {};

    OptixModule module;
    OptixModuleCompileOptions moduleCompileOptions = {};

    std::vector<OptixProgramGroup> rayGenProgramGroups;
    std::vector<OptixProgramGroup> missProgramGroups;
    std::vector<OptixProgramGroup> hitGroupProgramGroups;

    CUDABuffer rayGenRecordsBuffer;
    CUDABuffer missRecordsBuffer;
    CUDABuffer hitGroupRecordsBuffer;

    OptixShaderBindingTable shaderBindingTable = {};

    LaunchData launchData;
    
    CUDABuffer launchDataBuffer;

    CUDABuffer maxOpacitiesDataBuffer;
    CUDABuffer dataVariancesDataBuffer;

    CUDABuffer vertexDataBuffer;
    CUDABuffer dataDataBuffer;
    CUDABuffer triangleDataBuffer;
    CUDABuffer tetrahedron4DataBuffer;
    CUDABuffer tetrahedron10DataBuffer;
    CUDABuffer hexahedron8DataBuffer;
    CUDABuffer prism6DataBuffer;
    CUDABuffer pyramid5DataBuffer;
    CUDABuffer hexahedron20DataBuffer;
    CUDABuffer hitgroupRecordsDataBuffer; 
    CUDABuffer leastSquaresMatrixBuffer; // Hex20 10x20 M+
    CUDABuffer tet4MatrixBuffer;         // Tet4  10x4  M+
    CUDABuffer hex8MatrixBuffer;         // Hex8  10x8  M+
    CUDABuffer tet10MatrixBuffer;        // Tet10 10x10 M+
    CUDABuffer imageLinearBuffer;
    CUDABuffer imageHighOrderBuffer;

    cudaArray_t textureArray;
    cudaTextureObject_t textureObject;

public:
    explicit OptixRenderer(std::shared_ptr<Mesh> mesh, std::shared_ptr<Camera> camera);

    void render() override;
    bool onResize(int width, int height) override;

private:
    // Mesh/Boxes
    void triangularizeMesh();
    void triangularizeBoxes();

    // LVRC
    void renderUI() override;
    void onShadersReloadRequest() override;
    void updateOrCreateTransferFunctionTexture() override;
    void updateOrCreatePhysicalValuesSSBO() override;
    void updatePhysicalDataBuffer();
    void updateBoxesDataBuffer();
    void updateCamera();

    // OpenGL
    void initOpenGL() const;
    void loadOrReloadShaders();
    void initOpenGLRessources();
    
    // CUDA/OptiX
    void createContext();
    void createModule();
    void createRaygenPrograms();
    void createMissPrograms();
    void createHitgroupPrograms();
    void createPipeline();
    void createTFTexture();
    void buildSBT();
    void buildMeshAccelerationStructure();
    void buildBoxesAccelerationStructure(); 
    void computeImageDifference();
};

#endif
