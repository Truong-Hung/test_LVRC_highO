///
/// \file Boxes.hpp
/// \brief Header file of Boxes
///

#ifndef LVRC_BOXES_HPP
#define LVRC_BOXES_HPP

#include <vector>

#include <glm/glm.hpp>
#include <mesh/Mesh.hpp>
#include <ui/TransferFunction.hpp>



///
/// \class Boxes Boxes.hpp "Boxes.hpp"
/// \brief Represents the leaves of a balanced kd-tree on the cells of a mesh
///
class Boxes
{
public:
    std::vector<glm::vec3> boxAABBMins_;
    std::vector<glm::vec3> boxAABBMaxs_;

    std::vector<std::vector<float>> maxOpacities_;
    std::vector<std::vector<float>> dataVariances_;

    uint32_t numberOfBoxes_;
    
private:   
    struct Box
    {
        std::vector<uint32_t> vertices_;
        glm::vec3 AABB_[2];
        uint32_t currentSplitAxis_;
    };

    Mesh& mesh_;
    Box firstBox_;

    std::vector<std::vector<std::vector<float>>> boxValuesMins_;
    std::vector<std::vector<std::vector<float>>> boxValuesMaxs_;

    float normMin;
    float normMax;

public:
    Boxes(Mesh& mesh);

    void launch(uint32_t maxVerticesPerBox);
    void generateData(
        const std::vector<TransferFunction>& transferFunctions, 
        uint32_t currentAttribute);
    void generateData(
        const std::vector<TransferFunction>& transferFunctions, 
        uint32_t currentAttribute, 
        uint32_t currentTimestep);
    void initDataProgressive(
        const std::vector<TransferFunction>& transferFunctions, 
        uint32_t currentAttribute, 
        uint32_t currentTimestep);
    void updateDataProgressive(
        const std::vector<TransferFunction>& transferFunctions, 
        uint32_t currentAttribute, 
        uint32_t currentTimestep);

private:
    void split(Box root, Box& left, Box& right);
    void computeBoxesParameters(std::vector<Box>& boxes);
};

#endif