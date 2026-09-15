
#ifndef CUSTOMUMESHREADER_HPP
#define CUSTOMUMESHREADER_HPP

#include <fstream>
#include <sstream>

#include "mesh/reader/Reader.hpp"



//see ReadFrom function from https://gitlab.com/ingowald/umesh/-/blob/master/umesh/UMesh.cpp
class CustomUMESHReader : public Reader
{

private:
    std::ifstream filestream_;
	std::string file_path;

    uint32_t number_of_vertices_;
    std::vector<uint32_t> vertex_indices_;

    bool supportMultipleAttributes = false;

public:
    ///
    /// \fn CustomUMESHReader CustomUMESHReader::CustomUMESHReader(std::string file_path)
    /// \brief Generate a Reader object for .umesh files
    ///
    /// \param file_path Path to mesh file
    ///
    CustomUMESHReader(std::string file_path);

    ///
    /// \fn void CustomUMESHReader::~CustomUMESHReader()
    /// \brief Finalize reader
    ///
    ~CustomUMESHReader();

    /// \fn void CustomUMESHReader::read_magic_number()
    /// \brief read the magic number of the .umesh file
    /// @return true if the magic number is valid
    bool read_magic_number();

    ///
    /// \fn void CustomUMESHReader::read_vertices(std::vector<glm::vec3> &vertices)
    /// \brief Get the vertices coordinates of the .umesh mesh
    ///
    /// \param vertices vector of 3D coordinates (vertex indices are implicit)
    ///
    void read_vertices(std::vector<glm::vec3> &vertices) override;

    ///
    /// \fn void CustomUMESHReader::read_physical_data(std::vector<std::vector<float>> &physical_datas, std::vector<std::string> &physical_data_names)
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
    /// \fn void CustomUMESHReader::read_cells(std::vector<std::vector<uint32_t>> &cells)
    /// \brief Get the cells of the .umesh mesh
    ///
    /// \param cells vector for storing vertex indices
    /// \param number_of_cells_per_type vector for storing the number of cells per type
    ///
    void read_cells(std::vector<std::vector<uint32_t>>& cells,
                    std::vector<uint32_t>& number_of_cells_per_type) override;
    ///
    /// \fn int CustomUMESHReader::nb_vertices_cell_type(uint32_t msh_cell_type);
    /// \brief return the number of vertices of the cell
    ///
    /// \param msh_cell_type uint32_t to store cell type
    /// \return how many vertices the cell contains
    ///
	int nb_vertices_cell_type(uint32_t msh_cell_type);

    ///
    /// \fn std::vector<int> CustomUMESHReader::replace_vertices_indices_prism3_1(std::vector<int> vector);
    /// \brief rearrange vertices for wedge 1 (prism3_1) element
    ///
    /// \param vector vector which contains vertices indices
    /// \return rearranged vector 
    ///
    std::vector<int> replace_vertices_indices_prism3_1(std::vector<int> vector);


private:
    
};

#endif