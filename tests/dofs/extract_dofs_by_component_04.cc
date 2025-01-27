// ---------------------------------------------------------------------
//
// Copyright (C) 2000 - 2020 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE.md at
// the top level directory of deal.II.
//
// ---------------------------------------------------------------------



// test internal::extract_dofs_by_component for some corner cases that
// I was unsure about when refactoring some code in there
//
// this particular test checks the call path to
// internal::extract_dofs_by_component from DoFTools::extract_constant_modes


#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_nedelec.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_raviart_thomas.h>
#include <deal.II/fe/fe_system.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>

#include "../tests.h"



template <int dim>
void
check()
{
  Triangulation<dim> tr;
  GridGenerator::hyper_cube(tr, -1, 1);
  tr.refine_global(1);

  FESystem<dim>   element(FE_Q<dim>(1),
                        1,
                        FE_RaviartThomas<dim>(0),
                        1,
                        FE_Q<dim>(1),
                        1,
                        FE_Nedelec<dim>(0),
                        1);
  DoFHandler<dim> dof(tr);
  dof.distribute_dofs(element);

  // try all possible component
  // masks, which we encode as bit
  // strings
  for (unsigned int int_mask = 0; int_mask < (1U << element.n_components());
       ++int_mask)
    {
      ComponentMask component_mask(element.n_components(), false);
      for (unsigned int c = 0; c < element.n_components(); ++c)
        component_mask.set(c, (int_mask & (1 << c)));

      std::vector<std::vector<bool>> constant_modes;
      DoFTools::extract_constant_modes(dof, component_mask, constant_modes);

      for (unsigned int d = 0; d < constant_modes.size(); ++d)
        {
          deallog << "constant mode " << d << std::endl;
          for (unsigned int e = 0; e < constant_modes[d].size(); ++e)
            deallog << constant_modes[d][e];
          deallog << std::endl;
        }
    }
}


int
main()
{
  initlog();
  deallog << std::setprecision(2) << std::fixed;

  deallog.push("2d");
  check<2>();
  deallog.pop();
  deallog.push("3d");
  check<3>();
  deallog.pop();
}
