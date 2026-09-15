///
/// \file Mesh.cpp
/// \brief Source file of Mesh
///



#include "mesh/Mesh.hpp"

Mesh::Mesh(std::string file_path)
{
    // Initialize the file reader
    FileReader reader{ file_path };

    // Get the mesh name
    name_ = reader.get_mesh_name();

    load_default_mesh(reader);

    // Compute AABB
    compute_AABB();

    // Compute connectivity
    compute_connectivity();

    // Print mesh informations
    print_mesh();
}

void Mesh::load_default_mesh(FileReader& reader)
{
    // Read the vertices
    reader.read_vertices(vertices_);
    number_of_vertices_ = static_cast<uint32_t>(vertices_.size());

    // Read the physical data
    reader.read_physical_data(physical_datas_, physical_data_names_, physical_data_n_steps_);
    number_of_attributes_ = static_cast<uint32_t>(physical_datas_.size());

    // Read element-based physical scalar fields
    reader.read_element_physical_data(element_physical_datas_);

    // Normalize data
    normalize_physical_datas();
    normalize_element_physical_datas();

    // Read the cells
    uint32_t number_of_cell_types = Cell::get_number_of_cell_types();

    cells_.reserve(number_of_cell_types);
    cells_.resize(number_of_cell_types, {});
    number_of_cells_per_type_.reserve(number_of_cell_types);
    number_of_cells_per_type_.resize(number_of_cell_types, 0);
    reader.read_cells(cells_);

    for(uint32_t cell_type = 0; cell_type < number_of_cell_types; cell_type++){
        number_of_cells_per_type_[cell_type] = cells_[cell_type].size()/Cell::get_number_of_vertices(cell_type);
        number_of_cells_ += number_of_cells_per_type_[cell_type];
    }
}

uint32_t Mesh::get_cell_type(uint32_t cell_index) const
{
    if (cell_index >= number_of_cells_) {
        std::cerr << "[ERROR] Cell of index " << cell_index << " does not exist" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    uint32_t number_of_cell_types = Cell::get_number_of_cell_types();

    // For each cell type
    for (uint32_t cell_type = 0; cell_type < number_of_cell_types; cell_type++) {
        // If the index is in the corresponding interval
        if (cell_index < number_of_cells_per_type_[cell_type]) {
            return cell_type;
        }
        else {
            cell_index -= number_of_cells_per_type_[cell_type];
        }
    }

    return 0;
}

uint32_t Mesh::get_cell_offset(uint32_t cell_index) const
{
    if (cell_index >= number_of_cells_) {
        std::cerr << "[ERROR] Cell of index " << cell_index << " does not exist" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    uint32_t number_of_cell_types = Cell::get_number_of_cell_types();

    // For each cell type
    for (uint32_t cell_type = 0; cell_type < number_of_cell_types; cell_type++) {
        // If the index is in the corresponding interval
        if (cell_index < number_of_cells_per_type_[cell_type]) {
            return cell_index;
        }
        else {
            cell_index -= number_of_cells_per_type_[cell_type];
        }
    }

    return 0;
}

std::vector<uint32_t> Mesh::get_cell(uint32_t cell_index) const
{
    if (cell_index >= number_of_cells_) {
        std::cerr << "[ERROR] Cell of index " << cell_index << " does not exist" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    uint32_t number_of_cell_types = Cell::get_number_of_cell_types();

    // For each cell type
    for (uint32_t cell_type = 0; cell_type < number_of_cell_types; cell_type++) {
        // If the index is in the corresponding interval
        if (cell_index < number_of_cells_per_type_[cell_type]) {
            return std::vector<uint32_t>{cells_[cell_type].begin() + cell_index * Cell::get_number_of_vertices(cell_type),
                cells_[cell_type].begin() + (cell_index + 1) * Cell::get_number_of_vertices(cell_type)};
        }
        else {
            cell_index -= number_of_cells_per_type_[cell_type];
        }
    }

    return std::vector<uint32_t>{};
}

std::pair<std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator> Mesh::get_cell_iter(uint32_t cell_index) const
{
    using CellIter = std::vector<uint32_t>::const_iterator;
    if(cell_index >= number_of_cells_){
        std::cerr << "[ERROR] Cell of index " << cell_index << " does not exist" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    uint32_t number_of_cell_types = Cell::get_number_of_cell_types();

    // For each cell type
    for(uint32_t cell_type = 0; cell_type < number_of_cell_types; cell_type++){
        // If the index is in the corresponding interval
        if(cell_index < number_of_cells_per_type_[cell_type]){
            return std::make_pair<CellIter, CellIter>(cells_[cell_type].begin() + cell_index*Cell::get_number_of_vertices(cell_type), 
                                         cells_[cell_type].begin() + (cell_index + 1)*Cell::get_number_of_vertices(cell_type));
        }else{
            cell_index -= number_of_cells_per_type_[cell_type];
        }
    }
    return std::make_pair<CellIter, CellIter>(cells_[0].begin(), cells_[0].begin());
}

void Mesh::get_face_location(uint32_t face_index,
        uint32_t& cell_type, uint32_t& cell_offset, uint32_t& face_mask_index) const
{
    if (face_index >= number_of_faces_) {
        std::cerr << "[ERROR] Face of index " << face_index << " does not exist" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const uint32_t number_of_cell_types = Cell::get_number_of_cell_types();
    cell_offset = 0;
    face_mask_index = 0;

    // For each cell type
    for (cell_type = 0; cell_type < number_of_cell_types; cell_type++) {
	    const uint32_t number_of_faces_of_cell_type = Cell::get_number_of_faces(cell_type);
	    const uint32_t number_of_vertices_of_cell_type = Cell::get_number_of_vertices(cell_type);

        // If the index is in the corresponding interval
        if (face_index < number_of_cells_per_type_[cell_type] * number_of_faces_of_cell_type) {
            cell_offset = face_index / number_of_faces_of_cell_type;
            face_mask_index = face_index - cell_offset * number_of_faces_of_cell_type;
            cell_offset *= number_of_vertices_of_cell_type;
            break;
        }
	    face_index -= number_of_cells_per_type_[cell_type] * number_of_faces_of_cell_type;
    }
}

std::vector<uint32_t> Mesh::get_face(uint32_t face_index) const
{
    uint32_t cell_type, cell_offset, face_mask_index;

    get_face_location(face_index, cell_type, cell_offset, face_mask_index);

    // Retrieve face mask and apply on the cell
    std::vector<uint32_t> face = Cell::get_face_mask(cell_type, face_mask_index);

    for (uint32_t& vertex : face)
	    vertex = cells_[cell_type][cell_offset + vertex];

    return face;
}

std::vector<std::vector<uint32_t>> Mesh::get_faces(uint32_t cell_index) const
{
    if (cell_index >= number_of_cells_) {
        std::cerr << "[ERROR] Cell of index " << cell_index << " does not exist" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const uint32_t number_of_cell_types = Cell::get_number_of_cell_types();
    uint32_t cell_offset = 0;

    // For each cell type
    uint32_t cell_type;
    for (cell_type = 0; cell_type < number_of_cell_types; cell_type++) {
	    const uint32_t number_of_vertices_of_cell_type = Cell::get_number_of_vertices(cell_type);

        // If the index is in the corresponding interval
        if (cell_index < number_of_cells_per_type_[cell_type]) {
            cell_offset = cell_index * number_of_vertices_of_cell_type;
            break;
        }
        cell_index -= number_of_cells_per_type_[cell_type];
    }

    // Retrieve face masks and apply on the cell
    std::vector<std::vector<uint32_t>> faces = Cell::get_face_masks(cell_type);

    for (uint32_t face = 0; face < faces.size(); face++) {
        for (uint32_t vertex = 0; vertex < faces[face].size(); vertex++) {
            faces[face][vertex] = cells_[cell_type][cell_offset + faces[face][vertex]];
        }
    }

    return faces;
}

void Mesh::compute_AABB()
{
	AABB_[0] = vertices_[0];
    AABB_[1] = vertices_[0];

    for (uint32_t v = 1; v < number_of_vertices_; v++) {
        glm::vec3 current_vertex = vertices_[v];

        AABB_[0].x = std::min(AABB_[0].x, current_vertex.x);
        AABB_[0].y = std::min(AABB_[0].y, current_vertex.y);
        AABB_[0].z = std::min(AABB_[0].z, current_vertex.z);

        AABB_[1].x = std::max(AABB_[1].x, current_vertex.x);
        AABB_[1].y = std::max(AABB_[1].y, current_vertex.y);
        AABB_[1].z = std::max(AABB_[1].z, current_vertex.z);
    }
}

void Mesh::normalize_physical_datas()
{
    // For each attribute
    for (uint32_t attribute = 0; attribute < number_of_attributes_; attribute++) {
        // Normalize physical data
        float m = physical_datas_[attribute][0][0];
        float M = physical_datas_[attribute][0][0];
        float diff;

        for(uint32_t t = 0; t < physical_data_n_steps_[attribute]; t++){
            for(uint32_t data = 0; data < physical_datas_[attribute][t].size(); data++){
                m = glm::min(m, physical_datas_[attribute][t][data]);
                M = glm::max(M, physical_datas_[attribute][t][data]);
            }
        }

        diff = M - m;

        for(uint32_t t = 0; t < physical_data_n_steps_[attribute]; t++){
            for(uint32_t data = 0; data < physical_datas_[attribute][t].size(); data++){
                physical_datas_[attribute][t][data] = (physical_datas_[attribute][t][data] - m)/diff;
            }
        }
    }
}

void Mesh::compute_vertex_to_cell_incidence()
{
    // Each vertex has its incident cells
    vertex_to_cell_incidence_.reserve(number_of_vertices_);
    vertex_to_cell_incidence_.resize(number_of_vertices_);

    std::vector<uint32_t> current_cell;

    // For each cell
    for (uint32_t cell = 0; cell < number_of_cells_; cell++) {
        current_cell = get_cell(cell);

        // For each vertex of that cell
        for (const auto& vertex : current_cell) {
            // Add the index of the cell
            vertex_to_cell_incidence_[vertex].push_back(cell);
        }
    }
}

bool Mesh::compare_faces(std::vector<uint32_t> face_1, std::vector<uint32_t> face_2)
{
    // Sort the faces indices in ascending order
    std::sort(face_1.begin(), face_1.end());
    std::sort(face_2.begin(), face_2.end());

    // Check if faces share all vertices
    return std::equal(face_1.begin(), face_1.end(),
        face_2.begin(), face_2.end());
}

void Mesh::compute_connectivity()
{
    std::cout << "[Connectivity]" << std::endl;

    // Compute necessary structure for connectivity and bricking
    std::cout << "-- computing vertex/cell table ..." << std::endl;

    compute_vertex_to_cell_incidence();

    // Extracting faces
    std::cout << "-- extracting faces ..." << std::endl;

    uint32_t number_of_cell_types = Cell::get_number_of_cell_types();
    uint32_t global_cell = 0;
    uint32_t global_face = 0;
    uint32_t current_vertex;
    uint32_t current_incident_cell;
    uint32_t current_incident_cell_type;
    uint32_t current_incident_cell_offset;
    bool match_found = false;
    std::vector<std::vector<uint32_t>> current_cell_faces;
    std::vector<std::vector<uint32_t>> current_incident_cell_faces;
    std::vector<std::vector<bool>> extracted_faces{number_of_cell_types};

    // Initialize all faces as non processed
    for (uint32_t cell_type = 0; cell_type < number_of_cell_types; cell_type++) {
        extracted_faces[cell_type].reserve(number_of_cells_per_type_[cell_type] * Cell::get_number_of_faces(cell_type));
        extracted_faces[cell_type].resize(number_of_cells_per_type_[cell_type] * Cell::get_number_of_faces(cell_type), false);
        number_of_faces_ += static_cast<uint32_t>(extracted_faces[cell_type].size());
    }

    // Connectivity computing
    std::cout << "-- computing connectivity ..." << std::endl;

    // For each cell type
    for (uint32_t cell_type = 0; cell_type < number_of_cell_types; cell_type++) {
        // For each cell of that type
        for (uint32_t cell = 0; cell < number_of_cells_per_type_[cell_type]; cell++) {
            // Retrieve the faces of that cell
            current_cell_faces = get_faces(global_cell);

            // For each face of that cell
            for (uint32_t face = 0; face < current_cell_faces.size(); face++) {
                // If the face is not processed
                if (!extracted_faces[cell_type][cell * current_cell_faces.size() + face]) {
                    match_found = false;

                    // Process face
                    extracted_faces[cell_type][cell * current_cell_faces.size() + face] = true;
                    faces_.push_back(global_face);
                    back_cells_.push_back(global_cell);

                    // First vertex of current face
                    current_vertex = current_cell_faces[face][0];

                    // For each incident cell
                    for (uint32_t inc_c = 0; inc_c < vertex_to_cell_incidence_[current_vertex].size(); inc_c++) {
                        current_incident_cell = vertex_to_cell_incidence_[current_vertex][inc_c];

                        // If cell is not already processed
                        if (current_incident_cell > global_cell) {
                            // Retrieve the faces of the current incident cell
                            current_incident_cell_faces = get_faces(current_incident_cell);
                            current_incident_cell_type = get_cell_type(current_incident_cell);
                            current_incident_cell_offset = get_cell_offset(current_incident_cell);

                            // Check if there is a corresponding face
                            for (uint32_t current_incident_cell_face = 0; current_incident_cell_face < current_incident_cell_faces.size(); current_incident_cell_face++) {
                                if (compare_faces(current_cell_faces[face], current_incident_cell_faces[current_incident_cell_face])) {
                                    match_found = true;

                                    // Process face
                                    extracted_faces[current_incident_cell_type][current_incident_cell_offset * current_incident_cell_faces.size() + current_incident_cell_face] = true;
                                    front_cells_.push_back(current_incident_cell);

                                    // Face is not a boundary face
                                    boundary_flags_.push_back(false);

                                    break;
                                }
                            }
                        }

                        // No need to check the other incident cells
                        if (match_found)
                            break;
                    }

                    // No match was found, face is on the boundary
                    if (!match_found) {
                        front_cells_.push_back(0);
                        boundary_flags_.push_back(true);
                        number_of_boundary_faces_++;
                    }
                }

                global_face++;
            }

            global_cell++;
        }
    }

    number_of_unique_faces_ = static_cast<uint32_t>(faces_.size());
}

void Mesh::print_mesh() const
{
    std::cout << "[Mesh]" << std::endl;
    std::cout << "-- name : " << name_ << std::endl;
    std::cout << "-- " << number_of_vertices_ << " vertices" << std::endl;
    std::cout << "-- " << number_of_attributes_ << " attributes" << std::endl;
    
    for(uint32_t attribute = 0; attribute < number_of_attributes_; attribute++)
        std::cout << "   -- " << physical_data_names_[attribute] << " : " << physical_datas_[attribute].size() << " x " << physical_datas_[attribute][0].size() << " datapoints" << std::endl;

    if (!element_physical_datas_.empty()) {
        std::cout << "-- " << element_physical_datas_.size() << " element scalar fields" << std::endl;
        for (const auto& field : element_physical_datas_) {
            std::cout << "   -- " << field.name << " : cell_type=" << field.cell_type
                      << ", order=" << field.field_order
                      << ", dofs_per_cell=" << field.dofs_per_cell
                      << ", " << field.values.size() << " timesteps" << std::endl;
        }
    }

    std::cout << "-- " << number_of_cells_ << " cells" << std::endl;
    
    for(uint32_t cell_type = 0; cell_type < Cell::get_number_of_cell_types(); cell_type++){
        if(number_of_cells_per_type_[cell_type] > 0)
            std::cout << "   -- " << number_of_cells_per_type_[cell_type] << " " << Cell::get_name(cell_type) << std::endl;
    }

    std::cout << "-- " << number_of_unique_faces_ << " faces" << std::endl;
    std::cout << "   -- " << number_of_unique_faces_ - number_of_boundary_faces_ << " two-sided faces" << std::endl;
    std::cout << "   -- " << number_of_boundary_faces_ << " boundary faces" << std::endl;

    std::cout << "-- AABB : " << std::endl;;
    std::cout << "   -- [" << AABB_[0].x << ", " << AABB_[0].y << ", " << AABB_[0].z << "]" << std::endl;
    std::cout << "   -- [" << AABB_[1].x << ", " << AABB_[1].y << ", " << AABB_[1].z << "]" << std::endl;
}

void Mesh::update_vertex(size_t)
{
    compute_AABB();
    onMeshUpdated.execute();
}

void Mesh::add_element_scalar_field(const ElementScalarField& field)
{
    element_physical_datas_.push_back(field);
    normalize_element_physical_datas();
}

bool Mesh::has_element_scalar_fields() const
{
    return !element_physical_datas_.empty();
}

size_t Mesh::get_number_of_element_scalar_fields() const
{
    return element_physical_datas_.size();
}

const ElementScalarField& Mesh::get_element_scalar_field(size_t index) const
{
    if (index >= element_physical_datas_.size()) {
        std::cerr << "[ERROR] ElementScalarField index " << index << " out of bounds" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return element_physical_datas_[index];
}

ElementScalarField& Mesh::get_element_scalar_field(size_t index)
{
    if (index >= element_physical_datas_.size()) {
        std::cerr << "[ERROR] ElementScalarField index " << index << " out of bounds" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return element_physical_datas_[index];
}

void Mesh::normalize_element_physical_datas()
{
    for (auto& field : element_physical_datas_) {
        if (field.values.empty()) continue;

        float m = field.values[0][0];
        float M = field.values[0][0];

        for (size_t t = 0; t < field.values.size(); t++) {
            for (size_t d = 0; d < field.values[t].size(); d++) {
                m = std::min(m, field.values[t][d]);
                M = std::max(M, field.values[t][d]);
            }
        }

        float diff = M - m;
        if (diff > 1e-8f) {
            for (size_t t = 0; t < field.values.size(); t++) {
                for (size_t d = 0; d < field.values[t].size(); d++) {
                    field.values[t][d] = (field.values[t][d] - m) / diff;
                }
            }
        }
    }
}
