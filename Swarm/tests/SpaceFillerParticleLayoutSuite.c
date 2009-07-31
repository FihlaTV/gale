/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
** Copyright (C), 2003, Victorian Partnership for Advanced Computing (VPAC) Ltd, 110 Victoria Street, Melbourne, 3053, Australia.
**
** Authors:
**   Stevan M. Quenette, Senior Software Engineer, VPAC. (steve@vpac.org)
**   Patrick D. Sunter, Software Engineer, VPAC. (pds@vpac.org)
**   Luke J. Hodkinson, Computational Engineer, VPAC. (lhodkins@vpac.org)
**   Siew-Ching Tan, Software Engineer, VPAC. (siew@vpac.org)
**   Alan H. Lo, Computational Engineer, VPAC. (alan@vpac.org)
**   Raquibul Hassan, Computational Engineer, VPAC. (raq@vpac.org)
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
** Role:
**   Tests the SpaceFillerParticleLayoutSuite
**
** $Id: testTemplate.c 3462 2006-02-19 06:53:24Z WalterLandry $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <stdio.h>
#include <stdlib.h>

#include "pcu/pcu.h"
#include <StGermain/StGermain.h>
#include "StgDomain/Geometry/Geometry.h"
#include "StgDomain/Shape/Shape.h"
#include "StgDomain/Mesh/Mesh.h"
#include "StgDomain/Utils/Utils.h"
#include "StgDomain/Swarm/Swarm.h"

#include "SpaceFillerParticleLayoutSuite.h"

struct _Particle {
	__IntegrationPoint
};

typedef struct {
	unsigned							nDims;
	unsigned							meshSize[3];
	double							minCrds[3];
	double							maxCrds[3];
	ExtensionManager_Register*	extensionMgr_Register;
	Mesh*								mesh;
	ElementCellLayout*			elementCellLayout;
	Swarm*							swarm;
	SpaceFillerParticleLayout*	particleLayout;
	DomainContext*   				context;
	MPI_Comm       				comm;
   unsigned int   				rank;
   unsigned int   				nProcs;
} SpaceFillerParticleLayoutSuiteData;

Mesh* SpaceFillerParticleLayoutSuite_BuildMesh( unsigned nDims, unsigned* size, double* minCrds, double* maxCrds, ExtensionManager_Register* emReg ) {
	CartesianGenerator*	gen;
	Mesh*			mesh;

	gen = CartesianGenerator_New( "" );
	CartesianGenerator_SetDimSize( gen, nDims );
	CartesianGenerator_SetTopologyParams( gen, size, 0, NULL, NULL );
	CartesianGenerator_SetGeometryParams( gen, minCrds, maxCrds );

	mesh = Mesh_New( "" );
	Mesh_SetExtensionManagerRegister( mesh, emReg );
	Mesh_SetGenerator( mesh, gen );
	Mesh_SetAlgorithms( mesh, Mesh_RegularAlgorithms_New( "" ) );

	Stg_Component_Build( mesh, NULL, False );
	Stg_Component_Initialise( mesh, NULL, False );

	KillObject( mesh->generator );

	return mesh;
}

void SpaceFillerParticleLayoutSuite_Setup( SpaceFillerParticleLayoutSuiteData* data ) {
	Dimension_Index	dim;
	char					input_file[PCU_PATH_MAX];
	
	/* MPI Initializations */
	data->comm = MPI_COMM_WORLD;  
   MPI_Comm_rank( data->comm, &data->rank );
   MPI_Comm_size( data->comm, &data->nProcs );
   
   data->nDims = 3;
	data->meshSize[0] = 4;	data->meshSize[1] = 2;	data->meshSize[2] = 1;
	data->minCrds[0] = 0.0; data->minCrds[1] = 0.0; data->minCrds[2] = 0.0;
	data->maxCrds[0] = 400.0; data->maxCrds[1] = 200.0; data->maxCrds[2] = 100.0;
	
	/* Init mesh */
	data->extensionMgr_Register = ExtensionManager_Register_New();
	data->mesh = SpaceFillerParticleLayoutSuite_BuildMesh( data->nDims, data->meshSize, data->minCrds, data->maxCrds, data->extensionMgr_Register );
	
	/* Configure the element-cell-layout */
	data->elementCellLayout = ElementCellLayout_New( "elementCellLayout", data->mesh );
	
	/* Build the mesh */
	Stg_Component_Build( data->mesh, 0, False );
	Stg_Component_Initialise( data->mesh, 0, False );
	
	/* Configure the gauss-particle-layout */
	data->particleLayout = SpaceFillerParticleLayout_New( "spaceFillerParticleLayout", data->nDims, SpaceFillerParticleLayout_Invalid, 20 );
	
	data->swarm = Swarm_New( "testSwarm", data->elementCellLayout, data->particleLayout, dim, sizeof(Particle),
		data->extensionMgr_Register, NULL, data->comm, NULL );
	
	/* Build the swarm */
	Stg_Component_Build( data->swarm, 0, False );
	Stg_Component_Initialise( data->swarm, 0, False );
}

void SpaceFillerParticleLayoutSuite_Teardown( SpaceFillerParticleLayoutSuiteData* data ) {
	/* Destroy stuff */
	Stg_Class_Delete( data->particleLayout );
	Stg_Class_Delete( data->elementCellLayout );
	Stg_Class_Delete( data->swarm );
	Stg_Class_Delete( data->mesh );
	Stg_Class_Delete( data->extensionMgr_Register );
	remove( "spaceFillerParticle.dat" );
}

void SpaceFillerParticleLayoutSuite_TestSpaceFillerParticle( SpaceFillerParticleLayoutSuiteData* data ) {
	double 						x,y,z;
	unsigned int 				p, i, len;
	int							procToWatch;
	Stream*						stream;
	char 							expected_file[PCU_PATH_MAX];
	
	if( data->nProcs >= 2 ) {
		procToWatch = 1;
	}
	else {
		procToWatch = 0;
	}
	if( data->rank == procToWatch ) printf( "Watching rank: %i\n", data->rank );
	
	stream = Journal_Register( Info_Type, "ManualParticle" );
	
	if( data->rank == procToWatch ) {
		Stg_Class_Print( data->particleLayout, stream );
		/* Print out the particles on all cells */
		Stream_RedirectFile( stream, "spaceFillerParticle.dat" );
		Swarm_PrintParticleCoords_ByCell( data->swarm, stream );
	}
	pcu_filename_expected( "testSpaceFillerParticleLayoutOutput.expected", expected_file );
	pcu_check_fileEq( "spaceFillerParticle.dat", expected_file );
}

void SpaceFillerParticleLayoutSuite( pcu_suite_t* suite ) {
   pcu_suite_setData( suite, SpaceFillerParticleLayoutSuiteData );
   pcu_suite_setFixtures( suite, SpaceFillerParticleLayoutSuite_Setup, SpaceFillerParticleLayoutSuite_Teardown );
   pcu_suite_addTest( suite, SpaceFillerParticleLayoutSuite_TestSpaceFillerParticle );
}
