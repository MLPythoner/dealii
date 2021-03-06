// ---------------------------------------------------------------------
//
// Copyright (C) 2015 - 2016 by the deal.II authors
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

#ifndef dealii__constrained_linear_operator_h
#define dealii__constrained_linear_operator_h

#include <deal.II/lac/linear_operator.h>
#include <deal.II/lac/packaged_operation.h>
#include <deal.II/lac/constraint_matrix.h>


DEAL_II_NAMESPACE_OPEN


/**
 * @name Indirectly applying constraints to LinearOperator
 */
//@{


/**
 * This function takes a ConstraintMatrix @p constraint_matrix and an operator
 * exemplar @p exemplar (this exemplar is usually a linear operator that
 * describes the system matrix - it is only used to create domain and range
 * vectors of appropriate sizes, its action <tt>vmult</tt> is never used). A
 * LinearOperator object associated with the "homogeneous action" of the
 * underlying ConstraintMatrix object is returned:
 *
 * Applying the LinearOperator object on a vector <code>u</code> results in a
 * vector <code>v</code> that stores the result of calling
 * ConstraintMatrix::distribute() on <code>u</code> - with one important
 * difference: inhomogeneities are not applied, but always treated as 0
 * instead.
 *
 * The LinearOperator object created by this function is primarily used
 * internally in constrained_linear_operator() to build up a modified system
 * of linear equations. How to solve a linear system of equations with this
 * approach is explained in detail in the
 * @ref constraints
 * module.
 *
 * @author Mauro Bardelloni, Matthias Maier, 2015
 *
 * @note Currently, this function may not work correctly for distributed data
 * structures.
 *
 * @relates LinearOperator
 * @ingroup constraints
 */
template <typename Range, typename Domain>
LinearOperator<Range, Domain> distribute_constraints_linear_operator(
  const ConstraintMatrix &constraint_matrix,
  const LinearOperator<Range, Domain> &exemplar)
{
  LinearOperator<Range, Domain> return_op = exemplar;

  return_op.vmult_add = [&constraint_matrix](Range &v, const Domain &u)
  {
    Assert(!dealii::PointerComparison::equal(&v, &u),
           dealii::ExcMessage("The domain and range vectors must be different "
                              "storage locations"));

    // First, add vector u to v unconditionally and clean up constrained
    // degrees of freedom later.
    v += u;

    const auto &locally_owned_elements = v.locally_owned_elements();
    for (const auto &line : constraint_matrix.get_lines())
      {
        const auto i = line.index;
        if (locally_owned_elements.is_element(i))
          {
            v(i) -= u(i);
            const auto &entries = line.entries;
            for (types::global_dof_index j = 0; j < entries.size(); ++j)
              {
                const auto pos = entries[j].first;
                v(i) += u(pos) * entries[j].second;
              }
          }
      }

    v.compress(VectorOperation::add);
  };

  return_op.Tvmult_add = [&constraint_matrix](Domain &v, const Range &u)
  {
    Assert(!dealii::PointerComparison::equal(&v, &u),
           dealii::ExcMessage("The domain and range vectors must be different "
                              "storage locations"));

    // First, add vector u to v unconditionally and clean up constrained
    // degrees of freedom later.
    v += u;

    const auto &locally_owned_elements = v.locally_owned_elements();
    for (const auto &line : constraint_matrix.get_lines())
      {
        const auto i = line.index;

        if (locally_owned_elements.is_element(i))
          {
            v(i) -= u(i);
          }

        const auto &entries = line.entries;
        for (types::global_dof_index j = 0; j < entries.size(); ++j)
          {
            const auto pos = entries[j].first;
            if (locally_owned_elements.is_element(pos))
              v(pos) += u(i) * entries[j].second;
          }
      }

    v.compress(VectorOperation::add);
  };

  // lambda capture expressions are a C++14 feature...
  const auto vmult_add = return_op.vmult_add;
  return_op.vmult = [vmult_add](Range &v, const Domain &u)
  {
    v = 0.;
    vmult_add(v, u);
  };

  // lambda capture expressions are a C++14 feature...
  const auto Tvmult_add = return_op.Tvmult_add;
  return_op.Tvmult = [Tvmult_add](Domain &v, const Range &u)
  {
    v = 0.;
    Tvmult_add(v, u);
  };

  return return_op;
}


/**
 * Given a ConstraintMatrix @p constraint_matrix and an operator exemplar @p
 * exemplar, return a LinearOperator that is the projection to the subspace of
 * constrained degrees of freedom, i.e. all entries of the result vector that
 * correspond to unconstrained degrees of freedom are set to zero.
 *
 * @author Mauro Bardelloni, Matthias Maier, 2015
 *
 * @relates LinearOperator
 * @ingroup constraints
 */
template <typename Range, typename Domain>
LinearOperator<Range, Domain> project_to_constrained_linear_operator(
  const ConstraintMatrix &constraint_matrix,
  const LinearOperator<Range, Domain> &exemplar)
{
  LinearOperator<Range, Domain> return_op = exemplar;

  return_op.vmult_add = [&constraint_matrix](Range &v, const Domain &u)
  {
    const auto &locally_owned_elements = v.locally_owned_elements();
    for (const auto &line : constraint_matrix.get_lines())
      {
        const auto i = line.index;
        if (locally_owned_elements.is_element(i))
          {
            v(i) += u(i);
          }
      }

    v.compress(VectorOperation::add);
  };

  return_op.Tvmult_add = [&constraint_matrix](Domain &v, const Range &u)
  {
    const auto &locally_owned_elements = v.locally_owned_elements();
    for (const auto &line : constraint_matrix.get_lines())
      {
        const auto i = line.index;
        if (locally_owned_elements.is_element(i))
          {
            v(i) += u(i);
          }
      }

    v.compress(VectorOperation::add);
  };

  // lambda capture expressions are a C++14 feature...
  const auto vmult_add = return_op.vmult_add;
  return_op.vmult = [vmult_add](Range &v, const Domain &u)
  {
    v = 0.;
    vmult_add(v, u);
  };

  // lambda capture expressions are a C++14 feature...
  const auto Tvmult_add = return_op.Tvmult_add;
  return_op.Tvmult = [Tvmult_add](Domain &v, const Range &u)
  {
    v = 0.;
    Tvmult_add(v, u);
  };

  return return_op;
}


/**
 * Given a ConstraintMatrix object @p constraint_matrix and a LinearOperator
 * @p linop, this function creates a LinearOperator object consisting of the
 * composition of three operations and a regularization:
 * @code
 *   Ct * linop * C + Id_c;
 * @endcode
 * with
 * @code
 *   C = distribute_constraints_linear_operator(constraint_matrix, linop);
 *   Ct = transpose_operator(C);
 *   Id_c = project_to_constrained_linear_operator(constraint_matrix, linop);
 * @endcode
 * and <code>Id_c</code> is the projection to the subspace consisting of all
 * vector entries associated with constrained degrees of freedoms.
 *
 * This LinearOperator object is used together with
 * constrained_right_hand_side() to build up the following modified system of
 * linear equations:
 * @f[
 *   (C^T A C + Id_c) x = C^T (b - A\,k)
 * @f]
 * with a given (unconstrained) system matrix $A$, right hand side $b$, and
 * linear constraints $C$ with inhomogeneities $k$.
 *
 * A detailed explanation of this approach is given in the
 * @ref constraints
 * module.
 *
 * @author Mauro Bardelloni, Matthias Maier, 2015
 *
 * @note Currently, this function may not work correctly for distributed data
 * structures.
 *
 * @relates LinearOperator
 * @ingroup constraints
 */
template <typename Range, typename Domain>
LinearOperator<Range, Domain>
constrained_linear_operator(const ConstraintMatrix &constraint_matrix,
                            const LinearOperator<Range, Domain> &linop)
{
  const auto C =
    distribute_constraints_linear_operator(constraint_matrix, linop);
  const auto Ct = transpose_operator(C);
  const auto Id_c =
    project_to_constrained_linear_operator(constraint_matrix, linop);
  return Ct * linop * C + Id_c;
}


/**
 * Given a ConstraintMatrix object @p constraint_matrix, a LinearOperator @p
 * linop and a right-hand side @p right_hand_side, this function creates a
 * PackagedOperation that stores the following computation:
 * @code
 *   Ct * (right_hand_side - linop * k)
 * @endcode
 * with
 * @code
 *   C = distribute_constraints_linear_operator(constraint_matrix, linop);
 *   Ct = transpose_operator(C);
 * @endcode
 *
 * This LinearOperator object is used together with
 * constrained_right_hand_side() to build up the following modified system of
 * linear equations:
 * @f[
 *   (C^T A C + Id_c) x = C^T (b - A\,k)
 * @f]
 * with a given (unconstrained) system matrix $A$, right hand side $b$, and
 * linear constraints $C$ with inhomogeneities $k$.
 *
 * A detailed explanation of this approach is given in the
 * @ref constraints
 * module.
 *
 * @author Mauro Bardelloni, Matthias Maier, 2015
 *
 * @note Currently, this function may not work correctly for distributed data
 * structures.
 *
 * @relates LinearOperator
 * @ingroup constraints
 */
template <typename Range, typename Domain>
PackagedOperation<Range>
constrained_right_hand_side(const ConstraintMatrix &constraint_matrix,
                            const LinearOperator<Range, Domain> &linop,
                            const Range &right_hand_side)
{
  PackagedOperation<Range> return_comp;

  return_comp.reinit_vector = linop.reinit_range_vector;

  return_comp.apply_add =
    [&constraint_matrix, &linop, &right_hand_side](Range &v)
  {
    const auto C =
      distribute_constraints_linear_operator(constraint_matrix, linop);
    const auto Ct = transpose_operator(C);

    static GrowingVectorMemory<Domain> vector_memory;
    Domain *k = vector_memory.alloc();
    linop.reinit_domain_vector(*k, /*bool fast=*/ false);
    constraint_matrix.distribute(*k);

    v += Ct * (right_hand_side - linop **k);

    vector_memory.free(k);
  };

  // lambda capture expressions are a C++14 feature...
  const auto apply_add = return_comp.apply_add;
  return_comp.apply = [apply_add](Range &v)
  {
    v = 0.;
    apply_add(v);
  };

  return return_comp;
}

//@}

DEAL_II_NAMESPACE_CLOSE

#endif
