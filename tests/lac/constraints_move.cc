// ---------------------------------------------------------------------
//
// Copyright (C) 2017 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the deal.II distribution.
//
// ---------------------------------------------------------------------


#include "../tests.h"
#include <deal.II/base/logstream.h>
#include <deal.II/lac/constraint_matrix.h>

#include <fstream>

int main ()
{
  std::ofstream logfile("output");
  logfile.precision(2);

  deallog.attach(logfile);
  deallog.threshold_double(1.0e-10);

  ConstraintMatrix constraints;
  unsigned int IDs[] = {1, 2, 3, 5, 8, 13, 21};
  double vals[] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  for (unsigned int i=0; i < sizeof(IDs) / sizeof(IDs[0]); ++i)
    {
      constraints.add_line(IDs[i]);
      constraints.set_inhomogeneity(IDs[i], vals[i]);
    }

  constraints.print (deallog.get_file_stream());
  deallog << std::endl;

  ConstraintMatrix cm(std::move(constraints));
  cm.print (deallog.get_file_stream());
  deallog << constraints.n_constraints() << std::endl << std::endl;

  constraints = std::move(cm);
  constraints.print(deallog.get_file_stream());
  deallog << cm.n_constraints() << std::endl;

  deallog << "OK" << std::endl;
}
