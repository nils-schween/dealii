/* $Id$ */


#include "poisson.h"



template <int dim>
class BoundaryValuesSine : public Function<dim> {
  public:
    				     /**
				      * Return the value of the function
				      * at the given point.
				      */
    virtual double operator () (const Point<dim> &p) const {
      return cos(2*3.1415926536*p(0))*cos(2*3.1415926536*p(1));
    };
    

				     /**
				      * Set #values# to the point values
				      * of the function at the #points#.
				      * It is assumed that #values# be
				      * empty.
				      */
    virtual void value_list (const vector<Point<dim> > &points,
			     vector<double>            &values) const {
      for (unsigned int i=0; i<points.size(); ++i) 
	values.push_back (cos(2*3.1415926536*points[i](0)) *
			  cos(2*3.1415926536*points[i](1)));
    };
};



template <int dim>
class BoundaryValuesJump : public Function<dim> {
  public:
    				     /**
				      * Return the value of the function
				      * at the given point.
				      */
    virtual double operator () (const Point<dim> &p) const {
      switch (dim) 
	{
	  case 1:
		return 0;
	  default:
		if (p(0) == p(1))
		  return 0.5;
		else
		  return (p(0)>p(1) ? 0. : 1.);
	};
    };
};




template <int dim>
class RHSTrigPoly : public Function<dim> {
  public:
    				     /**
				      * Return the value of the function
				      * at the given point.
				      */
    virtual double operator () (const Point<dim> &p) const;
};



/**
  Right hand side constructed such that the exact solution is
  $x(1-x)$ in 1d, $x(1-x)*y(1-y)$ in 2d, etc.
  */
template <int dim>
class RHSPoly : public Function<dim> {
  public:
    				     /**
				      * Return the value of the function
				      * at the given point.
				      */
    virtual double operator () (const Point<dim> &p) const;
};



template <int dim>
class Solution : public Function<dim> {
  public:
    				     /**
				      * Return the value of the function
				      * at the given point.
				      */
    virtual double operator () (const Point<dim> &p) const;
};









template <int dim>
class CurvedLine :
  public StraightBoundary<dim> {
  public:
      virtual Point<dim> in_between (const PointArray &neighbors) const {
	Point<dim> middle = StraightBoundary<dim>::in_between(neighbors);
	double x=middle(0),
	       y=middle(1);
	
	if (y<x)
	  if (y<1-x)
	    middle(1) = 0.04*sin(6*3.141592*middle(0));
	  else
	    middle(0) = 1+0.04*sin(6*3.141592*middle(1));
	
	else
	  if (y<1-x)
	    middle(0) = 0.04*sin(6*3.141592*middle(1));
	  else
	    middle(1) = 1+0.04*sin(6*3.141592*middle(0));

	return middle;
      };
};




template <int dim>
double RHSTrigPoly<dim>::operator () (const Point<dim> &p) const {
  const double pi = 3.1415926536;
  switch (dim) 
    {
      case 1:
	    return p(0)*p(0)*cos(2*pi*p(0));
      case 2:
	    return (-2.0*cos(pi*p(0)/2)*p(1)*sin(pi*p(1)) +
		    2.0*p(0)*sin(pi*p(0)/2)*pi*p(1)*sin(pi*p(1)) +
		    5.0/4.0*p(0)*p(0)*cos(pi*p(0)/2)*pi*pi*p(1)*sin(pi*p(1)) -
		    2.0*p(0)*p(0)*cos(pi*p(0)/2)*cos(pi*p(1))*pi);
      default:
	    return 0;
    };
};



template <int dim>
double RHSPoly<dim>::operator () (const Point<dim> &p) const {
  double ret_val = 0;
  for (unsigned int i=0; i<dim; ++i)
    ret_val += 2*p(i)*(1.-p(i));
  return ret_val;
};


template <int dim>
double Solution<dim>::operator () (const Point<dim> &p) const {
  double ret_val = 1;
  for (unsigned int i=0; i<dim; ++i)
    ret_val *= p(i)*(1.-p(i));
  return ret_val;
};





template <int dim>
PoissonProblem<dim>::PoissonProblem () :
		tria(0), dof(0), rhs(0), boundary_values(0) {};




template <int dim>
void PoissonProblem<dim>::clear () {
  if (tria != 0) {
    delete tria;
    tria = 0;
  };
  
  if (dof != 0) {
    delete dof;
    dof = 0;
  };

  if (rhs != 0) 
    {
      delete rhs;
      rhs = 0;
    };

  if (boundary_values != 0) 
    {
      delete boundary_values;
      boundary_values = 0;
    };

  ProblemBase<dim>::clear ();
};




template <int dim>
void PoissonProblem<dim>::create_new (const unsigned int) {
  clear ();
  
  tria = new Triangulation<dim>();
  dof = new DoFHandler<dim> (tria);
  set_tria_and_dof (tria, dof);
};




template <int dim>
void PoissonProblem<dim>::declare_parameters (ParameterHandler &prm) {
  if (dim>=2)
    prm.declare_entry ("Test run", "zoom in",
		       "tensor\\|zoom in\\|ball\\|curved line\\|random\\|jump\\|L-region");
  else
    prm.declare_entry ("Test run", "zoom in", "tensor\\|zoom in\\|random");

  prm.declare_entry ("Global refinement", "0",
		     ParameterHandler::RegularExpressions::Integer);
  prm.declare_entry ("Right hand side", "zero",
		     "zero\\|constant\\|trigpoly\\|poly");
  prm.declare_entry ("Boundary values", "zero",
		     "zero\\|sine\\|jump");
  prm.declare_entry ("Output file", "gnuplot.1");
};




template <int dim>
bool PoissonProblem<dim>::make_grid (ParameterHandler &prm) {
  String test = prm.get ("Test run");
  unsigned int test_case;
  if (test=="zoom in") test_case = 1;
  else
    if (test=="ball") test_case = 2;
    else
      if (test=="curved line") test_case = 3;
      else
	if (test=="random") test_case = 4;
	else
	  if (test=="tensor") test_case = 5;
	  else
	    if (test=="jump") test_case = 6;
	    else
	      if (test=="L-region") test_case = 7;
	      else
		{
		  cerr << "This test seems not to be implemented!" << endl;
		  return false;
		};

  switch (test_case) 
    {
      case 1:
	    make_zoom_in_grid ();
	    break;
      case 2:
					     // make ball grid around origin with
					     // unit radius
      {
	    static const Point<dim> origin;
	    static const HyperBallBoundary<dim> boundary(origin, 1.);
	    tria->create_hyper_ball (origin, 1.);
	    tria->set_boundary (&boundary);
	    break;
      };
      case 3:
					     // set the boundary function
      {
	    static const CurvedLine<dim> boundary;
	    tria->create_hypercube ();
	    tria->set_boundary (&boundary);
	    break;
      };
      case 4:
	    make_random_grid ();
	    break;
      case 5:
	    tria->create_hypercube ();
	    break;
      case 6:
	    tria->create_hypercube ();
	    tria->refine_global (1);
	    for (unsigned int i=0; i<5; ++i)
	      {
		tria->begin_active(tria->n_levels()-1)->set_refine_flag();
		(--(tria->last_active()))->set_refine_flag();
		tria->execute_refinement ();
	      };
	    break;
      case 7:
	    tria->create_hyper_L ();
	    break;
      default:
	    return false;
    };

  int refine_global = prm.get_integer ("Global refinement");
  if ((refine_global < 0) || (refine_global>10))
    return false;
  else
    tria->refine_global (refine_global);

  return true;
};

	  


template <int dim>
void PoissonProblem<dim>::make_zoom_in_grid () {
  tria->create_hypercube ();
				   // refine first cell
  tria->begin_active()->set_refine_flag();
  tria->execute_refinement ();
				   // refine first active cell
				   // on coarsest level
  tria->begin_active()->set_refine_flag ();
  tria->execute_refinement ();
  
  Triangulation<dim>::active_cell_iterator cell;
  for (int i=0; i<17; ++i) 
    {
				       // refine the presently
				       // second last cell 17
				       // times
      cell = tria->last_active(tria->n_levels()-1);
      --cell;
      cell->set_refine_flag ();
      tria->execute_refinement ();
    };
};




template <int dim>
void PoissonProblem<dim>::make_random_grid () {
  tria->create_hypercube ();
  tria->refine_global (1);
	
  Triangulation<dim>::active_cell_iterator cell, endc;
  for (int i=0; i<12; ++i) 
    {
      int n_levels = tria->n_levels();
      cell = tria->begin_active();
      endc = tria->end();
      
      for (; cell!=endc; ++cell) 
	{
	  double r      = rand()*1.0/RAND_MAX,
		 weight = 1.*
			  (cell->level()*cell->level()) /
			  (n_levels*n_levels);
	  
	  if (r <= 0.5*weight)
	    cell->set_refine_flag ();
	};
      
      tria->execute_refinement ();
    };
};
  



template <int dim>
bool PoissonProblem<dim>::set_right_hand_side (ParameterHandler &prm) {
  String rhs_name = prm.get ("Right hand side");

  if (rhs_name == "zero")
    rhs = new ZeroFunction<dim>();
  else
    if (rhs_name == "constant")
      rhs = new ConstantFunction<dim>(1.);
    else
      if (rhs_name == "trigpoly")
	rhs = new RHSTrigPoly<dim>();
      else
	if (rhs_name == "poly")
	  rhs = new RHSPoly<dim> ();
	else
	  return false;

  if (rhs != 0)
    return true;
  else
    return false;
};



template <int dim>
bool PoissonProblem<dim>::set_boundary_values (ParameterHandler &prm) {
  String bv_name = prm.get ("Boundary values");

  if (bv_name == "zero")
    boundary_values = new ZeroFunction<dim> ();
  else
    if (bv_name == "sine")
      boundary_values = new BoundaryValuesSine<dim> ();
    else
      if (bv_name == "jump")
	boundary_values = new BoundaryValuesJump<dim> ();
      else
	return false;

  if (boundary_values != 0)
    return true;
  else
    return false;
};




template <int dim>
void PoissonProblem<dim>::run (ParameterHandler &prm) {
  cout << "Test case = " << prm.get ("Test run")
       << endl;
  
  cout << "    Making grid... ";
  if (!make_grid (prm))
    return;
  cout << tria->n_active_cells() << " active cells." << endl;
  
  if (!set_right_hand_side (prm))
    return;

  if (!set_boundary_values (prm))
    return;
  
  FELinear<dim>                   fe;
  PoissonEquation<dim>            equation (*rhs);
  QGauss3<dim>                    quadrature;
  
  cout << "    Distributing dofs... "; 
  dof->distribute_dofs (fe);
  cout << dof->n_dofs() << " degrees of freedom." << endl;

  cout << "    Assembling matrices..." << endl;
  FEValues<dim>::UpdateStruct update_flags;
  update_flags.q_points  = update_flags.gradients  = true;
  update_flags.jacobians = update_flags.JxW_values = true;
  
  ProblemBase<dim>::DirichletBC dirichlet_bc;
  dirichlet_bc[0] = boundary_values;
  assemble (equation, quadrature, fe, update_flags, dirichlet_bc);

  cout << "    Solving..." << endl;
  solve ();

  cout << "    Writing to file <" << prm.get("Output file") << ">..."
       << endl;

  Solution<dim> sol;
  dVector       l1_error_per_cell, l2_error_per_cell, linfty_error_per_cell;
  QGauss4<dim>  q;
  
  cout << "    Calculating L1 error... ";
  integrate_difference (sol, l1_error_per_cell, q, fe, L1_norm);
  cout << l1_error_per_cell.l1_norm() << endl;

  cout << "    Calculating L2 error... ";
  integrate_difference (sol, l2_error_per_cell, q, fe, L2_norm);
  cout << l2_error_per_cell.l2_norm() << endl;

  cout << "    Calculating L-infinity error... ";
  integrate_difference (sol, linfty_error_per_cell, q, fe, Linfty_norm);
  cout << linfty_error_per_cell.linfty_norm() << endl;

  dVector l1_error_per_dof, l2_error_per_dof, linfty_error_per_dof;
  dof->distribute_cell_to_dof_vector (l1_error_per_cell, l1_error_per_dof);
  dof->distribute_cell_to_dof_vector (l2_error_per_cell, l2_error_per_dof);
  dof->distribute_cell_to_dof_vector (linfty_error_per_cell, linfty_error_per_dof);

  DataOut<dim> out;
  String o_filename = prm.get ("Output file");
  ofstream gnuplot(o_filename);
  fill_data (out);
  out.add_data_vector (l1_error_per_dof, "L1-Error");
  out.add_data_vector (l2_error_per_dof, "L2-Error");
  out.add_data_vector (linfty_error_per_dof, "L3-Error");
  out.write_gnuplot (gnuplot);
  gnuplot.close ();

  cout << "Errors: "
       << dof->n_dofs() << "    "
       << l1_error_per_cell.l1_norm() << " "
       << l2_error_per_cell.l2_norm() << " "
       << linfty_error_per_cell.linfty_norm() << endl;
    


  cout << endl;
};





template class PoissonProblem<1>;
template class PoissonProblem<2>;
