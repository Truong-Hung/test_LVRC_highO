///
/// \file ElementScalarField.hpp
/// \brief Header file for element-local scalar fields
///

#ifndef ELEMENT_SCALAR_FIELD_HPP
#define ELEMENT_SCALAR_FIELD_HPP

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>


enum class FieldBasis : uint8_t
{
    GmshLagrange
};


struct ElementScalarField
{
    std::string name;

    // Internal LVRC cell type, e.g. TETRAHEDRON10.
    uint32_t cell_type = 0;

    // Polynomial interpolation order of the scalar field.
    uint32_t field_order = 1;

    // Number of local field DOFs in one cell.
    // Example: complete tetrahedral P3 Lagrange field -> 20.
    uint32_t dofs_per_cell = 0;

    // Number of components per DOF. The current renderer should require 1.
    uint32_t components = 1;

    FieldBasis basis = FieldBasis::GmshLagrange;

    // values[timestep][
    //     (localCellId * dofs_per_cell + localDof) * components
    //     + component
    // ]
    std::vector<std::vector<float>> values;

    size_t value_index(
        uint32_t local_cell_id,
        uint32_t local_dof,
        uint32_t component = 0) const
    {
        if (dofs_per_cell == 0)
            throw std::logic_error("ElementScalarField: dofs_per_cell must be non-zero");

        if (components == 0)
            throw std::logic_error("ElementScalarField: components must be non-zero");

        if (local_dof >= dofs_per_cell)
            throw std::out_of_range("ElementScalarField: local DOF index out of range");

        if (component >= components)
            throw std::out_of_range("ElementScalarField: component index out of range");

        return (static_cast<size_t>(local_cell_id) * dofs_per_cell + local_dof)
             * components + component;
    }

    size_t expected_value_count(uint32_t number_of_cells) const
    {
        if (dofs_per_cell == 0)
            throw std::logic_error("ElementScalarField: dofs_per_cell must be non-zero");

        if (components == 0)
            throw std::logic_error("ElementScalarField: components must be non-zero");

        return static_cast<size_t>(number_of_cells)
             * dofs_per_cell
             * components;
    }

    bool has_valid_size(
        uint32_t timestep,
        uint32_t number_of_cells) const
    {
        return timestep < values.size()
            && values[timestep].size() == expected_value_count(number_of_cells);
    }

    void validate(uint32_t number_of_cells) const
    {
        if (name.empty())
            throw std::runtime_error("ElementScalarField: field name is empty");

        if (dofs_per_cell == 0)
            throw std::runtime_error("ElementScalarField: dofs_per_cell must be non-zero");

        if (components == 0)
            throw std::runtime_error("ElementScalarField: components must be non-zero");

        const size_t expected = expected_value_count(number_of_cells);

        for (size_t timestep = 0; timestep < values.size(); ++timestep)
        {
            if (values[timestep].size() != expected)
            {
                throw std::runtime_error(
                    "ElementScalarField '" + name
                    + "': invalid value count at timestep "
                    + std::to_string(timestep)
                    + " (expected " + std::to_string(expected)
                    + ", got " + std::to_string(values[timestep].size())
                    + ")");
            }
        }
    }
};

#endif // ELEMENT_SCALAR_FIELD_HPP