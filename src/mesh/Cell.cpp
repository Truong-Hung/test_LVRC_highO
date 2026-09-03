///
/// \file Cell.cpp
/// \brief Source file of Cell
///


#include <algorithm>


#include "mesh/Cell.hpp"

const uint32_t Cell::number_of_cell_types_ = 10;

const std::vector<std::string> Cell::names_ =
{
    "Tetrahedron (order 1)",
    "Hexahedron (order 1)",
    "Prism-3 (order 1)",
    "Pyramid-4 (order 1)",
    "Tetrahedron (order 2)",
    "Hexahedron (order 2)",
    "Prism-3 (order 2)",
    "Pyramid-4 (order 2)",
    "Hexahedron (order 2, full)",
    "Tetrahedron (order 3)"
};

const std::vector<std::string> Cell::file_suffixes_ =
{
    ".tetra_1",
    ".hexa_1",
    ".prism3_1",
    ".pyramid4_1",
    ".tetra_2",
    ".hexa_2",
    ".prism3_2",
    ".pyramid4_2",
    ".hexa27_2",
    ".tetra_3"
};

const std::vector<uint32_t> Cell::number_of_vertices_ =
{
    4,  // TETRAHEDRON_1
    8,  // HEXAHEDRON_1
    6,  // PRISM3_1
    5,  // PYRAMID4_1
    10, // TETRAHEDRON_2
    20, // HEXAHEDRON_2
    15, // PRISM3_2
    13, // PYRAMID4_2
    27, // HEXAHEDRON_2_FULL
    20  // TETRAHEDRON_3
};

const std::vector<uint32_t> Cell::number_of_faces_ =
{
    4,  // TETRAHEDRON_1
    6,  // HEXAHEDRON_1
    5,  // PRISM3_1
    5,  // PYRAMID4_1
    4,  // TETRAHEDRON_2
    6,  // HEXAHEDRON_2
    5,  // PRISM3_2
    5,  // PYRAMID4_2
    6,   // HEXAHEDRON_2_FULL
    4    // TETRAHEDRON_3
};

//!!!! IF YOU MODIFY THIS PLEASE CHANGE IT ALSO IN scripts/generate_utils.py
//!!!! AND RUN THE SCRIPT ALSO PLEASE UPDATE THE SORTED ARRAY AND ITS PERMUTATION BELOW
const std::vector<std::vector<std::vector<uint32_t>>> Cell::face_masks_ =
{
    // TETRAHEDRON_1
    {{0, 2, 1},
     {0, 1, 3},
     {0, 3, 2},
     {1, 2, 3}},

    // HEXAHEDRON_1
    {{0, 3, 2, 1},
     {0, 4, 7, 3},
     {0, 1, 5, 4},
     {1, 2, 6, 5},
     {2, 3, 7, 6},
     {4, 5, 6, 7}},

    // PRISM3_1
    {{0, 2, 1},
     {3, 4, 5},
     {1, 2, 5, 4},
     {0, 3, 5, 2},
     {0, 1, 4, 3}},

    // PYRAMID4_1
    {{0, 1, 4},
     {0, 4, 3},
     {1, 2, 4},
     {2, 3, 4},
     {0, 3, 2, 1}},

    // TETRAHEDRON_2
    {{0, 6, 2, 5, 1, 4},
     {0, 4, 1, 9, 3, 7},
     {0, 7, 3, 8, 2, 6},
     {1, 5, 2, 8, 3, 9}},

    // HEXAHEDRON_2
    {{0, 9, 3, 13, 2, 11, 1, 8},
     {0, 10, 4, 17, 7, 15, 3, 9},
     {0, 8, 1, 12, 5, 16, 4, 10},
     {1, 11, 2, 14, 6, 18, 5, 12},
     {2, 13, 3, 15, 7, 19, 6, 14},
     {4, 16, 5, 18, 6, 19, 7, 17}},

    // PRISM3_2
    {{0, 6, 1, 9, 2, 7},
     {3, 13, 5, 14, 4, 12},
     {0, 7, 2, 11, 5, 13, 3, 8},
     {0, 8, 3, 12, 4, 10, 1, 6},
     {1, 10, 4, 14, 5, 11, 2, 9}},

    // PYRAMID4_2
    {{0, 5, 1, 9, 4, 7},
     {0, 7, 4, 12, 3, 6},
     {1, 8, 2, 11, 4, 9},
     {2, 10, 3, 12, 4, 11},
     {0, 6, 3, 10, 2, 8, 1, 5}},

    // HEXAHEDRON_2_FULL (27 nodes)
    {{0, 9, 3, 13, 2, 11, 1, 8, 20},
     {0, 10, 4, 17, 7, 15, 3, 9, 22},
     {0, 8, 1, 12, 5, 16, 4, 10, 21},
     {1, 11, 2, 14, 6, 18, 5, 12, 23},
     {2, 13, 3, 15, 7, 19, 6, 14, 24},
     {4, 16, 5, 18, 6, 19, 7, 17, 25}},

    // TETRAHEDRON_3
    // Different documentation for the face masks of TETRAHEDRON_3 as follows:
    // https://people.sc.fsu.edu/~jburkardt/datasets/tet_mesh_order20/tet_mesh_order20.html
    // https://onelab.info/pipermail/gmsh/2014/009142.html
    // but they give contradictory information about the face masks of TETRAHEDRON_3.
    // The following face masks seems to be correct.
    
    {{0, 8, 9, 2, 7, 6, 1, 5, 4, 16},
     {0, 4, 5, 1, 14, 15, 3, 11, 10, 17},
     {0, 10, 11, 3, 13, 12, 2, 9, 8, 18},
     {1, 6, 7, 2, 12, 13, 3, 15, 14, 19}}
};

const std::vector<std::vector<std::vector<uint32_t>>> Cell::face_sorted_masks_ =
{
     // TETRAHEDRON_1
     {{0, 1, 2},
     {0, 1, 3},
     {0, 2, 3},
     {1, 2, 3}},

     // HEXAHEDRON_1
     {{0, 1, 2, 3},
     {0, 1, 4, 5},
     {0, 3, 4, 7},
     {1, 2, 5, 6},
     {2, 3, 6, 7},
     {4, 5, 6, 7}},

     // PRISM3_1
     {{0, 1, 2},
     {0, 1, 3, 4},
     {0, 2, 3, 5},
     {1, 2, 4, 5},
     {3, 4, 5}},

     // PYRAMID4_1
     {{0, 1, 2, 3},
     {0, 1, 4},
     {0, 3, 4},
     {1, 2, 4},
     {2, 3, 4}},
     
     // TETRAHEDRON_2
     {{0, 1, 2, 4, 5, 6},
     {0, 1, 3, 4, 7, 9},
     {0, 2, 3, 6, 7, 8},
     {1, 2, 3, 5, 8, 9}},

     // HEXAHEDRON_2
     {{0, 1, 2, 3, 8, 9, 11, 13},
     {0, 1, 4, 5, 8, 10, 12, 16},
     {0, 3, 4, 7, 9, 10, 15, 17},
     {1, 2, 5, 6, 11, 12, 14, 18},
     {2, 3, 6, 7, 13, 14, 15, 19},
     {4, 5, 6, 7, 16, 17, 18, 19}},

     // PRISM3_2
     {{0, 1, 2, 6, 7, 9},
     {0, 1, 3, 4, 6, 8, 10, 12},
     {0, 2, 3, 5, 7, 8, 11, 13},
     {1, 2, 4, 5, 9, 10, 11, 14},
     {3, 4, 5, 12, 13, 14}},

     // PYRAMID4_2
     {{0, 1, 2, 3, 5, 6, 8, 10},
     {0, 1, 4, 5, 7, 9},
     {0, 3, 4, 6, 7, 12},
     {1, 2, 4, 8, 9, 11},
     {2, 3, 4, 10, 11, 12}},

     // HEXAHEDRON_2_FULL (27)
     {{0, 1, 2, 3, 8, 9, 11, 13, 20},
     {0, 1, 4, 5, 8, 10, 12, 16, 21},
     {0, 3, 4, 7, 9, 10, 15, 17, 22},
     {1, 2, 5, 6, 11, 12, 14, 18, 23},
     {2, 3, 6, 7, 13, 14, 15, 19, 24},
     {4, 5, 6, 7, 16, 17, 18, 19, 25}},

     // TETRAHEDRON_3
    //  {{0, 1, 2, 4, 5, 6, 7, 8, 9, 16},
    //   {0, 1, 3, 4, 5, 10, 11, 12, 13, 17},
    //   {0, 2, 3, 8, 9, 10, 11, 14, 15, 18},
    //   {1, 2, 3, 6, 7, 12, 13, 14, 15, 19}}
     {{0, 1, 2, 4, 5, 6, 7, 9, 8, 16},
      {0, 1, 3, 4, 5, 10, 11, 14, 15, 17},
      {0, 2, 3, 9, 8, 10, 11, 12, 13, 18},
      {1, 2, 3, 6, 7, 12, 13, 14, 14, 19}}
};

const std::vector<std::vector<uint32_t>> Cell::face_mask_permutation_ =
{
     // TETRAHEDRON_1
     {0,1,2,3},
     // HEXAHEDRON_1
     {0,2,1,3,4,5},
     // PRISM3_1
     {0,3,2,4,1},
     // PYRAMID4_1
     {4,0,1,2,3},
     // TETRAHEDRON_2
     {0,1,2,3},
     // HEXAHEDRON_2
     {0,2,1,3,4,5},
     // PRISM3_2
     {0,3,2,4,1},
     // PYRAMID4_2
     {4,0,1,2,3},
     // HEXAHEDRON_2_FULL
     {0,2,1,3,4,5},
     // TETRAHEDRON_3
     {0,1,2,3}
};

const uint32_t Cell::max_number_of_faces_ = *std::max_element(Cell::number_of_faces_.cbegin(), Cell::number_of_faces_.cend());

const uint32_t Cell::max_number_of_face_vertices_ = [](){
    uint32_t max = 0;
    for (const auto& cellFaceMask : Cell::face_masks_)
        max = std::max(max,
                (uint32_t)std::max_element(cellFaceMask.cbegin(), cellFaceMask.cend(),
                        [](const auto& a, const auto& b){ return a.size() < b.size(); })->size());
    return max;
}();
