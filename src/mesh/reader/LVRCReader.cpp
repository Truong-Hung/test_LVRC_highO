///
/// \file LVRCReader.cpp
/// \brief Source file of LVRCReader
///



#include "mesh/reader/LVRCReader.hpp"

LVRCReader::LVRCReader(std::string file_path)
{
    // Data
    std::vector<uint32_t> meta_data;

    meta_data.resize(4 + Cell::get_number_of_cell_types());

    // Read meta data file
    std::ifstream meta_data_stream{file_path};
    meta_data_stream.read((char*) meta_data.data(), sizeof(uint32_t)*(4 + Cell::get_number_of_cell_types()));

    // Retrieve meta data
    path_root_ = file_path.substr(0, file_path.size() - 5);
    number_of_vertices_ = meta_data[0];
    number_of_cells_ = meta_data[1];
    number_of_physical_data_fields_ = meta_data[10] + meta_data[11];
    number_of_vertex_centered_fields_ = meta_data[10];
    number_of_cell_centered_fields_ = meta_data[11];

    number_of_cells_per_type_.resize(Cell::get_number_of_cell_types());

    for(uint32_t c = 0; c < Cell::get_number_of_cell_types(); c++)
        number_of_cells_per_type_[c] = meta_data[c + 2];

    // Read number of timesteps
    number_of_timesteps_per_field_.resize(number_of_physical_data_fields_);

    meta_data_stream.read((char*) number_of_timesteps_per_field_.data(), sizeof(uint32_t)*number_of_physical_data_fields_);
    meta_data_stream.close(); 
}

LVRCReader::~LVRCReader()
{

}

void LVRCReader::read_vertices(std::vector<glm::vec3> &vertices)
{
    // Data
    vertices.resize(number_of_vertices_);

    // Read vertex coordinates file
    std::ifstream vertex_coords_stream{path_root_ + ".coords"};
    vertex_coords_stream.read((char*) vertices.data(), sizeof(glm::vec3)*number_of_vertices_);
    vertex_coords_stream.close();
    vertices.shrink_to_fit();
}

void LVRCReader::read_physical_data(
    std::vector<std::vector<std::vector<float>>> &physical_datas, 
    std::vector<std::string> &physical_data_names,
    std::vector<uint> &physical_data_n_steps,
    std::vector<PhysicalDataAssociation> &physical_data_associations)
{
    // Data
    physical_datas.resize(number_of_physical_data_fields_);
    physical_data_names.resize(number_of_physical_data_fields_);
    physical_data_n_steps.resize(number_of_physical_data_fields_);
    physical_data_associations.resize(number_of_physical_data_fields_, PhysicalDataAssociation::Element);

    // Read all physical data field files
    std::ifstream physical_data_stream;
    uint32_t current_file_index = 0;

    // Vertex centered
    for(uint32_t f = 0; f < number_of_vertex_centered_fields_; f++){
        physical_data_names[f] = std::to_string(f);
        physical_data_n_steps[f] = number_of_timesteps_per_field_[f];
        physical_datas[f].resize(number_of_timesteps_per_field_[f]);

        for(uint32_t t = 0; t < number_of_timesteps_per_field_[f]; t++){
            physical_data_stream = std::ifstream{path_root_ + "." + std::to_string(current_file_index) + ".data"};
            physical_datas[f][t].resize(number_of_vertices_);
            physical_data_stream.read((char*) physical_datas[f][t].data(), sizeof(float)*number_of_vertices_);
            physical_data_stream.close();
            physical_datas[f][t].shrink_to_fit();
            current_file_index++;
        }

        physical_data_associations[f] = PhysicalDataAssociation::Vertex;
    }

    // Cell centered
    for(uint32_t f = number_of_vertex_centered_fields_; f < number_of_physical_data_fields_; f++){
        physical_data_names[f] = std::to_string(f);
        physical_data_n_steps[f] = number_of_timesteps_per_field_[f];
        physical_datas[f].resize(number_of_timesteps_per_field_[f]);   

        for(uint32_t t = 0; t < number_of_timesteps_per_field_[f]; t++){
            physical_data_stream = std::ifstream{path_root_ + "." + std::to_string(current_file_index) + ".data"};
            physical_datas[f][t].resize(number_of_cells_);
            physical_data_stream.read((char*) physical_datas[f][t].data(), sizeof(float)*number_of_cells_);
            physical_data_stream.close();
            physical_datas[f][t].shrink_to_fit();
            current_file_index++;
        }

        physical_data_associations[f] = PhysicalDataAssociation::Element;
    }
}

void LVRCReader::read_cells(std::vector<std::vector<uint32_t>> &cells)
{
    // Data
    std::ifstream cells_stream;

    // Read all cells files
    for(uint32_t c = 0; c < Cell::get_number_of_cell_types(); c++){
        if(number_of_cells_per_type_[c] != 0){
            cells[c].resize(number_of_cells_per_type_[c]*Cell::get_number_of_vertices(c));      
            cells_stream = std::ifstream{path_root_ +  Cell::get_file_suffix(c)};
            cells_stream.read((char*) cells[c].data(), sizeof(uint32_t)*number_of_cells_per_type_[c]*Cell::get_number_of_vertices(c));  
            cells_stream.close();
            cells[c].shrink_to_fit();
        }
    }
}
