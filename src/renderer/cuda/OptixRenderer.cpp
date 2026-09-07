#include "renderer/cuda/OptixRenderer.hpp"

#include <cuda_runtime.h>
#include <optix.h>
#include <optix_function_table_definition.h>



extern "C" char embedded_ptx_code[];

template <typename T>
struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) Record
{
    char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T data;
};

typedef Record<RayGenData> RayGenRecord;
typedef Record<MissData> MissRecord;
typedef Record<HitGroupData> HitGroupRecord;

OptixRenderer::OptixRenderer(std::shared_ptr<Mesh> mesh, std::shared_ptr<Camera> camera):
	MeshBasicRenderer(mesh, "OptixRenderer", std::move(camera)),
    animationRate(15.f),
    timeSinceUpdate(0.f),
    animated(false),
    varianceType(0),
    interpolationStrategy(1), // Default to High-Order
    alphaThreshold(.999f),
    opacityThreshold(.0f),
    samplingPower(1.f),
    minSamplePeriod(1.f),
    maxSamplePeriod(2.f),
    boxes(*mesh)
{
    // Preprocessing
    std::cout << "[Preprocessing]" << std::endl;

    boxes.launch(std::max(mesh->number_of_vertices_/512, 1000u));
    boxes.generateData(transferFunctions, currentAttribute, currentTimestep);
    triangularizeMesh();
    triangularizeBoxes();

    std::cout << "[Optix Renderer]" << std::endl;
    
    // Init Optix
    createContext();
    createModule();
    createRaygenPrograms();
    createMissPrograms();
    createHitgroupPrograms();
    createPipeline();
    createTFTexture();
    buildMeshAccelerationStructure();
    buildBoxesAccelerationStructure();
    buildSBT();

    // Init OpenGL
    initOpenGL();
    loadOrReloadShaders();
    initOpenGLRessources();

    


    
    // Generate LS Matrix M+ for Hex20
    std::vector<float> mPlus(200);
    {
        // 1. Build exponent map (deg 2)
        std::vector<std::tuple<int,int,int>> exps;
        for(int deg=0; deg<=2; ++deg) {
            for(int j=0; j<=deg; ++j) {
                for(int k=0; k<=deg-j; ++k) {
                    exps.emplace_back(deg-j-k, j, k);
                }
            }
        }

        std::vector<glm::vec3> nodes = {
            {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1}, // Corners bottom (z=-1)
            {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}, // Corners top (z=1)
            { 0,-1,-1}, { -1, 0,-1}, { -1, -1, 0}, {1, 0,-1}, // Edges bottom
            { 1,-1, 0}, { 0, 1, -1}, { 1, 1, 0}, {-1, 1, 0}, // Edges top
            { 0,-1, 1}, {-1, 0, 1}, { 1, 0, 1}, {0, 1, 1}  // Edges vertical
        };
       

        // 3. Build M (20x10)
        // We use a flat vector for matrix operations: index = row * cols + col
        std::vector<float> M(200); 
        for(int r=0; r<20; ++r) {
            float g = nodes[r].x;
            float h = nodes[r].y;
            float r_coord = nodes[r].z;
            for(int c=0; c<10; ++c) {
                auto [u,v,w] = exps[c];
                M[r*10 + c] = std::pow(g, u) * std::pow(h, v) * std::pow(r_coord, w);
            }
        }

        // 4. Compute Pseudo-Inverse M+ = (M^T * M)^-1 * M^T
        // M^T (10x20)
        std::vector<float> MT(200);
        for(int r=0; r<10; ++r)
            for(int c=0; c<20; ++c)
                MT[r*20 + c] = M[c*10 + r];

        // M^T * M (10x10)
        std::vector<float> MTM(100, 0.f);
        for(int r=0; r<10; ++r)
            for(int c=0; c<10; ++c)
                for(int k=0; k<20; ++k)
                    MTM[r*10 + c] += MT[r*20 + k] * M[k*10 + c];

        // Inverse (M^T * M)^-1 using Gaussian elimination
        std::vector<float> MTM_Inv(100, 0.f);
        // Initialize identity
        for(int i=0; i<10; ++i) MTM_Inv[i*10 + i] = 1.f;

        // Gaussian elimination
        for(int i=0; i<10; ++i) {
            float pivot = MTM[i*10 + i];
            for(int j=0; j<10; ++j) {
                MTM[i*10 + j] /= pivot;
                MTM_Inv[i*10 + j] /= pivot;
            }
            for(int k=0; k<10; ++k) {
                if(k != i) {
                    float factor = MTM[k*10 + i];
                    for(int j=0; j<10; ++j) {
                        MTM[k*10 + j] -= factor * MTM[i*10 + j];
                        MTM_Inv[k*10 + j] -= factor * MTM_Inv[i*10 + j];
                    }
                }
            }
        }

        // M+ = (M^T * M)^-1 * M^T  (10x10 * 10x20 -> 10x20)
        // Result is 10 rows, 20 cols. 
        // We store it row-major, so mPlus[r*20 + c]
        for(int r=0; r<10; ++r) {
            for(int c=0; c<20; ++c) {
                float sum = 0.f;
                for(int k=0; k<10; ++k) {
                    sum += MTM_Inv[r*10 + k] * MT[k*20 + c];
                }
                mPlus[r*20 + c] = sum;
            }
        }
    }

    // Upload Hex20 M+ for the case where we use mononomial basis
    leastSquaresMatrixBuffer.alloc_and_upload(mPlus, "Least Squares Matrix Hex20");
    launchData.leastSquaresMatrix = (float*)leastSquaresMatrixBuffer.d_pointer();

    // Init launch parameters
    // Allocate extra images for comparison
    uint32_t imageSize = renderResolution.x * renderResolution.y * sizeof(float4);
    imageLinearBuffer.alloc(imageSize, "Image Linear Buffer");
    imageHighOrderBuffer.alloc(imageSize, "Image High-Order Buffer");

    launchDataBuffer.alloc(sizeof(LaunchData), "Launch data buffer");

    // Used memory info
    size_t freeMemory, usedMemory, totalMemory;

    cudaMemGetInfo(&freeMemory, &totalMemory);
    freeMemory /= 1048576;
    totalMemory /= 1048576;
    usedMemory = totalMemory - freeMemory;
    std::cout << "-- using " << usedMemory << " MB of " << totalMemory << " MB (" << (((float) usedMemory)/((float) totalMemory))*100.f << "%)" << std::endl;
}

void OptixRenderer::triangularizeMesh()
{
    std::cout << "-- triangularizing mesh ..." << std::endl;

    Triangle currentTriangle;
    std::vector<uint32_t> currentFaceVertexIDs;
    std::vector<uint32_t> localCellOffset;

    // Compute local cell index offset
    uint32_t currentOffset = 0;
    uint32_t numberOfCellTypes = Cell::get_number_of_cell_types();
    localCellOffset.resize(numberOfCellTypes);

    for(uint32_t cellType = 0; cellType < numberOfCellTypes; cellType++){
        for(uint32_t offset = cellType; offset < numberOfCellTypes; offset++){
            localCellOffset[offset] += currentOffset;
        }

        currentOffset = mesh->number_of_cells_per_type_[cellType];
    }

    // For each unique face
    for(uint32_t face = 0; face < mesh->number_of_unique_faces_; face++){
        // Retrieve the indices of the vertices of the face
        currentFaceVertexIDs = mesh->get_face(mesh->faces_[face]);

        // Retrieve additionnal information on the face
        currentTriangle.backCellID = mesh->back_cells_[face];
        currentTriangle.backCellType = mesh->get_cell_type(currentTriangle.backCellID);
        currentTriangle.backCellID -= localCellOffset[currentTriangle.backCellType];

        if(mesh->boundary_flags_[face]){
            currentTriangle.frontCellID = UINT32_MAX;
            currentTriangle.frontCellType = UINT32_MAX;
        }else{
            currentTriangle.frontCellID = mesh->front_cells_[face];
            currentTriangle.frontCellType = mesh->get_cell_type(currentTriangle.frontCellID);
            currentTriangle.frontCellID -= localCellOffset[currentTriangle.frontCellType];
        }

        // For each type of face
        // !! Only first order triangles, first order squares and second order squares for now
        switch(currentFaceVertexIDs.size()){
            // First order triangle
            case 3:
                // One triangle
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[0]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[1]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[2]);
                meshTrianglesVertexData.push_back(currentTriangle);
                break;
            // First order square
            case 4:
                // Two triangle
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[0]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[1]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[2]);
                meshTrianglesVertexData.push_back(currentTriangle);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[0]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[2]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[3]);
                meshTrianglesVertexData.push_back(currentTriangle);
                break;
            // Second order triangle - recently added for Tet10 element
            case 6:
                // One triangle
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[0]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[2]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[4]);
                meshTrianglesVertexData.push_back(currentTriangle);
                break;

            // Second order square (8 vertices, Hex20 face)
            case 8:
                // Two triangle
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[0]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[2]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[4]);
                meshTrianglesVertexData.push_back(currentTriangle);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[0]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[4]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[6]);
                meshTrianglesVertexData.push_back(currentTriangle);
                break;

            // Second order full square (9 vertices, Hex27 face)
            case 9:
                // Two triangle using corner vertices (indices 0, 2, 4, 6)
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[0]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[2]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[4]);
                meshTrianglesVertexData.push_back(currentTriangle);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[0]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[4]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[6]);
                meshTrianglesVertexData.push_back(currentTriangle);
                break;

            // Third order triangle (10 vertices, Tet20 face)
            case 10:
                // One triangle using 3 corner vertices (indices 0, 3, 6)
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[0]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[3]);
                meshTrianglesVertexIDs.push_back(currentFaceVertexIDs[6]);
                meshTrianglesVertexData.push_back(currentTriangle);
                break;
            default:
                break;
        }
    }
}

void OptixRenderer::triangularizeBoxes()
{
    std::cout << "-- triangularizing boxes ..." << std::endl;

    glm::vec3 min, max;
    uint32_t currentCubeOffset = 0;

    std::vector<uint32_t> trianglesLocalIDs = {
        0, 3, 2,    0, 2, 1,
        0, 4, 7,    0, 7, 3,
        0, 1, 5,    0, 5, 4,
        1, 2, 6,    1, 6, 5,
        2, 3, 7,    2, 7, 6,
        4, 5, 6,    4, 6, 7 
    };

    for(uint32_t b = 0; b < boxes.numberOfBoxes_; b++){
        // Retrieve bounding box min and max vertices
        min = boxes.boxAABBMins_[b];
        max = boxes.boxAABBMaxs_[b];

        // Create the 8 vertices of the box
        boxesTrianglesVertexCoords.push_back(glm::vec3(min.x, min.y, min.z));
        boxesTrianglesVertexCoords.push_back(glm::vec3(max.x, min.y, min.z));
        boxesTrianglesVertexCoords.push_back(glm::vec3(max.x, max.y, min.z));
        boxesTrianglesVertexCoords.push_back(glm::vec3(min.x, max.y, min.z));
        boxesTrianglesVertexCoords.push_back(glm::vec3(min.x, min.y, max.z));
        boxesTrianglesVertexCoords.push_back(glm::vec3(max.x, min.y, max.z));
        boxesTrianglesVertexCoords.push_back(glm::vec3(max.x, max.y, max.z));
        boxesTrianglesVertexCoords.push_back(glm::vec3(min.x, max.y, max.z));

        // Compute the offsetted triangles IDs
        for(uint32_t ID = 0; ID < trianglesLocalIDs.size(); ID++)
            boxesTrianglesVertexIDs.push_back(trianglesLocalIDs[ID] + currentCubeOffset);

        // Update index offset
        currentCubeOffset += 8;
    }
}

void OptixRenderer::render()
{
    MeshBasicRenderer::render();

    glViewport(0, 0, renderResolution.x, renderResolution.y);
    
    // CUDA/OpenGL interoperability
    size_t renderPBOSize = renderResolution.x*renderResolution.y*sizeof(float4);
    CUDA_CHECK(cudaGraphicsMapResources(1, &cudaPBODestResource, 0));
    CUDA_CHECK(cudaGraphicsResourceGetMappedPointer((void**) &launchData.image, &renderPBOSize, cudaPBODestResource));

    // Animation update
    if(animated){
        timeSinceUpdate += 1000.f/ImGui::GetIO().Framerate;

        if(timeSinceUpdate > 1000.f/animationRate){
            // One step variance
            if(varianceType == 0){
                boxes.generateData(transferFunctions, currentAttribute, currentTimestep);
                dataDataBuffer.upload(mesh->physical_datas_[currentAttribute][currentTimestep].data(), mesh->physical_datas_[currentAttribute][currentTimestep].size());   
                maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[0].data(), boxes.maxOpacities_[0].size());
                dataVariancesDataBuffer.upload(boxes.dataVariances_[0].data(), boxes.dataVariances_[0].size());
            }

            // Global variance
            if(varianceType == 1){
                dataDataBuffer.upload(mesh->physical_datas_[currentAttribute][currentTimestep].data(), mesh->physical_datas_[currentAttribute][currentTimestep].size());   
                maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[currentTimestep].data(), boxes.maxOpacities_[currentTimestep].size());
                dataVariancesDataBuffer.upload(boxes.dataVariances_[currentTimestep].data(), boxes.dataVariances_[currentTimestep].size());
            }

            // Progressive variance
            if(varianceType == 2){
                boxes.updateDataProgressive(transferFunctions, currentAttribute, currentTimestep);
                dataDataBuffer.upload(mesh->physical_datas_[currentAttribute][currentTimestep].data(), mesh->physical_datas_[currentAttribute][currentTimestep].size());   
                maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[0].data(), boxes.maxOpacities_[0].size());
                dataVariancesDataBuffer.upload(boxes.dataVariances_[0].data(), boxes.dataVariances_[0].size());
            }

            currentTimestep = (currentTimestep + 1)%(mesh->physical_data_n_steps_[currentAttribute]);
            timeSinceUpdate = 0.f;
        }
    }

    // Update Optix launch parameters
    updateCamera();
    
    launchData.imageWidth = renderResolution.x;
    launchData.imageHeight = renderResolution.y;

    launchData.alphaThreshold = alphaThreshold;
    launchData.opacityThreshold = opacityThreshold;
    launchData.samplingPower = samplingPower;
    launchData.minSamplePeriod = minSamplePeriod;
    launchData.maxSamplePeriod = maxSamplePeriod;
    launchData.interpolationStrategy = interpolationStrategy;
    launchData.tfMin = transferFunctions[currentAttribute].get_min();
    launchData.tfMax = transferFunctions[currentAttribute].get_max();

    // Use the already mapped PBO pointer
    float4 *d_image = launchData.image;

    if(interpolationStrategy == 2){
        computeImageDifference();
    }else{
        launchData.image = d_image;
        launchDataBuffer.upload(&launchData, 1);
        shaderBindingTable.raygenRecord = rayGenRecordsBuffer.d_pointer(); // Index 0: Standard

        // Optix launch
        OPTIX_CHECK(optixLaunch(
            pipeline,
            stream,
            launchDataBuffer.d_pointer(),
            launchDataBuffer.sizeInBytes,
            &shaderBindingTable,
            launchData.imageWidth,
            launchData.imageHeight,
            1));
    }

    // CUDA/OpenGL interoperability
    CUDA_CHECK(cudaGraphicsUnmapResources(1, &cudaPBODestResource, stream));

    // Texture for rendering
    getRenderTexture()->bind();

    // Download texture from destination PBO
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, renderPBO);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, renderResolution.x, renderResolution.y, GL_RGBA, GL_FLOAT, NULL);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // Draw render texture on screen
    glBindVertexArray(renderVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void OptixRenderer::renderUI()
{
    MeshBasicRenderer::renderUI();
    
    if(mesh->physical_data_n_steps_[currentAttribute] > 1){
        if(ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen|ImGuiTreeNodeFlags_AllowItemOverlap)){
            // Handle data with multiple timesteps
            int oldTimestep = currentTimestep;
            int selectedTimestep = currentTimestep;

            if(ImGui::Checkbox("Animate", &animated)) markRenderDirty();
            if(ImGui::SliderInt("Timestep", &selectedTimestep, 0, mesh->physical_data_n_steps_[currentAttribute] - 1)) markRenderDirty();

            // Variance type
            int oldVarianceType = varianceType;

            ImGui::RadioButton("One step variance", &varianceType, 0); 
            ImGui::SameLine();
            ImGui::RadioButton("Global variance", &varianceType, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Progressive variance", &varianceType, 2);

            // Regenerate variance
            if(oldVarianceType != varianceType){
                // One step variance
                if(varianceType == 0){
                    boxes.generateData(transferFunctions, currentAttribute, currentTimestep);
                    maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[0].data(), boxes.maxOpacities_[0].size());
                    dataVariancesDataBuffer.upload(boxes.dataVariances_[0].data(), boxes.dataVariances_[0].size());
                }

                // Global variance
                if(varianceType == 1){
                    boxes.generateData(transferFunctions, currentAttribute);
                    maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[currentTimestep].data(), boxes.maxOpacities_[currentTimestep].size());
                    dataVariancesDataBuffer.upload(boxes.dataVariances_[currentTimestep].data(), boxes.dataVariances_[currentTimestep].size());
                }

                // Progressive variance
                if(varianceType == 2){
                    boxes.initDataProgressive(transferFunctions, currentAttribute, currentTimestep);
                    dataDataBuffer.upload(mesh->physical_datas_[currentAttribute][currentTimestep].data(), mesh->physical_datas_[currentAttribute][currentTimestep].size());   
                    maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[0].data(), boxes.maxOpacities_[0].size());
                    dataVariancesDataBuffer.upload(boxes.dataVariances_[0].data(), boxes.dataVariances_[0].size());
                }
            }

            // Selection of timestep
            if(oldTimestep != selectedTimestep){
                currentTimestep = selectedTimestep;

                // One step variance
                if(varianceType == 0){
                    boxes.generateData(transferFunctions, currentAttribute, currentTimestep);
                    dataDataBuffer.upload(mesh->physical_datas_[currentAttribute][currentTimestep].data(), mesh->physical_datas_[currentAttribute][currentTimestep].size());   
                    maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[0].data(), boxes.maxOpacities_[0].size());
                    dataVariancesDataBuffer.upload(boxes.dataVariances_[0].data(), boxes.dataVariances_[0].size());
                }

                // Global variance
                if(varianceType == 1){
                    dataDataBuffer.upload(mesh->physical_datas_[currentAttribute][currentTimestep].data(), mesh->physical_datas_[currentAttribute][currentTimestep].size());   
                    maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[currentTimestep].data(), boxes.maxOpacities_[currentTimestep].size());
                    dataVariancesDataBuffer.upload(boxes.dataVariances_[currentTimestep].data(), boxes.dataVariances_[currentTimestep].size());
                }

                // Progressive variance
                if(varianceType == 2){
                    boxes.initDataProgressive(transferFunctions, currentAttribute, currentTimestep);
                    dataDataBuffer.upload(mesh->physical_datas_[currentAttribute][currentTimestep].data(), mesh->physical_datas_[currentAttribute][currentTimestep].size());   
                    maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[0].data(), boxes.maxOpacities_[0].size());
                    dataVariancesDataBuffer.upload(boxes.dataVariances_[0].data(), boxes.dataVariances_[0].size());
                }

                timeSinceUpdate = 0.f;
            }
        }
    }

    if(ImGui::CollapsingHeader("Render options", ImGuiTreeNodeFlags_DefaultOpen|ImGuiTreeNodeFlags_AllowItemOverlap))
        if(ImGui::SliderFloat("Alpha threshold", &alphaThreshold, 0.f, .99f)) markRenderDirty();

    if(ImGui::CollapsingHeader("Empty space skipping", ImGuiTreeNodeFlags_DefaultOpen|ImGuiTreeNodeFlags_AllowItemOverlap))
        if(ImGui::SliderFloat("Opacity threshold", &opacityThreshold, .0f, 1.f)) markRenderDirty();

    if(ImGui::CollapsingHeader("Adaptive sampling", ImGuiTreeNodeFlags_DefaultOpen|ImGuiTreeNodeFlags_AllowItemOverlap)){
        if(ImGui::SliderFloat("Sampling power", &samplingPower, 1.f, 5.f)) markRenderDirty();
        if(ImGui::SliderFloat("Min sample period", &minSamplePeriod, .005f, maxSamplePeriod)) markRenderDirty();
        if(ImGui::SliderFloat("Max sample period", &maxSamplePeriod, minSamplePeriod, 6.f)) markRenderDirty();
    }

    if(ImGui::CollapsingHeader("Interpolation Strategy", ImGuiTreeNodeFlags_DefaultOpen|ImGuiTreeNodeFlags_AllowItemOverlap)){
        if(ImGui::RadioButton("Linear", &interpolationStrategy, 0)) markRenderDirty();
        ImGui::SameLine();
        if(ImGui::RadioButton("High-Order (Least Squares)", &interpolationStrategy, 1)) markRenderDirty();
        ImGui::SameLine();
        if(ImGui::RadioButton("Difference", &interpolationStrategy, 2)) markRenderDirty();
    }
}

void OptixRenderer::updateOrCreateTransferFunctionTexture()
{
    uint32_t width = transferFunctionSamples[currentAttribute].size();
    uint32_t height = 1;
    uint32_t pitch = width*sizeof(float4);

    CUDA_CHECK(cudaMemcpy2DToArray(
        textureArray,
        0,
        0,
        reinterpret_cast<float4*>(transferFunctionSamples[currentAttribute].data()),
        pitch,
        pitch,
        height,
        cudaMemcpyHostToDevice));

    // One step variance
    if(varianceType == 0){
        boxes.generateData(transferFunctions, currentAttribute, currentTimestep);
    }

    // Global variance
    if(varianceType == 1){
        boxes.generateData(transferFunctions, currentAttribute);
    }

    // Progressive variance
    if(varianceType == 2){
        boxes.initDataProgressive(transferFunctions, currentAttribute, currentTimestep);
    }

    updateBoxesDataBuffer();
    markRenderDirty();
}

void OptixRenderer::updateOrCreatePhysicalValuesSSBO()
{
    currentTimestep = 0;

    // One step variance
    if(varianceType == 0){
        boxes.generateData(transferFunctions, currentAttribute, currentTimestep);
    }

    // Global variance
    if(varianceType == 1){
        boxes.generateData(transferFunctions, currentAttribute);
    }

    // Progressive variance
    if(varianceType == 2){
        boxes.initDataProgressive(transferFunctions, currentAttribute, currentTimestep);
    }

    updatePhysicalDataBuffer();
    updateBoxesDataBuffer();
    markRenderDirty();
}

void OptixRenderer::updatePhysicalDataBuffer()
{
    dataDataBuffer.upload(mesh->physical_datas_[currentAttribute][currentTimestep].data(), mesh->physical_datas_[currentAttribute][currentTimestep].size());
    markRenderDirty();
}

void OptixRenderer::updateBoxesDataBuffer()
{
    // One step variance
    if(varianceType == 0){
        maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[0].data(), boxes.maxOpacities_[0].size());
        dataVariancesDataBuffer.upload(boxes.dataVariances_[0].data(), boxes.dataVariances_[0].size());
    }

    // Global variance
    if(varianceType == 1){
        maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[currentTimestep].data(), boxes.maxOpacities_[currentTimestep].size());
        dataVariancesDataBuffer.upload(boxes.dataVariances_[currentTimestep].data(), boxes.dataVariances_[currentTimestep].size());
    }

    // Progressive variance
    if(varianceType == 2){
        maxOpacitiesDataBuffer.upload(boxes.maxOpacities_[0].data(), boxes.maxOpacities_[0].size());
        dataVariancesDataBuffer.upload(boxes.dataVariances_[0].data(), boxes.dataVariances_[0].size());
    }

    markRenderDirty();
}

void OptixRenderer::onShadersReloadRequest()
{

}

void OptixRenderer::updateCamera()
{
    // Retrive camera informations
    float aspectRatio = static_cast<float>(renderResolution.y)/static_cast<float>(renderResolution.x);
    float fieldOfView = camera->getFov();
    glm::mat4 glmViewMatrix = glm::inverse(camera->getViewMatrix());

    // Extract camera frame and position from matrix
    glm::vec4 glmRight = glmViewMatrix[0];
    glm::vec4 glmUp = glmViewMatrix[1];
    glm::vec4 glmDirection = glmViewMatrix[2];
    glm::vec4 glmEye = glmViewMatrix[3];

    // Set UVW frame and eye position
    launchData.cameraEye = make_float3(glmEye.x, glmEye.y, glmEye.z);
    launchData.cameraU = normalize(make_float3(glmRight.x, glmRight.y, glmRight.z));
    launchData.cameraV = normalize(make_float3(glmUp.x, glmUp.y, glmUp.z));
    launchData.cameraW = -1.f*make_float3(glmDirection.x, glmDirection.y, glmDirection.z);

    float ulen = tanf(fieldOfView*M_PIf/360.f);
    float vlen = ulen*aspectRatio;

    launchData.cameraV *= vlen;
    launchData.cameraU *= ulen;
}

void OptixRenderer::initOpenGL() const
{
    glClearColor(0., 0., 0., 0.);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void OptixRenderer::loadOrReloadShaders()
{
}

void OptixRenderer::initOpenGLRessources()
{
    // Create the pixel buffer object
    uint32_t renderPBOSize = renderResolution.x*renderResolution.y*sizeof(float4);
    glGenBuffers(1, &renderPBO);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, renderPBO);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, renderPBOSize, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // Register the pixel buffer object to the CUDA space
    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(&cudaPBODestResource, renderPBO, cudaGraphicsRegisterFlagsNone));

    glGenVertexArrays(1, &renderVAO);
}

bool OptixRenderer::onResize(int width, int height)
{	
    const auto resized = Renderer::onResize(width, height);

	if(resized){
        // Update pixel buffer object
        uint32_t renderPBOSize = renderResolution.x*renderResolution.y*sizeof(float4);
        glBindBuffer(GL_ARRAY_BUFFER, renderPBO);
        glBufferData(GL_ARRAY_BUFFER, renderPBOSize, nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        launchData.imageWidth = renderResolution.x;
        launchData.imageHeight = renderResolution.y;

        uint32_t imageSize = renderResolution.x * renderResolution.y * sizeof(float4);
        imageLinearBuffer.resize(imageSize, "Image Linear Buffer");
        imageHighOrderBuffer.resize(imageSize, "Image High-Order Buffer");
    }

    return resized;
}

void OptixRenderer::createContext()
{
    int numberOfDevices;
    const int deviceID = 0;
    CUresult cudaContextResult;

    cudaFree(0);
    CUDA_CHECK(cudaGetDeviceCount(&numberOfDevices));

    if(numberOfDevices == 0)
        std::cout << "[ERROR] : No CUDA capable devices found" << std::endl;

    OPTIX_CHECK(optixInit());
    CUDA_CHECK(cudaSetDevice(deviceID));
    CUDA_CHECK(cudaStreamCreate(&stream));
    CUDA_CHECK(cudaDeviceSetLimit(cudaLimitStackSize, 16384));
    cudaGetDeviceProperties(&deviceProps, deviceID);
    cudaContextResult = cuCtxGetCurrent(&cudaContext);

    if(cuCtxGetCurrent(&cudaContext) != CUDA_SUCCESS)
        std::cout << "[ERROR] : Cuda context error (code : " << cudaContextResult << ")" << std::endl;

    OPTIX_CHECK(optixDeviceContextCreate(cudaContext, nullptr, &optixContext));
}

void OptixRenderer::createModule()
{
    // Debug (Optix 8.0 only)
    // moduleCompileOptions.optLevel = OPTIX_COMPILE_OPTIMIZATION_LEVEL_0;
    // moduleCompileOptions.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_FULL;
    // No debug
    moduleCompileOptions.optLevel = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
    moduleCompileOptions.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_NONE;
    moduleCompileOptions.maxRegisterCount = 0;

    pipelineCompileOptions = {};
    pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
    pipelineCompileOptions.usesMotionBlur = false;
    pipelineCompileOptions.numPayloadValues = 3;
    pipelineCompileOptions.numAttributeValues = 0;
    pipelineCompileOptions.exceptionFlags = OPTIX_EXCEPTION_FLAG_NONE;
    pipelineCompileOptions.pipelineLaunchParamsVariableName = "launchData";
        
    pipelineLinkOptions.maxTraceDepth = 1;
        
    const std::string ptxCode = embedded_ptx_code;
    // Optix 8.0
    OPTIX_CHECK(optixModuleCreate(
    // Optix 7.4 
    // OPTIX_CHECK(optixModuleCreateFromPTX(
        optixContext,
        &moduleCompileOptions,
        &pipelineCompileOptions,
        ptxCode.c_str(),
        ptxCode.size(),
        nullptr,
        0,
        &module));
}

void OptixRenderer::createRaygenPrograms()
{
    // Two raygen programs: 0 for rendering, 1 for difference
    rayGenProgramGroups.resize(2);
        
    OptixProgramGroupOptions pgOptions = {};
    OptixProgramGroupDesc pgDesc[2] = {};

    pgDesc[0].kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    pgDesc[0].raygen.module = module;           
    pgDesc[0].raygen.entryFunctionName = "__raygen__launch";

    pgDesc[1].kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    pgDesc[1].raygen.module = module;           
    pgDesc[1].raygen.entryFunctionName = "__raygen__difference";

    OPTIX_CHECK(optixProgramGroupCreate(
        optixContext,
        pgDesc,
        2,
        &pgOptions,
        nullptr,
        0,
        rayGenProgramGroups.data()));
}

void OptixRenderer::createMissPrograms()
{
    // 1 miss program
    missProgramGroups.resize(1);

    OptixProgramGroupDesc pgDesc[1] = {};
    OptixProgramGroupOptions pgOptions[1] = {};

    // Miss program
    pgDesc[0].kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    pgDesc[0].miss.module = module;           
    pgDesc[0].miss.entryFunctionName = "__miss__any";;

    OPTIX_CHECK(optixProgramGroupCreate(
        optixContext,
        pgDesc,
        1,
        pgOptions,
        nullptr,
        0,
        missProgramGroups.data()));
}

void OptixRenderer::createHitgroupPrograms()
{
    // 2 closest hit programs
    hitGroupProgramGroups.resize(2);

    OptixProgramGroupDesc pgDesc[2] = {};
    OptixProgramGroupOptions pgOptions[2] = {};

    // Mesh closest hit program
    pgDesc[0].kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    pgDesc[0].hitgroup.moduleCH = module;
    pgDesc[0].hitgroup.entryFunctionNameCH = "__closesthit__mesh";

    // Boxes closest hit program
    pgDesc[1].kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    pgDesc[1].hitgroup.moduleCH = module;
    pgDesc[1].hitgroup.entryFunctionNameCH = "__closesthit__boxes";

    OPTIX_CHECK(optixProgramGroupCreate(
        optixContext,
        pgDesc,
        2,
        pgOptions,
        nullptr,
        0,
        hitGroupProgramGroups.data()));
}

void OptixRenderer::createPipeline()
{
    std::vector<OptixProgramGroup> programGroups;

    for(auto pg : rayGenProgramGroups) programGroups.push_back(pg);
    for(auto pg : missProgramGroups) programGroups.push_back(pg);
    for(auto pg : hitGroupProgramGroups) programGroups.push_back(pg);

    OPTIX_CHECK(optixPipelineCreate(
        optixContext,
        &pipelineCompileOptions,
        &pipelineLinkOptions,
        programGroups.data(),
        programGroups.size(),
        nullptr,
        0,
        &pipeline));

    // Increase stack size to support deep call trees in high-order interpolation
    // This provides enough memory for register spilling during intensive math operations
    uint32_t directStackTraceSize = 0;
    uint32_t continuationStackTraceSize = 4096; // Increased for high-order interpolation
    uint32_t maxTraversableGraphDepth = 1; // Single GAS requires 1

    OPTIX_CHECK(optixPipelineSetStackSize(
        pipeline, 
        directStackTraceSize,
        directStackTraceSize,
        continuationStackTraceSize,
        maxTraversableGraphDepth));
}

void OptixRenderer::createTFTexture()
{
    cudaResourceDesc res_desc = {};
    cudaChannelFormatDesc channel_desc;

    uint32_t width = transferFunctionSamples[currentAttribute].size();
    uint32_t height = 1;
    uint32_t pitch = width*sizeof(float4);

    channel_desc = cudaCreateChannelDesc<float4>();
    
    CUDA_CHECK(cudaMallocArray(
        &textureArray,
        &channel_desc,
        width,
        height));
    
    CUDA_CHECK(cudaMemcpy2DToArray(
        textureArray,
        0,
        0,
        reinterpret_cast<float4*>(transferFunctionSamples[currentAttribute].data()),
        pitch,
        pitch,
        height,
        cudaMemcpyHostToDevice));
    
    res_desc.resType = cudaResourceTypeArray;
    res_desc.res.array.array = textureArray;
    
    cudaTextureDesc tex_desc = {};
    
    tex_desc.addressMode[0] = cudaAddressModeClamp;
    tex_desc.filterMode = cudaFilterModeLinear;
    tex_desc.normalizedCoords = 1;
    
    // Create texture object
    CUDA_CHECK(cudaCreateTextureObject(
        &launchData.transferFunction,
        &res_desc,
        &tex_desc,
        nullptr));
}

void OptixRenderer::buildSBT()
{
    std::vector<RayGenRecord> rayGenRecords;
    std::vector<MissRecord> missRecords;
    std::vector<HitGroupRecord> hitGroupRecords;

    // Upload mesh data
    vertexDataBuffer.alloc_and_upload(mesh->vertices_, "Vertex coordinates");
    dataDataBuffer.alloc_and_upload(mesh->physical_datas_[currentAttribute][currentTimestep], "Physical data field");
    triangleDataBuffer.alloc_and_upload(meshTrianglesVertexData, "Shared triangles");
    tetrahedron4DataBuffer.alloc_and_upload(mesh->cells_[TETRAHEDRON4], "Tetrahedron4");
    hexahedron8DataBuffer.alloc_and_upload(mesh->cells_[HEXAHEDRON8], "Hexahedron8");
    prism6DataBuffer.alloc_and_upload(mesh->cells_[PRISM6], "Prism6");
    pyramid5DataBuffer.alloc_and_upload(mesh->cells_[PYRAMID5], "Pyramid5");
    tetrahedron10DataBuffer.alloc_and_upload(mesh->cells_[TETRAHEDRON10], "Tetrahedron10");
    hexahedron20DataBuffer.alloc_and_upload(mesh->cells_[HEXAHEDRON20], "Hexahedron20");
    hexahedron27DataBuffer.alloc_and_upload(mesh->cells_[HEXAHEDRON_2_FULL], "Hexahedron27");
    tetrahedron20DataBuffer.alloc_and_upload(mesh->cells_[TETRAHEDRON20], "Tetrahedron20");
    
    // Upload boxes data
    maxOpacitiesDataBuffer.alloc_and_upload(boxes.maxOpacities_[0], "Boxes max opacities");
    dataVariancesDataBuffer.alloc_and_upload(boxes.dataVariances_[0], "Boxes data variances");

    // Set ray generation data
    RayGenRecord rayGenData;

    rayGenData.data.mesh.vertices = (float3*) vertexDataBuffer.d_pointer();
    rayGenData.data.mesh.datas = (float*) dataDataBuffer.d_pointer();
    rayGenData.data.mesh.triangles = (Triangle*) triangleDataBuffer.d_pointer();
    rayGenData.data.mesh.cells.tetrahedrons4 = (Tetrahedron4*) tetrahedron4DataBuffer.d_pointer();
    rayGenData.data.mesh.cells.hexahedrons8 = (Hexahedron8*) hexahedron8DataBuffer.d_pointer();
    rayGenData.data.mesh.cells.prisms6 = (Prism6*) prism6DataBuffer.d_pointer();
    rayGenData.data.mesh.cells.pyramids5 = (Pyramid5*) pyramid5DataBuffer.d_pointer();
    rayGenData.data.mesh.cells.tetrahedrons10 = (Tetrahedron10*) tetrahedron10DataBuffer.d_pointer();
    rayGenData.data.mesh.cells.hexahedrons20 = (Hexahedron20*) hexahedron20DataBuffer.d_pointer();
    rayGenData.data.mesh.cells.hexahedrons27 = (Hexahedron27*) hexahedron27DataBuffer.d_pointer();
    rayGenData.data.mesh.cells.tetrahedrons20 = (Tetrahedron20*) tetrahedron20DataBuffer.d_pointer();
    rayGenData.data.boxes.maxOpacities = (float*) maxOpacitiesDataBuffer.d_pointer();
    rayGenData.data.boxes.dataVariances = (float*) dataVariancesDataBuffer.d_pointer();

    // Pack raygen records
    RayGenRecord rayGenDataLaunch;
    memcpy(&rayGenDataLaunch, &rayGenData, sizeof(RayGenRecord));
    OPTIX_CHECK(optixSbtRecordPackHeader(rayGenProgramGroups[0], &rayGenDataLaunch));
    rayGenRecords.push_back(rayGenDataLaunch);

    RayGenRecord rayGenDataDifference;
    memcpy(&rayGenDataDifference, &rayGenData, sizeof(RayGenRecord));
    OPTIX_CHECK(optixSbtRecordPackHeader(rayGenProgramGroups[1], &rayGenDataDifference));
    rayGenRecords.push_back(rayGenDataDifference);

    rayGenRecordsBuffer.alloc_and_upload(rayGenRecords, "Ray generation records");
    shaderBindingTable.raygenRecord = rayGenRecordsBuffer.d_pointer();
    
    // Pack miss records, empty data
    for(uint32_t i = 0; i < missProgramGroups.size(); i++){
        MissRecord missData;
        OPTIX_CHECK(optixSbtRecordPackHeader(missProgramGroups[i], &missData));
        missRecords.push_back(missData);
    }

    missRecordsBuffer.alloc_and_upload(missRecords, "Miss records");
    shaderBindingTable.missRecordBase = missRecordsBuffer.d_pointer();
    shaderBindingTable.missRecordStrideInBytes = sizeof(MissRecord);
    shaderBindingTable.missRecordCount = (int) missRecords.size();

    // Pack hit group records, empty data
    for(uint32_t i = 0; i < hitGroupProgramGroups.size(); i++){
        HitGroupRecord hitGroupData;
        OPTIX_CHECK(optixSbtRecordPackHeader(hitGroupProgramGroups[i], &hitGroupData));
        hitGroupRecords.push_back(hitGroupData);
    }

    hitGroupRecordsBuffer.alloc_and_upload(hitGroupRecords, "Hit group records");
    shaderBindingTable.hitgroupRecordBase = hitGroupRecordsBuffer.d_pointer();
    shaderBindingTable.hitgroupRecordStrideInBytes = sizeof(HitGroupRecord);
    shaderBindingTable.hitgroupRecordCount = (int) hitGroupRecords.size();
}

void OptixRenderer::buildMeshAccelerationStructure()
{
    CUDABuffer vertexBuffer;
    CUDABuffer indexBuffer;
    CUDABuffer asBuffer;

    CUdeviceptr d_vertices;
    CUdeviceptr d_indices;

    // Upload triangle data
    vertexBuffer.alloc_and_upload(mesh->vertices_, "Mesh vertex coordinates, AS");
    indexBuffer.alloc_and_upload(meshTrianglesVertexIDs, "Mesh triangle indices, AS");

    d_vertices = vertexBuffer.d_pointer();
    d_indices  = indexBuffer.d_pointer();
    
    // Set triangle input
    uint32_t triangleInputFlags[1] = {0};

    OptixBuildInput triangleInput = {};

    triangleInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
    triangleInput.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
    triangleInput.triangleArray.vertexStrideInBytes = sizeof(glm::vec3);
    triangleInput.triangleArray.numVertices = static_cast<int>(mesh->vertices_.size());
    triangleInput.triangleArray.vertexBuffers = &d_vertices;
    triangleInput.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
    triangleInput.triangleArray.indexStrideInBytes = sizeof(uint3);
    triangleInput.triangleArray.numIndexTriplets = static_cast<int>(meshTrianglesVertexIDs.size()/3);
    triangleInput.triangleArray.indexBuffer = d_indices;
    triangleInput.triangleArray.flags = triangleInputFlags;
    triangleInput.triangleArray.numSbtRecords = 1;
    triangleInput.triangleArray.sbtIndexOffsetBuffer = 0; 
    triangleInput.triangleArray.sbtIndexOffsetSizeInBytes = 0; 
    triangleInput.triangleArray.sbtIndexOffsetStrideInBytes = 0; 
      
    // Set acceleration structure parameters
    OptixAccelBuildOptions accelOptions = {};
    OptixAccelBufferSizes blasBufferSizes;

    accelOptions.buildFlags = OPTIX_BUILD_FLAG_NONE|OPTIX_BUILD_FLAG_ALLOW_COMPACTION;
    accelOptions.motionOptions.numKeys = 0;
    accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;
    
    OPTIX_CHECK(optixAccelComputeMemoryUsage(
        optixContext,
        &accelOptions,
        &triangleInput,
        1,
        &blasBufferSizes));
    
    // Prepare acceleration structure building
    CUDABuffer compactedSizeBuffer;
    CUDABuffer tempBuffer;
    CUDABuffer outputBuffer;

    OptixAccelEmitDesc emitDesc;

    compactedSizeBuffer.alloc(sizeof(uint64_t), "Mesh compacted AS size");
    
    emitDesc.type = OPTIX_PROPERTY_TYPE_COMPACTED_SIZE;
    emitDesc.result = compactedSizeBuffer.d_pointer();
    
    // Build acceleration structure
    tempBuffer.alloc(blasBufferSizes.tempSizeInBytes, "Mesh temp AS");
    outputBuffer.alloc(blasBufferSizes.outputSizeInBytes, "Mesh output AS");
      
    OPTIX_CHECK(optixAccelBuild(
        optixContext,
        stream,
        &accelOptions,
        &triangleInput,
        1,  
        tempBuffer.d_pointer(),
        tempBuffer.sizeInBytes,
        outputBuffer.d_pointer(),
        outputBuffer.sizeInBytes,
        &launchData.mesh,
        &emitDesc,1));

    CUDA_SYNC_CHECK();
    
    // Compact acceleration structure
    uint64_t compactedSize;

    compactedSizeBuffer.download(&compactedSize,1);
    asBuffer.alloc(compactedSize, "Mesh final AS");

    OPTIX_CHECK(optixAccelCompact(
        optixContext,
        stream,
        launchData.mesh,
        asBuffer.d_pointer(),
        asBuffer.sizeInBytes,
        &launchData.mesh));

    CUDA_SYNC_CHECK();
    
    // Clean unused buffers
    vertexBuffer.free();
    indexBuffer.free();
    outputBuffer.free();
    tempBuffer.free();
    compactedSizeBuffer.free();
}

void OptixRenderer::buildBoxesAccelerationStructure()
{
    CUDABuffer vertexBuffer;
    CUDABuffer indexBuffer;
    CUDABuffer asBuffer;

    CUdeviceptr d_vertices;
    CUdeviceptr d_indices;

    // Upload triangle data
    vertexBuffer.alloc_and_upload(boxesTrianglesVertexCoords, "Boxes vertex coordinates, GAS");
    indexBuffer.alloc_and_upload(boxesTrianglesVertexIDs, "Boxes triangle indices, GAS");

    d_vertices = vertexBuffer.d_pointer();
    d_indices  = indexBuffer.d_pointer();
    
    // Set triangle input
    uint32_t triangleInputFlags[1] = {0};

    OptixBuildInput triangleInput = {};

    triangleInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
    triangleInput.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
    triangleInput.triangleArray.vertexStrideInBytes = sizeof(glm::vec3);
    triangleInput.triangleArray.numVertices = static_cast<int>(boxesTrianglesVertexCoords.size());
    triangleInput.triangleArray.vertexBuffers = &d_vertices;
    triangleInput.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
    triangleInput.triangleArray.indexStrideInBytes = sizeof(uint3);
    triangleInput.triangleArray.numIndexTriplets = static_cast<int>(boxesTrianglesVertexIDs.size()/3);
    triangleInput.triangleArray.indexBuffer = d_indices;
    triangleInput.triangleArray.flags = triangleInputFlags;
    triangleInput.triangleArray.numSbtRecords = 1;
    triangleInput.triangleArray.sbtIndexOffsetBuffer = 0; 
    triangleInput.triangleArray.sbtIndexOffsetSizeInBytes = 0; 
    triangleInput.triangleArray.sbtIndexOffsetStrideInBytes = 0; 
      
    // Set acceleration structure parameters
    OptixAccelBuildOptions accelOptions = {};
    OptixAccelBufferSizes blasBufferSizes;

    accelOptions.buildFlags = OPTIX_BUILD_FLAG_NONE|OPTIX_BUILD_FLAG_ALLOW_COMPACTION;
    accelOptions.motionOptions.numKeys = 0;
    accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;
    
    OPTIX_CHECK(optixAccelComputeMemoryUsage(
        optixContext,
        &accelOptions,
        &triangleInput,
        1,
        &blasBufferSizes));
    
    // Prepare acceleration structure building
    CUDABuffer compactedSizeBuffer;
    CUDABuffer tempBuffer;
    CUDABuffer outputBuffer;

    OptixAccelEmitDesc emitDesc;

    compactedSizeBuffer.alloc(sizeof(uint64_t), "Boxes compacted AS size");
    
    emitDesc.type = OPTIX_PROPERTY_TYPE_COMPACTED_SIZE;
    emitDesc.result = compactedSizeBuffer.d_pointer();
    
    // Build acceleration structure
    tempBuffer.alloc(blasBufferSizes.tempSizeInBytes, "Boxes temp AS");
    outputBuffer.alloc(blasBufferSizes.outputSizeInBytes, "Boxes output AS");
      
    OPTIX_CHECK(optixAccelBuild(
        optixContext,
        stream,
        &accelOptions,
        &triangleInput,
        1,  
        tempBuffer.d_pointer(),
        tempBuffer.sizeInBytes,
        outputBuffer.d_pointer(),
        outputBuffer.sizeInBytes,
        &launchData.boxes,
        &emitDesc,1));

    CUDA_SYNC_CHECK();
    
    // Compact acceleration structure
    uint64_t compactedSize;

    compactedSizeBuffer.download(&compactedSize,1);
    asBuffer.alloc(compactedSize, "Boxes final AS");

    OPTIX_CHECK(optixAccelCompact(
        optixContext,
        stream,
        launchData.boxes,
        asBuffer.d_pointer(),
        asBuffer.sizeInBytes,
        &launchData.boxes));

    CUDA_SYNC_CHECK();
    
    // Clean unused buffers
    vertexBuffer.free();
    indexBuffer.free();
    outputBuffer.free();
    tempBuffer.free();
    compactedSizeBuffer.free();
}

void OptixRenderer::computeImageDifference()
{
    size_t size;
    float4 *d_image;
    CUDA_CHECK(cudaGraphicsResourceGetMappedPointer((void**) &d_image, &size, cudaPBODestResource));

    // Pass 1: Linear
    launchData.image = (float4*)imageLinearBuffer.d_pointer();
    launchData.interpolationStrategy = 0;
    launchDataBuffer.upload(&launchData, 1);
    shaderBindingTable.raygenRecord = rayGenRecordsBuffer.d_pointer(); // Index 0: Standard
    OPTIX_CHECK(optixLaunch(pipeline, stream, launchDataBuffer.d_pointer(), sizeof(LaunchData), &shaderBindingTable, launchData.imageWidth, launchData.imageHeight, 1));

    // Pass 2: High Order
    launchData.image = (float4*)imageHighOrderBuffer.d_pointer();
    launchData.interpolationStrategy = 1;
    launchDataBuffer.upload(&launchData, 1);
    OPTIX_CHECK(optixLaunch(pipeline, stream, launchDataBuffer.d_pointer(), sizeof(LaunchData), &shaderBindingTable, launchData.imageWidth, launchData.imageHeight, 1));

    // Pass 3: Difference
    launchData.image = d_image;
    launchData.image1 = (float4*)imageLinearBuffer.d_pointer();
    launchData.image2 = (float4*)imageHighOrderBuffer.d_pointer();
    launchData.interpolationStrategy = 2; // Restore state
    launchDataBuffer.upload(&launchData, 1);
    shaderBindingTable.raygenRecord = rayGenRecordsBuffer.d_pointer() + 1 * sizeof(RayGenRecord);
    OPTIX_CHECK(optixLaunch(pipeline, stream, launchDataBuffer.d_pointer(), sizeof(LaunchData), &shaderBindingTable, launchData.imageWidth, launchData.imageHeight, 1));
}