/*******************************************************************************
* Promesh                                                                      *
* Copyright (C) 2022, IllinoisRocstar LLC. All rights reserved.                *
*                                                                              *
* Promesh is the property of IllinoisRocstar LLC.                              *
*                                                                              *
* IllinoisRocstar LLC                                                          *
* Champaign, IL                                                                *
* www.illinoisrocstar.com                                                      *
* promesh@illinoisrocstar.com                                                  *
*******************************************************************************/
/*******************************************************************************
* This file is part of Promesh                                                 *
*                                                                              *
* This version of Promesh is free software: you can redistribute it and/or     *
* modify it under the terms of the GNU Lesser General Public License as        *
* published by the Free Software Foundation, either version 3 of the License,  *
* or (at your option) any later version.                                       *
*                                                                              *
* Promesh is distributed in the hope that it will be useful, but WITHOUT ANY   *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS    *
* FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more *
* details.                                                                     *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this program. If not, see <https://www.gnu.org/licenses/>.        *
*                                                                              *
*******************************************************************************/
#ifndef NEMOSYS_ORTHOPOLY3D_H_
#define NEMOSYS_ORTHOPOLY3D_H_

#include "nemosys_export.h"

#include <memory>
#include <vector>

#include "PatchRecovery/orthoPoly1D.H"

#include <Eigen/Core>

/* This class takes 3 dimensional points and n dimensional data
   to build a multidimensional orthogonal polynomial approximant
   to data in a cubic domain defined by the minmax x,y,z coordinates
   of the points. Orthogonality is accomplished by tensor-product construction
   using 1D orthogonal polynomials defined by x,y and z coordinates. As such,
   the points must be representable by a cartesian product. For example, the
   4 points (x1,y1),(x1,y2),(x2,y1),(x2,y2) can equivalently be written as
   (x1,x2) <X> (y1,y2) where <X> is the cartesian product */

/* Based on the requirements above, this class is suitable for use with
   rectilinear, structured quad/hex grids */ 


// TODO: Generalize to higher order elements with more than one internal node
//     per patch where averages need to be taken for duplicate recovered values


// defines dynamically sized matrix type with row-major storage
// this is used to optimize computations involving access to entire rows
using MatrixXdRM =
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

// remove given row from matrix
void removeRow(Eigen::MatrixXd &matrix, unsigned int rowToRemove);

// remove rows from given matrix
void removeRow(const MatrixXdRM &matrix,
               MatrixXdRM &matrix_red,
               const std::vector<unsigned int> &toRemove);
void removeRow(const Eigen::MatrixXd &matrix,
               MatrixXdRM &matrix_red,
               const std::vector<unsigned int> &toRemove);
// remove given column from matrix
void removeColumn(Eigen::MatrixXd &matrix, unsigned int colToRemove);


class NEMOSYS_EXPORT orthoPoly3D
{
  // constructors and destructors
  public:
    // default ctor
    orthoPoly3D();

    // complete ctor 
    orthoPoly3D(int _order,
                const Eigen::VectorXd &_sigma,
                const std::vector<double> &_x,
                const std::vector<double> &_y,
                const std::vector<double> &_z);

    // partial ctor, must manually call computeA
    orthoPoly3D(int _order, const std::vector<std::vector<double>> &coords);

    // move constructor
    orthoPoly3D(orthoPoly3D &&op);

    // move assignment  
    orthoPoly3D &operator=(orthoPoly3D &&op);

    // disable copy and copy-assignment constructors
    orthoPoly3D(const orthoPoly3D &) = delete;
    orthoPoly3D &operator=(const orthoPoly3D &) = delete;

    // factories
    static orthoPoly3D *
    Create(int _order, const std::vector<std::vector<double>> &coords);
    static std::unique_ptr<orthoPoly3D>
    CreateUnique(int _order, const std::vector<std::vector<double>> &coords);

    // dtor
    ~orthoPoly3D() = default;

  // methods
  public:
#ifdef NEMOSYS_DEBUG
    // compute multivariate polynomial approximation of prescribed order to sigma
    void run1(const Eigen::VectorXd &sigma);
#endif
    // compute coefficients for polynomial expansion of sampled function
    void computeA(const Eigen::VectorXd &sigma);
    // evaluate the fitting polynomial at coord
    double operator()(const std::vector<double> &coord);
    double eval(const std::vector<double> &coord);

    // return the status of the approximant object
    // 1 indicates approximant is available, 0 otherwise 
    bool status() const { return finished; }

    // reset coefficients to default state
    void resetA();
    // reset the object to its default state
    void Reset();

    Eigen::VectorXd getCoeffs() const { return a; }

  private:
    // total order of basis
    int order;
    // 1D orthogonal basis generated by x coords
    std::unique_ptr<orthoPoly1D> opx;
    // 1D orthogonal basis generated by y coords
    std::unique_ptr<orthoPoly1D> opy;
    // 1D orthogonal basis generated by z coords 
    std::unique_ptr<orthoPoly1D> opz;
    // coefficients of polynomial approximant
    Eigen::VectorXd a;
    // rows/cols to remove from basis matrices to limit total order
    std::vector<unsigned int> toRemove;
    // status bool
    bool finished;

  private:
    // checks order compatibility and determines toRemove array
    void initCheck();
};

#endif  // NEMOSYS_ORTHOPOLY3D_H_
