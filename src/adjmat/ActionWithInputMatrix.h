/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2013,2014 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed-code.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#ifndef __PLUMED_adjmat_ActionWithInputMatrix_h
#define __PLUMED_adjmat_ActionWithInputMatrix_h

#include "core/ActionWithValue.h"
#include "core/ActionAtomistic.h"
#include "multicolvar/MultiColvarBase.h"
#include "multicolvar/MultiColvarFunction.h"

namespace PLMD {
namespace adjmat {

class AdjacencyMatrixVessel;

class ActionWithInputMatrix : public multicolvar::MultiColvarFunction {
private:
/// The vessel that holds the adjacency matrix
  AdjacencyMatrixVessel* mymatrix;
/// The forces we are going to apply to things
  std::vector<double> forcesToApply;
protected:
/// Get number of base multicolvar types
  unsigned getNumberOfNodeTypes() const ;
/// Get number of atoms in each base multicolvar
  unsigned getNumberOfAtomsInGroup( const unsigned& igrp ) const ;
/// Get a pointer to the igrp th base multicolvar
  multicolvar::MultiColvarBase* getBaseMultiColvar( const unsigned& igrp ) const ;
public:
  static void registerKeywords( Keywords& keys );
  explicit ActionWithInputMatrix(const ActionOptions&);
/// Retrieve the vessel that holds the adjacency matrix
  AdjacencyMatrixVessel* getAdjacencyVessel() const ;
/// Check if one of the stored values is active
  bool isCurrentlyActive( const unsigned& ind ) const ;
/// Get the vector for task ind
  virtual void getVectorForTask( const unsigned& ind, const bool& normed, std::vector<double>& orient0 ) const ;
/// Get vector derivatives
  virtual void getVectorDerivatives( const unsigned& ind, const bool& normed, MultiValue& myder0 ) const ;
  virtual unsigned getNumberOfDerivatives();
///  Get the number of rows/cols in the adjacency matrix vessel
  virtual unsigned getNumberOfNodes() const ;
  bool isPeriodic(){ return false; }
  virtual unsigned getNumberOfQuantities() const ;
///
  virtual AtomNumber getAbsoluteIndexOfCentralAtom(const unsigned& i) const ;
/// No loop over tasks for ActionWithInputMatrix
  double compute( const unsigned& tindex, multicolvar::AtomValuePack& myatoms ) const { plumed_error(); }
};

}
}
#endif