/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
** Copyright (C), 2003-2006, Victorian Partnership for Advanced Computing (VPAC) Ltd, 110 Victoria Street,
**	Melbourne, 3053, Australia.
**
** Primary Contributing Organisations:
**	Victorian Partnership for Advanced Computing Ltd, Computational Software Development - http://csd.vpac.org
**	Australian Computational Earth Systems Simulator - http://www.access.edu.au
**	Monash Cluster Computing - http://www.mcc.monash.edu.au
**	Computational Infrastructure for Geodynamics - http://www.geodynamics.org
**
** Contributors:
**	Patrick D. Sunter, Software Engineer, VPAC. (pds@vpac.org)
**	Robert Turnbull, Research Assistant, Monash University. (robert.turnbull@sci.monash.edu.au)
**	Stevan M. Quenette, Senior Software Engineer, VPAC. (steve@vpac.org)
**	David May, PhD Student, Monash University (david.may@sci.monash.edu.au)
**	Louis Moresi, Associate Professor, Monash University. (louis.moresi@sci.monash.edu.au)
**	Luke J. Hodkinson, Computational Engineer, VPAC. (lhodkins@vpac.org)
**	Alan H. Lo, Computational Engineer, VPAC. (alan@vpac.org)
**	Raquibul Hassan, Computational Engineer, VPAC. (raq@vpac.org)
**	Julian Giordani, Research Assistant, Monash University. (julian.giordani@sci.monash.edu.au)
**	Vincent Lemiale, Postdoctoral Fellow, Monash University. (vincent.lemiale@sci.monash.edu.au)
**
**  This library is free software; you can redistribute it and/or
**  modify it under the terms of the GNU Lesser General Public
**  License as published by the Free Software Foundation; either
**  version 2.1 of the License, or (at your option) any later version.
**
**  This library is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**  Lesser General Public License for more details.
**
**  You should have received a copy of the GNU Lesser General Public
**  License along with this library; if not, write to the Free Software
**  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
**
** $Id: LaplacianStiffnessMatrixTerm.c 964 2007-10-11 08:03:06Z SteveQuenette $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StgDomain/StgDomain.h>
#include <StgFEM/Discretisation/Discretisation.h>
#include <StgFEM/SLE/SLE.h>

#include "types.h"
#include "LaplacianStiffnessMatrixTerm.h"

#include <assert.h>
#include <string.h>

/* Textual name of this class */
const Type LaplacianStiffnessMatrixTerm_Type = "LaplacianStiffnessMatrixTerm";

LaplacianStiffnessMatrixTerm* LaplacianStiffnessMatrixTerm_New( 
		Name                                                name,
		StiffnessMatrix*                                    stiffnessMatrix,
		Swarm*                                              integrationSwarm )
{
	LaplacianStiffnessMatrixTerm* self = (LaplacianStiffnessMatrixTerm*) _LaplacianStiffnessMatrixTerm_DefaultNew( name );

	self->isConstructed = False;	
	_StiffnessMatrixTerm_Init( self, stiffnessMatrix, integrationSwarm, NULL );
	_LaplacianStiffnessMatrixTerm_Init( self );

	return self;
}

/* Creation implementation / Virtual constructor */
LaplacianStiffnessMatrixTerm* _LaplacianStiffnessMatrixTerm_New( 
		SizeT                                               sizeOfSelf,  
		Type                                                type,
		Stg_Class_DeleteFunction*                           _delete,
		Stg_Class_PrintFunction*                            _print,
		Stg_Class_CopyFunction*                             _copy, 
		Stg_Component_DefaultConstructorFunction*           _defaultConstructor,
		Stg_Component_ConstructFunction*                    _construct,
		Stg_Component_BuildFunction*                        _build,
		Stg_Component_InitialiseFunction*                   _initialise,
		Stg_Component_ExecuteFunction*                      _execute,
		Stg_Component_DestroyFunction*                      _destroy,
		StiffnessMatrixTerm_AssembleElementFunction*        _assembleElement,
		Name                                                name )
{
	LaplacianStiffnessMatrixTerm* self;
	
	/* Allocate memory */
	assert( sizeOfSelf >= sizeof(LaplacianStiffnessMatrixTerm) );
	self = (LaplacianStiffnessMatrixTerm*) _StiffnessMatrixTerm_New( 
		sizeOfSelf, 
		type, 
		_delete, 
		_print, 
		_copy,
		_defaultConstructor,
		_construct,
		_build, 
		_initialise,
		_execute,
		_destroy,
		_assembleElement,
		name );
	
	/* Virtual info */
	
	return self;
}

void _LaplacianStiffnessMatrixTerm_Init( LaplacianStiffnessMatrixTerm* self ) {
}

void _LaplacianStiffnessMatrixTerm_Delete( void* matrixTerm ) {
	LaplacianStiffnessMatrixTerm* self = (LaplacianStiffnessMatrixTerm*)matrixTerm;

	_StiffnessMatrixTerm_Delete( self );
}

void _LaplacianStiffnessMatrixTerm_Print( void* matrixTerm, Stream* stream ) {
	LaplacianStiffnessMatrixTerm* self = (LaplacianStiffnessMatrixTerm*)matrixTerm;
	
	_StiffnessMatrixTerm_Print( self, stream );

	/* General info */
}

void* _LaplacianStiffnessMatrixTerm_DefaultNew( Name name ) {
	return (void*)_LaplacianStiffnessMatrixTerm_New( 
		sizeof(LaplacianStiffnessMatrixTerm), 
		LaplacianStiffnessMatrixTerm_Type,
		_LaplacianStiffnessMatrixTerm_Delete,
		_LaplacianStiffnessMatrixTerm_Print,
		NULL,
		_LaplacianStiffnessMatrixTerm_DefaultNew,
		_LaplacianStiffnessMatrixTerm_Construct,
		_LaplacianStiffnessMatrixTerm_Build,
		_LaplacianStiffnessMatrixTerm_Initialise,
		_LaplacianStiffnessMatrixTerm_Execute,
		_LaplacianStiffnessMatrixTerm_Destroy,
		_LaplacianStiffnessMatrixTerm_AssembleElement,
		name );
}

void _LaplacianStiffnessMatrixTerm_Construct( void* matrixTerm, Stg_ComponentFactory* cf, void* data ) {
	LaplacianStiffnessMatrixTerm*            self             = (LaplacianStiffnessMatrixTerm*)matrixTerm;

	/* Construct Parent */
	_StiffnessMatrixTerm_Construct( self, cf, data );

	_LaplacianStiffnessMatrixTerm_Init( self );
}

void _LaplacianStiffnessMatrixTerm_Build( void* matrixTerm, void* data ) {
	LaplacianStiffnessMatrixTerm*             self             = (LaplacianStiffnessMatrixTerm*)matrixTerm;

	_StiffnessMatrixTerm_Build( self, data );
}

void _LaplacianStiffnessMatrixTerm_Initialise( void* matrixTerm, void* data ) {
	LaplacianStiffnessMatrixTerm*             self             = (LaplacianStiffnessMatrixTerm*)matrixTerm;

	_StiffnessMatrixTerm_Initialise( self, data );
}

void _LaplacianStiffnessMatrixTerm_Execute( void* matrixTerm, void* data ) {
	_StiffnessMatrixTerm_Execute( matrixTerm, data );
}

void _LaplacianStiffnessMatrixTerm_Destroy( void* matrixTerm, void* data ) {
	_StiffnessMatrixTerm_Destroy( matrixTerm, data );
}

void _LaplacianStiffnessMatrixTerm_AssembleElement( 
		void*                                              matrixTerm,
		StiffnessMatrix*                                   stiffnessMatrix, 
		Element_LocalIndex                                 lElement_I, 
		SystemLinearEquations*                             sle,
		FiniteElementContext*                              context,
		double**                                           elStiffMat ) 
{
	LaplacianStiffnessMatrixTerm*       self         = Stg_CheckType( matrixTerm, LaplacianStiffnessMatrixTerm );
	Swarm*                              swarm        = self->integrationSwarm;
	FeVariable*                         variable1    = stiffnessMatrix->rowVariable;
	Dimension_Index                     dim          = stiffnessMatrix->dim;
	IntegrationPoint*                   currIntegrationPoint;
	double*                             xi;
	double                              weight;
	Particle_InCellIndex                cParticle_I, cellParticleCount;
	Index                               nodesPerEl;
	Index                               i,j;
	Dimension_Index                     dim_I;
	double**                            GNx;
	double                              detJac;
	
	Cell_Index                          cell_I;
	ElementType*                        elementType;
	
	/* Set the element type */
	elementType = FeMesh_GetElementType( variable1->feMesh, lElement_I );
	nodesPerEl = elementType->nodeCount;
	
	if( nodesPerEl > self->max_nElNodes ) {
		/* reallocate */
		self->GNx = ReallocArray2D( self->GNx, double, dim, nodesPerEl );
		self->max_nElNodes = nodesPerEl;
	}
	GNx = self->GNx;
	
	cell_I = CellLayout_MapElementIdToCellId( swarm->cellLayout, lElement_I );
	cellParticleCount = swarm->cellParticleCountTbl[ cell_I ];

	/* Slap the laplacian together */
	for( cParticle_I = 0 ; cParticle_I < cellParticleCount ; cParticle_I++ ) {
		
		currIntegrationPoint = (IntegrationPoint*)Swarm_ParticleInCellAt( swarm, cell_I, cParticle_I );

		xi = currIntegrationPoint->xi;
		weight = currIntegrationPoint->weight;
		
		ElementType_ShapeFunctionsGlobalDerivs( 
			elementType,
			variable1->feMesh, lElement_I,
			xi, dim, &detJac, GNx );
			
		for( i=0; i<nodesPerEl; i++ ) {
			for( j=0; j<nodesPerEl; j++ ) {
				for ( dim_I = 0; dim_I < dim ; dim_I++ ) { 
					elStiffMat[i][j] += detJac * weight * GNx[dim_I][i] * GNx[dim_I][j];
				}
			}
		}
	}
}
