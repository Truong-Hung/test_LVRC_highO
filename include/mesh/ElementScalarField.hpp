///
/// \file ElementScalarField.hpp
/// \brief Header file for ElementScalarField representing element-based scalar/vector fields
///

#ifndef ELEMENT_SCALAR_FIELD_HPP
#define ELEMENT_SCALAR_FIELD_HPP

#include <string>
#include <vector>
#include <cstdint>

enum class FieldBasis : uint8_t
{
    GmshLagrange
};

struct ElementScalarField
{
    std::string name;

    uint32_t cell_type = 0;

    // Interpolation order of the scalar field
    uint32_t field_order = 1;

    // Number of degrees of freedom per cell (depends on the field order and the cell type)
    uint32_t dofs_per_cell = 0;

    // Number of components per degree of freedom (1 for scalar fields, 3 for vector fields, etc.)
    uint32_t components = 1;

    // Basis used for the scalar field interpolation in case we know them.
    // We will stick to Lagrange basis for now, but we may want to support other basis in the future.
    FieldBasis basis = FieldBasis::GmshLagrange;

    // values[timestep][localCellId * dofs_per_cell + localDof]
    std::vector<std::vector<float>> values;
};

#endif // ELEMENT_SCALAR_FIELD_HPP
