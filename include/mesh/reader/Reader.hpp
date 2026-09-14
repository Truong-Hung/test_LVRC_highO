///
/// \file Reader.hpp
/// \brief Header file of Reader
///



#ifndef READER_HPP
#define READER_HPP

#include <iostream>
#include <vector>
#include <glm/glm.hpp>

#include "mesh/Cell.hpp"

enum class PhysicalDataAssociation : uint32_t
{
    Vertex = 0,
    Element = 1
};



///
/// \class Reader Reader.hpp "Reader.hpp"
/// \brief Abstract class for implementing specific extension readers
///
class Reader
{
public:
    ///
    /// \fn Reader Reader::Reader()
    /// \brief Default constructor for all readers, initialize API here if needed
    ///
    Reader(void){};

    ///
    /// \fn Reader Reader::~Reader()
    /// \brief Default destructor for all readers, finalize API here if needed
    ///
    ~Reader(void){};

    ///
    /// \fn void Reader::read_vertices(std::vector<glm::vec3> &vertices)
    /// \brief Read the vertices, all readers should implement this method
    ///
    /// \param vertices vector of 3D coordinates (vertex indices are implicit)
    ///
    virtual void read_vertices(std::vector<glm::vec3> &vertices) = 0;

    ///
    /// \fn void read_physical_data(std::vector<std::vector<float>> &physical_datas, std::vector<std::string> &physical_data_names)
    /// \brief Read the physical data, all readers should implement this method
    ///
    /// \param physical_datas vector of physical data vectors
    /// \param physical_data_names vector of physical data names
    /// \param physical_data_n_steps vector of number of timesteps per physical data
    /// \param physical_data_associations vector of source associations for each field
    ///
    virtual void read_physical_data(std::vector<std::vector<std::vector<float>>> &physical_datas, 
                                    std::vector<std::string> &physical_data_names,
                                    std::vector<uint> &physical_data_n_steps,
                                    std::vector<PhysicalDataAssociation> &physical_data_associations) = 0;

    ///
    /// \fn void Reader::read_cells(std::vector<std::vector<uint32_t>> &cells)
    /// \brief Read the cells, all readers should implement this method
    ///
    /// \param cells vector for storing vertex indices
    ///
    virtual void read_cells(std::vector<std::vector<uint32_t>> &cells) = 0;
};

#endif
