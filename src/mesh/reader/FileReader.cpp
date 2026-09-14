///
/// \file FileReader.cpp
/// \brief Source file of FileReader
///



#include "mesh/reader/FileReader.hpp"

FileReader::FileReader(std::string file_path): path_{file_path}
{   
    // Initialize file extension
    extension_ = get_file_extension();

    // Load the right reader
    load_file_reader();

    std::cout << "[Reader]" << std::endl;
    std::cout << "-- path : " << path_ << std::endl;
    std::cout << "-- extension : " << extension_ << std::endl;
}

std::string FileReader::get_mesh_name()
{
    // Return mesh name without extension
    return path_.stem().string();
}

void FileReader::read_vertices(std::vector<glm::vec3> &vertices)
{
    std::cout << "-- reading vertices ..." << std::endl;

    // Calls the corresponding readers method
    reader_->read_vertices(vertices);
}

void FileReader::read_physical_data(
    std::vector<std::vector<std::vector<float>>> &physical_datas, 
    std::vector<std::string> &physical_data_names,
    std::vector<uint> &physical_data_n_steps,
    std::vector<PhysicalDataAssociation> &physical_data_associations)
{
    std::cout << "-- reading datapoints ..." << std::endl;

    // Calls the corresponding readers method
    reader_->read_physical_data(physical_datas, physical_data_names, physical_data_n_steps, physical_data_associations);  
}

void FileReader::read_cells(std::vector<std::vector<uint32_t>> &cells)
{
    std::cout << "-- reading cells ..." << std::endl;

    // Calls the corresponding readers method
    reader_->read_cells(cells);
}

std::string FileReader::get_file_extension()
{
    // Check if file has an extension
    if(!path_.has_extension()){
        std::cerr << "[ERROR] Invalid file path (no extension)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Return file extension
    return path_.extension().string();
}

void FileReader::load_file_reader()
{
    // Reader for .lvrc files
    if(extension_ == ".lvrc"){
        reader_ = std::make_unique<LVRCReader>(path_.string());
        return;
    }
    
    // Reader for .msh files
    if(extension_ == ".msh"){
        reader_ = std::make_unique<CustomMSHReader>(path_.string());
        return;
    }

    // Reader for .umesh files
    if(extension_ == ".umesh"){
        reader_ = std::make_unique<CustomUMESHReader>(path_.string());
        return;
    }

    // Non implemented file extension
    std::cerr << "[ERROR] No reader implemented for extension : " << extension_ << std::endl;
    std::exit(EXIT_FAILURE);
}