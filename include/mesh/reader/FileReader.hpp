///
/// \file FileReader.hpp
/// \brief Header file of FileReader
///

#ifndef FILEREADER_HPP
#define FILEREADER_HPP

#include <iostream>
#include <memory>
#include <filesystem>

#include "mesh/reader/Reader.hpp"
#include "mesh/reader/LVRCReader.hpp"
#include "mesh/reader/CustomMSHReader.hpp"
#include "mesh/reader/CustomUMESHReader.hpp"



///
/// \class FileReader FileReader.hpp "FileReader.hpp"
/// \brief Reads a file representing a mesh
///
class FileReader
{
public:
    std::vector<std::string> fields_name;

private:
    std::filesystem::path path_;
    std::string extension_;
    std::unique_ptr<Reader> reader_;

public:
    ///
    /// \fn FileReader FileReader::FileReader(std::string file_path)
    /// \brief Generate a FileReader object for the given mesh file
    ///
    /// \param file_path Path to mesh file
    ///
    FileReader(std::string file_path);

    ///
    /// \fn std::string FileReader::get_mesh_name()
    /// \brief Read the name of the mesh
    ///
    /// \return name of the mesh (without file extension)
    ///
    std::string get_mesh_name();

    ///
    /// \fn std::string FileReader::get_file_extension()
    /// \brief Get the file extension
    ///
    /// \return file extension
    ///
    std::string get_file_extension();
    
    ///
    /// \fn void FileReader::read_vertices(std::vector<glm::vec3> &vertices)
    /// \brief Read the vertices coordinates of the mesh
    ///
    /// \param vertices vector for storing 3D coordinates (vertex indices are implicit)
    ///
    void read_vertices(std::vector<glm::vec3> &vertices);

    ///
    /// \fn void FileReader::read_physical_data(std::vector<std::vector<float>> &physical_datas, std::vector<std::string> &physical_data_names)
    /// \brief Read the physical data
    ///
    /// \param physical_datas vector of physical data vectors
    /// \param physical_data_names vector of physical data names
    /// \param physical_data_n_steps vector of number of timesteps per physical data
    /// \param physical_data_associations vector of source associations for each field
    ///
    void read_physical_data(std::vector<std::vector<std::vector<float>>> &physical_datas, 
                            std::vector<std::string> &physical_data_names,
                            std::vector<uint> &physical_data_n_steps,
                            std::vector<PhysicalDataAssociation> &physical_data_associations);

    ///
    /// \fn void FileReader::read_cells(std::vector<std::vector<uint32_t>> &cells)
    /// \brief Read the cells of the mesh
    ///
    /// \param cells vector for storing vertex indices
    ///
    void read_cells(std::vector<std::vector<uint32_t>> &cells);

private:
    ///
    /// \fn void FileReader::load_file_reader()
    /// \brief Load the right reader for the given file extension
    ///
    void load_file_reader();
};

#endif