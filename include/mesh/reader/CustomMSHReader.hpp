///
/// \file CustomMSHReader.hpp
/// \brief Header file of CustomMSHReader
///



#ifndef CUSTOMMSHREADER_HPP
#define CUSTOMMSHREADER_HPP

#include <fstream>
#include <sstream>

#include "mesh/reader/Reader.hpp"

#include <unordered_map>



///
/// \class CustomMSHReader CustomMSHReader.hpp "CustomMSHReader.hpp"
/// \brief Reads .msh files
///
class CustomMSHReader : public Reader
{
private:
    struct ElementLocation
    {
        uint32_t cell_type;
        uint32_t local_cell_id;
    };

    std::ifstream filestream_;
    std::stringstream bufferstream_;
    std::string line_buffer_;

    uint32_t number_of_vertices_;
    std::vector<uint32_t> vertex_indices_;

    // Number of 3D cells stored for each internal LVRC cell type.
    std::vector<uint32_t> number_of_cells_per_type_;

    std::unordered_map<uint64_t, ElementLocation>
        element_locations_;

public:
    ///
    /// \fn CustomMSHReader CustomMSHReader::CustomMSHReader(std::string file_path)
    /// \brief Generate a Reader object for .msh files
    ///
    /// \param file_path Path to mesh file
    ///
    CustomMSHReader(std::string file_path);

    ///
    /// \fn void CustomMSHReader::~CustomMSHReader()
    /// \brief Finalize reader
    ///
    ~CustomMSHReader();

    ///
    /// \fn void CustomMSHReader::read_vertices(std::vector<glm::vec3> &vertices)
    /// \brief Get the vertices coordinates of the .msh mesh
    ///
    /// \param vertices vector of 3D coordinates (vertex indices are implicit)
    ///
    void read_vertices(std::vector<glm::vec3> &vertices) override;

    ///
    /// \fn void CustomMSHReader::read_physical_data(std::vector<std::vector<float>> &physical_datas, std::vector<std::string> &physical_data_names)
    /// \brief Read the physical data
    ///
    /// \param physical_datas vector of physical data vectors
    /// \param physical_data_names vector of physical data names
    /// \param physical_data_n_steps vector of number of timesteps per physical data
    ///
    void read_physical_data(std::vector<std::vector<std::vector<float>>> &physical_datas, 
                            std::vector<std::string> &physical_data_names,
                            std::vector<uint> &physical_data_n_steps) override;

    void read_element_physical_data(std::vector<ElementScalarField>& element_fields) override;

    ///
    /// \fn void CustomMSHReader::read_cells(std::vector<std::vector<uint32_t>> &cells)
    /// \brief Get the cells of the .msh mesh
    ///
    /// \param cells vector for storing vertex indices
    ///
    void read_cells(std::vector<std::vector<uint32_t>>& cells,
                    std::vector<uint32_t>& number_of_cells_per_type) override;

private:
    ///
    /// \fn void CustomMSHReader::convert_cell_type(uint32_t msh_cell_type)
    /// \brief Convert a .msh cell type to the corresponding internal cell type
    ///
    /// \param msh_cell_type .msh cell type (see gmsh documentation)
    ///
    /// \return cell type
    ///
    uint32_t convert_cell_type(uint32_t msh_cell_type);

    static uint32_t get_tetrahedral_field_order(uint32_t dofs_per_cell);
    static uint32_t get_hexahedral_field_order(uint32_t dofs_per_cell);
};

#endif