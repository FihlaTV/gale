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
** $Id: MeshParticleLayout.c 3851 2006-10-12 08:57:22Z SteveQuenette $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <mpi.h>
#include <StGermain/StGermain.h>

#include <StgDomain/Geometry/Geometry.h>
#include <StgDomain/Shape/Shape.h>
#include <StgDomain/Mesh/Mesh.h>
#include <StgDomain/Utils/Utils.h>

#include "types.h"
#include "shortcuts.h"
#include "ParticleLayout.h"
#include "PerCellParticleLayout.h"
#include "MeshParticleLayout.h"

#include "SwarmClass.h"
#include "Random.h"
#include "StandardParticle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

const Type MeshParticleLayout_Type = "MeshParticleLayout";


MeshParticleLayout* MeshParticleLayout_New( 
   Name                 name, 
   AbstractContext* context,
   CoordSystem      coordSystem,
   Bool             weightsInitialisedAtStartup,
   Mesh*            mesh,
   Particle_InCellIndex cellParticleCount, 
   unsigned int         seed ) 
{
	MeshParticleLayout* self = (MeshParticleLayout*) _MeshParticleLayout_DefaultNew( name );

   _ParticleLayout_Init( self, context, coordSystem, weightsInitialisedAtStartup );
   _PerCellParticleLayout_Init( self );
	_MeshParticleLayout_Init( self, mesh, cellParticleCount, seed );

	return self;
}

MeshParticleLayout* _MeshParticleLayout_New( 
      SizeT                                        _sizeOfSelf,
      Type                                         type,
      Stg_Class_DeleteFunction*                    _delete,
      Stg_Class_PrintFunction*                     _print,
      Stg_Class_CopyFunction*                      _copy,
      Stg_Component_DefaultConstructorFunction*    _defaultConstructor,
      Stg_Component_ConstructFunction*             _construct,
      Stg_Component_BuildFunction*                 _build,
      Stg_Component_InitialiseFunction*            _initialise,
      Stg_Component_ExecuteFunction*               _execute,
      Stg_Component_DestroyFunction*               _destroy,
      Name                                         name,
      AllocationType                               nameAllocationType,
      ParticleLayout_SetInitialCountsFunction*     _setInitialCounts,
      ParticleLayout_InitialiseParticlesFunction*  _initialiseParticles,
      CoordSystem                                  coordSystem,
      Bool                                         weightsInitialisedAtStartup,
      PerCellParticleLayout_InitialCountFunction*  _initialCount,
      PerCellParticleLayout_InitialiseParticlesOfCellFunction* _initialiseParticlesOfCell,
      Mesh*                                        mesh,
		Particle_InCellIndex                         cellParticleCount,
		unsigned int                                 seed )
{
	MeshParticleLayout* self;

   coordSystem = GlobalCoordSystem;   
   weightsInitialisedAtStartup = False;
	/* Allocate memory */
	self = (MeshParticleLayout*)_PerCellParticleLayout_New( 
		_sizeOfSelf, 
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
      name, nameAllocationType,
		_setInitialCounts, _initialiseParticles,
      coordSystem, weightsInitialisedAtStartup,
		_initialCount, _initialiseParticlesOfCell );

   self->mesh = mesh;
   self->cellParticleCount = cellParticleCount;
   self->seed = seed;

	return self;
}

void _MeshParticleLayout_Init( void* meshParticleLayout, Mesh* mesh, Particle_InCellIndex cellParticleCount, unsigned int seed ) {
	MeshParticleLayout* self = (MeshParticleLayout*)meshParticleLayout;

	self->mesh = mesh;
	self->isConstructed     = True;
	self->cellParticleCount = cellParticleCount;
	self->seed              = seed;
	
	Swarm_Random_Seed( self->seed );
}


void _MeshParticleLayout_Delete( void* meshParticleLayout ) {
	MeshParticleLayout* self = (MeshParticleLayout*)meshParticleLayout;
	
	/* Stg_Class_Delete parent class */
	_PerCellParticleLayout_Delete( self );
}

void _MeshParticleLayout_Print( void* meshParticleLayout, Stream* stream ) {
	MeshParticleLayout* self = (MeshParticleLayout*)meshParticleLayout;
	
	/* Set the Journal for printing informations */
	Stream* meshParticleLayoutStream = stream;
	
	/* General info */
	Journal_Printf( meshParticleLayoutStream, "MeshParticleLayout (ptr): %p:\n", self );
	
	/* Parent class info */
	_PerCellParticleLayout_Print( self, stream );
	
	/* MeshParticleLayout */
	Journal_Printf( meshParticleLayoutStream, "\tcellParticleCount: %u\n", self->cellParticleCount );
	Journal_Printf( meshParticleLayoutStream, "\tseed: %u\n", self->seed );
}


void* _MeshParticleLayout_Copy( void* meshParticleLayout, void* dest, Bool deep, Name nameExt, PtrMap* ptrMap ) {
	MeshParticleLayout*		self = (MeshParticleLayout*)meshParticleLayout;
	MeshParticleLayout*		newMeshParticleLayout;
	
	newMeshParticleLayout = (MeshParticleLayout*)_PerCellParticleLayout_Copy( self, dest, deep, nameExt, ptrMap );
	
	newMeshParticleLayout->cellParticleCount = self->cellParticleCount;
	newMeshParticleLayout->seed = self->seed;
	
	return (void*)newMeshParticleLayout;
}


void* _MeshParticleLayout_DefaultNew( Name name ) {
	return (void*)_MeshParticleLayout_New( 
			sizeof(MeshParticleLayout),
			MeshParticleLayout_Type,
			_MeshParticleLayout_Delete,
			_MeshParticleLayout_Print, 
			_MeshParticleLayout_Copy,
			_MeshParticleLayout_DefaultNew,
			_MeshParticleLayout_AssignFromXML,
			_MeshParticleLayout_Build,
			_MeshParticleLayout_Initialise,
			_MeshParticleLayout_Execute,
			_MeshParticleLayout_Destroy,
         name, NON_GLOBAL,
			_PerCellParticleLayout_SetInitialCounts, _PerCellParticleLayout_InitialiseParticles,
         LocalCoordSystem, True,
         _MeshParticleLayout_InitialCount, _MeshParticleLayout_InitialiseParticlesOfCell,
         NULL, /* mesh */
			0, /* cellParticleCount */
			0  /* seed */ );
}

void _MeshParticleLayout_AssignFromXML( void* meshParticleLayout, Stg_ComponentFactory* cf, void* data ) {
	MeshParticleLayout*       self = (MeshParticleLayout*)meshParticleLayout;
	Particle_InCellIndex        cellParticleCount;
	unsigned int                seed;
   Mesh* mesh = NULL;

   _PerCellParticleLayout_AssignFromXML( self, cf, data );

	cellParticleCount = Stg_ComponentFactory_GetUnsignedInt( cf, self->name, "cellParticleCount", 0 );
	seed = Stg_ComponentFactory_GetUnsignedInt( cf, self->name, "seed", 13 );
	mesh = Stg_ComponentFactory_ConstructByKey( cf, self->name, "mesh", Mesh, True, data );

	_MeshParticleLayout_Init( self, mesh, cellParticleCount, seed );

}
	
void _MeshParticleLayout_Build( void* meshParticleLayout, void* data ) {
	MeshParticleLayout*       self = (MeshParticleLayout*)meshParticleLayout;

	assert( self );

	Stg_Component_Build( self->mesh, NULL, False );
}
	
void _MeshParticleLayout_Initialise( void* meshParticleLayout, void* data ) {
	MeshParticleLayout*       self = (MeshParticleLayout*)meshParticleLayout;

	assert( self );

	Stg_Component_Initialise( self->mesh, NULL, False );
}
	
void _MeshParticleLayout_Execute( void* meshParticleLayout, void* data ) {
	
}
	
void _MeshParticleLayout_Destroy( void* meshParticleLayout, void* data ) {
   MeshParticleLayout* self = (MeshParticleLayout*)meshParticleLayout;
   Stg_Component_Destroy( self->mesh, NULL, False );

   _PerCellParticleLayout_Destroy( self, data );
}


Particle_InCellIndex _MeshParticleLayout_InitialCount( void* meshParticleLayout, void* celllayout, Cell_Index cell_I ) {
	MeshParticleLayout* self = (MeshParticleLayout*)meshParticleLayout;
	return self->cellParticleCount;
}

void _MeshParticleLayout_InitialiseParticlesOfCell( void* meshParticleLayout, void* _swarm, Cell_Index cell_I ) {
	MeshParticleLayout*	self = (MeshParticleLayout*)meshParticleLayout;
	Swarm*              	swarm = (Swarm*)_swarm;
	Coord               	min = {-1.0, -1.0, -1.0};
	Coord               	max = {1.0, 1.0, 1.0};
	Coord			localCoord;
	double			basis[8];
	unsigned		nDims = Mesh_GetDimSize( self->mesh );
	double**		nodeCoords = self->mesh->verts;
	unsigned		nNodes, *incNodes;
	Particle_InCellIndex	particlesThisCell = swarm->cellParticleCountTbl[cell_I];
	Particle_InCellIndex	cParticle_I = 0;
	GlobalParticle*	        particle = NULL;
	IArray*			inc;
	unsigned		d_i, n_i;

	assert( nDims == 2 || nDims == 3 );

	inc = IArray_New();
	Mesh_GetIncidence( self->mesh, nDims, cell_I, MT_VERTEX, 
			   inc );
	nNodes = IArray_GetSize( inc );
	incNodes = IArray_GetPtr( inc );

	for ( cParticle_I = 0; cParticle_I < particlesThisCell; cParticle_I++ ) {	
		particle = (GlobalParticle*)Swarm_ParticleInCellAt( swarm, cell_I, cParticle_I );
		particle->owningCell = cell_I;
		
		for ( d_i = 0; d_i < nDims; d_i++ ) {
			localCoord[d_i] = Swarm_Random_Random_WithMinMax( min[d_i], max[d_i] );
		}

		/* Convert the coordinate to global. Assumes quad or hex mesh. */
		if( nDims == 2 ) {
			basis[0] = 0.25 * (1.0 - localCoord[0]) * (1.0 - localCoord[1]);
			basis[1] = 0.25 * (1.0 + localCoord[0]) * (1.0 - localCoord[1]);
			basis[2] = 0.25 * (1.0 - localCoord[0]) * (1.0 + localCoord[1]);
			basis[3] = 0.25 * (1.0 + localCoord[0]) * (1.0 + localCoord[1]);
		}
		else {
			basis[0] = 0.125 * (1.0 - localCoord[0]) * (1.0 - localCoord[1]) * (1.0 - localCoord[2]);
			basis[1] = 0.125 * (1.0 + localCoord[0]) * (1.0 - localCoord[1]) * (1.0 - localCoord[2]);
			basis[2] = 0.125 * (1.0 - localCoord[0]) * (1.0 + localCoord[1]) * (1.0 - localCoord[2]);
			basis[3] = 0.125 * (1.0 + localCoord[0]) * (1.0 + localCoord[1]) * (1.0 - localCoord[2]);
			basis[4] = 0.125 * (1.0 - localCoord[0]) * (1.0 - localCoord[1]) * (1.0 + localCoord[2]);
			basis[5] = 0.125 * (1.0 + localCoord[0]) * (1.0 - localCoord[1]) * (1.0 + localCoord[2]);
			basis[6] = 0.125 * (1.0 - localCoord[0]) * (1.0 + localCoord[1]) * (1.0 + localCoord[2]);
			basis[7] = 0.125 * (1.0 + localCoord[0]) * (1.0 + localCoord[1]) * (1.0 + localCoord[2]);
		}

		memset( particle->coord, 0, sizeof(double) * nDims );
		for( d_i = 0; d_i < nDims; d_i++ ) {
			for( n_i = 0; n_i < nNodes; n_i++ ) {
				particle->coord[d_i] += basis[n_i] * nodeCoords[incNodes[n_i]][d_i];
			}
		}
	}

	NewClass_Delete( inc );
}
