///
/// \file Mesh.hpp
/// \brief Header file of Mesh
///

#ifndef MESH_HPP
#define MESH_HPP

#include <iostream>
#include <algorithm>

#include "mesh/Cell.hpp"
#include "mesh/ElementScalarField.hpp"
#include "mesh/reader/FileReader.hpp"
#include "event_manager.h"

DECLARE_DELEGATE_MULTICAST(MeshUpdated)



///
/// \class Mesh Mesh.hpp "Mesh.hpp"
/// \brief Represents a mesh and its elements
///
class Mesh
{
public:
    // Mesh name
    std::string name_ = "default_mesh_name";

    // Count informations
    uint32_t number_of_vertices_ = 0;
    uint32_t number_of_attributes_ = 0;

    uint32_t number_of_cells_ = 0;
    std::vector<uint32_t> number_of_cells_per_type_;

    uint32_t number_of_faces_ = 0;
    uint32_t number_of_unique_faces_ = 0;
    uint32_t number_of_boundary_faces_ = 0;

    // Vertex informations
    std::vector<glm::vec3> vertices_;

    // Field informations
    std::vector<std::vector<std::vector<float>>> physical_datas_;
    std::vector<std::string> physical_data_names_;
    std::vector<uint> physical_data_n_steps_;

    // Cell informations
    std::vector<std::vector<uint32_t>> cells_;

    // Scalar-field interpolation support.
    std::vector<ElementScalarField> element_physical_datas_;

    // Axis-aligned bounding box
    glm::vec3 AABB_[2];

    // Face informations
    std::vector<uint32_t> faces_;
    std::vector<uint32_t> front_cells_;
    std::vector<uint32_t> back_cells_;
    std::vector<bool> boundary_flags_;

    // Temporary connectivity informations
    std::vector<std::vector<uint32_t>> vertex_to_cell_incidence_;

public:
    ///
    /// \fn Mesh Mesh::Mesh(std::string file_path)
    /// \brief Generate a mesh object from a mesh file
    ///
    /// \param file_path path to mesh file
    ///
    Mesh(std::string file_path);

    ///
    /// \fn uint32_t Mesh::get_cell_type(uint32_t cell_index)
    /// \brief Get the type of a cell
    ///
    /// \param cell_index index of the cell
    ///
    /// \return type of the cell
    ///
    uint32_t get_cell_type(uint32_t cell_index) const;

    ///
    /// \fn uint32_t Mesh::get_cell_offset(uint32_t cell_index)
    /// \brief Get the offset of a cell in the corresponding table
    ///
    /// \param cell_index index of the cell
    ///
    /// \return offset of the cell in the corresponding table
    ///
    uint32_t get_cell_offset(uint32_t cell_index) const;

    ///
    /// \fn std::vector<uint32_t> Mesh::get_cell(uint32_t cell_index)
    /// \brief Get the indices of the vertices of a cell
    ///
    /// \param cell_index index of the cell
    ///
    /// \return indices of the vertices of the cell
    ///
    std::vector<uint32_t> get_cell(uint32_t cell_index) const;


    /// @brief Return a couple of iterator on the vertives of the given cell.
    /// @param cell_index index of the cell
    /// @return (begin, end) iterator of the cell vertices
    std::pair<std::vector<uint32_t>::const_iterator, std::vector<uint32_t>::const_iterator> get_cell_iter(uint32_t cell_index) const;

    ///
    /// \fn void Mesh::get_face_location(uint32_t face_index, uint32_t& cell_type, uint32_t& cell_offset, uint32_t& face_mask_index)
    ///
    /// \param face_index index of the face
    /// \param cell_type reference to the variable to be written for the type of the face's cell
    /// \param cell_offset reference to the variable to be written for the offset of the face's cell
    /// \param face_mask_index reference to the variable to be written for the index of the face within its cell
    ///
    void get_face_location(uint32_t face_index, uint32_t& cell_type, uint32_t& cell_offset, uint32_t& face_mask_index) const;

    ///
    /// \fn std::vector<uint32_t> Mesh::get_face(uint32_t face_index)
    /// \brief Get the indices of the vertices of a face
    ///
    /// \param face_index index of the face
    ///
    /// \return indices of the vertices of the face
    ///
    std::vector<uint32_t> get_face(uint32_t face_index) const;

    ///
    /// \fn std:vector<std::vector<uint32_t>> Mesh::get_faces(uint32_t cell_index)
    /// \brief Get the indices of all the faces of a cell
    ///
    /// \param cell_index index of the cell
    ///
    /// \return indices of the vertices of all the faces of a cell
    ///
    std::vector<std::vector<uint32_t>> get_faces(uint32_t cell_index) const;

    ///
    /// \fn void Mesh::add_element_scalar_field(const ElementScalarField& field)
    /// \brief Add an element-based scalar field to the mesh
    ///
    void add_element_scalar_field(const ElementScalarField& field);

    ///
    /// \fn bool Mesh::has_element_scalar_fields() const
    /// \brief Check if the mesh has any element-based scalar fields
    ///
    bool has_element_scalar_fields() const;

    ///
    /// \fn size_t Mesh::get_number_of_element_scalar_fields() const
    /// \brief Get the number of element-based scalar fields
    ///
    size_t get_number_of_element_scalar_fields() const;

    ///
    /// \fn const ElementScalarField& Mesh::get_element_scalar_field(size_t index) const
    /// \brief Get an element-based scalar field by index
    ///
    const ElementScalarField& get_element_scalar_field(size_t index) const;

    ///
    /// \fn ElementScalarField& Mesh::get_element_scalar_field(size_t index)
    /// \brief Get a mutable element-based scalar field by index
    ///
    ElementScalarField& get_element_scalar_field(size_t index);

    ///
    /// \fn Mesh Mesh::print_mesh()
    /// \brief Print mesh information
    ///
    void print_mesh() const;

    ///
    /// \fn Call this method when vertice information is updated
    ///
    void update_vertex(size_t vertex_index);

    MeshUpdated onMeshUpdated;

private:
    ///
    /// \fn void Mesh::load_default_mesh()
    /// \brief Load a mesh
    ///
    /// \param reader reader used for the mesh
    ///
    void load_default_mesh(FileReader& reader);

    ///
    /// \fn void Mesh::compute_AABB()
    /// \brief Compute the axis-aligned bounding box
    ///
    void compute_AABB();

    ///
    /// \fn void Mesh::normalize_physical_datas()
    /// \brief Normalize all physical attributes
    ///
    void normalize_physical_datas();

    ///
    /// \fn void Mesh::normalize_element_physical_datas()
    /// \brief Normalize all element-based physical attributes
    ///
    void normalize_element_physical_datas();

    ///
    /// \fn void Mesh::compute_vertex_to_cell_incidence()
    /// \brief Compute the vertex to cell incidence table
    ///
    void compute_vertex_to_cell_incidence();

    ///
    /// \fn void Mesh::compute_connectivity()
    /// \brief Compute connectivity
    ///
    void compute_connectivity();

    ///
    /// \fn void Mesh::compare_faces(std::vector<uint32_t> face_1, std::vector<uint32_t> face_2)
    /// \brief Compare two faces
    ///
    /// \param face_1 first face
    /// \param face_2 second face
    ///
    /// \return true if both faces are the same, false instead
    ///
    bool compare_faces(std::vector<uint32_t> face_1, std::vector<uint32_t> face_2);
};

#endif
