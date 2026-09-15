#include "mesh/reader/CustomUMESHReader.hpp"
#include <memory>

CustomUMESHReader::CustomUMESHReader(std::string umesh_file_path)
{
    file_path = umesh_file_path;

    // open file stream
    filestream_.open(file_path);
    if(!read_magic_number())
    {
        std::cerr << "[ERROR] bad magic number" << std::endl;
        exit(EXIT_FAILURE);
    }
}

CustomUMESHReader::~CustomUMESHReader()
{
    filestream_.close();
}

bool CustomUMESHReader::read_magic_number()
{
    const size_t bum_magic = 0x234235567ULL;
    const size_t bum_magic_old = 0x234235566ULL;
    size_t magic;
    filestream_.read((char*)&magic, sizeof(magic));
    if(magic == bum_magic)
    {
        supportMultipleAttributes = true;
        return true;
    }
    return magic == bum_magic_old;
}


void CustomUMESHReader::read_vertices(std::vector<glm::vec3> &vertices)
{
    size_t size;
    filestream_.read((char*)&size, sizeof(size));
    number_of_vertices_ = static_cast<uint32_t>(size);
    vertices.resize(size);
    filestream_.read((char*)vertices.data(), size * sizeof(vertices[0]));
}


void CustomUMESHReader::read_physical_data(
    std::vector<std::vector<std::vector<float>>> &physical_datas, 
    std::vector<std::string> &physical_data_names,
    std::vector<uint> &physical_data_n_steps)
{
    std::vector<std::vector<float>> current_datas;
    std::vector<float> current_timestep;
    size_t num_per_vertex_attributes = 1;

    if(supportMultipleAttributes)
        filestream_.read((char*)&num_per_vertex_attributes, sizeof(num_per_vertex_attributes));

    if(num_per_vertex_attributes){
        if(supportMultipleAttributes){
            int size_string_argument;

            filestream_.read((char*)&size_string_argument, sizeof(size_string_argument));

            std::string s(size_string_argument,' ');
            size_t num_bytes = size_string_argument*sizeof(s.data());

            filestream_.read((char*)s.data(), num_bytes);

            if(s != "")
                physical_data_names.push_back(s);
        }

        size_t size;

        filestream_.read((char*)&size, sizeof(size));

        float current_physical_value;

        for(size_t i = 0; i < size; i++){
            filestream_.read((char*)&current_physical_value, sizeof(current_physical_value));
            current_timestep.push_back(current_physical_value);
        }

        current_datas.push_back(current_timestep);
        physical_datas.push_back(current_datas);
        physical_data_names.push_back("Unknown name");
        physical_data_n_steps.push_back(1);
    }

    // Dummy data
    current_datas.clear();
    current_timestep.resize(number_of_vertices_);
    
    for(uint32_t d = 0; d < number_of_vertices_; d++)
        current_timestep[d] = static_cast<float>(d)/static_cast<float>(number_of_vertices_);

    current_datas.push_back(current_timestep);
    physical_datas.push_back(current_datas);
    physical_data_names.push_back("Vertex index");
    physical_data_n_steps.push_back(1);
}

void CustomUMESHReader::read_cells(std::vector<std::vector<uint32_t>>& cells,
                                   std::vector<uint32_t>& number_of_cells_per_type)
{

    size_t numPerElementAttributes = 0;
    if (supportMultipleAttributes)
        filestream_.read((char*)&numPerElementAttributes, sizeof(numPerElementAttributes));

    //we must read triangle and quad but we don't use them
    std::vector<uint32_t> triangle;
    std::vector<uint32_t> quad;

    // We'll need to rearrange vertices ordering in each used element
    // No need for TETRAHEDRON_1, HEXAHEDRON_1, PYRAMID4_1

    // Indices are in order so there's no need of any modifications

    //triangle
    size_t size = 0;
    filestream_.read((char*)&size, sizeof(size));
    if(size)
    {
        int nb_vertices = 3;
        std::vector<int> buffer_value(nb_vertices);
        triangle.resize(size * nb_vertices);
        for(size_t i = 0; i < size; i++)
        { 
            for(int j = 0; j < nb_vertices; j++)
            {
                filestream_.read((char*)&buffer_value[j], sizeof(buffer_value[j]));
                triangle.push_back(buffer_value[j]);
            }

        }
    }

    //quad
    filestream_.read((char*)&size, sizeof(size));
    if(size)
    {
        int nb_vertices = 4;
        std::vector<int> buffer_value(nb_vertices);
        quad.resize(size * nb_vertices);
        for(size_t i = 0; i < size; i++)
        { 
            for(int j = 0; j < nb_vertices; j++)
            {
                filestream_.read((char*)&buffer_value[j], sizeof(buffer_value[j]));
                quad.push_back(buffer_value[j]);
            }

        }
    }

    //order is important 
    std::vector<int> vector_cell_type = {TETRAHEDRON_1, PYRAMID4_1, PRISM3_1, HEXAHEDRON_1};
    int nb_element_handled = static_cast<int>(vector_cell_type.size());

    for(int current_element = 0; current_element < nb_element_handled; current_element++)
    {
        filestream_.read((char*)&size, sizeof(size));
        if(size)
        {
            int nb_vertices = nb_vertices_cell_type(vector_cell_type[current_element]);     
            std::vector<int> current_cell;
            current_cell.resize(nb_vertices);
            for(size_t i = 0; i < size; i++)
            {
                for(int j = 0; j < nb_vertices; j++)
                    filestream_.read((char*)&current_cell[j], sizeof(current_cell[j]));

                if(vector_cell_type[current_element] == PRISM3_1)
                    current_cell = replace_vertices_indices_prism3_1(current_cell);
                cells[vector_cell_type[current_element]].insert(cells[vector_cell_type[current_element]].end(), current_cell.begin(), current_cell.end());
            }
        }
    }
}

void CustomUMESHReader::read_element_physical_data(
    std::vector<ElementScalarField>& element_fields)
{
    // The current UMESH format provides only global nodal fields.
    element_fields.clear();
}

std::vector<int> CustomUMESHReader::replace_vertices_indices_prism3_1(std::vector<int> vector)
{
    std::vector<int> vector_out;
    vector_out.push_back(vector[2]);
    vector_out.push_back(vector[1]);
    vector_out.push_back(vector[0]);
    vector_out.push_back(vector[5]);
    vector_out.push_back(vector[4]);
    vector_out.push_back(vector[3]);
    return vector_out;
}

int CustomUMESHReader::nb_vertices_cell_type(uint32_t msh_cell_type)
{
    switch (msh_cell_type)
    {
    case TETRAHEDRON_1:
        return 4;
    case HEXAHEDRON_1:
        return 8;
    case PRISM3_1:
        return 6;
    case PYRAMID4_1:
        return 5;
    case TETRAHEDRON_2:
        return 10;
    case TETRAHEDRON_3:
        return 20;
    case HEXAHEDRON_2:
        return 20;
    case HEXAHEDRON_2_FULL:
        return 27;
    case PRISM3_2:
        return 15;
    case PYRAMID4_2:
        return 13;
    default:
        std::cerr << "[ERROR] Non supported cell type encountered" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
