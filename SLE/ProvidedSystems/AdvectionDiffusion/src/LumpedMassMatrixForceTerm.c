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
** $Id: LumpedMassMatrixForceTerm.c 964 2007-10-11 08:03:06Z SteveQuenette $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StgDomain/StgDomain.h>
#include "StgFEM/Discretisation/Discretisation.h"
#include <StgFEM/SLE/LinearAlgebra/LinearAlgebra.h>
#include <StgFEM/SLE/SystemSetup/SystemSetup.h>

#include "types.h"
#include "LumpedMassMatrixForceTerm.h"

#include <assert.h>
#include <string.h>

/* Textual name of this class */
const Type LumpedMassMatrixForceTerm_Type = "LumpedMassMatrixForceTerm";

LumpedMassMatrixForceTerm* LumpedMassMatrixForceTerm_New( 
		Name                                                name,
		ForceVector*                                        forceVector,
		Swarm*                                              integrationSwarm )
{
	LumpedMassMatrixForceTerm* self = (LumpedMassMatrixForceTerm*) _LumpedMassMatrixForceTerm_DefaultNew( name );

	LumpedMassMatrixForceTerm_InitAll( 
			self,
			forceVector,
			integrationSwarm );

	return self;
}

/* Creation implementation / Virtual constructor */
LumpedMassMatrixForceTerm* _LumpedMassMatrixForceTerm_New( 
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
		ForceTerm_AssembleElementFunction*                   _assembleElement,		
		Name                                                name )
{
	LumpedMassMatrixForceTerm* self;
	
	/* Allocate memory */
	assert( sizeOfSelf >= sizeof(LumpedMassMatrixForceTerm) );
	self = (LumpedMassMatrixForceTerm*) _ForceTerm_New( 
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

void _LumpedMassMatrixForceTerm_Init( LumpedMassMatrixForceTerm* self ) {
}

void LumpedMassMatrixForceTerm_InitAll( 
		void*                                               forceTerm,
		ForceVector*                                        forceVector,
		Swarm*                                              integrationSwarm )
{
	LumpedMassMatrixForceTerm* self = (LumpedMassMatrixForceTerm*) forceTerm;

	ForceTerm_InitAll( self, forceVector, integrationSwarm, NULL );
	_LumpedMassMatrixForceTerm_Init( self );
}

void _LumpedMassMatrixForceTerm_Delete( void* forceTerm ) {
	LumpedMassMatrixForceTerm* self = (LumpedMassMatrixForceTerm*)forceTerm;

	_ForceTerm_Delete( self );
}

void _LumpedMassMatrixForceTerm_Print( void* forceTerm, Stream* stream ) {
	LumpedMassMatrixForceTerm* self = (LumpedMassMatrixForceTerm*)forceTerm;
	
	_ForceTerm_Print( self, stream );

	/* General info */
}

void* _LumpedMassMatrixForceTerm_DefaultNew( Name name ) {
	return (void*)_LumpedMassMatrixForceTerm_New( 
		sizeof(LumpedMassMatrixForceTerm), 
		LumpedMassMatrixForceTerm_Type,
		_LumpedMassMatrixForceTerm_Delete,
		_LumpedMassMatrixForceTerm_Print,
		NULL,
		_LumpedMassMatrixForceTerm_DefaultNew,
		_LumpedMassMatrixForceTerm_Construct,
		_LumpedMassMatrixForceTerm_Build,
		_LumpedMassMatrixForceTerm_Initialise,
		_LumpedMassMatrixForceTerm_Execute,
		_LumpedMassMatrixForceTerm_Destroy,
		_LumpedMassMatrixForceTerm_AssembleElement,
		name );
}

void _LumpedMassMatrixForceTerm_Construct( void* forceTerm, Stg_ComponentFactory* cf, void* data ) {
	LumpedMassMatrixForceTerm*            self             = (LumpedMassMatrixForceTerm*)forceTerm;

	/* Construct Parent */
	_ForceTerm_Construct( self, cf, data );

	_LumpedMassMatrixForceTerm_Init( self );
}

void _LumpedMassMatrixForceTerm_Build( void* forceTerm, void* data ) {
	LumpedMassMatrixForceTerm*             self             = (LumpedMassMatrixForceTerm*)forceTerm;

	_ForceTerm_Build( self, data );
}

void _LumpedMassMatrixForceTerm_Initialise( void* forceTerm, void* data ) {
	LumpedMassMatrixForceTerm*             self             = (LumpedMassMatrixForceTerm*)forceTerm;

	_ForceTerm_Initialise( self, data );
}

void _LumpedMassMatrixForceTerm_Execute( void* forceTerm, void* data ) {
	_ForceTerm_Execute( forceTerm, data );
}

void _LumpedMassMatrixForceTerm_Destroy( void* forceTerm, void* data ) {
	_ForceTerm_Destroy( forceTerm, data );
}

void _LumpedMassMatrixForceTerm_AssembleElement( void* forceTerm, ForceVector* forceVector ,Element_LocalIndex lElement_I, double* elForceVector ) {
	LumpedMassMatrixForceTerm* self              = Stg_CheckType( forceTerm, LumpedMassMatrixForceTerm );
	FeVariable*                feVariable        = forceVector->feVariable;
	FeMesh*				feMesh              = feVariable->feMesh;

#if 0
	if ( Stg_Class_IsInstance( mesh->layout->elementLayout, ParallelPipedHexaEL_Type ) ) {
		ForceTerm_SetAssembleElementFunction( self, _LumpedMassMatrixForceTerm_AssembleElement_Box  );
	}
	else {
#endif
		ForceTerm_SetAssembleElementFunction( self, _LumpedMassMatrixForceTerm_AssembleElement_General  );
#if 0
	}
#endif

	ForceTerm_AssembleElement( self, forceVector, lElement_I, elForceVector );
}

void _LumpedMassMatrixForceTerm_AssembleElement_General( void* forceTerm, ForceVector* forceVector, Element_LocalIndex lElement_I, double* elForceVector ) {
	LumpedMassMatrixForceTerm* self              = Stg_CheckType( forceTerm, LumpedMassMatrixForceTerm );
	FeVariable*                feVariable        = forceVector->feVariable;
	Dimension_Index            dim               = forceVector->dim;
	Swarm*                     swarm             = self->integrationSwarm;
	FeMesh*				feMesh              = feVariable->feMesh;
	ElementType*               elementType       = FeMesh_GetElementType( feMesh, lElement_I );
	Cell_Index                 cell_I            = CellLayout_MapElementIdToCellId( swarm->cellLayout, lElement_I );
	Particle_InCellIndex       cellParticleCount;
	Particle_InCellIndex       cParticle_I;
	IntegrationPoint*          particle;
	Node_Index                 nodeRow_I;
	Node_Index                 nodeColumn_I;
	double                     factor;
	double                     detJac;
	double                     shapeFunc[27];
	unsigned			elementNodeCount;

	elementNodeCount = FeMesh_GetElementNodeSize( feMesh, lElement_I );
	cellParticleCount = swarm->cellParticleCountTbl[ cell_I ];
	
	for( cParticle_I = 0 ; cParticle_I < cellParticleCount; cParticle_I++ ) {
		/* Find this particle in the element */
		particle = (IntegrationPoint*) Swarm_ParticleInCellAt( swarm, cell_I, cParticle_I );

		/* Evalutate Shape Functions and Jacobian Determinant */
		ElementType_EvaluateShapeFunctionsAt( elementType, particle->xi, shapeFunc );
		detJac = ElementType_JacobianDeterminant( elementType, feMesh, lElement_I, particle->xi, dim );
	
		/* Integrate \int_{\Omgea} N_i N_i d\Omega and lump onto vector in one step */
		factor = detJac * particle->weight;
		for ( nodeRow_I = 0 ; nodeRow_I < elementNodeCount ; nodeRow_I++ ) 
			for ( nodeColumn_I = 0 ; nodeColumn_I < elementNodeCount ; nodeColumn_I++ ) 
				elForceVector[ nodeRow_I ] += shapeFunc[ nodeRow_I ] * shapeFunc[ nodeColumn_I ] * factor ; 
	}
}

/** Shortcut for doing above calculation, optimised for a box shaped element */
void _LumpedMassMatrixForceTerm_AssembleElement_Box( void* forceTerm, ForceVector* forceVector, Element_LocalIndex lElement_I, double* elForceVector ) {
	FeVariable*                feVariable        = forceVector->feVariable;
	Dimension_Index            dim               = forceVector->dim;
	FeMesh*			feMesh              = feVariable->feMesh;
	Element_NodeIndex          elementNodeCount;
	ElementType*               elementType;
	Node_Index                 node_I;
	double                     detJac;
	Coord                      xi = {0.0,0.0,0.0};

	elementNodeCount = FeMesh_GetElementNodeSize( feMesh, lElement_I );
	elementType = FeMesh_GetElementType( feMesh, lElement_I );
	
	detJac = ElementType_JacobianDeterminant( elementType, feMesh, lElement_I, xi, dim );
	for ( node_I = 0 ; node_I < elementNodeCount ; node_I++ ) 
		elForceVector[ node_I ] = detJac;
}
