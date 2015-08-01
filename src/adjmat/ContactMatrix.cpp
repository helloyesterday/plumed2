/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2013-2015 The plumed team
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
#include "AdjacencyMatrixBase.h"
#include "multicolvar/AtomValuePack.h"
#include "core/ActionRegister.h"
#include "tools/SwitchingFunction.h"
#include "tools/Matrix.h"

//+PLUMEDOC MATRIX CONTACT_MATRIX 
/*
Adjacency matrix in which two atoms are adjacent if they are within a certain cutoff.

\par Examples

*/
//+ENDPLUMEDOC


namespace PLMD {
namespace adjmat {

class ContactMatrix : public AdjacencyMatrixBase {
private:
/// switching function
  Matrix<SwitchingFunction> switchingFunction;
public:
/// Create manual
  static void registerKeywords( Keywords& keys );
/// Constructor
  explicit ContactMatrix(const ActionOptions&);
/// This actually calculates the value of the contact function
  void calculateWeight( multicolvar::AtomValuePack& myatoms ) const ;
/// This does nothing
  double compute( const unsigned& tindex, multicolvar::AtomValuePack& myatoms ) const ;
};

PLUMED_REGISTER_ACTION(ContactMatrix,"CONTACT_MATRIX")

void ContactMatrix::registerKeywords( Keywords& keys ){
  AdjacencyMatrixBase::registerKeywords( keys );
  keys.add("atoms","ATOMS","The list of atoms for which you would like to calculate the contact matrix.  The atoms involved must be specified "
                           "as a list of labels of \\ref mcolv or labels of a \\ref multicolvarfunction actions.  If you would just like to use "
                           "the atomic positions you can use a \\ref DENSITY command to specify a group of atoms.  Specifying your atomic positions using labels of "
                           "other \\ref mcolv or \\ref multicolvarfunction commands is useful, however, as you can then exploit a much wider "
                           "variety of functions of the contact matrix as described in \\ref contactmatrix");
  keys.add("numbered","SWITCH","This keyword is used if you want to employ an alternative to the continuous swiching function defined above. "
                               "The following provides information on the \\ref switchingfunction that are available. "
                               "When this keyword is present you no longer need the NN, MM, D_0 and R_0 keywords.");
}

ContactMatrix::ContactMatrix( const ActionOptions& ao ):
Action(ao),
AdjacencyMatrixBase(ao)
{
  // Read in the atomic positions
  std::vector<AtomNumber> atoms; parseAtomList("ATOMS",-1,true,atoms);
  // Read in the switching function
  if( getNumberOfNodeTypes()==1 ){
      switchingFunction.resize(1,1);
      std::string sw, errors; parse("SWITCH",sw);
      if(sw.length()==0) error("missing SWITCH keyword");
      switchingFunction(0,0).set(sw,errors);
      log.printf("  constructing adjacency matrix between atoms that are within %s\n", ( switchingFunction(0,0).description() ).c_str() );
  } else {
      unsigned nfunc=getNumberOfNodeTypes();
      switchingFunction.resize( nfunc,nfunc ); 
      for(unsigned i=0;i<nfunc;++i){
          // Retrieve the base number  
          unsigned ibase;
          if( nfunc<10 ){ 
             ibase=(i+1)*10; 
          } else if ( nfunc<100 ){
             ibase=(i+1)*100;
          } else {
             error("wow this is an error I never would have expected");
          }

          for(unsigned j=i;j<nfunc;++j){
             std::string sw, errors; parseNumbered("SWITCH",ibase+j+1,sw);
             if(sw.length()==0){
                std::string num; Tools::convert(ibase+j+1,num);
                error("could not find SWITCH" + num + " keyword. Need one SWITCH keyword for each distinct base-multicolvar-pair type");
             }
             switchingFunction(j,i).set(sw,errors);
             if( j!=i) switchingFunction(i,j).set(sw,errors);
             log.printf("  %d th and %d th multicolvar groups must be within %s\n",i+1,j+1,(switchingFunction(i,j).description()).c_str() );
          }
      }
  }

  // Find the largest sf cutoff
  double sfmax=switchingFunction(0,0).get_dmax();
  for(unsigned i=0;i<getNumberOfNodeTypes();++i){
      for(unsigned j=0;j<getNumberOfNodeTypes();++j){
          double tsf=switchingFunction(i,j).get_dmax();
          if( tsf>sfmax ) sfmax=tsf;
      }
  }
  // And set the link cell cutoff
  setLinkCellCutoff( sfmax );

  // Create the task list
  nblock = getNumberOfNodes(); resizeBookeepingArray( nblock , nblock );
  ablocks.resize(2); ablocks[0].resize( getNumberOfNodes() ); ablocks[1].resize( nblock );
  for(unsigned i=0;i<nblock;++i) ablocks[0][i]=ablocks[1][i]=i;
  for(unsigned i=1;i<nblock;++i){
     for(unsigned j=0;j<i;++j){
        bookeeping(i,j).first=getFullNumberOfTasks();
        addTaskToList( i*nblock + j );
        bookeeping(i,j).second=getFullNumberOfTasks();
     }
  }
  // And request the atoms involved in this colvar
  requestAtoms( atoms );
}

void ContactMatrix::calculateWeight( multicolvar::AtomValuePack& myatoms ) const {
  Vector distance = getSeparation( myatoms.getPosition(0), myatoms.getPosition(1) );
  double dfunc, sw = switchingFunction( getBaseColvarNumber( myatoms.getIndex(0) ), getBaseColvarNumber( myatoms.getIndex(1) ) ).calculate( distance.modulo(), dfunc );
  myatoms.setValue(0,sw);
}

double ContactMatrix::compute( const unsigned& tindex, multicolvar::AtomValuePack& myatoms ) const {
  if( dertime && !doNotCalculateDerivatives() ){
      Vector distance = getSeparation( myatoms.getPosition(0), myatoms.getPosition(1) );
      double dfunc, sw = switchingFunction( getBaseColvarNumber( myatoms.getIndex(0) ), getBaseColvarNumber( myatoms.getIndex(1) ) ).calculate( distance.modulo(), dfunc );
      addAtomDerivatives( 0, (-dfunc)*distance, myatoms );
      addAtomDerivatives( 1, (+dfunc)*distance, myatoms ); 
      myatoms.addBoxDerivatives( 1, (-dfunc)*Tensor(distance,distance) ); 
  }
  double val=myatoms.getValue(0); myatoms.setValue(0,1.0);
  return val;
}

}
}
