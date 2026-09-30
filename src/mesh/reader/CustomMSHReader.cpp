///
/// \file CustomMSHReader.cpp
/// \brief Source file of CustomMSHReader
///


#include <unordered_map>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
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
    std::vector<uint> &physical_data_n_steps)
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
}

void CustomMSHReader::read_cells(std::vector<std::vector<uint32_t>>& cells,
                                 std::vector<uint32_t>& number_of_cells_per_type)
{
    uint32_t number_of_entities, number_of_cells, start_cell, end_cell;
    uint32_t dimension, tag, msh_cell_type, cells_in_block;
    uint32_t current_cells_type = 0;
    std::vector<uint32_t> current_cell;
    std::string current_read_buffer;
    uint32_t current_read_value;
    uint32_t current_read_offset;
    uint64_t current_cell_tag;
    uint32_t local_cell_id;

    // Return to the beginning of the file
    filestream_.clear( );
    filestream_.seekg(0, std::ios::beg);
    // Reset the cell counts and the Gmsh-to-LVRC mapping.
    // This reader stores cells separately for every LVRC cell type.
    number_of_cells_per_type_.assign(static_cast<uint32_t>(cells.size()), 0);
    number_of_cells_per_type.assign(static_cast<uint32_t>(cells.size()), 0);

    element_locations_.clear();

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
                    if(dimension == 3)
                    {
                        bufferstream_ = std::stringstream(line_buffer_);

                        current_cell.clear();
                        current_read_offset = 0;

                        while(std::getline(
                            bufferstream_,
                            current_read_buffer,
                            ' '))
                        {
                            // Ignore tokens created by repeated spaces.
                            if(current_read_buffer.empty())
                                continue;

                            // First value is the Gmsh element tag.
                            if(current_read_offset == 0)
                            {
                                current_cell_tag =
                                    static_cast<uint64_t>(
                                        std::stoull(current_read_buffer));
                            }
                            else
                            {
                                current_read_value =
                                    static_cast<uint32_t>(
                                        std::stoul(current_read_buffer));

                                current_cell.push_back(
                                    vertex_indices_[current_read_value]);
                            }

                            current_read_offset++;
                        }

                        // Index in the LVRC table for this cell type,
                        // before inserting its connectivity.
                        local_cell_id =
                            number_of_cells_per_type_[current_cells_type];

                        cells[current_cells_type].insert(
                            cells[current_cells_type].end(),
                            current_cell.begin(),
                            current_cell.end());

                        number_of_cells_per_type_[current_cells_type]++;

                        element_locations_[current_cell_tag] =
                        {
                            current_cells_type,
                            local_cell_id
                        };
                    }
                }
            }

            break;
        }
    }

    number_of_cells_per_type = number_of_cells_per_type_;
}

void CustomMSHReader::read_element_physical_data(
    std::vector<ElementScalarField>& element_fields)
{
    uint32_t number_of_attributes;
    std::vector<uint32_t> integer_attributes;

    std::string current_name;
    uint32_t current_data_number = 0;

    double current_time;
    uint64_t current_cell_tag;
    uint32_t current_number_of_values;
    float current_data;

    uint32_t current_cell_type;
    uint32_t current_cell_id;

    std::unordered_map<std::string, size_t>
        field_indices;


    // Return to the beginning of the file
    filestream_.clear();
    filestream_.seekg(0, std::ios::beg);


    // Search for all element-based physical data
    while(std::getline(filestream_, line_buffer_))
    {
        if(line_buffer_ == "$ElementNodeData")
        {
            // Read the string attributes
            std::getline(filestream_, line_buffer_);
            bufferstream_ = std::stringstream(line_buffer_);
            bufferstream_ >> number_of_attributes;


            // First string attribute is the field name
            if(number_of_attributes >= 1)
            {
                std::getline(filestream_, line_buffer_);
                current_name = line_buffer_;

                if(current_name.size() >= 2
                && current_name.front() == '"'
                && current_name.back() == '"')
                {
                    current_name = current_name.substr(
                        1,
                        current_name.size() - 2);
                }
            }
            else
            {
                current_name =
                    "Element data "
                    + std::to_string(current_data_number);
            }


            // Skip remaining string attributes
            for(uint32_t l = 1;
                l < number_of_attributes;
                l++)
            {
                std::getline(filestream_, line_buffer_);
            }


            // Read the real attributes
            std::getline(filestream_, line_buffer_);
            bufferstream_ = std::stringstream(line_buffer_);
            bufferstream_ >> number_of_attributes;

            current_time = 0.0;

            for(uint32_t l = 0;
                l < number_of_attributes;
                l++)
            {
                std::getline(filestream_, line_buffer_);
                bufferstream_ = std::stringstream(line_buffer_);

                if(l == 0)
                    bufferstream_ >> current_time;
            }


            // Read integer attributes
            std::getline(filestream_, line_buffer_);
            bufferstream_ = std::stringstream(line_buffer_);
            bufferstream_ >> number_of_attributes;

            if(number_of_attributes < 3)
            {
                std::cerr
                    << "[ERROR] Not enough integer attributes in "
                    << "$ElementNodeData" << std::endl;

                std::exit(EXIT_FAILURE);
            }

            integer_attributes.resize(number_of_attributes);

            for(uint32_t l = 0;
                l < number_of_attributes;
                l++)
            {
                std::getline(filestream_, line_buffer_);
                bufferstream_ = std::stringstream(line_buffer_);
                bufferstream_ >> integer_attributes[l];
            }


            const uint32_t timestep =
                integer_attributes[0];

            const uint32_t components =
                integer_attributes[1];

            const uint32_t number_of_element_entries =
                integer_attributes[2];


            // Only scalar data for now
            if(components != 1)
            {
                std::cerr
                    << "[ERROR] Only scalar $ElementNodeData is "
                    << "supported for now" << std::endl;

                std::exit(EXIT_FAILURE);
            }

            if(number_of_element_entries == 0)
            {
                std::cerr
                    << "[ERROR] Empty $ElementNodeData block"
                    << std::endl;

                std::exit(EXIT_FAILURE);
            }


            // -------------------------------------------------
            // Read the first element line separately.
            //
            // It lets us identify:
            // - the LVRC cell type,
            // - the field DOF count per cell.
            // -------------------------------------------------
            std::getline(filestream_, line_buffer_);
            bufferstream_ = std::stringstream(line_buffer_);

            bufferstream_ >> current_cell_tag
                          >> current_number_of_values;

            const auto first_location =
                element_locations_.find(current_cell_tag);

            if(first_location == element_locations_.end())
            {
                std::cerr
                    << "[ERROR] $ElementNodeData references "
                    << "an unknown 3D element tag "
                    << current_cell_tag
                    << std::endl;

                std::exit(EXIT_FAILURE);
            }

            current_cell_type =
                first_location->second.cell_type;

            current_cell_id =
                first_location->second.local_cell_id;


            // -------------------------------------------------
            // Find or create the ElementScalarField.
            // This is the actual implementation of step 9.
            // -------------------------------------------------
            size_t field_index;

            const auto field_location =
                field_indices.find(current_name);

            if(field_location == field_indices.end())
            {
                field_index = element_fields.size();

                field_indices[current_name] =
                    field_index;

                element_fields.push_back(
                    ElementScalarField());

                ElementScalarField& new_field =
                    element_fields[field_index];

                new_field.name = current_name;
                new_field.cell_type = current_cell_type;
                new_field.dofs_per_cell =
                    current_number_of_values;
                new_field.components = components;
                new_field.basis =
                    FieldBasis::GmshLagrange;


                // Support tetrahedral and hexahedral Gmsh Lagrange fields
                if(new_field.cell_type == TETRAHEDRON_1 ||
                   new_field.cell_type == TETRAHEDRON_2 ||
                   new_field.cell_type == TETRAHEDRON_3)
                {
                    new_field.field_order = get_tetrahedral_field_order(new_field.dofs_per_cell);
                }
                else if(new_field.cell_type == HEXAHEDRON_1 ||
                        new_field.cell_type == HEXAHEDRON_2 ||
                        new_field.cell_type == HEXAHEDRON_2_FULL ||
                        new_field.cell_type == HEXAHEDRON_3)
                {
                    new_field.field_order = get_hexahedral_field_order(new_field.dofs_per_cell);
                }
                else
                {
                    std::cerr
                        << "[ERROR] $ElementNodeData is not supported for cell type "
                        << new_field.cell_type << std::endl;

                    std::exit(EXIT_FAILURE);
                }
            }
            else
            {
                field_index =
                    field_location->second;
            }


            ElementScalarField& current_field =
                element_fields[field_index];


            // -------------------------------------------------
            // Check that this timestep/block matches the field
            // metadata created by its first block.
            // -------------------------------------------------
            if(current_field.cell_type != current_cell_type)
            {
                std::cerr
                    << "[ERROR] Field "
                    << current_field.name
                    << " has multiple cell types"
                    << std::endl;

                std::exit(EXIT_FAILURE);
            }

            if(current_field.dofs_per_cell
            != current_number_of_values)
            {
                std::cerr
                    << "[ERROR] Field "
                    << current_field.name
                    << " has inconsistent local DOF counts"
                    << std::endl;

                std::exit(EXIT_FAILURE);
            }

            if(current_field.components != components)
            {
                std::cerr
                    << "[ERROR] Field "
                    << current_field.name
                    << " has inconsistent component counts"
                    << std::endl;

                std::exit(EXIT_FAILURE);
            }


            const uint32_t number_of_cells =
                number_of_cells_per_type_[
                    current_field.cell_type];

            // Check if any cell is missing or counted twice
            if(number_of_element_entries != number_of_cells)
            {
                std::cerr
                    << "[ERROR] Field "
                    << current_field.name
                    << " contains "
                    << number_of_element_entries
                    << " element entries, but the mesh contains "
                    << number_of_cells
                    << " cells of type "
                    << Cell::get_name(current_field.cell_type)
                    << std::endl;

                std::exit(EXIT_FAILURE);
            }

            // Make room for this timestep.
            if(current_field.values.size() <= timestep)
            {
                current_field.values.resize(
                    timestep + 1);
            }

            if(!current_field.values[timestep].empty())
            {
                std::cerr
                    << "[ERROR] Field "
                    << current_field.name
                    << " already contains timestep "
                    << timestep
                    << std::endl;

                std::exit(EXIT_FAILURE);
            }

            current_field.values[timestep].resize(
                current_field.expected_value_count(
                    number_of_cells));

            std::vector<bool> cell_was_read(number_of_cells, false);

            if(current_cell_id >= number_of_cells)
            {
                std::cerr
                    << "[ERROR] Invalid local cell ID "
                    << current_cell_id
                    << " for Gmsh element "
                    << current_cell_tag
                    << std::endl;

                std::exit(EXIT_FAILURE);
            }

            if(cell_was_read[current_cell_id])
            {
                std::cerr
                    << "[ERROR] Duplicate $ElementNodeData entry "
                    << "for Gmsh element "
                    << current_cell_tag
                    << std::endl;

                std::exit(EXIT_FAILURE);
            }

            cell_was_read[current_cell_id] = true;

            // -------------------------------------------------
            // Store the first line already read.
            // -------------------------------------------------
            for(uint32_t local_dof = 0;
                local_dof < current_field.dofs_per_cell;
                local_dof++)
            {
                bufferstream_ >> current_data;

                if(bufferstream_.fail())
                {
                    std::cerr
                        << "[ERROR] Not enough coefficients for "
                        << "element "
                        << current_cell_tag
                        << std::endl;

                    std::exit(EXIT_FAILURE);
                }

                current_field.values[timestep][
                    current_field.value_index(
                        current_cell_id,
                        local_dof)
                ] = current_data;
            }


            // -------------------------------------------------
            // Read remaining element lines in this block.
            // -------------------------------------------------
            for(uint32_t data = 1;
                data < number_of_element_entries;
                data++)
            {
                std::getline(filestream_, line_buffer_);
                bufferstream_ = std::stringstream(line_buffer_);

                bufferstream_ >> current_cell_tag
                              >> current_number_of_values;

                const auto location =
                    element_locations_.find(
                        current_cell_tag);

                if(location == element_locations_.end())
                {
                    std::cerr
                        << "[ERROR] $ElementNodeData references "
                        << "an unknown 3D element tag "
                        << current_cell_tag
                        << std::endl;

                    std::exit(EXIT_FAILURE);
                }

                current_cell_type =
                    location->second.cell_type;

                current_cell_id =
                    location->second.local_cell_id;

                if(current_cell_type
                != current_field.cell_type)
                {
                    std::cerr
                        << "[ERROR] Mixed cell types in one "
                        << "$ElementNodeData block are not "
                        << "supported"
                        << std::endl;

                    std::exit(EXIT_FAILURE);
                }

                if(current_number_of_values
                != current_field.dofs_per_cell)
                {
                    std::cerr
                        << "[ERROR] Inconsistent number of "
                        << "values in $ElementNodeData"
                        << std::endl;

                    std::exit(EXIT_FAILURE);
                }

                if(current_cell_id >= number_of_cells)
                {
                    std::cerr
                        << "[ERROR] Invalid local cell ID "
                        << current_cell_id
                        << " for Gmsh element "
                        << current_cell_tag
                        << std::endl;

                    std::exit(EXIT_FAILURE);
                }

                if(cell_was_read[current_cell_id])
                {
                    std::cerr
                        << "[ERROR] Duplicate $ElementNodeData entry "
                        << "for Gmsh element "
                        << current_cell_tag
                        << std::endl;

                    std::exit(EXIT_FAILURE);
                }

                cell_was_read[current_cell_id] = true;

                for(uint32_t local_dof = 0;
                    local_dof < current_field.dofs_per_cell;
                    local_dof++)
                {
                    bufferstream_ >> current_data;

                    if(bufferstream_.fail())
                    {
                        std::cerr
                            << "[ERROR] Not enough coefficients for "
                            << "element "
                            << current_cell_tag
                            << std::endl;

                        std::exit(EXIT_FAILURE);
                    }

                    current_field.values[timestep][
                        current_field.value_index(
                            current_cell_id,
                            local_dof)
                    ] = current_data;
                }
            }

            for(uint32_t local_cell_id = 0;
                local_cell_id < number_of_cells;
                ++local_cell_id)
            {
                if(!cell_was_read[local_cell_id])
                {
                    std::cerr
                        << "[ERROR] Missing $ElementNodeData entry "
                        << "for field "
                        << current_field.name
                        << ", local cell "
                        << local_cell_id
                        << std::endl;

                    std::exit(EXIT_FAILURE);
                }
            }

            // Verify final size of this timestep buffer.
            current_field.validate(number_of_cells);

            current_data_number++;
        }
    }
}

uint32_t CustomMSHReader::get_tetrahedral_field_order(uint32_t dofs_per_cell)
{
    for(uint32_t order = 1; order <= 10; ++order)
    {
        const uint32_t expected_dofs =
            (order + 1)
            * (order + 2)
            * (order + 3)
            / 6;

        if(expected_dofs == dofs_per_cell)
            return order;
    }

    throw std::runtime_error(
        "Unsupported tetrahedral ElementNodeData size: "
        + std::to_string(dofs_per_cell));
}

uint32_t CustomMSHReader::get_hexahedral_field_order(uint32_t dofs_per_cell)
{
    // Hex20 is the incomplete/serendipity
    // second-order hexahedron.
    if(dofs_per_cell == 20)
    {
        return 2;
    }

    // A complete tensor-product hexahedral field
    // of order p contains:
    //
    //     (p + 1)^3
    //
    // local DOFs.
    const uint32_t nodes_per_direction =
        static_cast<uint32_t>(
            std::llround(
                std::cbrt(
                    static_cast<double>(
                        dofs_per_cell))));

    if(nodes_per_direction < 2)
    {
        throw std::runtime_error(
            "Invalid hexahedral ElementNodeData size: "
            + std::to_string(dofs_per_cell));
    }

    const uint64_t reconstructed_dof_count =
        static_cast<uint64_t>(
            nodes_per_direction)
        * static_cast<uint64_t>(
            nodes_per_direction)
        * static_cast<uint64_t>(
            nodes_per_direction);

    if(reconstructed_dof_count
    != static_cast<uint64_t>(
        dofs_per_cell))
    {
        throw std::runtime_error(
            "Unsupported hexahedral ElementNodeData "
            "size: "
            + std::to_string(dofs_per_cell)
            + ". Expected 20 DOFs for Hex20 or "
              "(p + 1)^3 DOFs for a complete "
              "tensor-product hexahedral field.");
    }

    return nodes_per_direction - 1;
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
        case 92:
            return HEXAHEDRON_3;    // Third order hexahedron (Hex64)
        default:
            std::cerr << "[ERROR] Non supported cell type encountered " << msh_cell_type << std::endl;
            std::exit(EXIT_FAILURE);
    }
}