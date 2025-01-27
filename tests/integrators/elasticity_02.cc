// ---------------------------------------------------------------------
//
// Copyright (C) 2012 - 2020 by the deal.II authors
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


// Test nitsche_tangential in Elasticity
// Output matrices and assert consistency of residuals
#include <deal.II/base/vector_slice.h>

#include <deal.II/fe/fe_dgq.h>
#include <deal.II/fe/fe_nedelec.h>
#include <deal.II/fe/fe_raviart_thomas.h>
#include <deal.II/fe/fe_system.h>

#include <deal.II/integrators/elasticity.h>

#include "../tests.h"

#include "../test_grids.h"

using namespace LocalIntegrators::Elasticity;

template <int dim>
void
test_boundary(const FEValuesBase<dim> &fev)
{
  const unsigned int n = fev.dofs_per_cell;
  unsigned int       d = fev.get_fe().n_components();
  FullMatrix<double> M(n, n);
  nitsche_tangential_matrix(M, fev, 17);
  {
    deallog << "bdry" << std::endl;
    M.print_formatted(deallog.get_file_stream(), 3, true, 0, "0.");
  }

  Vector<double>                   u(n), v(n), w(n);
  std::vector<std::vector<double>> uval(
    d, std::vector<double>(fev.n_quadrature_points)),
    null_val(d, std::vector<double>(fev.n_quadrature_points, 0.));
  std::vector<std::vector<Tensor<1, dim>>> ugrad(
    d, std::vector<Tensor<1, dim>>(fev.n_quadrature_points));

  std::vector<types::global_dof_index> indices(n);
  for (unsigned int i = 0; i < n; ++i)
    indices[i] = i;

  {
    deallog << "Residuals" << std::endl;
    for (unsigned int i = 0; i < n; ++i)
      {
        u    = 0.;
        u(i) = 1.;
        w    = 0.;
        fev.get_function_values(u, indices, make_array_view(uval), true);
        fev.get_function_gradients(u, indices, make_array_view(ugrad), true);
        nitsche_tangential_residual<dim>(w, fev, uval, ugrad, null_val, 17);
        M.vmult(v, u);
        w.add(-1., v);
        deallog << ' ' << w.l2_norm();
      }
    deallog << std::endl;
  }
}


template <int dim>
void
test_fe(Triangulation<dim> &tr, FiniteElement<dim> &fe)
{
  deallog << fe.get_name() << std::endl << "cell matrix" << std::endl;
  typename Triangulation<dim>::cell_iterator cell1 = tr.begin(1);

  QGauss<dim - 1>   face_quadrature(fe.tensor_degree() + 1);
  FEFaceValues<dim> fef1(fe,
                         face_quadrature,
                         update_values | update_gradients |
                           update_normal_vectors | update_JxW_values);
  for (const unsigned int i : GeometryInfo<dim>::face_indices())
    {
      deallog << "boundary_matrix " << i << std::endl;
      fef1.reinit(cell1, i);
      test_boundary(fef1);
    }
}



template <int dim>
void
test(Triangulation<dim> &tr)
{
  FE_DGQ<dim>   q1(1);
  FESystem<dim> fe1(q1, dim);
  test_fe(tr, fe1);

  FE_DGQ<dim>   q2(2);
  FESystem<dim> fe2(q2, dim);
  test_fe(tr, fe2);

  FE_Nedelec<dim> n1(1);
  test_fe(tr, n1);

  FE_RaviartThomas<dim> rt1(1);
  test_fe(tr, rt1);
}


int
main()
{
  initlog();
  deallog.precision(8);

  Triangulation<2> tr2;
  TestGrids::hypercube(tr2, 1);
  test(tr2);

  Triangulation<3> tr3;
  TestGrids::hypercube(tr3, 1);
  test(tr3);
}
