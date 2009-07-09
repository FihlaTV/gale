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
** $Id: testList.c 2136 2004-09-30 02:47:13Z PatrickSunter $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pcu/pcu.h"
#include <StGermain/StGermain.h>
#include <StgDomain/StgDomain.h>
#include <StgFEM/StgFEM.h>
#include "PICellerator/PopulationControl/PopulationControl.h"
#include "PICellerator/Weights/Weights.h"
#include "PICellerator/MaterialPoints/MaterialPoints.h"


struct _Node {
   Coord            coord;
};

struct _Element {
   Coord            coord;
};

struct _Particle {
   __GlobalParticle
};

void UpdateParticlePositionsToLeft( Swarm* swarm );

typedef struct {
   Dictionary*                dictionary;
   ExtensionManager_Register* extensionMgr_Register;
   Mesh*                      mesh;
   ElementCellLayout*         elementCellLayout;
   ParticleLayout*            particleLayout;
   Swarm*                     swarm;
   PeriodicBoundariesManager* perBCsManager;
   Stg_Shape*                 shape;
} PeriodicBoundariesManagerSuiteData;


FeMesh* configureFeMesh( double minCrd[3], double maxCrd[3], unsigned int sizes[3], unsigned nDims ) {
   CartesianGenerator*   gen;
   FeMesh*         feMesh;
   unsigned      maxDecomp[3] = {0, 1, 1};

   gen = CartesianGenerator_New( "" );
   CartesianGenerator_SetDimSize( gen, nDims );
   CartesianGenerator_SetTopologyParams( gen, sizes, 0, NULL, maxDecomp );
   CartesianGenerator_SetGeometryParams( gen, minCrd, maxCrd );
   CartesianGenerator_SetShadowDepth( gen, 0 );

   feMesh = FeMesh_New( "" );
   Mesh_SetGenerator( feMesh, gen );
   FeMesh_SetElementFamily( feMesh, "linear" );

   return feMesh;
}


void PeriodicBoundariesManagerSuite_Setup( PeriodicBoundariesManagerSuiteData* data ) {
   unsigned                   numElements[3];
   double                     minCoords[3];
   double                     maxCoords[3];
   Coord                      shapeCenter;
   Coord                      shapeWidth;
   unsigned int               dim = 2;

   /* *** Journal stuff *** */
   Journal_Enable_TypedStream( DebugStream_Type, False );
   Stream_EnableBranch( Journal_Register( Info_Type, ParticleCommHandler_Type ), False );

   /* Read input */
   data->dictionary = Dictionary_New();
   numElements[0] = 10;
   numElements[1] = 10;
   numElements[2] = 1;
   minCoords[0] = 0.0f;
   minCoords[1] = 0.0f;
   minCoords[2] = 0.0f;
   maxCoords[0] = 1.0f;
   maxCoords[1] = 1.0f;
   maxCoords[2] = 1.0f;
   Dictionary_Add( data->dictionary, "particlesPerCell", Dictionary_Entry_Value_FromUnsignedInt( 1 ) );
   Dictionary_Add( data->dictionary, "seed", Dictionary_Entry_Value_FromUnsignedInt( 13 ) );
   /*  TODO: a 2nd test with the periodic shadowing enabled. Its handy to keep the orig one */
   /*    without it though. */

   /* +++ CONSTRUCT PHASE +++ */
   data->extensionMgr_Register = ExtensionManager_Register_New();
   data->mesh = (Mesh*)configureFeMesh( minCoords, maxCoords, numElements, dim );
   data->elementCellLayout = ElementCellLayout_New( "elementCellLayout", data->mesh );
   shapeCenter[0] = 0.05;
   shapeCenter[1] = 0.5;
   shapeCenter[2] = 0.5;
   shapeWidth[0] = 0.04;
   shapeWidth[1] = 1;
   shapeWidth[2] = 1;
   data->shape = (Stg_Shape*)_Box_DefaultNew( "shape" );
   Box_InitAll( data->shape, dim, shapeCenter, 0, 0, 0, shapeWidth );
   data->particleLayout = (ParticleLayout*)WithinShapeParticleLayout_New( "particleLayout", dim, 10, data->shape );
   data->swarm = Swarm_New( "testSwarm", data->elementCellLayout, data->particleLayout, 3, sizeof(Particle),
      data->extensionMgr_Register, NULL, MPI_COMM_WORLD, NULL );
   data->perBCsManager = PeriodicBoundariesManager_New( "perBCsManager", data->mesh, data->swarm,
      data->dictionary );
   
   /* +++ BUILD PHASE +++ */
   Stg_Component_Build( data->mesh, 0, False );
   Stg_Component_Build( data->swarm, 0, False );
   Stg_Component_Build( data->perBCsManager, 0, False );

   PeriodicBoundariesManager_AddPeriodicBoundary( data->perBCsManager, I_AXIS );

   /* +++ INITIALISE PHASE +++ */
   Stg_Component_Initialise( data->mesh, 0, False );
   Stg_Component_Initialise( data->swarm, 0, False );
   Stg_Component_Initialise( data->perBCsManager, 0, False );
} 


void PeriodicBoundariesManagerSuite_Teardown( PeriodicBoundariesManagerSuiteData* data ) {
   /* Destroy stuff */
   Stg_Class_Delete( data->perBCsManager );
   Stg_Class_Delete( data->swarm );
   Stg_Class_Delete( data->shape );
   Stg_Class_Delete( data->particleLayout );
   Stg_Class_Delete( data->elementCellLayout );
   Stg_Class_Delete( data->mesh );
   Stg_Class_Delete( data->extensionMgr_Register );
   Stg_Class_Delete( data->dictionary );
}


void PeriodicBoundariesManagerSuite_TestAdvectOverLeftBoundary( PeriodicBoundariesManagerSuiteData* data ) {
   Index                      timeStep;
   GlobalParticle*            currParticle;
   Particle_Index             lParticle_I;

   for ( timeStep=1; timeStep <= 10; timeStep++ ) {
      UpdateParticlePositionsToLeft( data->swarm );
      for ( lParticle_I = 0; lParticle_I < data->swarm->particleLocalCount ; lParticle_I++ ) {
         PeriodicBoundariesManager_UpdateParticle( data->perBCsManager, lParticle_I );
      }
      Swarm_UpdateAllParticleOwners( data->swarm );
   }

   /* After 10 timesteps, all the particles should have been advected from the left row of cells, to the right row,
    *  to a coord somewhere between 0.9 and 1.0 */   
   /* TODO: first check the particles all still exist */
   for( lParticle_I=0; lParticle_I < data->swarm->particleLocalCount; lParticle_I++ ) {
      currParticle = (GlobalParticle*)Swarm_ParticleAt( data->swarm, lParticle_I );
      pcu_check_true( currParticle->coord[0] >= 0.9 && currParticle->coord[0] <= 1.0 ); 
   }
   /* TODO: check that their cell ownership is correct */
}


void PeriodicBoundariesManagerSuite( pcu_suite_t* suite ) {
   pcu_suite_setData( suite, PeriodicBoundariesManagerSuiteData );
   pcu_suite_setFixtures( suite, PeriodicBoundariesManagerSuite_Setup, PeriodicBoundariesManagerSuite_Teardown );
   pcu_suite_addTest( suite, PeriodicBoundariesManagerSuite_TestAdvectOverLeftBoundary );
}


void UpdateParticlePositionsToLeft( Swarm* swarm ) {
   Cell_LocalIndex         lCell_I;
   Particle_InCellIndex    cParticle_I;
   GlobalParticle*         currParticle;
   Index                   dim_I;
   Stream*                 debugStream = Journal_Register( Debug_Type, "UpdateParticlesLeft" );

   Stream_Indent( debugStream );
   for ( lCell_I=0; lCell_I < swarm->cellLocalCount; lCell_I++ ) {
      Journal_Printf( debugStream, "Updating Particles positions in local cell %d:\n", lCell_I );
      for ( cParticle_I=0; cParticle_I < swarm->cellParticleCountTbl[lCell_I]; cParticle_I++ ) {
         Coord movementVector = {0,0,0};
         Coord newParticleCoord = {0,0,0};
         Coord* oldCoord;

         currParticle = (GlobalParticle*)Swarm_ParticleInCellAt( swarm, lCell_I, cParticle_I );
         oldCoord = &currParticle->coord;

         Stream_Indent( debugStream );
         Journal_Printf( debugStream, "Updating particleInCell %d:\n", cParticle_I );

         movementVector[I_AXIS] = -0.01;

         for ( dim_I=0; dim_I < 3; dim_I++ ) {
            newParticleCoord[dim_I] = (*oldCoord)[dim_I] + movementVector[dim_I];
         }

         Journal_Printf( debugStream, "Changing its coords from (%f,%f,%f) to (%f,%f,%f):\n",
            (*oldCoord)[0], (*oldCoord)[1], (*oldCoord)[2],
            newParticleCoord[0], newParticleCoord[1], newParticleCoord[2] );

         for ( dim_I=0; dim_I < 3; dim_I++ ) {
            currParticle->coord[dim_I] = newParticleCoord[dim_I];
         }
         Stream_UnIndent( debugStream );
      }
   }
   Stream_UnIndent( debugStream );
}
