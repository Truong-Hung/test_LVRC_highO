///
/// \file CustomMSHReader.cpp
/// \brief Source file of CustomMSHReader
///



#include "mesh/reader/CustomMSHReader.hpp"

CustomMSHReader::CustomMSHReader(std::string file_path)
{
    // Open file stream
    filestream_ = std::ifstream(file_path);
    if (!filestream_.is_open())
    {
        std::cerr << "[ERROR] Failed to load mesh " << file_path << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Confirm .msh file version
    std::string version_number;
    uint32_t file_type;

    // Search for the version informations
    while(std::getline(filestream_, line_buffer_)){
        if(line_buffer_ == "$MeshFormat"){
            std::getline(filestream_, line_buffer_);
            bufferstream_ = std::stringstream(line_buffer_);
            bufferstream_ >> version_number >> file_type;
            break;         
        }
    }

    // If version is not supported
    if(version_number != "4.1"){
        std::cerr << "[ERROR] .msh file version " << version_number << " is not supported" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // If file is in binary format
    if(file_type){
        std::cerr << "[ERROR] Binary .msh files are not supported" << std::endl;
        std::exit(EXIT_FAILURE);       
    }
}

CustomMSHReader::~CustomMSHReader()
{   
    filestream_.close();
}

void CustomMSHReader::read_vertices(std::vector<glm::vec3> &vertices)
{
    uint32_t number_of_entities, number_of_vertices, start_vertex, end_vertex;
    uint32_t dimension, tag, parametric, vertices_in_block;
    uint32_t vertex_count;
    uint32_t current_index;
    glm::vec3 current_vertex;

    // Return to the beginning of the file
    filestream_.clear();
    filestream_.seekg(0, std::ios::beg);

    // Search for the vertices
    while(std::getline(filestream_, line_buffer_)){
        if(line_buffer_ == "$Nodes"){
            // Get informations about the vertices
            std::getline(filestream_, line_buffer_);
            bufferstream_ = std::stringstream(line_buffer_);
            bufferstream_ >> number_of_entities >> number_of_vertices >> start_vertex >> end_vertex;

            vertices.resize(number_of_vertices);
            vertex_indices_.resize(end_vertex + 1);
            vertex_count = 0;

            // For each entity
            for(uint32_t entity = 0; entity < number_of_entities; entity++){
                // Get informations about the entity
                std::getline(filestream_, line_buffer_);
                bufferstream_ = std::stringstream(line_buffer_);
                bufferstream_ >> dimension >> tag >> parametric >> vertices_in_block;

                // For each vertex, read the vertex indices
                for(uint32_t vertex = 0; vertex < vertices_in_block; vertex++){
                    std::getline(filestream_, line_buffer_);
                    bufferstream_ = std::stringstream(line_buffer_);
                    bufferstream_ >> current_index;

                    vertex_indices_[current_index] = vertex + vertex_count;
                }

                // For each vertex, read the vertex coordinates
                for(uint32_t vertex = 0; vertex < vertices_in_block; vertex++){
                    std::getline(filestream_, line_buffer_);
                    bufferstream_ = std::stringstream(line_buffer_);
                    bufferstream_ >> current_vertex.x >> current_vertex.y >> current_vertex.z;

                    vertices[vertex + vertex_count] = current_vertex;
                }

                vertex_count += vertices_in_block;
            }

            break;
        }
    }

    // Save the number of used vertices
    number_of_vertices_ = static_cast<uint32_t>(vertices.size());
}

void CustomMSHReader::read_physical_data(
    std::vector<std::vector<std::vector<float>>> &physical_datas, 
    std::vector<std::string> &physical_data_names,
    std::vector<uint> &physical_data_n_steps,
    std::vector<PhysicalDataAssociation> &physical_data_associations)
{
    uint32_t number_of_attributes;
    std::vector<uint32_t> integer_attributes;
    uint32_t current_index;
    std::string current_name;
    uint32_t current_data_number = 0;
    float current_data;
    std::vector<std::vector<float>> current_datas;
    std::vector<float> current_timestep;

    // Return to the beginning of the file
    filestream_.clear( );
    filestream_.seekg(0, std::ios::beg);

    // Search for the physical data
    while(std::getline(filestream_, line_buffer_)){
        if(line_buffer_ == "$NodeData"){
            current_datas.clear();

            // Read the string attributes
            std::getline(filestream_, line_buffer_);
            bufferstream_ = std::stringstream(line_buffer_);
            bufferstream_ >> number_of_attributes;

            // First attribute is the name of the data
            if(number_of_attributes >= 1){
                // Retrieve the name
                std::getline(filestream_, line_buffer_);
                bufferstream_ = std::stringstream(line_buffer_);
                bufferstream_ >> current_name;
            }else{
                // Default name
                current_name = "Data " + std::to_string(current_data_number);
            }
        
            // Skip the rest of the string attributes
            for(uint32_t l = 1; l < number_of_attributes; l++)
                std::getline(filestream_, line_buffer_);

            // Skip the non-used real attributes
            std::getline(filestream_, line_buffer_);
            bufferstream_ = std::stringstream(line_buffer_);
            bufferstream_ >> number_of_attributes;

            for(uint32_t l = 0; l < number_of_attributes; l++)
                std::getline(filestream_, line_buffer_);

            // Read the integer attributes
            std::getline(filestream_, line_buffer_);
            bufferstream_ = std::stringstream(line_buffer_);
            bufferstream_ >> number_of_attributes;

            // Not enough informations on physical data
            if(number_of_attributes < 3)
                break;

            integer_attributes.resize(number_of_attributes);
        
            for(uint32_t l = 0; l < number_of_attributes; l++){
                std::getline(filestream_, line_buffer_);
                bufferstream_ = std::stringstream(line_buffer_);
                bufferstream_ >> integer_attributes[l];
            }
         
            // Only scalar data for now
            if(integer_attributes[1] != 1)
                break;

            // Vector for storing data
            current_timestep.reserve(integer_attributes[2]);
            current_timestep.resize(integer_attributes[2]);

            // Read the data
            for(uint32_t data = 0; data < integer_attributes[2]; data++){
                std::getline(filestream_, line_buffer_);
                bufferstream_ = std::stringstream(line_buffer_);
                bufferstream_ >> current_index >> current_data;

                current_timestep[vertex_indices_[current_index]] = current_data;
            }

            current_datas.push_back(current_timestep);
            physical_datas.push_back(current_datas);
            physical_data_names.push_back(current_name);
            physical_data_n_steps.push_back(1);
            physical_data_associations.push_back(PhysicalDataAssociation::Vertex);
            current_data_number++;
        }
    }

    current_datas.resize(number_of_vertices_);

    // Dummy data with 256 timesteps
    current_datas.resize(256);

    for(uint32_t t = 0; t < 256; t++)
        current_datas[t].resize(number_of_vertices_);
    
    for(uint32_t t = 0; t < 256; t++){
        for(uint32_t d = 0; d < number_of_vertices_; d++){
            current_datas[t][d] = static_cast<float>((d + t*(number_of_vertices_/256))%number_of_vertices_)/static_cast<float>(number_of_vertices_);
        }
    }
    
    physical_datas.push_back(current_datas);
    physical_data_names.push_back("Vertex index");
    physical_data_n_steps.push_back(256);
    physical_data_associations.push_back(PhysicalDataAssociation::Vertex);
}

void CustomMSHReader::read_cells(std::vector<std::vector<uint32_t>> &cells)
{
    uint32_t number_of_entities, number_of_cells, start_cell, end_cell;
    uint32_t dimension, tag, msh_cell_type, cells_in_block;
    uint32_t current_cells_type = 0;
    std::vector<uint32_t> current_cell;
    std::string current_read_buffer;
    uint32_t current_read_value;
    uint32_t current_read_offset;

    // Return to the beginning of the file
    filestream_.clear( );
    filestream_.seekg(0, std::ios::beg);

    // Search for the cells
    while(std::getline(filestream_, line_buffer_)){
        if(line_buffer_ == "$Elements"){
            // Get informations about the cells
            std::getline(filestream_, line_buffer_);
            bufferstream_ = std::stringstream(line_buffer_);
            bufferstream_ >> number_of_entities >> number_of_cells >> start_cell >> end_cell;

            // For each entity
            for(uint32_t entity = 0; entity < number_of_entities; entity++){
                // Get informations about the entity
                std::getline(filestream_, line_buffer_);
                bufferstream_ = std::stringstream(line_buffer_);
                bufferstream_ >> dimension >> tag >> msh_cell_type >> cells_in_block;

                // Get 3D cell type
                if(dimension == 3)
                    current_cells_type = convert_cell_type(msh_cell_type);

                // For each cell, read the cell's vertices
                for(uint32_t cell = 0; cell < cells_in_block; cell++){
                    std::getline(filestream_, line_buffer_);

                    // Read only 3D cells
                    if(dimension == 3){
                        bufferstream_ = std::stringstream(line_buffer_);

                        current_cell.clear();
                        current_read_offset = 0;

                        while(std::getline(bufferstream_, current_read_buffer, ' ')){
                            // Skip the first value representing the cell's index
                            if(current_read_offset != 0){
                                current_read_value = std::stoul(current_read_buffer);
                                current_cell.push_back(vertex_indices_[current_read_value]);
                            }

                            current_read_offset++;
                        }

                        cells[current_cells_type].insert(cells[current_cells_type].end(), current_cell.begin(), current_cell.end());
                    }
                }
            }

            break;
        }
    }
}

uint32_t CustomMSHReader::convert_cell_type(uint32_t msh_cell_type)
{
    switch(msh_cell_type){
        case 4:
            return TETRAHEDRON_1;
        case 5:
            return HEXAHEDRON_1;
        case 6:
            return PRISM3_1;
        case 7:
            return PYRAMID4_1;
        case 11:
            return TETRAHEDRON_2;
        case 12: 
            return HEXAHEDRON_2_FULL;  // Hex27 full
        case 17:
            return HEXAHEDRON_2;    // Hex20 serendipity
        case 18:
            return PRISM3_2;
        case 19:
            return PYRAMID4_2;
        case 29:
            return TETRAHEDRON_3;   // Third order tetrahedron
        default:
            std::cerr << "[ERROR] Non supported cell type encountered" << std::endl;
            std::exit(EXIT_FAILURE);
    }
}