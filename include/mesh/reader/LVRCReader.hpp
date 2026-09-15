///
/// \file LVRCReader.hpp
/// \brief Header file of LVRCReader
///



#ifndef LVRCREADER_HPP
#define LVRCREADER_HPP

#include <fstream>
#include <sstream>

#include "mesh/reader/Reader.hpp"



///
/// \class LVRCReader LVRCReader.hpp "LVRCReader.hpp"
/// \brief Reads .lvrc files
///
class LVRCReader : public Reader
{
private:
    std::string path_root_;

    uint32_t number_of_vertices_;

    uint32_t number_of_cells_;
    std::vector<uint32_t> number_of_cells_per_type_;

    uint32_t number_of_physical_data_fields_;
    uint32_t number_of_vertex_centered_fields_;
    uint32_t number_of_cell_centered_fields_;
    std::vector<uint32_t> number_of_timesteps_per_field_;

public:
    ///
    /// \fn LVRCReader LVRCReader::LVRCReader(std::string file_path)
    /// \brief Generate a Reader object for .lvrc files
    ///
    /// \param file_path Path to mesh file
    ///
    LVRCReader(std::string file_path);

    ///
    /// \fn void LVRCReader::~LVRCReader()
    /// \brief Finalize reader
    ///
    ~LVRCReader();

    ///
    /// \fn void LVRCReader::read_vertices(std::vector<glm::vec3> &vertices)
    /// \brief Get the vertices coordinates of the .lvrc mesh
    ///
    /// \param vertices vector of 3D coordinates (vertex indices are implicit)
    ///
    void read_vertices(std::vector<glm::vec3> &vertices) override;

    ///
    /// \fn void LVRCReader::read_physical_data(std::vector<std::vector<float>> &physical_datas, std::vector<std::string> &physical_data_names)
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
    /// \fn void LVRCReader::read_cells(std::vector<std::vector<uint32_t>> &cells)
    /// \brief Get the cells of the .lvrc mesh
    ///
    /// \param cells vector for storing vertex indices
    /// \param number_of_cells_per_type vector for storing the number of cells per type
    ///
    void read_cells(std::vector<std::vector<uint32_t>>& cells,
                    std::vector<uint32_t>& number_of_cells_per_type) override;
};

#endif