///
/// \file Cell.hpp
/// \brief Header file of Cell
///



#ifndef CELL_HPP
#define CELL_HPP

#include <stdint.h>
#include <string>
#include <vector>

// Order 1
#define TETRAHEDRON_1       0
#define HEXAHEDRON_1        1
#define PRISM3_1            2
#define PYRAMID4_1          3

// Order 2
#define TETRAHEDRON_2       4
#define HEXAHEDRON_2        5  //Hexahderon with 20 nodes (quadratic)
#define PRISM3_2            6
#define PYRAMID4_2          7
#define HEXAHEDRON_2_FULL   8  //Hexahderon with 27 nodes (full quadratic)

// Order 3
#define TETRAHEDRON_3       9




///
/// \class Cell Cell.hpp "Cell.hpp"
/// \brief Represents a cell
///
class Cell
{
private:
    static const uint32_t number_of_cell_types_;
    static const std::vector<std::string> names_;
    static const std::vector<std::string> file_suffixes_;
    static const std::vector<uint32_t> number_of_vertices_ ;
    static const std::vector<uint32_t> number_of_faces_;
    static const uint32_t max_number_of_faces_;
    static const uint32_t max_number_of_face_vertices_;
    static const std::vector<std::vector<std::vector<uint32_t>>> face_masks_;
    static const std::vector<std::vector<std::vector<uint32_t>>> face_sorted_masks_;
    static const std::vector<std::vector<uint32_t>> face_mask_permutation_;

public:

    ///
    /// \fn inline static const uint32_t& Cell::get_number_of_cell_types()
    /// \brief Get the number of supported cell types
    ///
    /// \return number of supported cell types
    ///
    inline static uint32_t get_number_of_cell_types()
    {
        return number_of_cell_types_;
    }

    ///
    /// \fn inline static const std::string& Cell::get_name(uint32_t cell_type)
    /// \brief Get the name of a given cell type
    ///
    /// \param cell_type type of cell
    ///
    /// \return name of the cell type
    ///
    inline static std::string get_name(uint32_t cell_type)
    {
        return names_[cell_type];
    }

    ///
    /// \fn inline static const std::string& Cell::get_file_suffix(uint32_t cell_type)
    /// \brief Get the name of a given cell type
    ///
    /// \param cell_type type of cell
    ///
    /// \return get_file_suffix of the cell type (in .lvrc format)
    ///
    inline static std::string get_file_suffix(uint32_t cell_type)
    {
        return file_suffixes_[cell_type];
    }

    ///
    /// \fn inline static const uint32_t& Cell::get_number_of_vertices(uint32_t cell_type)
    /// \brief Get the number of vertices of a given cell type
    ///
    /// \param cell_type type of cell
    ///
    /// \return number of vertices of the cell type
    ///
    inline static uint32_t get_number_of_vertices(uint32_t cell_type)
    {
        return number_of_vertices_[cell_type];
    }

    ///
    /// \fn inline static const uint32_t& Cell::get_number_of_faces(uint32_t cell_type)
    /// \brief Get the number of faces of a given cell type
    ///
    /// \param cell_type type of cell
    ///
    /// \return number of faces of the cell type
    ///
    inline static uint32_t get_number_of_faces(uint32_t cell_type)
    {
        return number_of_faces_[cell_type];
    }

    ///
    /// \fn inline static const uint32_t& Cell::get_max_number_of_faces()
    /// \brief Get the maximum possible number of faces in a cell
    ///
    /// \return maximum number of faces for any cell type
    ///
    inline static const uint32_t& get_max_number_of_faces()
    {
        return max_number_of_faces_;
    }

    ///
    /// \fn inline static const uint32_t& Cell::get_max_number_of_face_vertices()
    /// \brief Get the maximum possible number of vertices in a face
    ///
    /// \return maximum number of vertices in any face for any cell type
    ///
    inline static const uint32_t& get_max_number_of_face_vertices()
    {
        return max_number_of_face_vertices_;
    }

    ///
    /// \fn inline static const std::vector<std::vector<uint32_t>>& Cell::get_face_masks(uint32_t cell_type)
    /// \brief Get the face masks of a given cell type
    ///
    /// The face mask is a vector of faces where each face is a vector of vertex index offsets from the cell offset
    ///
    /// \param cell_type type of cell
    ///
    /// \return face masks corresponding to the given cell type
    ///
    inline static const std::vector<std::vector<uint32_t>>& get_face_masks(uint32_t cell_type)
    {
        return face_masks_[cell_type];
    }

     ///
    /// \fn inline static const std::vector<std::vector<uint32_t>>& Cell::get_sorted_face_masks(uint32_t cell_type)
    /// \brief Get the face masks of a given cell type
    ///
    /// \return face masks corresponding to the given cell type where each index is sorted
    ///
    inline static const std::vector<std::vector<uint32_t>>& get_sorted_face_masks(uint32_t cell_type)
    {
        return face_sorted_masks_[cell_type];
    }

    /// @brief Get the face mask index for a given cell type from the index of the same face mask in the sorted version of the array.
    /// @return index of the face mask in the array of get_face_masks(cell_type)
    inline static uint32_t get_face_mask_index_from_sorted(uint32_t cell_type, uint32_t sorted_face_index)
    {
        return face_mask_permutation_[cell_type][sorted_face_index];
    }

    ///
    /// \fn inline static const std::vector<uint32_t>& Cell::get_face_mask(uint32_t cell_type, uint32_t face_mask_index)
    /// \brief Get the face mask of a given face mask index and cell type
    ///
    /// \param cell_type type of cell
    /// \param face_mask_index index of the face mask
    ///
    /// \return face mask corresponding to the given face mask index and cell type
    ///
    inline static const std::vector<uint32_t>& get_face_mask(uint32_t cell_type, uint32_t face_mask_index)
    {
        return face_masks_[cell_type][face_mask_index];
    }
};

#endif