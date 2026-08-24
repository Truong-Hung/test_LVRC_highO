///
/// \file Boxes.cpp
/// \brief Source file of Boxes
///

#include <algorithm>

#include "mesh/preprocessing/boxes/Boxes.hpp"



Boxes::Boxes(Mesh& mesh) :
    mesh_(mesh)
{
    // Unsorted vertex indices
    firstBox_.vertices_.resize(mesh_.number_of_vertices_);

    for(uint32_t v = 0; v < mesh.number_of_vertices_; v++)
        firstBox_.vertices_[v] = v;

    // AABB
    firstBox_.AABB_[0] = mesh_.AABB_[0];
    firstBox_.AABB_[1] = mesh_.AABB_[1];
    
    // Splitting axis (0 : X, 1 : Y, 2 : Z)
    // Initialized at Z, so next split is X
    firstBox_.currentSplitAxis_ = 2;
}

void Boxes::launch(uint32_t maxVerticesPerBox)
{
    std::cout << "-- generating boxes (" << maxVerticesPerBox << " vertices max.)" << std::endl;

    // Init box list
    std::vector<Box> boxes;

    boxes.push_back(firstBox_);

    // Start splitting
    uint32_t currentBox = 0;
    Box left, right;

    // Split while there are still splittables boxes
    while(currentBox < boxes.size()){
        // If box in non splittable, skip
        if(boxes[currentBox].vertices_.size() <= maxVerticesPerBox){
            currentBox++;
        // Else split that box
        }else{
            split(boxes[currentBox], left, right);

            // Replace current box by left child
            boxes[currentBox] = left;

            // Append right child to the list
            boxes.push_back(right);
        }
    }

    numberOfBoxes_ = boxes.size();
    std::cout << "-- " << numberOfBoxes_ << " boxes created" << std::endl;

    // Update the boxes informations
    computeBoxesParameters(boxes);
}

void Boxes::generateData(
    const std::vector<TransferFunction>& transferFunctions, 
    uint32_t currentAttribute)
{
    // Box attributes
    float numberOfValues;
    float currentMaxOpacity;

    glm::vec4 currentData;
    glm::vec3 currentDataVariance;
    glm::vec3 currentDataMean;
 
    // Variance for each timestep
    maxOpacities_.resize(mesh_.physical_data_n_steps_[currentAttribute]);
    dataVariances_.resize(mesh_.physical_data_n_steps_[currentAttribute]);

    // For each variance   
    for(uint32_t t = 0; t < mesh_.physical_data_n_steps_[currentAttribute]; t++){
        // One value per box
        maxOpacities_[t].resize(numberOfBoxes_);
        dataVariances_[t].resize(numberOfBoxes_);
        
        // For each box
        for(uint32_t b = 0; b < numberOfBoxes_; b++){
            // Initialize values
            currentMaxOpacity = 0.f;
            currentDataVariance = glm::vec3(0.f);
            currentDataMean = glm::vec3(0.f);
            numberOfValues = 0;

            // Get transfer function samples between minimum and maximum values of the box
            float step = 1.f/128.f;

            for(float v = boxValuesMins_[currentAttribute][t][b]; v < boxValuesMaxs_[currentAttribute][t][b]; v += step){
                // Sample transfer function for current data
                currentData = transferFunctions[currentAttribute].getSample(v);

                // Increases number of values for mean computation
                numberOfValues += 1.f;
                
                // Update maximum opacity
                if(currentMaxOpacity < currentData.w)
                    currentMaxOpacity = currentData.w;

                // Accumulate for mean computation
                currentDataMean += glm::vec3(currentData);
            }

            // Mean value
            currentDataMean = currentDataMean/numberOfValues;

            // Compute variance
            for(float v = boxValuesMins_[currentAttribute][t][b]; v < boxValuesMaxs_[currentAttribute][t][b]; v += step){
                // Sample transfer function for current data
                currentData = transferFunctions[currentAttribute].getSample(v);

                // Accumulate for mean computation
                currentDataVariance += glm::vec3(
                    (currentData.x - currentDataMean.x)*(currentData.x - currentDataMean.x),
                    (currentData.y - currentDataMean.y)*(currentData.y - currentDataMean.y),
                    (currentData.z - currentDataMean.z)*(currentData.z - currentDataMean.z));
            }

            // Variance of each of the RGB channels
            currentDataVariance = currentDataVariance/numberOfValues;

            // Maximum opacity for the box
            maxOpacities_[t][b] = currentMaxOpacity;

            // Set maximum RGB variance as maximum variance for the box
            dataVariances_[t][b] = glm::max(glm::max(currentDataVariance.x, currentDataVariance.y), currentDataVariance.z);
        }
    }

    // Normalize the variance
    float m = dataVariances_[0][0];
    float M = dataVariances_[0][0];
    float diff;

    for(uint32_t t = 0; t < mesh_.physical_data_n_steps_[currentAttribute]; t++){
        for(uint32_t b = 1; b < numberOfBoxes_; b++){
            m = std::min(m, dataVariances_[t][b]);
            M = std::max(M, dataVariances_[t][b]);
        }
    }

    diff = M - m;

    for(uint32_t t = 0; t < mesh_.physical_data_n_steps_[currentAttribute]; t++){
        for(uint32_t b = 0; b < numberOfBoxes_; b++){
            dataVariances_[t][b] = (dataVariances_[t][b] - m)/diff;
        }
    }
}

void Boxes::generateData(
    const std::vector<TransferFunction>& transferFunctions, 
    uint32_t currentAttribute, 
    uint32_t currentTimestep)
{
    // Box attributes
    float numberOfValues;
    float currentMaxOpacity;

    glm::vec4 currentData;
    glm::vec3 currentDataVariance;
    glm::vec3 currentDataMean;

    // Only one timestep stored
    maxOpacities_.resize(1);
    dataVariances_.resize(1);

    // One value per box
    maxOpacities_[0].resize(numberOfBoxes_);
    dataVariances_[0].resize(numberOfBoxes_);

    // For each box
    for(uint32_t b = 0; b < numberOfBoxes_; b++){
        // Initialize values
        currentMaxOpacity = 0.f;
        currentDataVariance = glm::vec3(0.f);
        currentDataMean = glm::vec3(0.f);
        numberOfValues = 0.f;

        // Get transfer function samples between minimum and maximum values of the box
        float step = 1.f/128.f;

        for(float v = boxValuesMins_[currentAttribute][currentTimestep][b]; v < boxValuesMaxs_[currentAttribute][currentTimestep][b]; v += step){
            // Sample transfer function for current data
            currentData = transferFunctions[currentAttribute].getSample(v);

            // Increases number of values for mean computation
            numberOfValues += 1.f;
            
            // Update maximum opacity
            if(currentMaxOpacity < currentData.w)
                currentMaxOpacity = currentData.w;

            // Accumulate for mean computation
            currentDataMean += glm::vec3(currentData);
        }

        // Mean value
        currentDataMean = currentDataMean/numberOfValues;

        // Compute variance
        for(float v = boxValuesMins_[currentAttribute][currentTimestep][b]; v < boxValuesMaxs_[currentAttribute][currentTimestep][b]; v += step){
            // Sample transfer function for current data
            currentData = transferFunctions[currentAttribute].getSample(v);

            // Accumulate for mean computation
            currentDataVariance += glm::vec3(
                (currentData.x - currentDataMean.x)*(currentData.x - currentDataMean.x),
                (currentData.y - currentDataMean.y)*(currentData.y - currentDataMean.y),
                (currentData.z - currentDataMean.z)*(currentData.z - currentDataMean.z));
        }

        // Variance of each of the RGB channels
        currentDataVariance = currentDataVariance/numberOfValues;

        // Maximum opacity for the box
        maxOpacities_[0][b] = currentMaxOpacity;

        // Set maximum RGB variance as maximum variance for the box
        dataVariances_[0][b] = glm::max(glm::max(currentDataVariance.x, currentDataVariance.y), currentDataVariance.z);
    }

    // Normalize the variance
    float m = dataVariances_[0][0];
    float M = dataVariances_[0][0];
    float diff;

    for(uint32_t b = 1; b < numberOfBoxes_; b++){
        m = std::min(m, dataVariances_[0][b]);
        M = std::max(M, dataVariances_[0][b]);
    }

    diff = M - m;

    for(uint32_t b = 0; b < numberOfBoxes_; b++)
        dataVariances_[0][b] = (dataVariances_[0][b] - m)/diff;
}

void Boxes::initDataProgressive(
    const std::vector<TransferFunction>& transferFunctions, 
    uint32_t currentAttribute, 
    uint32_t currentTimestep)
{
    // Box attributes
    float numberOfValues;
    float currentMaxOpacity;

    glm::vec4 currentData;
    glm::vec3 currentDataVariance;
    glm::vec3 currentDataMean;

    // Currrent and next timestep stored
    maxOpacities_.resize(2);
    dataVariances_.resize(2);

    // One value per box
    maxOpacities_[0].resize(numberOfBoxes_);
    dataVariances_[0].resize(numberOfBoxes_);

    // For each box
    for(uint32_t b = 0; b < numberOfBoxes_; b++){
        // Initialize values
        currentMaxOpacity = 0.f;
        currentDataVariance = glm::vec3(0.f);
        currentDataMean = glm::vec3(0.f);
        numberOfValues = 0.f;

        // Get transfer function samples between minimum and maximum values of the box
        float step = 1.f/128.f;

        for(float v = boxValuesMins_[currentAttribute][currentTimestep][b]; v < boxValuesMaxs_[currentAttribute][currentTimestep][b]; v += step){
            // Sample transfer function for current data
            currentData = transferFunctions[currentAttribute].getSample(v);

            // Increases number of values for mean computation
            numberOfValues += 1.f;
            
            // Update maximum opacity
            if(currentMaxOpacity < currentData.w)
                currentMaxOpacity = currentData.w;

            // Accumulate for mean computation
            currentDataMean += glm::vec3(currentData);
        }

        // Mean value
        currentDataMean = currentDataMean/numberOfValues;

        // Compute variance
        for(float v = boxValuesMins_[currentAttribute][currentTimestep][b]; v < boxValuesMaxs_[currentAttribute][currentTimestep][b]; v += step){
            // Sample transfer function for current data
            currentData = transferFunctions[currentAttribute].getSample(v);

            // Accumulate for mean computation
            currentDataVariance += glm::vec3(
                (currentData.x - currentDataMean.x)*(currentData.x - currentDataMean.x),
                (currentData.y - currentDataMean.y)*(currentData.y - currentDataMean.y),
                (currentData.z - currentDataMean.z)*(currentData.z - currentDataMean.z));
        }

        // Variance of each of the RGB channels
        currentDataVariance = currentDataVariance/numberOfValues;

        // Maximum opacity for the box
        maxOpacities_[0][b] = currentMaxOpacity;

        // Set maximum RGB variance as maximum variance for the box
        dataVariances_[0][b] = glm::max(glm::max(currentDataVariance.x, currentDataVariance.y), currentDataVariance.z);
    }

    // Next timestep
    uint32_t nextTimestep = (currentTimestep + 1)%mesh_.physical_data_n_steps_[currentAttribute];
    
    // One value per box
    maxOpacities_[1].resize(numberOfBoxes_);
    dataVariances_[1].resize(numberOfBoxes_);

    // For each box
    for(uint32_t b = 0; b < numberOfBoxes_; b++){
        // Initialize values
        currentMaxOpacity = 0.f;
        currentDataVariance = glm::vec3(0.f);
        currentDataMean = glm::vec3(0.f);
        numberOfValues = 0.f;

        // Get transfer function samples between minimum and maximum values of the box
        float step = 1.f/128.f;

        for(float v = boxValuesMins_[currentAttribute][nextTimestep][b]; v < boxValuesMaxs_[currentAttribute][nextTimestep][b]; v += step){
            // Sample transfer function for current data
            currentData = transferFunctions[currentAttribute].getSample(v);

            // Increases number of values for mean computation
            numberOfValues += 1.f;
            
            // Update maximum opacity
            if(currentMaxOpacity < currentData.w)
                currentMaxOpacity = currentData.w;

            // Accumulate for mean computation
            currentDataMean += glm::vec3(currentData);
        }

        // Mean value
        currentDataMean = currentDataMean/numberOfValues;

        // Compute variance
        for(float v = boxValuesMins_[currentAttribute][nextTimestep][b]; v < boxValuesMaxs_[currentAttribute][nextTimestep][b]; v += step){
            // Sample transfer function for current data
            currentData = transferFunctions[currentAttribute].getSample(v);

            // Accumulate for mean computation
            currentDataVariance += glm::vec3(
                (currentData.x - currentDataMean.x)*(currentData.x - currentDataMean.x),
                (currentData.y - currentDataMean.y)*(currentData.y - currentDataMean.y),
                (currentData.z - currentDataMean.z)*(currentData.z - currentDataMean.z));
        }

        // Variance of each of the RGB channels
        currentDataVariance = currentDataVariance/numberOfValues;

        // Maximum opacity for the box
        maxOpacities_[1][b] = currentMaxOpacity;

        // Set maximum RGB variance as maximum variance for the box
        dataVariances_[1][b] = glm::max(glm::max(currentDataVariance.x, currentDataVariance.y), currentDataVariance.z);
    }

   
    // Normalize the variance
    normMin = dataVariances_[0][0];
    normMax = dataVariances_[0][0];
    
    for(uint32_t b = 1; b < numberOfBoxes_; b++){
        normMin = std::min(normMin, dataVariances_[0][b]);
        normMax = std::max(normMax, dataVariances_[0][b]);        
        normMin = std::min(normMin, dataVariances_[1][b]);
        normMax = std::max(normMax, dataVariances_[1][b]);
    }

    float diff;
    diff = normMax - normMin;

    // Normalize the variance
    for(uint32_t b = 0; b < numberOfBoxes_; b++)
        dataVariances_[0][b] = (dataVariances_[0][b] - normMin)/diff;
}

void Boxes::updateDataProgressive(
    const std::vector<TransferFunction>& transferFunctions, 
    uint32_t currentAttribute, 
    uint32_t currentTimestep)
{
    // Box attributes
    float numberOfValues;
    float currentMaxOpacity;

    glm::vec4 currentData;
    glm::vec3 currentDataVariance;
    glm::vec3 currentDataMean;

    // Currrent and next timestep stored
    maxOpacities_.resize(2);
    dataVariances_.resize(2);

    // One value per box
    maxOpacities_[0].resize(numberOfBoxes_);
    dataVariances_[0].resize(numberOfBoxes_);

    // For each box
    for(uint32_t b = 0; b < numberOfBoxes_; b++){
        // Initialize values
        currentMaxOpacity = 0.f;
        currentDataVariance = glm::vec3(0.f);
        currentDataMean = glm::vec3(0.f);
        numberOfValues = 0.f;

        // Get transfer function samples between minimum and maximum values of the box
        float step = 1.f/128.f;

        for(float v = boxValuesMins_[currentAttribute][currentTimestep][b]; v < boxValuesMaxs_[currentAttribute][currentTimestep][b]; v += step){
            // Sample transfer function for current data
            currentData = transferFunctions[currentAttribute].getSample(v);

            // Increases number of values for mean computation
            numberOfValues += 1.f;
            
            // Update maximum opacity
            if(currentMaxOpacity < currentData.w)
                currentMaxOpacity = currentData.w;

            // Accumulate for mean computation
            currentDataMean += glm::vec3(currentData);
        }

        // Mean value
        currentDataMean = currentDataMean/numberOfValues;

        // Compute variance
        for(float v = boxValuesMins_[currentAttribute][currentTimestep][b]; v < boxValuesMaxs_[currentAttribute][currentTimestep][b]; v += step){
            // Sample transfer function for current data
            currentData = transferFunctions[currentAttribute].getSample(v);

            // Accumulate for mean computation
            currentDataVariance += glm::vec3(
                (currentData.x - currentDataMean.x)*(currentData.x - currentDataMean.x),
                (currentData.y - currentDataMean.y)*(currentData.y - currentDataMean.y),
                (currentData.z - currentDataMean.z)*(currentData.z - currentDataMean.z));
        }

        // Variance of each of the RGB channels
        currentDataVariance = currentDataVariance/numberOfValues;

        // Maximum opacity for the box
        maxOpacities_[0][b] = currentMaxOpacity;

        // Set maximum RGB variance as maximum variance for the box
        dataVariances_[0][b] = glm::max(glm::max(currentDataVariance.x, currentDataVariance.y), currentDataVariance.z);
    }

    // Next timestep
    uint32_t nextTimestep = (currentTimestep + 1)%mesh_.physical_data_n_steps_[currentAttribute];
    
    // One value per box
    maxOpacities_[1].resize(numberOfBoxes_);
    dataVariances_[1].resize(numberOfBoxes_);

    // For each box
    for(uint32_t b = 0; b < numberOfBoxes_; b++){
        // Initialize values
        currentMaxOpacity = 0.f;
        currentDataVariance = glm::vec3(0.f);
        currentDataMean = glm::vec3(0.f);
        numberOfValues = 0.f;

        // Get transfer function samples between minimum and maximum values of the box
        float step = 1.f/128.f;

        for(float v = boxValuesMins_[currentAttribute][nextTimestep][b]; v < boxValuesMaxs_[currentAttribute][nextTimestep][b]; v += step){
            // Sample transfer function for current data
            currentData = transferFunctions[currentAttribute].getSample(v);

            // Increases number of values for mean computation
            numberOfValues += 1.f;
            
            // Update maximum opacity
            if(currentMaxOpacity < currentData.w)
                currentMaxOpacity = currentData.w;

            // Accumulate for mean computation
            currentDataMean += glm::vec3(currentData);
        }

        // Mean value
        currentDataMean = currentDataMean/numberOfValues;

        // Compute variance
        for(float v = boxValuesMins_[currentAttribute][nextTimestep][b]; v < boxValuesMaxs_[currentAttribute][nextTimestep][b]; v += step){
            // Sample transfer function for current data
            currentData = transferFunctions[currentAttribute].getSample(v);

            // Accumulate for mean computation
            currentDataVariance += glm::vec3(
                (currentData.x - currentDataMean.x)*(currentData.x - currentDataMean.x),
                (currentData.y - currentDataMean.y)*(currentData.y - currentDataMean.y),
                (currentData.z - currentDataMean.z)*(currentData.z - currentDataMean.z));
        }

        // Variance of each of the RGB channels
        currentDataVariance = currentDataVariance/numberOfValues;

        // Maximum opacity for the box
        maxOpacities_[1][b] = currentMaxOpacity;

        // Set maximum RGB variance as maximum variance for the box
        dataVariances_[1][b] = glm::max(glm::max(currentDataVariance.x, currentDataVariance.y), currentDataVariance.z);
    }

    // Progressive min/max
    float oldMin = normMin;
    float oldMax = normMax;
    float oldDiff = oldMax - oldMin;

    for(uint32_t b = 1; b < numberOfBoxes_; b++){
        normMin = std::min(normMin, dataVariances_[0][b]);
        normMax = std::max(normMax, dataVariances_[0][b]);        
        normMin = std::min(normMin, dataVariances_[1][b]);
        normMax = std::max(normMax, dataVariances_[1][b]);
    }

    float diff;
    diff = normMax - normMin;

    // Denormalize and renormalize the variance
    for(uint32_t b = 0; b < numberOfBoxes_; b++)
        dataVariances_[0][b] = (dataVariances_[0][b] - normMin)/diff;
}

void Boxes::split(Box root, Box& left, Box& right)
{
    // Sort the vertices on the splitting axis
    uint32_t splittingAxis = (root.currentSplitAxis_ + 1)%3;
    uint32_t splitIndex;
    float splitPosition;

    std::sort(
        root.vertices_.begin(),
        root.vertices_.end(),
        [=](uint32_t a, uint32_t b)
        {
            return mesh_.vertices_[a][splittingAxis] < mesh_.vertices_[b][splittingAxis];
        });
    
    // Create left and right boxes
    left.currentSplitAxis_ = splittingAxis;
    right.currentSplitAxis_ = splittingAxis;

    // Handle odd number of vertices
    if(root.vertices_.size()%2){
        splitIndex = (root.vertices_.size() - 1)/2;
        splitPosition = mesh_.vertices_[root.vertices_[splitIndex]][splittingAxis];
    }else{
        splitIndex = root.vertices_.size()/2;
        splitPosition = .5f*(
            mesh_.vertices_[root.vertices_[splitIndex - 1]][splittingAxis] + 
            mesh_.vertices_[root.vertices_[splitIndex]][splittingAxis]);
    }

    // Set vertices
    left.vertices_ = std::vector<uint32_t>{root.vertices_.begin(), root.vertices_.begin() + splitIndex};
    right.vertices_ = std::vector<uint32_t>{root.vertices_.begin() + splitIndex, root.vertices_.end()};

    // Set AABBs
    left.AABB_[0] = root.AABB_[0];
    left.AABB_[1] = root.AABB_[1];
    right.AABB_[0] = root.AABB_[0];
    right.AABB_[1] = root.AABB_[1];

    left.AABB_[1][splittingAxis] = splitPosition;
    right.AABB_[0][splittingAxis] = splitPosition;
}

void Boxes::computeBoxesParameters(std::vector<Box>& boxes)
{
    float minVal, maxVal, currentVal;

    boxAABBMins_.resize(numberOfBoxes_);
    boxAABBMaxs_.resize(numberOfBoxes_);
    boxValuesMins_.resize(mesh_.number_of_attributes_);
    boxValuesMaxs_.resize(mesh_.number_of_attributes_);

    for(uint32_t attribute = 0; attribute < mesh_.number_of_attributes_; attribute++){
        boxValuesMins_[attribute].resize(mesh_.physical_data_n_steps_[attribute]);
        boxValuesMaxs_[attribute].resize(mesh_.physical_data_n_steps_[attribute]);

        for(uint32_t timestep = 0; timestep < mesh_.physical_data_n_steps_[attribute]; timestep++){
            boxValuesMins_[attribute][timestep].resize(numberOfBoxes_);
            boxValuesMaxs_[attribute][timestep].resize(numberOfBoxes_);
        }
    }

    // For each box
    for(uint32_t b = 0; b < numberOfBoxes_; b++){
        // Set AABBs
        boxAABBMins_[b] = boxes[b].AABB_[0];
        boxAABBMaxs_[b] = boxes[b].AABB_[1];

        // For each physical attribute of the mesh
        for(uint32_t attribute = 0; attribute < mesh_.number_of_attributes_; attribute++){
            // For each timestep
            for(uint32_t timestep = 0; timestep < mesh_.physical_data_n_steps_[attribute]; timestep++){
                // Set min and max default
                minVal = mesh_.physical_datas_[attribute][timestep][0];
                maxVal = mesh_.physical_datas_[attribute][timestep][0];

                // For each vertex of the box
                for(uint32_t v = 0; v < boxes[b].vertices_.size(); v++){
                    currentVal = mesh_.physical_datas_[attribute][timestep][boxes[b].vertices_[v]];

                    if(currentVal < minVal) 
                        minVal = currentVal;

                    if(currentVal > maxVal)
                        maxVal = currentVal;
                }

                // Set min and max value of the box
                boxValuesMins_[attribute][timestep][b] = minVal;
                boxValuesMaxs_[attribute][timestep][b] = maxVal;
            }
        }
    }
}