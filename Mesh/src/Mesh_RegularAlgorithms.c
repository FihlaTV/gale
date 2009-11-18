/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
** Copyright (C), 2003, Victorian Partnership for Advanced Computing (VPAC) Ltd, 110 Victoria Street, Melbourne, 3053, Australia.
**
** Authors:
**	Stevan M. Quenette, Senior Software Engineer, VPAC. (steve@vpac.org)
**	Patrick D. Sunter, Software Engineer, VPAC. (pds@vpac.org)
**	Luke J. Hodkinson, Computational Engineer, VPAC. (lhodkins@vpac.org)
**	Siew-Ching Tan, Software Engineer, VPAC. (siew@vpac.org)
**	Alan H. Lo, Computational Engineer, VPAC. (alan@vpac.org)
**	Raquibul Hassan, Computational Engineer, VPAC. (raq@vpac.org)
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
** $Id: Mesh_RegularAlgorithms.c 3584 2006-05-16 11:11:07Z PatrickSunter $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>

#include <StGermain/StGermain.h>
#include <StgDomain/Geometry/Geometry.h>
#include "Mesh.h"


/* Textual name of this class */
const Type Mesh_RegularAlgorithms_Type = "Mesh_RegularAlgorithms";


/*----------------------------------------------------------------------------------------------------------------------------------
** Constructors
*/

Mesh_RegularAlgorithms* Mesh_RegularAlgorithms_New( Name name, AbstractContext* context ) {
	Mesh_RegularAlgorithms* self = _Mesh_RegularAlgorithms_New( sizeof(Mesh_RegularAlgorithms), 
					    Mesh_RegularAlgorithms_Type, 
					    _Mesh_RegularAlgorithms_Delete, 
					    _Mesh_RegularAlgorithms_Print, 
					    NULL, 
					    (void* (*)(Name))_Mesh_RegularAlgorithms_New, 
					    _Mesh_RegularAlgorithms_AssignFromXML, 
					    _Mesh_RegularAlgorithms_Build, 
					    _Mesh_RegularAlgorithms_Initialise, 
					    _Mesh_RegularAlgorithms_Execute, 
					    _Mesh_RegularAlgorithms_Destroy, 
					    name, 
					    NON_GLOBAL, 
					    Mesh_RegularAlgorithms_SetMesh, 
					    Mesh_RegularAlgorithms_Update, 
					    _Mesh_Algorithms_NearestVertex, 
					    _Mesh_Algorithms_Search, 
					    Mesh_RegularAlgorithms_SearchElements, 
					    _Mesh_Algorithms_GetMinimumSeparation, 
					    _Mesh_Algorithms_GetLocalCoordRange, 
					    _Mesh_Algorithms_GetDomainCoordRange, 
					    _Mesh_Algorithms_GetGlobalCoordRange );

	/* Mesh_RegularAlgorithms info */
	_Mesh_Algorithms_Init( self, context );
	_Mesh_RegularAlgorithms_Init( self );

   return self;
}

Mesh_RegularAlgorithms* _Mesh_RegularAlgorithms_New( MESH_REGULARALGORITHMS_DEFARGS ) {
	Mesh_RegularAlgorithms* self;
	
	/* Allocate memory */
	assert( sizeOfSelf >= sizeof(Mesh_RegularAlgorithms) );
	self = (Mesh_RegularAlgorithms*)_Mesh_Algorithms_New( MESH_ALGORITHMS_PASSARGS );

	return self;
}

void _Mesh_RegularAlgorithms_Init( void* algorithms ) {
	Mesh_RegularAlgorithms*	self = (Mesh_RegularAlgorithms*)algorithms;

	assert( self && Stg_CheckType( self, Mesh_RegularAlgorithms ) );

	self->sep = NULL;
}


/*----------------------------------------------------------------------------------------------------------------------------------
** Virtual functions
*/

void _Mesh_RegularAlgorithms_Delete( void* algorithms ) {
	Mesh_RegularAlgorithms*	self = (Mesh_RegularAlgorithms*)algorithms;

	/* Delete the parent. */
	_Mesh_Algorithms_Delete( self );
}

void _Mesh_RegularAlgorithms_Print( void* algorithms, Stream* stream ) {
	Mesh_RegularAlgorithms*	self = (Mesh_RegularAlgorithms*)algorithms;
	
	/* Set the Journal for printing informations */
	Stream* algorithmsStream;
	algorithmsStream = Journal_Register( InfoStream_Type, "Mesh_RegularAlgorithmsStream" );

	/* Print parent */
	Journal_Printf( stream, "Mesh_RegularAlgorithms (ptr): (%p)\n", self );
	_Mesh_Algorithms_Print( self, stream );
}

void _Mesh_RegularAlgorithms_AssignFromXML( void* algorithms, Stg_ComponentFactory* cf, void* data ) {
	_Mesh_Algorithms_AssignFromXML( algorithms, cf, data );
   _Mesh_RegularAlgorithms_Init( algorithms );
}

void _Mesh_RegularAlgorithms_Build( void* algorithms, void* data ) {
    _Mesh_Algorithms_Build( algorithms, data );
}

void _Mesh_RegularAlgorithms_Initialise( void* algorithms, void* data ) {
    _Mesh_Algorithms_Initialise( algorithms, data );
}

void _Mesh_RegularAlgorithms_Execute( void* algorithms, void* data ) {
    _Mesh_Algorithms_Execute( algorithms, data );
}

void _Mesh_RegularAlgorithms_Destroy( void* algorithms, void* data ) {
	Mesh_RegularAlgorithms*	self = (Mesh_RegularAlgorithms*)algorithms;

	Mesh_RegularAlgorithms_Destruct( self );

   _Mesh_Algorithms_Destroy( algorithms, data );
}

void Mesh_RegularAlgorithms_SetMesh( void* algorithms, void* mesh ) {
	Mesh_RegularAlgorithms*	self = (Mesh_RegularAlgorithms*)algorithms;

	assert( self && Stg_CheckType( self, Mesh_RegularAlgorithms ) );

	Mesh_RegularAlgorithms_Destruct( self );
	_Mesh_Algorithms_SetMesh( self, mesh );
}

void Mesh_RegularAlgorithms_Update( void* algorithms ) {
	Mesh_RegularAlgorithms*	self = (Mesh_RegularAlgorithms*)algorithms;
	unsigned		nDims;
	Grid*			eGrid;
	int			ii;

	assert( self && Stg_CheckType( self, Mesh_RegularAlgorithms ) );
	assert( self->mesh );

	Mesh_RegularAlgorithms_Destruct( self );
	_Mesh_Algorithms_Update( self );

	nDims = Mesh_GetDimSize( self->mesh );
	self->minCrd = AllocArray( double, nDims );
	self->maxCrd = AllocArray( double, nDims );
	Mesh_GetGlobalCoordRange( self->mesh, self->minCrd, self->maxCrd );

	self->sep = AllocArray( double, nDims );
	eGrid = *Mesh_GetExtension( self->mesh, Grid**, "elementGrid" );
	for( ii = 0; ii < nDims; ii++ )
		self->sep[ii] = (self->maxCrd[ii] - self->minCrd[ii]) / eGrid->sizes[ii];
}

Bool _Mesh_RegularAlgorithms_Search( void* algorithms, void* _mesh, double* point, 
				     MeshTopology_Dim* dim, unsigned* ind )
{
	Mesh_RegularAlgorithms*	self = (Mesh_RegularAlgorithms*)algorithms;
	Mesh*			mesh = (Mesh*)_mesh;

	assert( self );
	assert( mesh );
	assert( dim );
	assert( ind );

	/* TODO */
	abort();

	return False;
}

Bool Mesh_RegularAlgorithms_SearchElements( void* algorithms, double* point, unsigned* elInd ) {
	Mesh_RegularAlgorithms*	self = (Mesh_RegularAlgorithms*)algorithms;
	Mesh*			mesh;
	unsigned		nDims;
	unsigned		inds[3];
	Grid			*elGrid;
	double			out, frac, integer;
	unsigned		d_i;

	assert( self );
	assert( Mesh_GetDimSize( self->mesh ) <= 3 );
	assert( elInd );

	mesh = self->mesh;
	nDims = Mesh_GetDimSize( mesh );
	elGrid = *(Grid**)ExtensionManager_Get( mesh->info, mesh, 
						ExtensionManager_GetHandle( mesh->info, "elementGrid" ) );
	for( d_i = 0; d_i < nDims; d_i++ ) {
		if( Num_Approx( point[d_i] - self->maxCrd[d_i], 0.0 ) )
			inds[d_i] = elGrid->sizes[d_i] - 1;
		else if( point[d_i] < self->minCrd[d_i] || point[d_i] > self->maxCrd[d_i] )
			return False;
		else {
			out = (point[d_i] - self->minCrd[d_i]) / self->sep[d_i];
			frac = modf( out, &integer );
			inds[d_i] = (unsigned)integer;
			if( inds[d_i] > 0 && Num_Approx( frac, 0.0 ) )
				inds[d_i]--;
		}
	}

	*elInd = Grid_Project( elGrid, inds );
	return Mesh_GlobalToDomain( mesh, nDims, *elInd, elInd );
}

double _Mesh_RegularAlgorithms_GetMinimumSeparation( void* algorithms, void* _mesh, double* perDim ) {
	Mesh*			mesh = (Mesh*)_mesh;

	/* TODO */
	abort();

	return 0.0;
}

void _Mesh_RegularAlgorithms_GetLocalCoordRange( void* algorithms, void* _mesh, double* min, double* max ) {
	Mesh_RegularAlgorithms*	self = (Mesh_RegularAlgorithms*)algorithms;
	Mesh*			mesh = (Mesh*)_mesh;

	assert( self );
	assert( mesh );
	assert( min );
	assert( max );

	/* TODO */
	abort();
}

void _Mesh_RegularAlgorithms_GetDomainCoordRange( void* algorithms, void* _mesh, double* min, double* max ) {
	Mesh_RegularAlgorithms*	self = (Mesh_RegularAlgorithms*)algorithms;
	Mesh*			mesh = (Mesh*)_mesh;

	assert( self );
	assert( mesh );
	assert( min );
	assert( max );

	/* TODO */
	abort();
}

void _Mesh_RegularAlgorithms_GetGlobalCoordRange( void* algorithms, void* _mesh, double* min, double* max ) {
	Mesh_RegularAlgorithms*	self = (Mesh_RegularAlgorithms*)algorithms;
	Mesh*			mesh = (Mesh*)_mesh;

	assert( self );
	assert( mesh );
	assert( min );
	assert( max );

	/* TODO */
	abort();
}


/*--------------------------------------------------------------------------------------------------------------------------
** Public Functions
*/


/*----------------------------------------------------------------------------------------------------------------------------------
** Private Functions
*/

void Mesh_RegularAlgorithms_Destruct( Mesh_RegularAlgorithms* self ) {
	assert( self && Stg_CheckType( self, Mesh_RegularAlgorithms ) );

	KillArray( self->sep );
	KillArray( self->minCrd );
	KillArray( self->maxCrd );
}
