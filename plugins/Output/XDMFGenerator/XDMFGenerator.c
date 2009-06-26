/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
** Copyright (C), 2003-2006, Victorian Partnership for Advanced Computing (VPAC) Ltd, 110 Victoria Street,
** Melbourne, 3053, Australia.
**
** Primary Contributing Organisations:
** Victorian Partnership for Advanced Computing Ltd, Computational Software Development - http://csd.vpac.org
** Australian Computational Earth Systems Simulator - http://www.access.edu.au
** Monash Cluster Computing - http://www.mcc.monash.edu.au
** Computational Infrastructure for Geodynamics - http://www.geodynamics.org
**
** Contributors:
** Patrick D. Sunter, Software Engineer, VPAC. (pds@vpac.org)
** Robert Turnbull, Research Assistant, Monash University. (robert.turnbull@sci.monash.edu.au)
** Stevan M. Quenette, Senior Software Engineer, VPAC. (steve@vpac.org)
** David May, PhD Student, Monash University (david.may@sci.monash.edu.au)
** Louis Moresi, Associate Professor, Monash University. (louis.moresi@sci.monash.edu.au)
** Luke J. Hodkinson, Computational Engineer, VPAC. (lhodkins@vpac.org)
** Alan H. Lo, Computational Engineer, VPAC. (alan@vpac.org)
** Raquibul Hassan, Computational Engineer, VPAC. (raq@vpac.org)
** Julian Giordani, Research Assistant, Monash University. (julian.giordani@sci.monash.edu.au)
** Vincent Lemiale, Postdoctoral Fellow, Monash University. (vincent.lemiale@sci.monash.edu.au)
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
** $Id: $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StgDomain/StgDomain.h>
#include <StgFEM/StgFEM.h>
#include <PICellerator/PICellerator.h>
#include <Underworld/Underworld.h>
#include "XDMFGenerator.h"

#include <stdlib.h>
#include <string.h>

#ifndef MASTER
	#define MASTER 0
#endif

const Type Underworld_XDMFGenerator_Type = "Underworld_XDMFGenerator";

void _Underworld_XDMFGenerator_Construct( void* component, Stg_ComponentFactory* cf, void *data ) {
	Underworld_XDMFGenerator* self         = (Underworld_XDMFGenerator*) component;
   UnderworldContext*        context;

   context = (UnderworldContext*)Stg_ComponentFactory_ConstructByName( cf, "context", UnderworldContext, True, data );

/** only run this plugin if we are writing HDF5 files **/
#ifdef WRITE_HDF5

   ContextEP_Append( context, AbstractContext_EP_Dump, _Underworld_XDMFGenerator_GenerateAll );

#endif
}

void* _Underworld_XDMFGenerator_DefaultNew( Name name ) {
   return Codelet_New(
         Underworld_XDMFGenerator_Type,
         _Underworld_XDMFGenerator_DefaultNew,
         _Underworld_XDMFGenerator_Construct,
         _Codelet_Build,
         _Codelet_Initialise,
         _Codelet_Execute,
         _Codelet_Destroy,
         name );
}

Index Underworld_XDMFGenerator_Register( PluginsManager* pluginsManager ) {
   Journal_DPrintf( StgFEM_Debug, "In: %s( void* )\n", __func__ );

   return PluginsManager_Submit( pluginsManager, Underworld_XDMFGenerator_Type, "0", _Underworld_XDMFGenerator_DefaultNew );
}


void _Underworld_XDMFGenerator_GenerateAll( void* _context ) {
   UnderworldContext*   context    = (UnderworldContext*)_context;
   Stream*              stream;
      
   /** Only generate XDML if current timestep is a checkpointing step **/
   if( (context->timeStep % context->checkpointEvery != 0) || (context->timeStep == 0))
      return;

   /** only the MASTER process writes to the file.  other processes send information to the MASTER where required **/   
   if(context->rank == MASTER) {
      Name                 filename;
      Bool                 fileOpened;
      Stream*              errorStream  = Journal_Register( Error_Type, CURR_MODULE_NAME );

      /** Create Stream **/
      stream = Journal_Register( InfoStream_Type, "XDMFOutputFile" );
   
      /** Set auto flush on stream **/
      Stream_SetAutoFlush( stream, True );
   
      /** Get name of frequent output file **/
      Stg_asprintf( &filename, "%s/XDMF.%05d.xmf", context->checkpointWritePath, context->timeStep );
   
      /** Init file, always overwriting any existing **/
      fileOpened = Stream_RedirectFile( stream, filename );
      Journal_Firewall( fileOpened, errorStream, 
            "Could not open file %s/%s. Possibly directory %s does not exist or is not writable.\n"
            "Check 'checkpointWritePath' in input file.\n", context->checkpointWritePath, filename, context->checkpointWritePath );
      Memory_Free( filename );
      
      /** write header information **/
      _Underworld_XDMFGenerator_WriteHeader( context, stream );

      /** Write all (checkpointed) FeVariable field information  **/
      _Underworld_XDMFGenerator_WriteFieldSchema( context, stream );

      /** Write all (checkpointed) FeVariable field information  **/
      _Underworld_XDMFGenerator_WriteSwarmSchema( context, stream);

      /** writes footer information and close file/stream **/
      _Underworld_XDMFGenerator_WriteFooter( context, stream );

      /** close the file **/
      Stream_CloseFile( stream);	

   } else {
      /** other process send information about swarms populations to MASTER **/
      _Underworld_XDMFGenerator_SendInfo( context );
   }

}

void _Underworld_XDMFGenerator_WriteFieldSchema( UnderworldContext* context, Stream* stream ) {
   FieldVariable*       fieldVar = NULL;
   FeVariable*          feVar    = NULL;
   Mesh*                mesh;
   unsigned             nDims;
   unsigned             totalVerts;
   Index                maxNodes;
   Index                elementGlobalSize;
   Element_GlobalIndex  gElement_I;
	Index                var_I = 0;
   Bool                 saveCoords  = Dictionary_GetBool_WithDefault( context->dictionary, "saveCoordsWithFields", False );
   Stream*              errorStream = Journal_Register( Error_Type, CURR_MODULE_NAME );
   Name                 variableType;
   Name                 topologyType;
   
   /** we follow the method in SystemSetup/Context.c to get the mesh info. 
       this method seems unsatisfactory, and may be updated soon JohnMansour 20090626  **/ 
   fieldVar = FieldVariable_Register_GetByIndex( context->fieldVariable_Register, 0 );
   if ( Stg_Class_IsInstance( fieldVar, FeVariable_Type ) ) {
      feVar  = (FeVariable*)fieldVar;
      mesh   = (Mesh*)feVar->feMesh;
      
      nDims             = Mesh_GetDimSize( mesh );
      totalVerts        = Mesh_GetGlobalSize( mesh, 0 );
      elementGlobalSize = FeMesh_GetElementGlobalSize(mesh);

      /* get connectivity array size */
      if (mesh->nElTypes == 1)
         maxNodes = FeMesh_GetElementNodeSize( mesh, 0);
      else {
         /* determine the maximum number of nodes each element has */
         maxNodes = 0;
         for ( gElement_I = 0 ; gElement_I < FeMesh_GetElementGlobalSize(mesh); gElement_I++ ) {
            unsigned numNodes;
            numNodes = FeMesh_GetElementNodeSize( mesh, gElement_I);
            if( maxNodes < numNodes ) maxNodes = numNodes;
         }
      }
      /** now write all the xdmf geometry info **/
      /**----------------------- START GEOMETRY   ------------------------------------------------------------------------------------------------------------------- **/
      if(         maxNodes == 4 ){
         Stg_asprintf( &topologyType, "Quadrilateral" );
      } else if ( maxNodes == 8  ) {
         Stg_asprintf( &topologyType, "Hexahedron" );
      } else {
         Journal_Firewall( NULL, errorStream, "\n\n number of element nodes %u not supported...\n should be 4 (2D quadrilateral) " 
                                 "or should be 8 (3D hexahedron). \n\n", maxNodes );
      }
      /** first create the grid which is applicable to the checkpointed fevariables **/
                              Journal_Printf( stream, "   <Grid Name=\"FeVariable_Grid\">\n\n" );
                              Journal_Printf( stream, "      <Time Value=\"%f\" />\n\n", context->currentTime );
      /** now print out topology info, only quadrilateral elements are supported at the moment **/
                              Journal_Printf( stream, "         <Topology Type=\"%s\" NumberOfElements=\"%u\"> \n", topologyType, elementGlobalSize );
                              Journal_Printf( stream, "            <DataItem Format=\"HDF\" DataType=\"Int\"  Dimensions=\"%u %u\">Mesh.%05d.h5:/connectivity</DataItem>\n", elementGlobalSize, maxNodes, context->timeStep );
                              Journal_Printf( stream, "         </Topology>\n\n" );
                              Journal_Printf( stream, "         <Geometry Type=\"XYZ\">\n" );

      Stg_asprintf( &variableType, "NumberType=\"Float\" Precision=\"8\"" );                              
      if(         nDims == 2 ){
         /** note that for 2d, we feed back a quasi 3d array, with the 3rd Dof zeroed.  so in effect we always work in 3d.
             this is done because paraview in particular seems to do everything in 3d, and if you try and give it a 2d vector 
             or array, it complains.... and hence the verbosity of the following 2d definitions**/
                              Journal_Printf( stream, "            <DataItem ItemType=\"Function\"  Dimensions=\"%u 3\" Function=\"JOIN($0, $1, 0*$1)\">\n", totalVerts );
                              Journal_Printf( stream, "               <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 1\" Name=\"XCoords\">\n", totalVerts );
                              Journal_Printf( stream, "                  <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 0 1 1 %u 1 </DataItem>\n", totalVerts );
                              Journal_Printf( stream, "                  <DataItem Format=\"HDF\" %s Dimensions=\"%u 2\">Mesh.%05d.h5:/data</DataItem>\n", variableType, totalVerts, context->timeStep );
                              Journal_Printf( stream, "               </DataItem>\n" );
                              Journal_Printf( stream, "               <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 1\" Name=\"YCoords\">\n", totalVerts );
                              Journal_Printf( stream, "                  <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 1 1 1 %u 1 </DataItem>\n", totalVerts );
                              Journal_Printf( stream, "                  <DataItem Format=\"HDF\" %s Dimensions=\"%u 2\">Mesh.%05d.h5:/data</DataItem>\n", variableType, totalVerts, context->timeStep );
                              Journal_Printf( stream, "               </DataItem>\n" );
                              Journal_Printf( stream, "            </DataItem>\n" );
      } else if ( nDims == 3 ) {
         /** in 3d we simply feed back the 3d hdf5 array, nice and easy **/
                              Journal_Printf( stream, "            <DataItem Format=\"HDF\" %s Dimensions=\"%u 3\">Mesh.%05d.h5:/data</DataItem>\n", variableType, totalVerts, context->timeStep );
      } else {
         Journal_Firewall( NULL, errorStream, "\n\n Position SwarmVariable is not of dofCount 2 or 3.  Should this swarm be checkpointed?\n\n" );
      }
                              Journal_Printf( stream, "         </Geometry>\n\n" );
      /**----------------------- FINISH GEOMETRY  ------------------------------------------------------------------------------------------------------------------- **/
                
                              
   }

   /** now write FeVariable data **/   
	for ( var_I = 0; var_I < context->fieldVariable_Register->objects->count; var_I++ ) {
		fieldVar = FieldVariable_Register_GetByIndex( context->fieldVariable_Register, var_I );

		if ( Stg_Class_IsInstance( fieldVar, FeVariable_Type ) ) {
			feVar = (FeVariable*)fieldVar;
            if ( feVar->isCheckpointedAndReloaded ) {
                  Name   variableType;
                  Name   centering;
                  Index  offset = 0;
                  Index  meshSize = Mesh_GetGlobalSize( feVar->feMesh, 0 );
                  Index  dofCountIndex;
                  Index  dofAtEachNodeCount;

            /**----------------------- START ATTRIBUTES ------------------------------------------------------------------------------------------------------------------- **/
                  /** if coordinates are being stored with feVariable, account for this **/
                  if( saveCoords) offset = nDims; 
                  /** all feVariables are currently stored as doubles **/
                  Stg_asprintf( &variableType, "NumberType=\"Float\" Precision=\"8\"" );
                  /** determine whether feVariable data is cell centered (like Pressure), or on the nodes (like Velocity) **/
                  if(        meshSize == elementGlobalSize ){
                     Stg_asprintf( &centering, "Cell" );
                  } else if( meshSize == totalVerts ) {
                     Stg_asprintf( &centering, "Node" );
                  } else {
                     Journal_Firewall( NULL, errorStream, "\n\n mesh global size must equal number of nodes, or number of elements... something is amiss! \n\n" );
                  }
                  /** how many degrees of freedom does the fevariable have? **/
                  dofAtEachNodeCount = feVar->fieldComponentCount;
                  if (        dofAtEachNodeCount == 1 ) {
                              Journal_Printf( stream, "         <Attribute Type=\"Scalar\" Center=\"%s\" Name=\"%s\">\n", centering,  feVar->name);
                              Journal_Printf( stream, "            <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 1\" >\n", meshSize );
                              Journal_Printf( stream, "               <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 %u 1 1 %u 1 </DataItem>\n", offset, meshSize );
                              Journal_Printf( stream, "               <DataItem Format=\"HDF\" %s Dimensions=\"%u %u\">%s.%05d.h5:/data</DataItem>\n", variableType, meshSize, (offset + dofAtEachNodeCount), feVar->name, context->timeStep);
                              Journal_Printf( stream, "            </DataItem>\n" );
                              Journal_Printf( stream, "         </Attribute>\n\n" );
                  } else if ( dofAtEachNodeCount == 2 ){
                     /** note that for 2d, we feed back a quasi 3d array, with the 3rd Dof zeroed.  so in effect we always work in 3d.
                         this is done because paraview in particular seems to do everything in 3d, and if you try and give it a 2d vector 
                         or array, it complains.... and hence the verbosity of the following 2d definitions **/
                              Journal_Printf( stream, "         <Attribute Type=\"Vector\" Center=\"%s\" Name=\"%s\">\n", centering,  feVar->name);
                              Journal_Printf( stream, "            <DataItem ItemType=\"Function\"  Dimensions=\"%u 3\" Function=\"JOIN($0, $1, 0*$1)\">\n", meshSize );
                              Journal_Printf( stream, "               <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 1\" Name=\"XValue\">\n", meshSize );
                              Journal_Printf( stream, "                  <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 %u 1 1 %u 1 </DataItem>\n", offset, meshSize );
                              Journal_Printf( stream, "                  <DataItem Format=\"HDF\" %s Dimensions=\"%u %u\">%s.%05d.h5:/data</DataItem>\n", variableType, meshSize, (offset + dofAtEachNodeCount), feVar->name, context->timeStep);
                              Journal_Printf( stream, "               </DataItem>\n" );
                              Journal_Printf( stream, "               <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 1\" Name=\"YValue\">\n", meshSize );
                              Journal_Printf( stream, "                  <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 %u 1 1 %u 1 </DataItem>\n", (offset+1), meshSize );
                              Journal_Printf( stream, "                  <DataItem Format=\"HDF\" %s Dimensions=\"%u %u\">%s.%05d.h5:/data</DataItem>\n", variableType, meshSize, (offset + dofAtEachNodeCount), feVar->name, context->timeStep);
                              Journal_Printf( stream, "               </DataItem>\n" );
                              Journal_Printf( stream, "            </DataItem>\n" );
                              Journal_Printf( stream, "         </Attribute>\n\n" );
                  } else if ( dofAtEachNodeCount == 3 ) {
                     /** in 3d we simply feed back the 3d hdf5 array, nice and easy **/
                              Journal_Printf( stream, "         <Attribute Type=\"Vector\" Center=\"%s\" Name=\"%s\">\n", centering,  feVar->name);
                              Journal_Printf( stream, "            <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 3\" >\n", meshSize );
                              Journal_Printf( stream, "               <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 %u 1 1 %u 3 </DataItem>\n", offset, meshSize );
                              Journal_Printf( stream, "               <DataItem Format=\"HDF\" %s Dimensions=\"%u %u\">%s.%05d.h5:/data</DataItem>\n", variableType, meshSize, (offset + dofAtEachNodeCount), feVar->name, context->timeStep);
                              Journal_Printf( stream, "            </DataItem>\n" );
                              Journal_Printf( stream, "         </Attribute>\n\n" );
                  } else {
                     /** where there are more than 3 components, we write each one out as a scalar **/
                     for(dofCountIndex = 0 ; dofCountIndex < dofAtEachNodeCount ; ++dofCountIndex){
                              Journal_Printf( stream, "         <Attribute Type=\"Scalar\" Center=\"%s\" Name=\"%s-Component-%u\">\n", centering,  feVar->name, dofCountIndex);
                              Journal_Printf( stream, "            <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 1\" >\n", meshSize );
                              Journal_Printf( stream, "               <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 %u 1 1 %u 1 </DataItem>\n", (offset+dofCountIndex), meshSize );
                              Journal_Printf( stream, "               <DataItem Format=\"HDF\" %s Dimensions=\"%u %u\">%s.%05d.h5:/data</DataItem>\n", variableType, meshSize, (offset + dofAtEachNodeCount), feVar->name, context->timeStep);
                              Journal_Printf( stream, "            </DataItem>\n" );
                              Journal_Printf( stream, "         </Attribute>\n" );
                     }
                              Journal_Printf( stream, "\n" );
                  }
            /**----------------------- END ATTRIBUTES   ------------------------------------------------------------------------------------------------------------------- **/
            Memory_Free( centering );
            }
      }
   }
                              Journal_Printf( stream, "   </Grid>\n\n" );
Memory_Free( variableType );
Memory_Free( topologyType );
}

void _Underworld_XDMFGenerator_WriteSwarmSchema( UnderworldContext* context, Stream* stream ) {
   Swarm_Register* swarmRegister = Swarm_Register_GetSwarm_Register();
   Index           swarmCount;
   Index           swarmcountindex;
   Index           variablecount;
   Index           dofCountIndex;
   Index           swarmParticleLocalCount;
   Index           countindex;
   Index           ii;
   Swarm*          currentSwarm;
   SwarmVariable*  swarmVar;
   Name            swarmVarName;
   Name            variableType;
   Name            filename_part;
   Stream*         errorStream  = Journal_Register( Error_Type, CURR_MODULE_NAME );
	const int       FINISHED_WRITING_TAG = 100;
	MPI_Status      status;
   
   /** get total number of different swarms **/
   swarmCount  = swarmRegister->swarmList->count;
   
   /** parse swarm list, checking which are actually stored to HDF5.  **/
   /** We assume that all processes have the same number of swarms on them, with the same swarms checkpointed**/
   for(swarmcountindex = 0; swarmcountindex < swarmCount; ++swarmcountindex){
      currentSwarm  = Swarm_Register_At( swarmRegister, swarmcountindex );

      if ( currentSwarm->isSwarmTypeToCheckPointAndReload != True ) 
         continue;

      swarmParticleLocalCount = currentSwarm->particleLocalCount;
      /** first create a grid collection which will contain the collection of swarms from each process. 
          there will be one of these collections for each swarm (not swarmvariable) that is checkpointed **/
                              Journal_Printf( stream, "   <Grid Name=\"%s\" GridType=\"Collection\">\n\n", currentSwarm->name );
                              Journal_Printf( stream, "      <Time Value=\"%f\" />\n\n", context->currentTime );

      Stg_asprintf( &filename_part, "" );
      for (ii = 0 ; ii < context->nproc ; ++ii) {

         /** get the number of particles in each swarm from each process **/
         if (ii != MASTER        ) MPI_Recv( &swarmParticleLocalCount, 1, MPI_INT, ii, FINISHED_WRITING_TAG, context->communicator, &status );
         if (context->nproc != 1 ) Stg_asprintf( &filename_part, ".%uof%u", (ii + 1), context->nproc );
         
         /** first write all the MASTER procs swarm info **/
         if (swarmParticleLocalCount != 0) {
                              Journal_Printf( stream, "      <Grid Name=\"%s_proc_%u\">\n\n", currentSwarm->name, ii );
            
            /** now write all the xdmf geometry info **/
            /**----------------------- START GEOMETRY   ------------------------------------------------------------------------------------------------------------------- **/
                              Journal_Printf( stream, "         <Topology Type=\"POLYVERTEX\" NodesPerElement=\"%u\"> </Topology>\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "         <Geometry Type=\"XYZ\">\n" );
            Stg_asprintf( &swarmVarName, "%s-Position", currentSwarm->name );
            swarmVar = SwarmVariable_Register_GetByName( currentSwarm->swarmVariable_Register, swarmVarName );
            Journal_Firewall( swarmVar, errorStream, "\n\n Could not find required Position SwarmVariable.  Should this swarm be checkpointed?\n\n" );
            Memory_Free( swarmVarName );
            
            /** check what precision it Position variable is stored at **/
            if(        swarmVar->variable->dataTypes[0] == Variable_DataType_Int ){
               Journal_Firewall( NULL, errorStream, "\n\n Position variable can not be of type Int... something is amiss! \n\n" );
            } else if ( swarmVar->variable->dataTypes[0] == Variable_DataType_Char){
               Journal_Firewall( NULL, errorStream, "\n\n Position variable can not be of type Char... something is amiss! \n\n" );
            } else if ( swarmVar->variable->dataTypes[0] == Variable_DataType_Float ){
               Stg_asprintf( &variableType, "NumberType=\"Float\" Precision=\"4\"" );
            } else {
               Stg_asprintf( &variableType, "NumberType=\"Float\" Precision=\"8\"" );
            }
           
            if(         swarmVar->dofCount == 2 ){
               /** note that for 2d, we feed back a quasi 3d array, with the 3rd Dof zeroed.  so in effect we always work in 3d.
                   this is done because paraview in particular seems to do everything in 3d, and if you try and give it a 2d vector 
                   or array, it complains.... and hence the verbosity of the following 2d definitions**/
                              Journal_Printf( stream, "            <DataItem ItemType=\"Function\"  Dimensions=\"%u 3\" Function=\"JOIN($0, $1, 0*$1)\">\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "               <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 1\" Name=\"XCoords\">\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "                  <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 0 1 1 %u 1 </DataItem>\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "                  <DataItem Format=\"HDF\" %s Dimensions=\"%u 2\">%s.%05d%s.h5:/%s-Position</DataItem>\n", variableType, swarmParticleLocalCount, currentSwarm->name, context->timeStep, filename_part, currentSwarm->name );
                              Journal_Printf( stream, "               </DataItem>\n" );
                              Journal_Printf( stream, "               <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 1\" Name=\"YCoords\">\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "                  <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 1 1 1 %u 1 </DataItem>\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "                  <DataItem Format=\"HDF\" %s Dimensions=\"%u 2\">%s.%05d%s.h5:/%s-Position</DataItem>\n", variableType, swarmParticleLocalCount, currentSwarm->name, context->timeStep, filename_part, currentSwarm->name );
                              Journal_Printf( stream, "               </DataItem>\n" );
                              Journal_Printf( stream, "            </DataItem>\n" );
            } else if ( swarmVar->dofCount == 3 ) {
               /** in 3d we simply feed back the 3d hdf5 array, nice and easy **/
                              Journal_Printf( stream, "            <DataItem Format=\"HDF\" %s Dimensions=\"%u 3\">%s.%05d%s.h5:/%s-Position</DataItem>\n", variableType, swarmParticleLocalCount, currentSwarm->name, context->timeStep, filename_part, currentSwarm->name );
            } else {
               Journal_Firewall( NULL, errorStream, "\n\n Position SwarmVariable is not of dofCount 2 or 3.  Should this swarm be checkpointed?\n\n" );
            }
                              Journal_Printf( stream, "         </Geometry>\n\n" );
            /**----------------------- FINISH GEOMETRY  ------------------------------------------------------------------------------------------------------------------- **/
   
            
            /** now write all the swarm attributes.. ie all the checkpointed swarmVariables **/
            
            variablecount = currentSwarm->swarmVariable_Register->objects->count;
            for(countindex = 0; countindex < variablecount; ++countindex){
               swarmVar = SwarmVariable_Register_GetByIndex( currentSwarm->swarmVariable_Register, countindex );
               if( swarmVar->isCheckpointedAndReloaded ) {
            /**----------------------- START ATTRIBUTES ------------------------------------------------------------------------------------------------------------------- **/
                  if(         swarmVar->variable->dataTypes[0] == Variable_DataType_Int ){
                     Stg_asprintf( &variableType, "NumberType=\"Int\"" );
                  } else if ( swarmVar->variable->dataTypes[0] == Variable_DataType_Char){
                     Stg_asprintf( &variableType, "NumberType=\"Char\"" );
                  } else if ( swarmVar->variable->dataTypes[0] == Variable_DataType_Float ){
                     Stg_asprintf( &variableType, "NumberType=\"Float\" Precision=\"4\"" );
                  } else {
                     Stg_asprintf( &variableType, "NumberType=\"Float\" Precision=\"8\"" );
                  }
                  if (        swarmVar->dofCount == 1 ) {
                              Journal_Printf( stream, "         <Attribute Type=\"Scalar\" Center=\"Node\" Name=\"%s\">\n", swarmVar->name);
                              Journal_Printf( stream, "            <DataItem Format=\"HDF\" %s Dimensions=\"%u 1\">%s.%05d%s.h5:/%s</DataItem>\n", variableType, swarmParticleLocalCount, currentSwarm->name, context->timeStep, filename_part, swarmVar->name);
                              Journal_Printf( stream, "         </Attribute>\n\n" );
                  } else if ( swarmVar->dofCount == 2 ){
                     /** note that for 2d, we feed back a quasi 3d array, with the 3rd Dof zeroed.  so in effect we always work in 3d.
                         this is done because paraview in particular seems to do everything in 3d, and if you try and give it a 2d vector 
                         or array, it complains.... and hence the verbosity of the following 2d definitions **/
                              Journal_Printf( stream, "         <Attribute Type=\"Vector\" Center=\"Node\" Name=\"%s\">\n", swarmVar->name);
                              Journal_Printf( stream, "            <DataItem ItemType=\"Function\"  Dimensions=\"%u 3\" Function=\"JOIN($0, $1, 0*$1)\">\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "               <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 1\" Name=\"XValue\">\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "                  <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 0 1 1 %u 1 </DataItem>\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "                  <DataItem Format=\"HDF\" %s Dimensions=\"%u 2\">%s.%05d%s.h5:/%s</DataItem>\n", variableType, swarmParticleLocalCount, currentSwarm->name, context->timeStep, filename_part, swarmVar->name);
                              Journal_Printf( stream, "               </DataItem>\n" );
                              Journal_Printf( stream, "               <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 1\" Name=\"YValue\">\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "                  <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 1 1 1 %u 1 </DataItem>\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "                  <DataItem Format=\"HDF\" %s Dimensions=\"%u 2\">%s.%05d%s.h5:/%s</DataItem>\n", variableType, swarmParticleLocalCount, currentSwarm->name, context->timeStep, filename_part, swarmVar->name);
                              Journal_Printf( stream, "               </DataItem>\n" );
                              Journal_Printf( stream, "            </DataItem>\n" );
                              Journal_Printf( stream, "         </Attribute>\n\n" );
                  } else if ( swarmVar->dofCount == 3 ) {
                     /** in 3d we simply feed back the 3d hdf5 array, nice and easy **/
                              Journal_Printf( stream, "         <Attribute Type=\"Vector\" Center=\"Node\" Name=\"%s\">\n", swarmVar->name);
                              Journal_Printf( stream, "            <DataItem Format=\"HDF\" %s Dimensions=\"%u 3\">%s.%05d%s.h5:/%s</DataItem>\n", variableType, swarmParticleLocalCount, currentSwarm->name, context->timeStep, filename_part, swarmVar->name);
                              Journal_Printf( stream, "         </Attribute>\n\n" );
                  } else {
                     /** where there are more than 3 components, we write each one out as a scalar **/
                     for(dofCountIndex = 0 ; dofCountIndex < swarmVar->dofCount ; ++dofCountIndex){
                              Journal_Printf( stream, "         <Attribute Type=\"Scalar\" Center=\"Node\" Name=\"%s-Component-%u\">\n", swarmVar->name, dofCountIndex);
                              Journal_Printf( stream, "            <DataItem ItemType=\"HyperSlab\" Dimensions=\"%u 1\" >\n", swarmParticleLocalCount );
                              Journal_Printf( stream, "               <DataItem Dimensions=\"3 2\" Format=\"XML\"> 0 %u 1 1 %u 1 </DataItem>\n", dofCountIndex, swarmParticleLocalCount );
                              Journal_Printf( stream, "               <DataItem Format=\"HDF\" %s Dimensions=\"%u %u\">%s.%05d%s.h5:/%s</DataItem>\n", variableType, swarmParticleLocalCount, swarmVar->dofCount, currentSwarm->name, context->timeStep, filename_part, swarmVar->name);
                              Journal_Printf( stream, "            </DataItem>\n" );
                              Journal_Printf( stream, "         </Attribute>\n" );
                     }
                              Journal_Printf( stream, "\n" );
                  }
            /**----------------------- END ATTRIBUTES   ------------------------------------------------------------------------------------------------------------------- **/
               }
         
            }
   
                              Journal_Printf( stream, "      </Grid>\n\n" );
   
         }
      
      }

                              Journal_Printf( stream, "   </Grid>\n\n" );
      
   }

Memory_Free( variableType );
Memory_Free( filename_part );

}

void  _Underworld_XDMFGenerator_WriteHeader( UnderworldContext* context, Stream* stream ) {
   
	/** Print XDMF header info **/
                              Journal_Printf( stream, "<?xml version=\"1.0\" ?>\n" );
                              Journal_Printf( stream, "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n" );
                              Journal_Printf( stream, "<Xdmf xmlns:xi=\"http://www.w3.org/2001/XInclude\" Version=\"2.0\">\n" );
                              Journal_Printf( stream, "\n" );
                              Journal_Printf( stream, "<Domain>\n" );
                              Journal_Printf( stream, "\n" );

}


void _Underworld_XDMFGenerator_WriteFooter( UnderworldContext* context, Stream* stream ) {

                              Journal_Printf( stream, "</Domain>\n" );
                              Journal_Printf( stream, "\n" );
                              Journal_Printf( stream, "</Xdmf>\n" );
                              Journal_Printf( stream, "\n" );

}

void _Underworld_XDMFGenerator_SendInfo( UnderworldContext* context ) {
   Swarm_Register* swarmRegister = Swarm_Register_GetSwarm_Register();
   Index           swarmCount;
   Index           swarmcountindex;
   Index           swarmParticleLocalCount;
   Swarm*          currentSwarm;
   SwarmVariable*  swarmVar;
   Stream*         errorStream  = Journal_Register( Error_Type, CURR_MODULE_NAME );
	const int       FINISHED_WRITING_TAG = 100;
   
   /** get total number of different swarms **/
   swarmCount  = swarmRegister->swarmList->count;
   
   /** parse swarm list, checking which are actually stored to HDF5.  **/
   /** We assume that all processes have the same number of swarms on them, with the same swarms checkpointed**/
   for(swarmcountindex = 0; swarmcountindex < swarmCount; ++swarmcountindex){
      currentSwarm  = Swarm_Register_At( swarmRegister, swarmcountindex );

      if ( currentSwarm->isSwarmTypeToCheckPointAndReload != True ) 
         continue;

      swarmParticleLocalCount = currentSwarm->particleLocalCount;
		MPI_Ssend( &swarmParticleLocalCount, 1, MPI_INT, MASTER, FINISHED_WRITING_TAG, context->communicator );
   }
}
