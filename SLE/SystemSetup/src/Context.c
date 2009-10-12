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
** $Id: Context.c 1207 2008-08-17 15:16:25Z LukeHodkinson $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StgDomain/StgDomain.h>
#include "StgFEM/Discretisation/Discretisation.h"
#include "units.h"
#include "types.h"
#include "shortcuts.h"
#include "Context.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "SystemLinearEquations.h"
#include "SolutionVector.h"

#ifdef WRITE_HDF5
#include <hdf5.h>
#endif

#define FINISHED_WRITING_TAG 9

/* Textual name of this class */
const Type FiniteElementContext_Type = "FiniteElementContext";
const Name defaultFiniteElementContextETypeRegisterName = "finiteElementContext";
const Name FiniteElementContext_EP_CalcDt = "FiniteElementContext_EP_CalcDt";


/* Constructors ------------------------------------------------------------------------------------------------------------------*/

void* FiniteElementContext_DefaultNew( Name name )
{
	return _FiniteElementContext_New(
		sizeof(FiniteElementContext),
		FiniteElementContext_Type,
		_FiniteElementContext_Delete,
		_FiniteElementContext_Print,
		NULL,
		FiniteElementContext_DefaultNew,
		_FiniteElementContext_Construct,
		_AbstractContext_Build,
		_AbstractContext_Initialise,
		_AbstractContext_Execute,
		_AbstractContext_Destroy,
		name,
		False,
		_FiniteElementContext_SetDt,
		0,
		0,
		MPI_COMM_WORLD,
		NULL );
}

FiniteElementContext*				FiniteElementContext_New(
		Name						name,
		double						start,
		double						stop,
		MPI_Comm					communicator,
		Dictionary*					dictionary )
{
	return _FiniteElementContext_New(
		sizeof(FiniteElementContext),
		FiniteElementContext_Type,
		_FiniteElementContext_Delete,
		_FiniteElementContext_Print,
		NULL,
		FiniteElementContext_DefaultNew,
		_FiniteElementContext_Construct,
		_AbstractContext_Build,
		_AbstractContext_Initialise,
		_AbstractContext_Execute,
		_AbstractContext_Destroy,
		name,
		True,
		_FiniteElementContext_SetDt,
		start,
		stop,
		communicator,
		dictionary );
}	


FiniteElementContext* _FiniteElementContext_New( 
		SizeT						sizeOfSelf,
		Type						type,
		Stg_Class_DeleteFunction*				_delete,
		Stg_Class_PrintFunction*				_print,
		Stg_Class_CopyFunction*				_copy, 
		Stg_Component_DefaultConstructorFunction*    _defaultConstructor,
		Stg_Component_ConstructFunction*_construct,
		Stg_Component_BuildFunction*    _build,
		Stg_Component_InitialiseFunction*   _initialise,
		Stg_Component_ExecuteFunction*  _execute,
		Stg_Component_DestroyFunction*  _destroy,
		Name 							name,
		Bool							initFlag,
		AbstractContext_SetDt*				_setDt,
		double						start,
		double						stop,
		MPI_Comm					communicator,
		Dictionary*					dictionary )
{
	FiniteElementContext* self;
	
	/* Allocate memory */
	self = (FiniteElementContext*)_DomainContext_New( 
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
		name,
		initFlag,
		_setDt, 
		start, 
		stop, 
		communicator, 
		dictionary );
	
	/* General info */
	
	/* Virtual info */
	
	if( initFlag ){
		_FiniteElementContext_Init( self );
	}
	
	return self;
}


void _FiniteElementContext_Init( FiniteElementContext* self ) {
	Stream*  errorStream = Journal_Register( Error_Type, self->type );

	/* Set up stream preferences */
	Journal_Enable_NamedStream( InfoStream_Type, "StgFEM_VerboseConfig", False );

	/* register this current stream (the context) as a child of the FE stream */
	/* TODO: Want to be able to recombine this with the Abs context's debug stream at some stage */
	self->isConstructed = True;
	self->debug = Stream_RegisterChild( StgFEM_Debug, FiniteElementContext_Type );
	
	/* set up s.l.e list */
	self->slEquations = Stg_ObjectList_New();

	/* Create Entry Point for Calculating timestep */
	self->calcDtEP = EntryPoint_New( FiniteElementContext_EP_CalcDt, EntryPoint_Minimum_VoidPtr_CastType );
	EntryPoint_Register_Add( self->entryPoint_Register, self->calcDtEP );
	
	/* Add hooks to existing entry points... use name "default" so that plugin, etc can exert same behaviour on other contexts*/
	EntryPoint_Prepend( 
		Context_GetEntryPoint( self, AbstractContext_EP_Build ),
		"default", 
		_FiniteElementContext_Build, 
		FiniteElementContext_Type );
	EntryPoint_Prepend( 
		Context_GetEntryPoint( self, AbstractContext_EP_Initialise ),
		"default", 
		_FiniteElementContext_Initialise, 
		FiniteElementContext_Type );
	EntryPoint_Append( 
		Context_GetEntryPoint( self, AbstractContext_EP_Solve ),
		"default", 
		_FiniteElementContext_Solve, 
		FiniteElementContext_Type );
	EntryPoint_Append( 
		Context_GetEntryPoint( self, AbstractContext_EP_Solve ),
		"postSolve", 
		_FiniteElementContext_PostSolve, 
		FiniteElementContext_Type );
	EntryPoint_Append( 
		Context_GetEntryPoint( self, AbstractContext_EP_Dt ),
		"default", 
		_FiniteElementContext_GetDt, 
		FiniteElementContext_Type );

	EntryPoint_Append(
		Context_GetEntryPoint( self, AbstractContext_EP_Save ),
		"saveFeVariables",
		_FiniteElementContext_SaveFeVariables,
		FiniteElementContext_Type );

	EntryPoint_Append(
		Context_GetEntryPoint( self, AbstractContext_EP_DataSave ),
		"saveFeVariables",
		_FiniteElementContext_SaveFeVariables,
		FiniteElementContext_Type );
	/* The FEM context needs to save gauss swarms so they can be re-loaded for restart later.
	   This will automatically save material point swarms too if PICellerator is used.
	 */
	EntryPoint_Append(
		Context_GetEntryPoint( self, AbstractContext_EP_Save ),
		"saveSwarms",
		_FiniteElementContext_SaveSwarms,
		FiniteElementContext_Type );

	EntryPoint_Append(
		Context_GetEntryPoint( self, AbstractContext_EP_Save ),
		"saveMesh",
		_FiniteElementContext_SaveMesh,
		FiniteElementContext_Type );

	EntryPoint_Append(
		Context_GetEntryPoint( self, AbstractContext_EP_DataSave ),
		"saveMesh",
		_FiniteElementContext_SaveMesh,
		FiniteElementContext_Type );
}


/* Virtual Functions -------------------------------------------------------------------------------------------------------------*/

void _FiniteElementContext_Delete( void* context ) {
	FiniteElementContext* self = (FiniteElementContext*)context;
	
	Journal_DPrintf( self->debug, "In: %s()\n", __func__ );

	Stream_IndentBranch( StgFEM_Debug );
	Journal_DPrintfL( self->debug, 2, "Deleting the element type register (and hence all element types).\n" );
	Journal_DPrintfL( self->debug, 2, "Deleting all SLEs and the SLE list.\n" );
	Stg_ObjectList_DeleteAllObjects( self->slEquations ); 
	Stg_Class_Delete( self->slEquations ); 
	Stream_UnIndentBranch( StgFEM_Debug );

	/* Stg_Class_Delete parent */
	_DomainContext_Delete( self );
}


void _FiniteElementContext_Print( void* context, Stream* stream ) {
	FiniteElementContext* self = (FiniteElementContext*)context;
	
	/* General info */
	Journal_Printf( (void*) stream, "FiniteElementContext (ptr): %p\n", self );
	
	/* Print parent */
	_DomainContext_Print( self, stream );

	Journal_Printf( (void*) stream, "\tslEquations (ptr): %p\n", self->slEquations );
	Stg_Class_Print( self->slEquations, stream );
}


void _FiniteElementContext_SetDt( void* context, double dt ) {
	FiniteElementContext* self = (FiniteElementContext*)context;
	
	self->dt = dt;
}


/* Public Functions --------------------------------------------------------------------------------------------------------------*/

void FiniteElementContext_AddSLE_Func( void* context, void* sle ) {
	FiniteElementContext* self = (FiniteElementContext*)context;
	
	FiniteElementContext_AddSLE_Macro( self, sle );
}


SystemLinearEquations* FiniteElementContext_GetSLE_Func( void* context, Name sleName ) {
	FiniteElementContext* self = (FiniteElementContext*)context;
	
	return FiniteElementContext_GetSLE_Macro( self, sleName );
}


/* EntryPoint Hooks --------------------------------------------------------------------------------------------------------------*/

void _FiniteElementContext_Construct( void* context, Stg_ComponentFactory* cf, void* data ){
	FiniteElementContext *self = (FiniteElementContext*) context;
	Stream*  errorStream = Journal_Register( Error_Type, self->type );

	_DomainContext_Construct( context, cf, data );
	_FiniteElementContext_Init( self );

	self->dictionary = cf->rootDict;

	self->dt = 0.0f;
	self->prevTimestepDt = 0.0;
	self->limitTimeStepIncreaseRate = Dictionary_GetBool_WithDefault( self->dictionary, "limitTimeStepIncreaseRate", False );
	self->maxTimeStepIncreasePercentage = Dictionary_GetDouble_WithDefault( self->dictionary,
		"maxTimeStepIncreasePercentage", 10.0 );
	Journal_Firewall( self->maxTimeStepIncreasePercentage >= 0, errorStream,
		"Error - in %s(): maxTimeStepIncreasePercentage must be >= 0\n", __func__ );

        self->maxTimeStepSize = Dictionary_GetDouble_WithDefault(
           self->dictionary, "maxTimeStepSize", 0.0 );
}

void _FiniteElementContext_Build( void* context ) {
	FiniteElementContext* self = (FiniteElementContext*)context;
	SystemLinearEquations_Index sle_I;
	
	Stream_IndentBranch( StgFEM_Debug );
	Journal_DPrintf( self->debug, "In: %s()\n", __func__ );

	/* build all the systems of linear equations */
	for ( sle_I = 0; sle_I < self->slEquations->count; sle_I++ ) {
		Stg_Component_Build( self->slEquations->data[sle_I], self, False );
	}

	/* TODO:
	This call shouldn't really be necessary - each variable used should be built
	by the FeVariable that needs it, via its dofLayout. However, unfortunately with
	"Vector" variables that use it, the app fails without this call - since otherwise
	the "velocity" variable doesn't get build, only "vx", "vy" and "vz".
	Need to debug this properly later.
	*/
	Variable_Register_BuildAll( self->variable_Register );

	Stream_UnIndentBranch( StgFEM_Debug );
}


void _FiniteElementContext_Initialise( void* context ) {
	FiniteElementContext* self = (FiniteElementContext*)context;
	SystemLinearEquations_Index sle_I;
	
	Stream_IndentBranch( StgFEM_Debug );
	Journal_DPrintf( self->debug, "In: %s()\n", __func__ );

	/* initialise all the systems of linear equations */
	for ( sle_I = 0; sle_I < self->slEquations->count; sle_I++ ) {
		Stg_Component_Initialise( self->slEquations->data[sle_I], self, False );
	}

	Stream_UnIndentBranch( StgFEM_Debug );
}


void _FiniteElementContext_Solve( void* context ) {
	FiniteElementContext* self = (FiniteElementContext*)context;
	SystemLinearEquations_Index sle_I;
	
	Journal_DPrintf( self->debug, "In: %s()\n", __func__ );
	Stream_IndentBranch( StgFEM_Debug );

	/* solve all the systems of linear equations */
	for ( sle_I = 0; sle_I < self->slEquations->count; sle_I++ ) {
		SystemLinearEquations*	currentSLE = (SystemLinearEquations*)self->slEquations->data[sle_I];
		Journal_DPrintf( self->debug, "Solving for this timestep the %s SLE:\n", self->slEquations->data[sle_I]->name );
		/* TODO: FeVariable should have the option of rebuilding ID and LM, based on sim.
		loop if geometry or BCs change...need to improve interface. */
		/* We set the "force" flag to True here - want the SLE to be re-solved every timestep */
		Stg_Component_Execute( currentSLE, self, True );
	}
	
	Stream_UnIndentBranch( StgFEM_Debug );
}

void _FiniteElementContext_PostSolve( void* context ) {
	FiniteElementContext* self = (FiniteElementContext*)context;

	Journal_DPrintf( self->debug, "In: %s()\n", __func__ );
	Stream_IndentBranch( StgFEM_Debug );

	FiniteElementContext_CalcNewDt( self ) ;

	Stream_UnIndentBranch( StgFEM_Debug );
}

double _FiniteElementContext_GetDt( void* context ) {
	FiniteElementContext* self = (FiniteElementContext*)context;

	return self->dt;
}

double FiniteElementContext_CalcNewDt( void* context ) {
	FiniteElementContext* self = (FiniteElementContext*)context;

	self->prevTimestepDt = self->dt;
	
	if ( self->calcDtEP->hooks->count == 0 ) {
		self->dt = 0.0;
	}
	else {
		self->dt = _EntryPoint_Run_Class_Minimum_VoidPtr( self->calcDtEP, self );
	}	
		
	if ( ( self->timeStep > 1 ) && ( self->limitTimeStepIncreaseRate == True ) ) {
		double  maxAllowedDt = self->prevTimestepDt * ( 1 + self->maxTimeStepIncreasePercentage / 100 );

		if ( self->dt > maxAllowedDt ) {
			int prevContextPrintingRank = Stream_GetPrintingRank( self->info );
			/* We assume the dt calculation will be the same across all procs since its a global
			  operation, so only print this once */
			Stream_SetPrintingRank( self->info, 0 );
			Journal_Printf( 
				self->info, 
				"In %s(): dt calculated was %g (time), but prev timestep's dt\n"
				"was %g (time) and max allowed increase percentage is %.2f\n, thus limiting current\n"
				"dt to %g (time).\n", 
				__func__, 
				self->dt, 
				self->prevTimestepDt,
				self->maxTimeStepIncreasePercentage, 
				maxAllowedDt );
			self->dt = maxAllowedDt;
			Stream_SetPrintingRank( self->info, prevContextPrintingRank );
		}
	}

        if( self->maxTimeStepSize > 0.0 ) {
           if( self->dt > self->maxTimeStepSize )
              self->dt = self->maxTimeStepSize;
        }
	
	return self->dt;
}


void _FiniteElementContext_SaveFeVariables( void* context ) {
	FiniteElementContext*     self = (FiniteElementContext*) context;
	Index                     var_I = 0;
	FieldVariable*            fieldVar = NULL;
	FeVariable*               feVar = NULL;

    /* Save the variables that have had their "isCheckpointedAndReloaded" flag enabled - 
	 *  default is true, but the user may restrict the list by specifying the "FieldVariablesToCheckpoint"
	 *  flag in their constructor - see _FeVariable_Construct().
	 */ 	
	for ( var_I = 0; var_I < self->fieldVariable_Register->objects->count; var_I++ ) {
		fieldVar = FieldVariable_Register_GetByIndex( self->fieldVariable_Register, var_I );

		if ( Stg_Class_IsInstance( fieldVar, FeVariable_Type ) ) {
			feVar = (FeVariable*)fieldVar;	
			if ( (feVar->isCheckpointedAndReloaded && self->isDataSave==False) || (feVar->isSavedData && self->isDataSave==True) ) {
            char*			feVarSaveFileName     = NULL;
            char*			feVarSaveFileNamePart = NULL;   

            feVarSaveFileNamePart = Context_GetCheckPointWritePrefixString( self );

#ifdef WRITE_HDF5
            Stg_asprintf( &feVarSaveFileName, "%s%s.%.5u.h5" , feVarSaveFileNamePart, feVar->name, self->timeStep );
#else
            Stg_asprintf( &feVarSaveFileName, "%s%s.%.5u.dat", feVarSaveFileNamePart, feVar->name, self->timeStep );
#endif
            FeVariable_SaveToFile( feVar, feVarSaveFileName, 
                   Dictionary_GetBool_WithDefault( self->dictionary, "saveCoordsWithFields", False ) );           

            Memory_Free( feVarSaveFileName );
            Memory_Free( feVarSaveFileNamePart );
         }
		}
	}
}


void _FiniteElementContext_SaveSwarms( void* context ) {

	Swarm_Register_SaveAllRegisteredSwarms( 
		Swarm_Register_GetSwarm_Register(), context );

}


void _FiniteElementContext_SaveMesh( void* context ) {
   FiniteElementContext*   self = (FiniteElementContext*) context;
	char*			meshSaveFileName;
	char*			meshSaveFileNamePart;   
	Index			meshStrLen;
   Stream*         	info = Journal_Register( Info_Type, "Context" );
   
	Journal_Printf( info, "In %s(): about to save the mesh to disk:\n", __func__ );
	
   meshSaveFileNamePart = Context_GetCheckPointWritePrefixString( self );

#ifdef WRITE_HDF5
   Stg_asprintf( &meshSaveFileName, "%sMesh.%.5u.h5", meshSaveFileNamePart, self->timeStep );
   _FiniteElementContext_DumpMeshHDF5( context, meshSaveFileName );
#else
   Stg_asprintf( &meshSaveFileName, "%sMesh.%.5u.dat", meshSaveFileNamePart, self->timeStep );
   _FiniteElementContext_DumpMeshAscii( context, meshSaveFileName );
#endif

	Memory_Free( meshSaveFileName );
   Memory_Free( meshSaveFileNamePart );
	Journal_Printf( info, "%s: saving of mesh completed.\n", __func__ );
}

#ifndef WRITE_HDF5
void _FiniteElementContext_DumpMeshAscii( void* context, char* filename ) {
   FiniteElementContext*   self = (FiniteElementContext*) context;
   FieldVariable*    fieldVar = NULL;
	FeVariable*       feVar = NULL;
	Mesh* 			   mesh;
	int 			      rank, nRanks;  
	FILE*			      outputFile;
	int               confirmation = 0;
	MPI_Status        status;
	Node_LocalIndex   lNode_I = 0;
	Node_GlobalIndex  gNode_I = 0;
	double*           coord;
	unsigned          nDims;
	
	MPI_Comm_rank( self->communicator, &rank);
	MPI_Comm_size( self->communicator, &nRanks );	

   fieldVar = FieldVariable_Register_GetByIndex( self->fieldVariable_Register, 0 );
	feVar = (FeVariable*)fieldVar;
	mesh = (Mesh*)feVar->feMesh;
	
	nDims = Mesh_GetDimSize( mesh );	
		
	/* wait for go-ahead from process ranked lower than me, to avoid competition writing to file */
	if ( rank != 0 ) {
		MPI_Recv( &confirmation, 1, MPI_INT, rank - 1, FINISHED_WRITING_TAG, self->communicator, &status );
	}	

	if ( rank == 0 ) {
		outputFile = fopen( filename, "w" );
		assert( outputFile );
		
		/* Write min and max coords to file */
		if( nDims == 2 ) {
            		fprintf( outputFile, "Min: %.15g %.15g 0\n", mesh->minGlobalCrd[0], mesh->minGlobalCrd[1] );
            		fprintf( outputFile, "Max: %.15g %.15g 0\n", mesh->maxGlobalCrd[0], mesh->maxGlobalCrd[1] );
      		} 
		else {
            		fprintf( outputFile, "Min: %.15g %.15g %.15g\n", mesh->minGlobalCrd[0], mesh->minGlobalCrd[1], mesh->minGlobalCrd[2] );
            		fprintf( outputFile, "Max: %.15g %.15g %.15g\n", mesh->maxGlobalCrd[0], mesh->maxGlobalCrd[1], mesh->maxGlobalCrd[2] );  
		}
	}
	else {
		outputFile = fopen( filename, "a" );
		assert( outputFile );
	}	

   for ( lNode_I = 0; lNode_I < FeMesh_GetNodeLocalSize( mesh ); lNode_I++ ) {
	   gNode_I = FeMesh_NodeDomainToGlobal( mesh, lNode_I );
	   fprintf( outputFile, "%u ", gNode_I );
	   coord = Mesh_GetVertex( mesh, lNode_I );
                   
      if(self->dim==2)
         fprintf( outputFile, "%.15g %.15g 0\n", coord[0], coord[1] );
      else
         fprintf( outputFile, "%.15g %.15g %.15g\n", coord[0], coord[1], coord[2] );
	}
	
	fclose( outputFile );
	
	/* send go-ahead from process ranked lower than me, to avoid competition writing to file */
	if ( rank != nRanks - 1 ) {
		MPI_Ssend( &confirmation, 1, MPI_INT, rank + 1, FINISHED_WRITING_TAG, self->communicator );
	}	
}
#endif

#ifdef WRITE_HDF5
void _FiniteElementContext_DumpMeshHDF5( void* context, char* filename ) {
   FiniteElementContext*   self = (FiniteElementContext*) context;
   int                     rank, nRanks;
   FieldVariable*          fieldVar = NULL;
   FeVariable*             feVar = NULL;
   Mesh*                   mesh;
   unsigned                nDims;
   hid_t                   file, fileSpace, fileData, fileSpace2, fileData2;
   hid_t                   memSpace;
   hsize_t                 start[2], count[2], size[2];
   unsigned                totalVerts;
   Node_LocalIndex         lNode_I = 0;
   Node_GlobalIndex        gNode_I = 0;
   double*                 coord;
   double                  buf[4];
   int                     buf_int[5];
	MPI_Status              status;
	int                     confirmation = 0;
	Stream*                 errorStr = Journal_Register( Error_Type, self->type );
   Element_LocalIndex      lElement_I;
   Element_GlobalIndex     gElement_I;
   Index                   maxNodes;
   IArray*                 iarray = IArray_New();

   MPI_Comm_rank( self->communicator, &rank);
   MPI_Comm_size( self->communicator, &nRanks );   

   fieldVar = FieldVariable_Register_GetByIndex( self->fieldVariable_Register, 0 );

   if ( Stg_Class_IsInstance( fieldVar, FeVariable_Type ) ) {
      feVar  = (FeVariable*)fieldVar;
      mesh   = (Mesh*)feVar->feMesh;
   
      nDims  = Mesh_GetDimSize( mesh );

      /* wait for go-ahead from process ranked lower than me, to avoid competition writing to file */
      if ( rank != 0 ) {
         MPI_Recv( &confirmation, 1, MPI_INT, rank - 1, FINISHED_WRITING_TAG, self->communicator, &status );
      }	

      if ( rank == 0 ) {
         hid_t      attribData_id, attrib_id, group_id;
         hsize_t    a_dims;
         int        attribData;
         int        res[3];
         Grid**     grid;
         unsigned*  sizes;

         /* Open the HDF5 output file. */
         file = H5Fcreate( filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );
         assert( file );

         /** create file attribute */
         /** first store the fevariable checkpointing version */
         a_dims = 1;
         attribData = MeshCHECKPOINT_V2;
         attribData_id = H5Screate_simple(1, &a_dims, NULL);
         #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
            group_id  = H5Gopen(file, "/");
            attrib_id = H5Acreate(group_id, "checkpoint file version", H5T_STD_I32BE, attribData_id, H5P_DEFAULT);
         #else
            group_id  = H5Gopen(file, "/", H5P_DEFAULT);
            attrib_id = H5Acreate(group_id, "checkpoint file version", H5T_STD_I32BE, attribData_id, H5P_DEFAULT, H5P_DEFAULT);
         #endif
         H5Awrite(attrib_id, H5T_NATIVE_INT, &attribData);
         H5Aclose(attrib_id);
         H5Gclose(group_id);
         H5Sclose(attribData_id);

         /** store the mesh dimensionality */
         a_dims = 1;
         attribData = self->dim;
         attribData_id = H5Screate_simple(1, &a_dims, NULL);
         #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
            group_id  = H5Gopen(file, "/");
            attrib_id = H5Acreate(group_id, "dimensions", H5T_STD_I32BE, attribData_id, H5P_DEFAULT);
         #else
            group_id  = H5Gopen(file, "/", H5P_DEFAULT);
            attrib_id = H5Acreate(group_id, "dimensions", H5T_STD_I32BE, attribData_id, H5P_DEFAULT, H5P_DEFAULT);
         #endif
         H5Awrite(attrib_id, H5T_NATIVE_INT, &attribData);
         H5Aclose(attrib_id);
         H5Gclose(group_id);
         H5Sclose(attribData_id);
          
         /** store the mesh resolution if mesh is cartesian */
         if ( !strcmp( feVar->feMesh->generator->type, "CartesianGenerator" ) ) {
            a_dims = self->dim;
            grid   = (Grid**) Mesh_GetExtension( feVar->feMesh, Grid*, "elementGrid" );	
            sizes  =          Grid_GetSizes( *grid ); /** global no. of elements in each dim */
            
            attribData_id = H5Screate_simple(1, &a_dims, NULL);
            #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
               group_id  = H5Gopen(file, "/");
               attrib_id = H5Acreate(group_id, "mesh resolution", H5T_STD_I32BE, attribData_id, H5P_DEFAULT);
            #else
               group_id  = H5Gopen(file, "/", H5P_DEFAULT);
               attrib_id = H5Acreate(group_id, "mesh resolution", H5T_STD_I32BE, attribData_id, H5P_DEFAULT, H5P_DEFAULT);
            #endif
            H5Awrite(attrib_id, H5T_NATIVE_INT, sizes);
            H5Aclose(attrib_id);
            H5Gclose(group_id);
            H5Sclose(attribData_id);
         }       
      
         /* Dump the min and max coords, and number of processes. */
         count[0] = (hsize_t)nDims;
         fileSpace = H5Screate_simple( 1, count, NULL );         
         #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
         fileData = H5Dcreate( file, "/min", H5T_NATIVE_DOUBLE, fileSpace, H5P_DEFAULT );
         #else
         fileData = H5Dcreate( file, "/min", H5T_NATIVE_DOUBLE, fileSpace,
                                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );
         #endif
                     
         H5Dwrite( fileData, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, mesh->minGlobalCrd );
         H5Dclose( fileData );
         H5Sclose( fileSpace );
            
         fileSpace = H5Screate_simple( 1, count, NULL );       
         #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
         fileData = H5Dcreate( file, "/max", H5T_NATIVE_DOUBLE, fileSpace, H5P_DEFAULT );
         #else
         fileData = H5Dcreate( file, "/max", H5T_NATIVE_DOUBLE, fileSpace,
                                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );
         #endif
               
         H5Dwrite( fileData, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, mesh->maxGlobalCrd );
         H5Dclose( fileData );
         H5Sclose( fileSpace );
          
         /* Write vertex coords to file */   
         /* Create our output space and data objects. */
         totalVerts = Mesh_GetGlobalSize( mesh, 0 );
         size[0] = (hsize_t)totalVerts;
         size[1] = (hsize_t)nDims;
         
         fileSpace = H5Screate_simple( 2, size, NULL );
         #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
         fileData = H5Dcreate( file, "/vertices", H5T_NATIVE_DOUBLE, fileSpace, H5P_DEFAULT );
         #else
         fileData = H5Dcreate( file, "/vertices", H5T_NATIVE_DOUBLE, fileSpace,
                               H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );
         #endif

         /* setup element connectivity dataspaces */
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
         
         size[0] = (hsize_t)FeMesh_GetElementGlobalSize(mesh);
         size[1] = (hsize_t)maxNodes;
         /* Create our output space and data objects. */         
         fileSpace2 = H5Screate_simple( 2, size, NULL );
         #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
         fileData2 = H5Dcreate( file, "/connectivity", H5T_NATIVE_INT, fileSpace2, H5P_DEFAULT );
         #else
         fileData2 = H5Dcreate( file, "/connectivity", H5T_NATIVE_INT, fileSpace2,
                               H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );
         #endif

      } else {
         /* Open the HDF5 output file. */
         file = H5Fopen( filename, H5F_ACC_RDWR, H5P_DEFAULT );
         Journal_Firewall( 
            file >= 0, 
            errorStr,
            "Error in %s for %s '%s' - Cannot open file %s.\n", 
            __func__, 
            self->type, 
            self->name, 
            filename );
   
         /* get the node filespace */   
         #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
         fileData  = H5Dopen( file, "/vertices" );
         #else
         fileData  = H5Dopen( file, "/vertices", H5P_DEFAULT );
         #endif
         /* get the filespace handle */
         fileSpace = H5Dget_space(fileData);

         /* get the connectivity */   
         #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
         fileData2  = H5Dopen( file, "/connectivity" );
         #else
         fileData2  = H5Dopen( file, "/connectivity", H5P_DEFAULT );
         #endif
         /* get the filespace handle */
         fileSpace2 = H5Dget_space(fileData2);
      }	
         
      count[0] = 1;
      count[1] = nDims;
      memSpace = H5Screate_simple( 2, count, NULL );
      H5Sselect_all( memSpace );
                        
      for ( lNode_I = 0; lNode_I < FeMesh_GetNodeLocalSize( mesh ); lNode_I++ ) {
         gNode_I = FeMesh_NodeDomainToGlobal( mesh, lNode_I );
         
         coord = Mesh_GetVertex( mesh, lNode_I );
         
         /* select the region of dataspace to write to  */
         start[1] = 0;
         start[0] = gNode_I;
         H5Sselect_hyperslab( fileSpace, H5S_SELECT_SET, start, NULL, count, NULL );
         
         H5Dwrite( fileData, H5T_NATIVE_DOUBLE, memSpace, fileSpace, H5P_DEFAULT, coord );
      }
      
      /* Close off all our handles. */
      H5Sclose( memSpace );
      H5Dclose( fileData );
      H5Sclose( fileSpace );

      H5Sget_simple_extent_dims( fileSpace2, size, NULL );
      count[0] = 1;
      count[1] = size[1];
      memSpace = H5Screate_simple( 2, count, NULL );
      H5Sselect_all( memSpace );
                        
      for ( lElement_I = 0 ; lElement_I < FeMesh_GetElementLocalSize(mesh); lElement_I++ ) {
         int*    nodeList, nodesPerEl;
         int     node_I;
         
         gElement_I = FeMesh_ElementDomainToGlobal( mesh,lElement_I );

         /* get element nodes */
         FeMesh_GetElementNodes( mesh, lElement_I, iarray );
         nodesPerEl = IArray_GetSize( iarray );
         nodeList   = IArray_GetPtr(  iarray );
         
         for( node_I = 0 ; node_I < nodesPerEl ; node_I++ )
            buf_int[node_I] = Mesh_DomainToGlobal( mesh, MT_VERTEX, nodeList[node_I] );
         /* some reordering is required to account for standard node ordering */ 
         buf_int[3] = Mesh_DomainToGlobal( mesh, MT_VERTEX, nodeList[2] );
         buf_int[2] = Mesh_DomainToGlobal( mesh, MT_VERTEX, nodeList[3] );
         if( nDims == 3 ) {
            buf_int[7] = Mesh_DomainToGlobal( mesh, MT_VERTEX, nodeList[6] );
            buf_int[6] = Mesh_DomainToGlobal( mesh, MT_VERTEX, nodeList[7] );
         }
         /* select the region of dataspace to write to  */
         start[1] = 0;
         start[0] = gElement_I;
         H5Sselect_hyperslab( fileSpace2, H5S_SELECT_SET, start, NULL, count, NULL );
         
         H5Dwrite( fileData2, H5T_NATIVE_INT, memSpace, fileSpace2, H5P_DEFAULT, buf_int );
      }
      
      /* Close off all our handles. */
      H5Sclose( memSpace );
      H5Dclose( fileData2 );
      H5Sclose( fileSpace2 );
      H5Fclose( file );

      /* send go-ahead from process ranked lower than me, to avoid competition writing to file */
      if ( rank != nRanks - 1 ) {
         MPI_Ssend( &confirmation, 1, MPI_INT, rank + 1, FINISHED_WRITING_TAG, self->communicator );
      }	
      
   }
   
   NewClass_Delete( iarray );
}
#endif


