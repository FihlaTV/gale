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
** $Id: FeVariable.c 1224 2008-09-10 13:28:46Z DavidLee $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StgDomain/StgDomain.h>
#include "units.h"
#include "types.h"
#include "ElementType.h"
#include "ElementType_Register.h"
#include "Element.h"
#include "FeMesh.h"
#include "FeEquationNumber.h"
#include "FeVariable.h"
#include "LinkedDofInfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#if defined(READ_HDF5) || defined(WRITE_HDF5)
#include <hdf5.h>
#endif

const Type   FeVariable_Type = "FeVariable";
const Name   defaultFeVariableFeEquationNumberName = "defaultFeVariableFeEqName";

/* MPI Tags */
static const int DOF_VALUES_TAG = 10;

/* Global objects */
Stg_ObjectList*    FeVariable_FileFormatImportExportList = NULL;

void* FeVariable_DefaultNew( Name name )
{
	return _FeVariable_New(
		sizeof(FeVariable),
		FeVariable_Type,
		_FeVariable_Delete,
		_FeVariable_Print,
		_FeVariable_Copy,
		(Stg_Component_DefaultConstructorFunction*)FeVariable_DefaultNew,
		_FeVariable_Construct,
		_FeVariable_Build, 
		_FeVariable_Initialise,
		_FeVariable_Execute,
		_FeVariable_Destroy,
		name,
		False,
		_FeVariable_InterpolateValueAt,
		_FeVariable_GetMinGlobalFieldMagnitude,
		_FeVariable_GetMaxGlobalFieldMagnitude,
		_FeVariable_GetMinAndMaxLocalCoords,
		_FeVariable_GetMinAndMaxGlobalCoords,
		_FeVariable_InterpolateNodeValuesToElLocalCoord,
		_FeVariable_GetValueAtNode,
		_FeVariable_SyncShadowValues, 
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		0,
		0,
		False,
		False,
		False,
		MPI_COMM_WORLD,
		NULL );
}


FeVariable* FeVariable_New(
		Name                                            name,
		void*                                           feMesh,
		void*                                           geometryMesh,
		DofLayout*                                      dofLayout, 
		void*                                          	bcs,
		void*                                          	ics,
		void*                                           linkedDofInfo,
		Dimension_Index                                 dim,
		Bool                                            isCheckpointedAndReloaded,
		Bool                                            isReferenceSolution,
		Bool                                            loadReferenceEachTimestep,
		FieldVariable_Register*                         fV_Register )		
{
	assert( Class_IsSuper( ((FeMesh*)feMesh)->topo, IGraph ) );
	return FeVariable_New_Full(
		name,
		feMesh,
		geometryMesh,
		dofLayout,
		bcs,
		ics,
		linkedDofInfo,
		NULL,
		dofLayout->_totalVarCount,
		dim,
	  isCheckpointedAndReloaded,
		isReferenceSolution,
		loadReferenceEachTimestep,
		((IGraph*)((FeMesh*)feMesh)->topo)->remotes[MT_VERTEX]->comm->mpiComm, 
		fV_Register );
}


FeVariable* FeVariable_New_FromTemplate(
		Name                                            name,
		void*                                          	_templateFeVariable,
		DofLayout*                                      dofLayout, 
		void*                                          	ics,
		Bool                                            isReferenceSolution,
		Bool                                            loadReferenceEachTimestep,
		FieldVariable_Register*                         fV_Register )
{
	FeVariable*	templateFeVariable = _templateFeVariable;
	FeVariable*	newFeVariable = NULL;
	
	newFeVariable = FeVariable_New_Full(
		name, 
		templateFeVariable->feMesh,
		templateFeVariable->geometryMesh,
		dofLayout,
		templateFeVariable->bcs,
		ics,
		templateFeVariable->linkedDofInfo,
		templateFeVariable,
		templateFeVariable->fieldComponentCount,
		templateFeVariable->dim,
		templateFeVariable->isCheckpointedAndReloaded,
		isReferenceSolution,
		loadReferenceEachTimestep,
		templateFeVariable->communicator,
		fV_Register );

	newFeVariable->templateFeVariable = templateFeVariable;
	return newFeVariable;
}


FeVariable* FeVariable_New_Full(
		Name                                            name,
		void*                                           feMesh,
		void*                                           geometryMesh,
		DofLayout*                                      dofLayout, 
		void*                                          	bcs,
		void*                                          	ics,
		void*                                           linkedDofInfo,
		void*                                          	templateFeVariable,
		Index                                           fieldComponentCount,
		Dimension_Index                                 dim,
		Bool                                            isCheckpointedAndReloaded,
		Bool                                            isReferenceSolution,
		Bool                                            loadReferenceEachTimestep,
		MPI_Comm                                        communicator,
		FieldVariable_Register*                         fV_Register )		
{
	return _FeVariable_New(
		sizeof(FeVariable),
		FeVariable_Type,
		_FeVariable_Delete,
		_FeVariable_Print,
		_FeVariable_Copy,
		(Stg_Component_DefaultConstructorFunction*)FeVariable_DefaultNew,
		_FeVariable_Construct,
		_FeVariable_Build, 
		_FeVariable_Initialise,
		_FeVariable_Execute,
		_FeVariable_Destroy,
		name,
		True,
		_FeVariable_InterpolateValueAt,
		_FeVariable_GetMinGlobalFieldMagnitude,
		_FeVariable_GetMaxGlobalFieldMagnitude,
		_FeVariable_GetMinAndMaxLocalCoords,
		_FeVariable_GetMinAndMaxGlobalCoords,
		_FeVariable_InterpolateNodeValuesToElLocalCoord,
		_FeVariable_GetValueAtNode,
		_FeVariable_SyncShadowValues, 
		feMesh,
		geometryMesh,
		dofLayout,
		bcs,
		ics,
		linkedDofInfo,
		templateFeVariable,
		fieldComponentCount,
		dim,
		isCheckpointedAndReloaded,
		isReferenceSolution,
		loadReferenceEachTimestep,
		communicator,
		fV_Register );
}


FeVariable* _FeVariable_New( 
		SizeT                                           _sizeOfSelf,
		Type                                            type,
		Stg_Class_DeleteFunction*                       _delete,
		Stg_Class_PrintFunction*                        _print,
		Stg_Class_CopyFunction*                         _copy, 
		Stg_Component_DefaultConstructorFunction*       _defaultConstructor,
		Stg_Component_ConstructFunction*                _construct,
		Stg_Component_BuildFunction*                    _build,
		Stg_Component_InitialiseFunction*               _initialise,
		Stg_Component_ExecuteFunction*                  _execute,
		Stg_Component_DestroyFunction*                  _destroy,
		Name                                            name,
		Bool                                            initFlag,
		FieldVariable_InterpolateValueAtFunction*       _interpolateValueAt,
		FieldVariable_GetValueFunction*                 _getMinGlobalFieldMagnitude,
		FieldVariable_GetValueFunction*                 _getMaxGlobalFieldMagnitude,
		FieldVariable_GetCoordFunction*                 _getMinAndMaxLocalCoords,
		FieldVariable_GetCoordFunction*                 _getMinAndMaxGlobalCoords,		
		FeVariable_InterpolateWithinElementFunction*    _interpolateWithinElement,	
		FeVariable_GetValueAtNodeFunction*              _getValueAtNode,	
		FeVariable_SyncShadowValuesFunc*                _syncShadowValues, 
		void*                                           feMesh,
		void*                                           geometryMesh,
		DofLayout*                                      dofLayout, 
		void*                                          	bcs,
		void*                                          	ics,
		void*                                           linkedDofInfo,
		void*                                          	templateFeVariable,
		Index                                           fieldComponentCount,
		Dimension_Index                                 dim,
		Bool                                            isCheckpointedAndReloaded,
		Bool                                            isReferenceSolution,
		Bool                                            loadReferenceEachTimestep,
		MPI_Comm                                        communicator,
		FieldVariable_Register*                         fV_Register )
{
	FeVariable*		   self;
	
	/* Allocate memory */
	assert( _sizeOfSelf >= sizeof(FeVariable) );
	
	self = (FeVariable*)
		_FieldVariable_New(
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
				name,
				initFlag,
				_interpolateValueAt,
				_getMinGlobalFieldMagnitude,
				_getMaxGlobalFieldMagnitude,
				_getMinAndMaxLocalCoords,
				_getMinAndMaxGlobalCoords,
				fieldComponentCount,
				dim,
				isCheckpointedAndReloaded,
				communicator,
				fV_Register );
	
	/* General info */
	
	/* Virtual functions */
	self->_interpolateWithinElement = _interpolateWithinElement;
	self->_getValueAtNode           = _getValueAtNode;
	self->_syncShadowValues = _syncShadowValues;
	
	/* FeVariable info */
	if( initFlag ){
		_FeVariable_Init( self, feMesh, geometryMesh, dofLayout, bcs, ics, linkedDofInfo,
			templateFeVariable, isReferenceSolution, loadReferenceEachTimestep  );
	}
	
	return self;
}


void _FeVariable_Init( 
		FeVariable*                                     self,
		void*                                           feMesh,
		void*                                           geometryMesh,
		DofLayout*                                      dofLayout, 
		void*                                           bcs,
		void*                                           ics,
		void*                                           linkedDofInfo,
		void*                                           templateFeVariable,
		Bool                                            isReferenceSolution,
		Bool                                            loadReferenceEachTimestep )
{
	Stream*              errorStream = Journal_Register( Error_Type, self->type );
	/* General and Virtual info should already be set */
	
	/* FeVariable info */
	self->isConstructed = True;
	self->debug = Stream_RegisterChild( StgFEM_Discretisation_Debug, self->type );
	self->feMesh = Stg_CheckType( feMesh, FeMesh );
	/* Set pointer for geometry mesh - if none is provided then it'll use the feMesh */
	self->geometryMesh = ( geometryMesh ? 
			Stg_CheckType( geometryMesh, FeMesh ) : 
			Stg_CheckType( feMesh, FeMesh ) );
	self->dofLayout = dofLayout;
	if ( bcs )
		self->bcs = Stg_CheckType( bcs, VariableCondition );
	if ( ics )
		self->ics = Stg_CheckType( ics, VariableCondition );
	if ( linkedDofInfo )
		self->linkedDofInfo = Stg_CheckType( linkedDofInfo, LinkedDofInfo );
	self->shadowValuesSynchronised = False;

	if ( templateFeVariable )
		self->templateFeVariable = Stg_CheckType( templateFeVariable, FeVariable );
	if( !self->isReferenceSolution ) {
		if ( self->templateFeVariable ) {
			self->eqNum = self->templateFeVariable->eqNum;
		}
		else {
			self->eqNum = FeEquationNumber_New( defaultFeVariableFeEquationNumberName, self->feMesh,
				self->dofLayout, self->bcs, linkedDofInfo );
			self->eqNum->removeBCs = self->removeBCs;
		}
	}
	else
		self->eqNum = NULL;

	self->isReferenceSolution = isReferenceSolution;
	self->loadReferenceEachTimestep = loadReferenceEachTimestep;
	Journal_Firewall( (self->loadReferenceEachTimestep != True), errorStream,
		 "The loadReferenceEachTimestep feature isn't implemented yet, sorry.\n" );

	self->dynamicBCs[0] = NULL;
	self->dynamicBCs[1] = NULL;
	self->dynamicBCs[2] = NULL;

	self->buildEqNums = True;

	self->inc = IArray_New();
}


void _FeVariable_Delete( void* variable ) {
	FeVariable* self = (FeVariable*)variable;
	
	Journal_DPrintf( self->debug, "In %s- for \"%s\":\n", __func__, self->name );
	Stream_IndentBranch( StgFEM_Debug );
	if( self->eqNum && ( NULL == self->templateFeVariable ) ) {
		Stg_Class_Delete( self->eqNum );
	}
	/* feMesh bc and doflayout are purposely not deleted */

	NewClass_Delete( self->inc );

	/* Stg_Class_Delete parent*/
	_Stg_Component_Delete( self );
	Stream_UnIndentBranch( StgFEM_Debug );
}


/* --- Virtual Function Implementations --- */

void _FeVariable_Print( void* variable, Stream* stream ) {
	FeVariable* self = (FeVariable*)variable;
	
	/* General info */
	Journal_Printf( stream, "FeVariable (ptr): %p\n", self );
	
	/* Print parent */
	_Stg_Component_Print( self, stream );
	
	/* Virtual info */
	
	/* FeVariable info */
	Stg_Class_Print( self->feMesh, stream );
	if ( self->dofLayout ) {
		Stg_Class_Print( self->dofLayout, stream );
	}
	else {
		Journal_Printf( stream, "\tdofLayout: (null)... not provided (may be Operator type)\n" );
	}
	if ( self->bcs ) {
		Stg_Class_Print( self->bcs, stream );
	}
	else {
		Journal_Printf( stream, "\tbcs: (null)... not provided (may be Operator type)\n" );
	}
	if ( self->ics ) {
		Stg_Class_Print( self->ics, stream );
	}
	else {
		Journal_Printf( stream, "\tics: (null)... not provided (may be Operator type)\n" );
	}

	if ( self->linkedDofInfo ) {
		Stg_Class_Print( self->linkedDofInfo, stream );
	}
	else {
		Journal_Printf( stream, "\tlinkedDofInfo: (null)... not provided\n" );
	}
	
	if( self->eqNum ) {
		Stg_Class_Print( self->eqNum, stream );
	}
	else {
		Journal_Printf( stream, "\teqNum: (null)... not built yet\n" );
	}
}


void* _FeVariable_Copy( void* feVariable, void* dest, Bool deep, Name nameExt, PtrMap* ptrMap ) {
	FeVariable*	self = (FeVariable*)feVariable;
	FeVariable*	newFeVariable;
	PtrMap*		map = ptrMap;
	Bool		ownMap = False;
	
	if( !map ) {
		map = PtrMap_New( 10 );
		ownMap = True;
	}
	
	newFeVariable = _FieldVariable_Copy( self, dest, deep, nameExt, map );

	newFeVariable->templateFeVariable = self->templateFeVariable;
	
	if( deep ) {
		newFeVariable->debug = self->debug; 
		newFeVariable->feMesh = (FeMesh*)Stg_Class_Copy( self->feMesh, NULL, deep, nameExt, map );
		newFeVariable->dofLayout = (DofLayout*)Stg_Class_Copy( self->dofLayout, NULL, deep, nameExt, map );
		newFeVariable->bcs = (VariableCondition*)Stg_Class_Copy( self->bcs, NULL, deep, nameExt, map );
		newFeVariable->ics = self->ics ? (VariableCondition*)Stg_Class_Copy( self->ics, NULL, deep, nameExt, map ) : NULL;
		if ( self->linkedDofInfo == NULL ) {
			newFeVariable->linkedDofInfo = NULL;
		}
		else {
			newFeVariable->linkedDofInfo = (LinkedDofInfo*)Stg_Class_Copy( self->linkedDofInfo, NULL,
				deep, nameExt, map );
		}

		if( !self->isReferenceSolution ) {
			if ( self->templateFeVariable ) {
				newFeVariable->eqNum = self->eqNum;
			}
			else {
				newFeVariable->eqNum = (FeEquationNumber*)Stg_Class_Copy( self->eqNum, NULL, deep, nameExt, map );
			}
		}
		else
			newFeVariable->eqNum = NULL;
	}
	else {
		newFeVariable->debug = self->debug;
		newFeVariable->feMesh = self->feMesh;
		newFeVariable->geometryMesh = self->geometryMesh;
		newFeVariable->dofLayout = self->dofLayout;
		newFeVariable->bcs = self->bcs;
		newFeVariable->ics = self->ics;
		newFeVariable->linkedDofInfo = self->linkedDofInfo;
		newFeVariable->eqNum = self->eqNum;
	}
	
	if( ownMap ) {
		Stg_Class_Delete( map );
	}
	
	return (void*)newFeVariable;
}


void _FeVariable_Build( void* variable, void* data ) {
	FeVariable* self = (FeVariable*)variable;
	DomainContext*   context = (DomainContext*)data;
	unsigned dim, numNodes;
	
	if ( False == self->isBuilt ) {
		self->isBuilt = True;
	
		Journal_DPrintf( self->debug, "In %s- for %s:\n", __func__, self->name );
		Stream_IndentBranch( StgFEM_Debug );

		/* build the BCs */
		Stg_Component_Build( self->feMesh, data, False );
		Stg_Component_Build( self->dofLayout, data, False );
		if ( self->bcs ){
			Stg_Component_Build( self->bcs, data, False );
		}
		/* only bother building the ics specified via XML/construct if we are not in restart mode
		  - otherwise, we will use the checkpointed values anyway */
		if ( self->ics && !(context && (True == context->loadFromCheckPoint) ) ) {
			Stg_Component_Build( self->ics, data, False );
		}
		if ( self->linkedDofInfo ) {
			Stg_Component_Build( self->linkedDofInfo, data, False );
		}

		/* Extract component count. */
		self->fieldComponentCount = self->dofLayout->_totalVarCount;

		dim = Mesh_GetDimSize(self->feMesh);
		/** allocate GNx here */
		if( !strcmp( self->feMesh->name, "linearMesh" ) ) {
			numNodes = ( dim == 2 ) ? 4 : 8;
		}
		else if( !strcmp( self->feMesh->name, "quadraticMesh" ) ) {
			numNodes = ( dim == 2 ) ? 9 : 27;
		} 
		else { numNodes = 0; } /* for constantMesh type */

		self->GNx = Memory_Alloc_2DArray( double, dim, numNodes, "Global Shape Function Derivatives" );
		
		/* don't build the equation numbers for fields that aren't being solved for 
		 * (ie: error and reference fields) */
		if( !self->isReferenceSolution && self->buildEqNums ) {
			FeEquationNumber_Build( self->eqNum );
		}

		Stream_UnIndentBranch( StgFEM_Debug );
	}
}

void _FeVariable_Construct( void* variable, Stg_ComponentFactory* cf, void* data ) 
{
	FeVariable*         self          	= (FeVariable*)variable;
	FeMesh*             feMesh        	= NULL;
	FeMesh*             geometryMesh  	= NULL;
	DofLayout*          dofLayout     	= NULL;
	VariableCondition*  bc            	= NULL;
	VariableCondition*  ic            	= NULL;
	LinkedDofInfo*      linkedDofInfo 	= NULL;
	Bool                isReferenceSolution = False;
	Bool                loadReferenceEachTimestep = False;

	_FieldVariable_Construct( self, cf, data );

	feMesh        = Stg_ComponentFactory_ConstructByKey( cf, self->name, "FEMesh",        FeMesh, True, data );
	geometryMesh  = Stg_ComponentFactory_ConstructByKey( cf, self->name, "GeometryMesh",  FeMesh, False, data );
	dofLayout     = Stg_ComponentFactory_ConstructByKey( cf, self->name, DofLayout_Type,  DofLayout,          True, data );

	ic            = Stg_ComponentFactory_ConstructByKey( cf, self->name, "IC",            VariableCondition,  False, data );
	bc            = Stg_ComponentFactory_ConstructByKey( cf, self->name, "BC",            VariableCondition,  False, data );
	linkedDofInfo = Stg_ComponentFactory_ConstructByKey( cf, self->name, "LinkedDofInfo", LinkedDofInfo,      False, data );

	isReferenceSolution = Stg_ComponentFactory_GetBool( cf, self->name, "isReferenceSolution", False );
	loadReferenceEachTimestep = Stg_ComponentFactory_GetBool( cf, self->name, "loadReferenceEachTimestep", False );

	/* TODO: should really be a parameter */
	self->removeBCs = Stg_ComponentFactory_GetBool( cf, self->name, "removeBCs", True );

	_FeVariable_Init( self, feMesh, geometryMesh, dofLayout, bc, ic, linkedDofInfo, NULL,
		isReferenceSolution, loadReferenceEachTimestep );
}

void _FeVariable_Initialise( void* variable, void* data ) {
	FeVariable*              self = (FeVariable*)variable;
	DomainContext*   context = (DomainContext*)data;
	char*                    inputPathString = NULL;
   Dictionary_Entry_Value* feVarsList = NULL;
	
	Journal_DPrintf( self->debug, "In %s- for %s:\n", __func__, self->name );
	Stream_IndentBranch( StgFEM_Debug );
	
	/* do basic mesh initialisation */
	Stg_Component_Initialise( self->feMesh, data, False );
	Stg_Component_Initialise( self->dofLayout, data, False );

	if ( self->linkedDofInfo ) {
		Stg_Component_Initialise( self->linkedDofInfo, data, False );
	}

	if( !self->isReferenceSolution ) {	
		FeEquationNumber_Initialise( self->eqNum );
	}

	if ( context ) {
		/* Get the input path string once here - single point of control */
		inputPathString = Context_GetCheckPointInputPrefixString( context );
	}
	/* If the reference solution option is enabled, just load this up regardless of checkpointing options below.
	 * Want to allow option of disabling this feature, if you're manually setting up a FeVariable without a context etc. */
	if ( context && self->isReferenceSolution ) {
		char * filename = NULL;
		Journal_DPrintf( self->debug, "Reference FeVariable -> loading nodal values from file.\n" );
		
		/* Make filename */
		/*                                                prefix             self->name       . 00000 .  dat \0 */
		filename = Memory_Alloc_Array_Unnamed( char, strlen(inputPathString) + strlen(self->name) + 1 + 5 + 1 + 3 + 1 );
		 
#ifdef READ_HDF5
		sprintf( filename, "%s%s.%.5u.h5", inputPathString, self->name, context->restartTimestep );
#else
		sprintf( filename, "%s%s.%.5u.dat", inputPathString, self->name, context->restartTimestep );
#endif
		FeVariable_ReadFromFile( self, filename );

		Memory_Free( filename );
		Memory_Free( inputPathString );
		Stream_UnIndentBranch( StgFEM_Debug );
		return;
	}

	/*Setting up whether to load from checkpointing */
	if ( self->ics || ((context && (True == context->loadFromCheckPoint) )&& (self->isCheckpointedAndReloaded)) )  {
		Journal_DPrintf( self->debug, "applying the I.C.s for this Variable:\n" ); 
		Stream_Indent( self->debug );
	
		if ( self->ics && !(context && (True == context->loadFromCheckPoint) && (True == self->isCheckpointedAndReloaded)) ) {
			Journal_DPrintf( self->debug, "regular (non-restart) mode -> applying ICs specified in XML/constructor\n" );
			Stg_Component_Initialise( self->ics, data, False );
			VariableCondition_Apply( self->ics, data );
		}
		else {
			char * filename = NULL;
			Journal_DPrintf( self->debug, "restart from checkpoint mode -> loading checkpointed "
				"nodal values as initial conditions, ignoring ics specified via XML/constructor\n" );
			/* Make filename */

			/*                                                prefix             self->name       . 00000 .  dat \0 */
			filename = Memory_Alloc_Array_Unnamed( char, strlen(inputPathString) + strlen(self->name) + 1 + 5 + 1 + 3 + 1 );
			 
#ifdef READ_HDF5
			sprintf( filename, "%s%s.%.5u.h5", inputPathString, self->name, context->restartTimestep );
#else
			sprintf( filename, "%s%s.%.5u.dat", inputPathString, self->name, context->restartTimestep );
#endif
			FeVariable_ReadFromFile( self, filename );
			/* TODO: maybe we want a mechanism in future to over-ride the checkpointed ICs in certain regions too
			so the user can introduce new phenomena into the model */

			Memory_Free( filename );
			Memory_Free( inputPathString );
		}
	}
	Stream_UnIndent( self->debug );

   /* also include check to see if this fevariable should be checkpointed, just incase it didn't go through the fieldvariable construct phase */ 
   feVarsList = Dictionary_Get( context->dictionary, "fieldVariablesToCheckpoint" );
   if ( NULL == feVarsList ) {
      feVarsList = Dictionary_Get( context->dictionary, "FieldVariablesToCheckpoint" );
   }
   if (feVarsList != NULL ) {
      Index                    listLength = Dictionary_Entry_Value_GetCount( feVarsList );
      Index                    var_I = 0;
      Dictionary_Entry_Value*  feVarDictValue = NULL;
      char*                    fieldVariableName;
   
      for ( var_I = 0; var_I < listLength; var_I++ ) {
         feVarDictValue = Dictionary_Entry_Value_GetElement( feVarsList, var_I );
         fieldVariableName = Dictionary_Entry_Value_AsString( feVarDictValue ); 
         if ( 0 == strcmp( self->name, fieldVariableName ) ) {
            self->isCheckpointedAndReloaded = True;
            break;
         }
      }
   }

	if ( self->bcs ) {
		Stg_Component_Initialise( self->bcs, data, False );
		Journal_DPrintf( self->debug, "applying the B.C.s for this Variable.\n" ); 
		VariableCondition_Apply( self->bcs, data );
	}

	Stream_UnIndentBranch( StgFEM_Debug );
}


void FeVariable_ApplyBCs( void* variable, void* data ) {
	FeVariable* self = (FeVariable*)variable;

        /* This is an unpleasant hack; we really need to reevaluate our BC code. */
	if( self->dynamicBCs[0] ) {
		VariableCondition_Apply( self->dynamicBCs[0], data );
	}
	if( self->dynamicBCs[1] ) {
		VariableCondition_Apply( self->dynamicBCs[1], data );
	}
	if( self->dynamicBCs[2] ) {
		VariableCondition_Apply( self->dynamicBCs[2], data );
	}
	if ( self->bcs ) {
		Journal_DPrintf( self->debug, "In %s- for %s:\n", __func__, self->name );
		Journal_DPrintf( self->debug, "applying the B.C.s for this Variable.\n" ); 
		VariableCondition_Apply( self->bcs, data );
	}
}

Bool FeVariable_IsBC( void* variable, int node, int dof ) {
	FeVariable* self = (FeVariable*)variable;

	assert( self );
	if( self->dynamicBCs[0] && 
	    self->dynamicBCs[0]->var == DofLayout_GetVariable( self->dofLayout, node, dof ) && 
	    IMap_Has( self->dynamicBCs[0]->vcMap, node ) )
	{
		return True;
	}

	if( self->dynamicBCs[1] && 
	    self->dynamicBCs[1]->var == DofLayout_GetVariable( self->dofLayout, node, dof ) && 
	    IMap_Has( self->dynamicBCs[1]->vcMap, node ) )
	{
		return True;
	}

	if( self->dynamicBCs[2] && 
	    self->dynamicBCs[2]->var == DofLayout_GetVariable( self->dofLayout, node, dof ) && 
	    IMap_Has( self->dynamicBCs[2]->vcMap, node ) )
	{
		return True;
	}

	if( self->bcs && 
	    VariableCondition_IsCondition( self->bcs, node, self->dofLayout->varIndices[node][dof] ) )
	{
		return True;
	}

	return False;
}


void _FeVariable_Execute( void* variable, void* data ) {
}

void _FeVariable_Destroy( void* variable, void* data ) {
	FeVariable* self = (FeVariable*)variable;

	Memory_Free( self->GNx );
	_FieldVariable_Destroy( self, data );
}

void FeVariable_PrintLocalDiscreteValues( void* variable, Stream* stream ) {
	FeVariable* self = (FeVariable*)variable;

	Journal_Printf( stream, "In %s: for FeVariable \"%s\":\n", __func__, self->name );

	_FeVariable_PrintLocalOrDomainValues( variable, FeMesh_GetNodeLocalSize( self->feMesh ), stream );
}


unsigned _FeVariable_ClosestNode( FeVariable* self, double* crd ) {
	assert( self );
	return Mesh_NearestVertex( self->feMesh, crd );
}


InterpolationResult _FeVariable_InterpolateValueAt( void* variable, double* globalCoord, double* value ) {
	FeVariable*		self = (FeVariable*)variable;
	Element_DomainIndex	elementCoordIn = (unsigned)-1;
	Coord			elLocalCoord={0,0,0};
	InterpolationResult	retValue;


	retValue = FeVariable_GetElementLocalCoordAtGlobalCoord( self, globalCoord, elLocalCoord, &elementCoordIn );
	
	if ( retValue == LOCAL ) {
		/* Now interpolate the value at that coordinate, using shape functions */
		self->_interpolateWithinElement( self, elementCoordIn, elLocalCoord, value );
	}
	else if ( retValue == SHADOW ) {
		if ( False == self->shadowValuesSynchronised ) {
			Stream* warningStr = Journal_Register( Error_Type, self->type );
			Journal_Printf( warningStr, "Warning - in %s: user asking to interpolate a value at "
				"coord (%g,%g,%g), which is in shadow space, but "
				"FeVariable_SyncShadowValues() hasn't been called yet.\n", 
				__func__, globalCoord[0], globalCoord[1], globalCoord[2] );
			return retValue;		
		}
		/* Now interpolate the value at that coordinate, using shape functions */
		self->_interpolateWithinElement( self, elementCoordIn, elLocalCoord, value );
	}
	
	return retValue;
}


double _FeVariable_GetMinGlobalFieldMagnitude( void* feVariable ) {
	FeVariable*	self = (FeVariable*)feVariable;
	FeMesh*		feMesh = self->feMesh;
	int		node_lI=0;
	int		nodeLocalCount = FeMesh_GetNodeLocalSize( feMesh );
	double		min = 0;
	double		globalMin = 0;
	double		currValue;

	min = FeVariable_GetScalarAtNode( self, 0 );
	
	/* Find upper and lower bounds on this processor */
	for ( node_lI = 0 ; node_lI < nodeLocalCount ; node_lI++ ) {
		currValue = FeVariable_GetScalarAtNode( self, node_lI );
		if ( currValue < min ) {
			min = currValue;
		}	
	}

	/* Find upper and lower bounds on all processors */
	MPI_Allreduce( &min, &globalMin, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD );
	return globalMin;
}


double _FeVariable_GetMaxGlobalFieldMagnitude( void* feVariable ) {
	FeVariable*	self = (FeVariable*)feVariable;
	int		node_lI=0;
	int		nodeLocalCount = FeMesh_GetNodeLocalSize( self->feMesh );
	double		max = 0;
	double		globalMax = 0;
	double		currValue;

	max = FeVariable_GetScalarAtNode( self, 0 );
	
	/* Find upper and lower bounds on this processor */
	for ( node_lI = 0 ; node_lI < nodeLocalCount ; node_lI++ ) {
		currValue = FeVariable_GetScalarAtNode( self, node_lI );
		if ( currValue > max ) {
			max = currValue;
		}	
	}

	/* Find upper and lower bounds on all processors */
	MPI_Allreduce( &max, &globalMax, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD );
	return globalMax;
}

void _FeVariable_GetMinAndMaxLocalCoords( void* feVariable, double* min, double* max ) {
	FeVariable*	self = (FeVariable*)feVariable;

	assert( self && Stg_CheckType( self, FeVariable ) );

	Mesh_GetLocalCoordRange( self->feMesh, min, max );
}


void _FeVariable_GetMinAndMaxGlobalCoords( void* feVariable, double* min, double* max ) {
	FeVariable*	self = (FeVariable*)feVariable;

	assert( self && Stg_CheckType( self, FeVariable ) );

	Mesh_GetGlobalCoordRange( self->feMesh, min, max );
}

double FeVariable_GetScalarAtNode( void* feVariable, Node_LocalIndex lNode_I ) {
	FeVariable*	self = (FeVariable*)feVariable;
	Dof_Index	dofCountThisNode = 0;
	Dof_Index	nodeLocalDof_I = 0;
	double      value[ MAX_FIELD_COMPONENTS ];
	
	/*
	if( self->type != OperatorFeVariable_Type && self->dofLayout )
		dofCountThisNode = self->dofLayout->dofCounts[lNode_I];
	else {
	*/
	dofCountThisNode = self->fieldComponentCount;
	/*
	}
	*/

	FeVariable_GetValueAtNode( self, lNode_I, value );

	if ( dofCountThisNode > 1) {
		double		magnitude=0;
		for ( nodeLocalDof_I=0; nodeLocalDof_I < dofCountThisNode; nodeLocalDof_I++ ) {
			magnitude += value[ nodeLocalDof_I ] * value[ nodeLocalDof_I ];
		}
		return sqrt( magnitude );
	}
	else 
		return value[0];

}	


void _FeVariable_GetValueAtNode( void* feVariable, Node_DomainIndex dNode_I, double* value ) {
	FeVariable*	self = (FeVariable*)feVariable;
	Variable*	currVariable = NULL;
	Dof_Index	dofCountThisNode = 0;
	Dof_Index	nodeLocalDof_I = 0;

	dofCountThisNode = self->dofLayout->dofCounts[dNode_I];
	
	for ( nodeLocalDof_I=0; nodeLocalDof_I < dofCountThisNode; nodeLocalDof_I++ ) {
		currVariable = DofLayout_GetVariable( self->dofLayout, dNode_I, nodeLocalDof_I );
		value[ nodeLocalDof_I ] = Variable_GetValueDouble( currVariable, dNode_I );
	}
}

/** Finds the value of the field at the node and broadcasts it to the rest of the processors */
void FeVariable_GetValueAtNodeGlobal( void* feVariable, Node_GlobalIndex gNode_I, double* value ) {
	FeVariable*        self         = (FeVariable*) feVariable;
	FeMesh*            mesh         = self->feMesh;
	Element_LocalIndex lNode_I;
	int                rootRankL     = 0;
	int                rootRankG     = 0;
	MPI_Comm           comm         = self->communicator;
	
	/* Find Local Index */
	if ( Mesh_GlobalToDomain( mesh, MT_VERTEX, gNode_I, &lNode_I ) ) {
		/* If node is on local processor, then get value of field */
		FeVariable_GetValueAtNode( self, lNode_I, value );
		MPI_Comm_rank( comm, (int*)&rootRankL );
	}
	
	/* Send to other processors */
	MPI_Allreduce( &rootRankL, &rootRankG, 1, MPI_INT, MPI_MAX, comm );
	MPI_Bcast( value, self->fieldComponentCount, MPI_DOUBLE, rootRankG, comm );
}

/** Finds the coordinate of the node and broadcasts it to the rest of the processors */
void FeVariable_GetCoordAtNodeGlobal( void* feVariable, Node_GlobalIndex gNode_I, double* coord ) {
	FeVariable*        self         = (FeVariable*) feVariable;
	FeMesh*            mesh         = self->feMesh;
	Element_LocalIndex lNode_I;
	int                rootRankL     = 0;
	int                rootRankG     = 0;
	MPI_Comm           comm         = self->communicator;

	/* Find Local Index */
	if ( Mesh_GlobalToDomain( mesh, MT_VERTEX, gNode_I, &lNode_I ) ) {
		/* If node is on local processor, then get value of field */
		memcpy( coord, Mesh_GetVertex( mesh, lNode_I ), self->dim * sizeof(double) );
		MPI_Comm_rank( comm, (int*)&rootRankL );
	}
	
	/* Send to other processors */
	MPI_Allreduce( &rootRankL, &rootRankG, 1, MPI_INT, MPI_MAX, comm );
	MPI_Bcast( coord, self->dim, MPI_DOUBLE, rootRankG, comm );
}


void FeVariable_ZeroField( void* feVariable ) {
	FeVariable* self = (FeVariable*) feVariable;
	double*     values =  Memory_Alloc_Array( double, self->fieldComponentCount, "tempValues" );
	Index       lNode_I, lNodeCount;

	lNodeCount = FeMesh_GetNodeLocalSize( self->feMesh );

	memset( values, 0, self->fieldComponentCount * sizeof(double) );

	for( lNode_I = 0 ; lNode_I < lNodeCount; lNode_I++ ) {
		FeVariable_SetValueAtNode( self, lNode_I, values );
	}

	Memory_Free( values );
}
	
/* --- Public Functions --- */

InterpolationResult FeVariable_GetElementLocalCoordAtGlobalCoord( void* feVariable, double* globalCoord, double* elLocalCoord,
		Element_DomainIndex* elementCoordInPtr )
{
	FeVariable*		self = (FeVariable*)feVariable;
	InterpolationResult	retValue;
	unsigned		elInd;

	/* locate which mesh element given coord is in : use inclusive upper boundaries to save
	   the need to use shadow space if possible */
	if( !Mesh_SearchElements( self->feMesh, globalCoord, &elInd ) ) {
		Bool			outsideGlobal = False;
		double			min[3], max[3];
		Dimension_Index		dim_I=0;

		FieldVariable_GetMinAndMaxGlobalCoords( self, min, max );
		for ( dim_I = 0; dim_I < self->dim; dim_I++ ) {
			if ( ( globalCoord[dim_I] < min[dim_I] ) || (globalCoord[dim_I] > max[dim_I] ) ) {
				outsideGlobal = True;
			}
		}

		if ( outsideGlobal == True ) {
			return OUTSIDE_GLOBAL;
		}
		else {
			return OTHER_PROC;
		}
	}
	else /* We found the coord is within a local or shadow element */ {
		ElementType*		elementType = NULL;

		*elementCoordInPtr = elInd;
		if ( elInd < FeMesh_GetElementLocalSize( self->feMesh ) ) {
			retValue = LOCAL;
		}
		else {
			retValue = SHADOW;
		}

		/* convert global coordinate to local co-ordinates of element the coord is in */
		elementType = FeMesh_GetElementType( self->feMesh, (*elementCoordInPtr) );
		ElementType_ConvertGlobalCoordToElLocal( elementType, self->feMesh, *elementCoordInPtr, 
							 globalCoord, elLocalCoord );
	}

	return retValue;
}


void FeVariable_SetValueAtNode( void* feVariable, Node_DomainIndex dNode_I, double* componentValues ) {
	FeVariable*	self = (FeVariable*)feVariable;
	Dof_Index	dofCountThisNode = 0;
	Dof_Index	nodeLocalDof_I = 0;

	dofCountThisNode = self->dofLayout->dofCounts[dNode_I];
	
	for ( nodeLocalDof_I=0; nodeLocalDof_I < dofCountThisNode; nodeLocalDof_I++ ) {
		DofLayout_SetValueDouble( (self)->dofLayout, dNode_I, nodeLocalDof_I, componentValues[nodeLocalDof_I] );
	}
}


void FeVariable_PrintLocalDiscreteValues_2dBox( void* variable, Stream* stream ) {
	FeVariable*		self = (FeVariable*)variable;
	Node_LocalIndex		node_lI=0;
	Index			x_I, y_I;
	Index			ii;
	Dof_Index		dof_I=0;
	Dof_Index		currNodeNumDofs=0;
	Index			nx = 0;
	Index			ny = 0;
	double			dx = 0;
	double			dy = 0;
	DofLayout*		dofLayout = self->dofLayout;
	Stream*			eStream = Journal_Register( Error_Type, self->type );
	Index			minLocalNodeX;
	Index			minLocalNodeY;
	Index			maxLocalNodeX;
	Index			maxLocalNodeY;
	Grid*			vertGrid;
	unsigned		inds[2];
	unsigned		vertInd;
	double*			verts[2];
	unsigned		*localOrigin, *localRange;
	double			min[2], max[2];

	if( ExtensionManager_GetHandle( self->feMesh->info, "vertexGrid" ) == (unsigned)-1 || 
	    Mesh_GetDimSize( self->feMesh ) != 2 )
	  {
		Journal_Printf( eStream, "Warning: %s called on variable \"%s\", but this isn't stored on a "
			"regular 2D mesh - so just returning.\n", __func__, self->name );
		return;
	}

	vertGrid = *(Grid**)ExtensionManager_Get( self->feMesh->info, self->feMesh, 
					      ExtensionManager_GetHandle( self->feMesh->info, "vertexGrid" ) );
	localOrigin = (unsigned*)ExtensionManager_Get( self->feMesh->info, self->feMesh, 
						       ExtensionManager_GetHandle( self->feMesh->info, "localOrigin" ) );
	localRange = (unsigned*)ExtensionManager_Get( self->feMesh->info, self->feMesh, 
						      ExtensionManager_GetHandle( self->feMesh->info, "localRange" ) );

	memcpy( inds, localOrigin, Mesh_GetDimSize( self->feMesh ) * sizeof(unsigned) );
	insist( Mesh_GlobalToDomain( self->feMesh, MT_VERTEX, Grid_Project( vertGrid, inds ), &vertInd ), == True );
	verts[0] = Mesh_GetVertex( self->feMesh, vertInd );
	inds[0]++;
	inds[1]++;
	insist( Mesh_GlobalToDomain( self->feMesh, MT_VERTEX, Grid_Project( vertGrid, inds ), &vertInd ), == True );
	verts[1] = Mesh_GetVertex( self->feMesh, vertInd );
	
	nx = vertGrid->sizes[0];
	ny = vertGrid->sizes[1];
	dx = verts[1][0] - verts[0][0];
	dy = verts[1][1] - verts[0][1];



	minLocalNodeX = localOrigin[0];
	minLocalNodeY = localOrigin[1];
	maxLocalNodeX = minLocalNodeX + localRange[0] + 1;
	maxLocalNodeY = minLocalNodeY + localRange[1] + 1;

	Mesh_GetGlobalCoordRange( self->feMesh, min, max );

	Journal_Printf( stream, "display of Values in 2D box X:{%5.2f-%5.2f}, Y:{%5.2f-%5.2f}\n",
		min[I_AXIS], max[I_AXIS],
		min[J_AXIS], max[J_AXIS] );
	Journal_Printf( stream, "\twith %d elements in X (dx=%5.2f) and %d elements in Y (dy=%5.2f)\n\n",
		nx-1, dx, ny-1, dy );

	/*Header*/
	for (ii=0;ii<10;ii++) Journal_Printf( stream, " " );
	for ( x_I=0; x_I < nx; x_I++ ) {
		Journal_Printf( stream, "|  xNode=%3d   ", x_I );
	}
	Journal_Printf( stream, "|\n", x_I );

	for ( y_I= ny-1; y_I != (unsigned)-1; y_I-- ) {
		/*Blocks */
		for (ii=0;ii<10;ii++) Journal_Printf( stream, " " );
		for ( x_I=0; x_I < nx; x_I++ ) {
			if (y_I == ny-1) {
				Journal_Printf( stream, "-" );
			}
			else if (x_I==0) {
				Journal_Printf( stream, "|" );
			}	
			else {
				Journal_Printf( stream, "*" );
			}	
			for (ii=0;ii<14;ii++) Journal_Printf( stream, "-" );
		}
		if (y_I == ny-1) {
			Journal_Printf( stream, "-\n" );
		}	
		else {
			Journal_Printf( stream, "|\n" );
		}	


		/* Now a row of y values */
		Journal_Printf( stream, "yNode=%3d |", y_I );
		for ( x_I=0; x_I < nx; x_I++ ) {
		
			if ( ( y_I >= minLocalNodeY ) && ( y_I < maxLocalNodeY )
				&& ( x_I >= minLocalNodeX ) && ( x_I < maxLocalNodeX ) ) {

				inds[0] = x_I;
				inds[1] = y_I;
				node_lI = RegularMeshUtils_Node_3DTo1D( self->feMesh, inds );
				insist( Mesh_GlobalToDomain( self->feMesh, MT_VERTEX, node_lI, &node_lI ), == True );
				currNodeNumDofs = dofLayout->dofCounts[node_lI];

				if ( currNodeNumDofs == 1 ) {
					Journal_Printf( stream, "   " );
				}
				Journal_Printf( stream, "(" );
				for ( dof_I=0; dof_I < currNodeNumDofs - 1 ; dof_I++ ) {
					Journal_Printf( stream, "%5.2f,", DofLayout_GetValueDouble( dofLayout, node_lI, dof_I ) );
				}
				Journal_Printf( stream, "%5.2f )", DofLayout_GetValueDouble( dofLayout, node_lI, dof_I ) );
				
				if ( currNodeNumDofs == 1 ) {
					Journal_Printf( stream, "   " );
				}
				Journal_Printf( stream, "|" );
			}
			else {
				for (ii=0;ii<14;ii++) Journal_Printf( stream, "X" );
				Journal_Printf( stream, "|" );
			}
		}
		Journal_Printf( stream, "\n" );
	}
	
	/*Blocks */
	for (ii=0;ii<10;ii++) Journal_Printf( stream, " " );
	for ( x_I=0; x_I < nx; x_I++ ) {
		Journal_Printf( stream, "-" );
		for (ii=0;ii<14;ii++) Journal_Printf( stream, "-" );
	}
	Journal_Printf( stream, "-\n", x_I );
}


Bool FeVariable_InterpolateDerivativesAt( void* variable, double* globalCoord, double* value ) {
	FeVariable*	        self                 = (FeVariable*)variable;
	Element_DomainIndex	elementCoordIn       = (unsigned)-1;
	Coord               elLocalCoord         = {0,0,0};

	/* Need a special rule for points on this processor's boundary: instead of the normal
	   rule, "round" the point to lie inside the local space, rather than shadow */
	
	/* locate which mesh element given coord is in : use inclusive upper boundaries to save
		the need to use shadow space if possible */
	if ( !Mesh_Algorithms_SearchElements( self->feMesh->algorithms, globalCoord, 
					     &elementCoordIn ) )
	{
		/* If coord isn't inside domain elements list, bail out */
		return False;
	}	
	else /* We found the coord is within a local or shadow element */ {
		if ( elementCoordIn >= FeMesh_GetElementLocalSize( self->feMesh ) ) {
			if ( False == self->shadowValuesSynchronised ) {
				Stream* warningStr = Journal_Register( Error_Type, self->type );
				Journal_Printf( warningStr, "Warning - in %s: user asking to interpolate derivatives "
					"to coord (%g,%g,%g), which is in shadow space, but "
					"FeVariable_SyncShadowValues() hasn't been called yet.\n", 
					__func__, globalCoord[0], globalCoord[1], globalCoord[2] );
				return False;	
			}
		}

		/* convert global coordinate to local co-ordinates of element the coord is in */
		FeMesh_CoordGlobalToLocal( self->feMesh, elementCoordIn, globalCoord, elLocalCoord );

		/* Now interpolate the value at that coordinate, using shape functions */
		FeVariable_InterpolateDerivativesToElLocalCoord( self, elementCoordIn, elLocalCoord, value );
	}	
	
	return True;
}

void FeVariable_InterpolateDerivativesToElLocalCoord( void* _feVariable, Element_DomainIndex lElement_I, Coord elLocalCoord, double* value ) {
	FeVariable*    self             = (FeVariable*) _feVariable;
	ElementType*            elementType      = FeMesh_GetElementType( self->feMesh, lElement_I );
	Node_Index              elementNodeCount = elementType->nodeCount;
	double**                GNx; 
	double                  detJac;
	Dimension_Index         dim         = self->dim;

	GNx = self->GNx;

	/* Evaluate Global Shape Functions */
	ElementType_ShapeFunctionsGlobalDerivs( 
			elementType,
			self->feMesh, lElement_I,
			elLocalCoord, dim, &detJac, GNx );

	/* Do Interpolation */
	FeVariable_InterpolateDerivatives_WithGNx( self, lElement_I, GNx, value );
}

void FeVariable_InterpolateDerivatives_WithGNx( void* _feVariable, Element_LocalIndex lElement_I, double** GNx, double* value ) {
	FeVariable*             self        = (FeVariable*) _feVariable;
	Node_ElementLocalIndex  elLocalNode_I;
	Node_LocalIndex         lNode_I;
	Dof_Index               dof_I;
	Dof_Index               dofCount;
	Variable*               dofVariable;
	double                  nodeValue;
	unsigned		nInc, *inc;
	Dimension_Index         dim         = self->dim;

	/* Gets number of degrees of freedom - assuming it is the same throughout the mesh */
	dofCount = self->dofLayout->dofCounts[0];

	/* Initialise */
	memset( value, 0, sizeof( double ) * dofCount * dim );

	FeMesh_GetElementNodes( self->feMesh, lElement_I, self->inc );
	nInc = IArray_GetSize( self->inc );
	inc = IArray_GetPtr( self->inc );

	for ( dof_I = 0 ; dof_I < dofCount ; dof_I++ ) {
		/* Interpolate derivative from nodes */
		for ( elLocalNode_I = 0 ; elLocalNode_I < nInc ; elLocalNode_I++) {
			lNode_I      = inc[ elLocalNode_I ];
			dofVariable  = DofLayout_GetVariable( self->dofLayout, lNode_I, dof_I );
			nodeValue    = Variable_GetValueDouble( dofVariable, lNode_I );
			
			value[dof_I*dim + 0] += GNx[0][elLocalNode_I] * nodeValue;
			value[dof_I*dim + 1] += GNx[1][elLocalNode_I] * nodeValue;
			if( dim == 3 ) 
				value[dof_I*dim + 2] += GNx[2][elLocalNode_I] * nodeValue;	
		}
	}
}

void FeVariable_InterpolateValue_WithNi( void* _feVariable, Element_LocalIndex lElement_I, double* Ni, double* value ) {
	FeVariable*             self        = (FeVariable*) _feVariable;
	Node_ElementLocalIndex  elLocalNode_I;
	Node_LocalIndex         lNode_I;
	Dof_Index               dof_I;
	Dof_Index               dofCount;
	Variable*               dofVariable;
	double                  nodeValue;
	unsigned		nInc, *inc;

	/* Gets number of degrees of freedom - assuming it is the same throughout the mesh */
	dofCount = self->dofLayout->dofCounts[0];

	/* Initialise */
	memset( value, 0, sizeof( double ) * dofCount );

	FeMesh_GetElementNodes( self->feMesh, lElement_I, self->inc );
	nInc = IArray_GetSize( self->inc );
	inc = IArray_GetPtr( self->inc );

	for ( dof_I = 0 ; dof_I < dofCount ; dof_I++ ) {
		/* Interpolate derivative from nodes */
		for ( elLocalNode_I = 0 ; elLocalNode_I < nInc ; elLocalNode_I++) {
			lNode_I      = inc[ elLocalNode_I ];
			dofVariable  = DofLayout_GetVariable( self->dofLayout, lNode_I, dof_I );
			nodeValue    = Variable_GetValueDouble( dofVariable, lNode_I );

			value[dof_I] += Ni[elLocalNode_I] * nodeValue;
		}
	}
}

void FeVariable_GetMinimumSeparation( void* feVariable, double* minSeparationPtr, double minSeparationEachDim[3] ) {
	FeVariable*	self = (FeVariable*)feVariable;

	assert( self && Stg_CheckType( self, FeVariable ) );

	Mesh_GetMinimumSeparation( self->feMesh, minSeparationPtr, minSeparationEachDim );
}	


void _FeVariable_SyncShadowValues( void* feVariable ) {
	FeVariable*		self = (FeVariable*)feVariable;
	DofLayout*		dofLayout;
	Sync*			vertSync;
	unsigned		var_i;

	assert( self );

	/* Shortcuts. */
	dofLayout = self->dofLayout;
	if( !dofLayout ) {
		self->shadowValuesSynchronised = True;
		return;
	}

	/* Create a distributed array based on the mesh's vertices. */
	vertSync = Mesh_GetSync( self->feMesh, MT_VERTEX );

	/*
	** For each variable in the dof layout, we need to create a distributed array and update
	** shadow values.
	*/

	for( var_i = 0; var_i < dofLayout->_totalVarCount; var_i++ ) {
		unsigned	varInd;
		Variable*	var;
		unsigned	field_i;

		/* Get the variable. */
		varInd = dofLayout->_varIndicesMapping[var_i];
		var = Variable_Register_GetByIndex( dofLayout->_variableRegister, varInd );

		/* Each field of the variable will need to be handled individually. */
		for( field_i = 0; field_i < var->offsetCount; field_i++ ) {
			unsigned	offs, size;
			Stg_Byte	*arrayStart, *arrayEnd;

			offs = var->offsets[field_i];
			size = var->dataSizes[field_i];

			arrayStart = (Stg_Byte*)var->arrayPtr + offs;
			arrayEnd = arrayStart + var->structSize * FeMesh_GetNodeLocalSize( self->feMesh );
			Sync_SyncArray( vertSync, arrayStart, var->structSize, 
					arrayEnd, var->structSize, 
					size );
		}
	}

	self->shadowValuesSynchronised = True;

#if 0
	Neighbour_Index			nbr_I = 0;
	Node_Index			node_stI = 0;
	Node_DomainIndex		node_dI = 0;
	Node_LocalIndex			node_lI = 0;
	Dof_Index			nodalDof_I = 0;
	Processor_Index			nbrRank = 0;
	Index*				incomingDofTotals = NULL;			
	double**			incomingDofValues = NULL;
	MPI_Request**			incomingDofValRequests = NULL;
	Node_Index			myShadowNodesOnThisNbrCount = 0;
	Dof_Index		incomingDof_I;
	Index*			outgoingDofTotals = NULL;
	double**		outgoingDofValues = NULL;
	MPI_Request**		outgoingDofValRequests = NULL;
	Node_Index		nbrShadowNodesOnMeCount = 0;
	Dof_Index		outgoingDof_I;
	MPI_Status			status;
	int				incomingDofValueSetsYetToReceive;
	Bool*				incomingDofValueSetsReceived;

	Journal_DPrintf( self->debug, "In %s- for feVariable \"%s\":\n", __func__, self->name );
	Stream_Indent( self->debug );

	if ( ( 1 == mesh->layout->decomp->procsInUse ) || ( 0 == mesh->layout->decomp->shadowDepth ) ) {
		Journal_DPrintf( self->debug, "No shadow nodes: nothing to do - returning.\n" );
		Stream_UnIndent( self->debug );
		return;
	}

	self->shadowValuesSynchronised = True;

	/* allocate memory for incoming info */
	incomingDofTotals = Memory_Alloc_Array( Index, mesh->procNbrInfo->procNbrCnt, "incomingDofTotals" );
	incomingDofValues = Memory_Alloc_Array( double*, mesh->procNbrInfo->procNbrCnt, "incomingDofValues" );
	incomingDofValRequests = Memory_Alloc_Array( MPI_Request*, mesh->procNbrInfo->procNbrCnt, "incomingDofValRequests" );
	incomingDofValueSetsReceived = Memory_Alloc_Array( Bool, mesh->procNbrInfo->procNbrCnt, "incomingDofValueSets" );
	incomingDofValueSetsYetToReceive = 0;
	for ( nbr_I=0; nbr_I < mesh->procNbrInfo->procNbrCnt; nbr_I++ ) {
		myShadowNodesOnThisNbrCount = mesh->nodeShadowInfo->procShadowCnt[nbr_I];
		incomingDofTotals[nbr_I] = 0;
		incomingDofValues[nbr_I] = NULL;
		incomingDofValRequests[nbr_I] = NULL;
		incomingDofValueSetsReceived[nbr_I] = False;

		if ( myShadowNodesOnThisNbrCount > 0 ) {
			for( node_stI = 0; node_stI < myShadowNodesOnThisNbrCount; node_stI++ ) {
				node_dI = mesh->nodeShadowInfo->procShadowTbl[nbr_I][node_stI];
				incomingDofTotals[nbr_I] += self->dofLayout->dofCounts[node_dI];
			}	
			incomingDofValues[nbr_I] = Memory_Alloc_Array( double, incomingDofTotals[nbr_I],
				"incomingDofValues[]" );
			incomingDofValRequests[nbr_I] = Memory_Alloc( MPI_Request, "incomingDofValRequest" );	
			incomingDofValueSetsYetToReceive++;
		}
	}
	/* allocate memory for outgoing info */
	outgoingDofTotals = Memory_Alloc_Array( Index, mesh->procNbrInfo->procNbrCnt, "outgoingDofTotals" );
	outgoingDofValues = Memory_Alloc_Array( double*, mesh->procNbrInfo->procNbrCnt, "outgoingDofValues" );
	outgoingDofValRequests = Memory_Alloc_Array( MPI_Request*, mesh->procNbrInfo->procNbrCnt, "outgoingDofValRequests" );
	for ( nbr_I=0; nbr_I < mesh->procNbrInfo->procNbrCnt; nbr_I++ ) {
		nbrShadowNodesOnMeCount = mesh->nodeShadowInfo->procShadowedCnt[nbr_I];
		outgoingDofTotals[nbr_I] = 0;
		outgoingDofValues[nbr_I] = NULL;
		outgoingDofValRequests[nbr_I] = NULL;

		if ( nbrShadowNodesOnMeCount > 0 ) {
			for( node_stI = 0; node_stI < nbrShadowNodesOnMeCount; node_stI++ ) {
				node_lI = mesh->nodeShadowInfo->procShadowedTbl[nbr_I][node_stI];
				outgoingDofTotals[nbr_I] += self->dofLayout->dofCounts[node_lI];
			}	
			outgoingDofValues[nbr_I] = Memory_Alloc_Array( double, outgoingDofTotals[nbr_I],
				"outgoingDofValues[]" );
			outgoingDofValRequests[nbr_I] = Memory_Alloc( MPI_Request, "outgoingDofValRequest" );	
		}
	}	

	Journal_DPrintfL( self->debug, 2, "Starting non-blocking recv's of incoming values\n" );
	Stream_Indent( self->debug );
	for ( nbr_I=0; nbr_I < mesh->procNbrInfo->procNbrCnt; nbr_I++ ) {
		myShadowNodesOnThisNbrCount = mesh->nodeShadowInfo->procShadowCnt[nbr_I];
		if ( myShadowNodesOnThisNbrCount > 0 ) {
			nbrRank = mesh->procNbrInfo->procNbrTbl[nbr_I];
			Journal_DPrintfL( self->debug, 2, "Start recv from proc %u - %u values\n", nbrRank, incomingDofTotals[nbr_I] );
			MPI_Irecv( incomingDofValues[nbr_I], incomingDofTotals[nbr_I], MPI_DOUBLE, nbrRank,
				DOF_VALUES_TAG, self->communicator, incomingDofValRequests[nbr_I] );
		}
	}
	Stream_UnIndent( self->debug );

	Journal_DPrintfL( self->debug, 2, "Non-blocking send out required shadow values to neighbours\n" );
	Stream_Indent( self->debug );
	for ( nbr_I=0; nbr_I < mesh->procNbrInfo->procNbrCnt; nbr_I++ ) {
		if ( mesh->nodeShadowInfo->procShadowedCnt[nbr_I] > 0 ) {
			nbrRank = mesh->procNbrInfo->procNbrTbl[nbr_I];

			outgoingDof_I = 0;
			for( node_stI = 0; node_stI < mesh->nodeShadowInfo->procShadowedCnt[nbr_I]; node_stI++ ) {
				node_lI = mesh->nodeShadowInfo->procShadowedTbl[nbr_I][node_stI];
				for ( nodalDof_I=0; nodalDof_I < self->dofLayout->dofCounts[node_lI]; nodalDof_I++ ) {
					outgoingDofValues[nbr_I][outgoingDof_I] =
						DofLayout_GetValueDouble( self->dofLayout, node_lI, nodalDof_I );
					outgoingDof_I++;
				}
			}
			Journal_DPrintfL( self->debug, 2, "Start send to proc %u - %u values\n", nbrRank, outgoingDofTotals[nbr_I] );
			MPI_Isend( outgoingDofValues[nbr_I], outgoingDofTotals[nbr_I], MPI_DOUBLE, nbrRank,
				DOF_VALUES_TAG, self->communicator, outgoingDofValRequests[nbr_I] );
		}
	}
	Stream_UnIndent( self->debug );

	Journal_DPrintfL( self->debug, 2, "Receiving and updating shadow values I need from neighbours:\n" );
	Stream_Indent( self->debug );
	while ( incomingDofValueSetsYetToReceive > 0 ) {
		int	testFlag = 0;

		for ( nbr_I=0; nbr_I < mesh->procNbrInfo->procNbrCnt; nbr_I++ ) {

			if ( ( mesh->nodeShadowInfo->procShadowCnt[nbr_I] > 0 )
				&& ( False == incomingDofValueSetsReceived[nbr_I] ) )
			{
				MPI_Test( incomingDofValRequests[nbr_I], &testFlag, &status );
				if ( False == testFlag ) {
					continue;
				}
				else {
					Journal_DPrintfL( self->debug, 2, "Recv'd a batch of values from proc %u: updating...\n",
						nbrRank );
					/* update the appropriate values from recv'd set */
					incomingDof_I = 0;
					for( node_stI = 0; node_stI < mesh->nodeShadowInfo->procShadowCnt[nbr_I]; node_stI++ ) {
						node_dI = mesh->nodeShadowInfo->procShadowTbl[nbr_I][node_stI];
						for ( nodalDof_I=0; nodalDof_I < self->dofLayout->dofCounts[node_dI]; nodalDof_I++ ) {
							DofLayout_SetValueDouble( self->dofLayout, node_dI, nodalDof_I,
								incomingDofValues[nbr_I][incomingDof_I] );
							incomingDof_I++;	
						}
					}
					incomingDofValueSetsReceived[nbr_I] = True;
					incomingDofValueSetsYetToReceive--;
				}
			}	
		}	
	}
	Journal_DPrintfL( self->debug, 2, "Done.\n" );
	Stream_UnIndent( self->debug );

	Journal_DPrintfL( self->debug, 2, "Making sure outgoing sends have completed...\n" );
	for ( nbr_I=0; nbr_I < mesh->procNbrInfo->procNbrCnt; nbr_I++ ) {
		if ( mesh->nodeShadowInfo->procShadowCnt[nbr_I] > 0 ) {
			MPI_Wait( outgoingDofValRequests[nbr_I], &status );
		}
	}
	Journal_DPrintfL( self->debug, 2, "Done.\n" );

	/* clean up temporary memory */
	for ( nbr_I=0; nbr_I < mesh->procNbrInfo->procNbrCnt; nbr_I++ ) {
		myShadowNodesOnThisNbrCount = mesh->nodeShadowInfo->procShadowCnt[nbr_I];
		if ( myShadowNodesOnThisNbrCount > 0 ) {
			Memory_Free( incomingDofValues[nbr_I] );
			Memory_Free( incomingDofValRequests[nbr_I] );
		}	
	}
	Memory_Free( incomingDofTotals );
	Memory_Free( incomingDofValues );
	Memory_Free( incomingDofValRequests );
	for ( nbr_I=0; nbr_I < mesh->procNbrInfo->procNbrCnt; nbr_I++ ) {
		nbrShadowNodesOnMeCount = mesh->nodeShadowInfo->procShadowedCnt[nbr_I];
		if ( nbrShadowNodesOnMeCount > 0 ) {
			Memory_Free( outgoingDofValues[nbr_I] );
			Memory_Free( outgoingDofValRequests[nbr_I] );
		}	
	}
	Memory_Free( outgoingDofTotals );
	Memory_Free( outgoingDofValues );
	Memory_Free( outgoingDofValRequests );
	
	Stream_UnIndent( self->debug );
#endif
}


void FeVariable_PrintDomainDiscreteValues( void* variable, Stream* stream ) {
	FeVariable* self = (FeVariable*)variable;

	Journal_Printf( stream, "In %s: for FeVariable \"%s\":\n", __func__, self->name );

	_FeVariable_PrintLocalOrDomainValues( variable, FeMesh_GetNodeDomainSize( self->feMesh ), stream );
}

void FeVariable_PrintCoordsAndValues( void* _feVariable, Stream* stream ) {
	FeVariable*         self            = (FeVariable*) _feVariable;
	Node_LocalIndex     node_I          = 0;
	Node_LocalIndex     nodeLocalCount  = FeMesh_GetNodeLocalSize( self->feMesh );
	Dof_Index           currNodeNumDofs;
	Dof_Index           nodeLocalDof_I;
	Variable*           currVariable;
	double*             nodeCoord;
	
	/* Print Header of stream */
	Journal_Printf( stream, "# FeVariable - %s\n", self->name );
	Journal_Printf( stream, "#    x coord   |    y coord   |    z coord" );
	currNodeNumDofs = self->dofLayout->dofCounts[ 0 ];
	for ( nodeLocalDof_I = 0; nodeLocalDof_I < currNodeNumDofs; nodeLocalDof_I++ ) {
		currVariable = DofLayout_GetVariable( self->dofLayout, node_I, nodeLocalDof_I );
		Journal_Printf( stream, "  |  %s", currVariable->name );
	}
	Journal_Printf(stream, "\n");
	
	/* Loop over local nodes */
	for( node_I=0; node_I < nodeLocalCount ; node_I++ ) {
		currNodeNumDofs = self->dofLayout->dofCounts[ node_I ];

		/* Get Coordinate of Node */
		nodeCoord = Mesh_GetVertex( self->feMesh, node_I );
		Journal_Printf( stream, "%12.6g   %12.6g   %12.6g   ", 
				nodeCoord[ I_AXIS ], nodeCoord[ J_AXIS ], nodeCoord[ K_AXIS ] );
		
		/* Print each dof */
		for ( nodeLocalDof_I = 0; nodeLocalDof_I < currNodeNumDofs; nodeLocalDof_I++ ) {
			currVariable = DofLayout_GetVariable( self->dofLayout, node_I, nodeLocalDof_I );
			Journal_Printf( stream, "%12.6g   ", Variable_GetValueDouble( currVariable, node_I ) );
		}
		Journal_Printf( stream, "\n" );
	}	
}


/* --- Private Functions --- */

void _FeVariable_InterpolateNodeValuesToElLocalCoord( void* feVariable, Element_DomainIndex element_lI, Coord elLocalCoord, double* value ) {
	FeVariable*         self       = (FeVariable*) feVariable;
	ElementType*		elementType=NULL;
	Dof_Index		nodeLocalDof_I=0;
	Dof_Index		dofCountThisNode=0;
	Node_ElementLocalIndex	elLocalNode_I=0;
	double*			shapeFuncsEvaluatedAtCoord=NULL;
	Node_LocalIndex		lNode_I=0;
	Variable*		currVariable=NULL;
	double			dofValueAtCurrNode=0;
	unsigned		nInc, *inc;

	FeMesh_GetElementNodes( self->feMesh, element_lI, self->inc );
	nInc = IArray_GetSize( self->inc );
	inc = IArray_GetPtr( self->inc );

	/* Gets number of degrees of freedom - assuming it is the same throughout the mesh */
	dofCountThisNode = self->dofLayout->dofCounts[lNode_I];

	/* evaluate shape function values of current element at elLocalCoords */
	elementType = FeMesh_GetElementType( self->feMesh, element_lI );
	shapeFuncsEvaluatedAtCoord = AllocArray( double, nInc );
	ElementType_EvaluateShapeFunctionsAt( elementType, elLocalCoord, shapeFuncsEvaluatedAtCoord );

	for ( nodeLocalDof_I=0; nodeLocalDof_I < dofCountThisNode; nodeLocalDof_I++ ) {
		value[nodeLocalDof_I] = 0;
	}

	/* Now for each node, add that node's contribution at point */
	for ( elLocalNode_I=0; elLocalNode_I < nInc; elLocalNode_I++ ) {
		lNode_I = inc[elLocalNode_I];
		
		for ( nodeLocalDof_I=0; nodeLocalDof_I < dofCountThisNode; nodeLocalDof_I++ ) {
			currVariable = DofLayout_GetVariable( self->dofLayout, lNode_I, nodeLocalDof_I );
			dofValueAtCurrNode = Variable_GetValueDouble( currVariable, lNode_I );
			value[nodeLocalDof_I] += dofValueAtCurrNode * shapeFuncsEvaluatedAtCoord[elLocalNode_I];
		}	
	}
	FreeArray( shapeFuncsEvaluatedAtCoord );
}

void _FeVariable_PrintLocalOrDomainValues( void* variable, Index localOrDomainCount, Stream* stream ) {
	FeVariable* self = (FeVariable*)variable;
	Node_Index 		node_I = 0;
	Node_GlobalIndex 	gNode_I = 0;
	Dof_Index		currNodeNumDofs;
	Dof_Index		nodeLocalDof_I;
	Dof_EquationNumber	currEqNum;
	Variable*		currVariable;
	
	for( node_I=0; node_I < localOrDomainCount; node_I++ ) {
		gNode_I = FeMesh_NodeDomainToGlobal( self->feMesh, node_I );
		Journal_Printf( stream, "node %d (global index %d):\n", node_I, gNode_I );
		
		currNodeNumDofs = self->fieldComponentCount;
		
		
		/* Print each dof */
		for ( nodeLocalDof_I = 0; nodeLocalDof_I < currNodeNumDofs; nodeLocalDof_I++ ) {
			currVariable = DofLayout_GetVariable( self->dofLayout, node_I, nodeLocalDof_I );
			Journal_Printf( stream, "\tdof %d \"%s\": %6g - ", nodeLocalDof_I, currVariable->name,
				Variable_GetValueDouble( currVariable, node_I ) );
			currEqNum = self->eqNum->destinationArray[node_I][nodeLocalDof_I];
			if ( currEqNum == -1 ) {
				Journal_Printf( stream, "(from BC)", currEqNum );
			}
			else {
				Journal_Printf( stream, "(eq num %d)", currEqNum );
			}	
			Journal_Printf( stream, "\n", currEqNum );
		}
	}
}

void FeVariable_InterpolateFromMeshLocalCoord( void* feVariable, FeMesh* mesh, Element_DomainIndex dElement_I, double* localCoord, double* value ) {
	FeVariable*          self               = (FeVariable*)         feVariable;

	if ( mesh == self->feMesh ) {
		/* If the meshes are identical - then we can just interpolate within the elements because the elements are the same */
		FeVariable_InterpolateWithinElement( self, dElement_I, localCoord, value );
	}
	else {
		Coord               globalCoord;

		/* If the meshes are different - then we must find the global coordinates and interpolate to that */
		FeMesh_CoordLocalToGlobal( mesh, dElement_I, localCoord, globalCoord );
		FieldVariable_InterpolateValueAt( feVariable, globalCoord, value );
	}
	
}

/* TODO: can't assume all swarms have particles of type integrationPoint anymore.
   should check that the given swarm does have I.P for the rest of these functions.*/
double FeVariable_IntegrateElement_AxisIndependent( 
		void* feVariable, void* _swarm, 
		Element_DomainIndex dElement_I, Dimension_Index dim, 
		Axis axis0, Axis axis1, Axis axis2 ) 
{
	FeVariable*          self               = (FeVariable*)         feVariable;
	Swarm*               swarm              = (Swarm*)              _swarm;
	FeMesh*			feMesh             = self->feMesh;
	FeMesh*			mesh;
	ElementType*         elementType;
	Cell_LocalIndex      cell_I;
	Particle_InCellIndex cParticle_I;
	Particle_InCellIndex cellParticleCount;
	IntegrationPoint*    particle;
	double               detJac;
	double               integral;
	double               value;
	
	/* Initialise Summation of Integral */
	integral = 0.0;

	/* Use feVariable's mesh as geometry mesh if one isn't passed in */
	if( Stg_Class_IsInstance( feMesh->algorithms, Mesh_CentroidAlgorithms_Type ) )
		mesh = (FeMesh*)((Mesh_CentroidAlgorithms*)feMesh->algorithms)->elMesh;
	else
		mesh = feMesh;
	elementType = FeMesh_GetElementType( mesh, dElement_I );

	/* Determine number of particles in element */
	cell_I = CellLayout_MapElementIdToCellId( swarm->cellLayout, dElement_I );
	cellParticleCount = swarm->cellParticleCountTbl[ cell_I ];

	/* Loop over all particles in element */
	for( cParticle_I = 0 ; cParticle_I < cellParticleCount ; cParticle_I++ ) {
		/* Get Pointer to particle */
		particle = (IntegrationPoint*) Swarm_ParticleInCellAt( swarm, cell_I, cParticle_I );

		/* Interpolate Value of Field at Particle */
		FeVariable_InterpolateWithinElement( feVariable, dElement_I, particle->xi, &value );

		Journal_DPrintfL( self->debug, 3, "%s: Integrating element %d - particle %d - Value = %g\n", self->name, dElement_I, cParticle_I, value );

		/* Calculate Determinant of Jacobian */
		detJac = ElementType_JacobianDeterminant_AxisIndependent( 
				elementType, mesh, dElement_I, particle->xi, dim, axis0, axis1, axis2 );

		/* Sum Integral */
		integral += detJac * particle->weight * value;
	}
	
	return integral;
}

double FeVariable_Integrate( void* feVariable, void* _swarm ) {
	FeVariable*          self               = (FeVariable*)         feVariable;
	Swarm*               swarm              = (Swarm*)              _swarm;
	FeMesh*			feMesh             = self->feMesh;
	Element_LocalIndex   lElement_I;
	Element_LocalIndex   elementLocalCount  = FeMesh_GetElementLocalSize( feMesh );
	double               integral, integralGlobal;
	
	/* Initialise Summation of Integral */
	integral = 0.0;

	/* Loop over all local elements */
	for ( lElement_I = 0 ; lElement_I < elementLocalCount ; lElement_I++ ) {
		integral += FeVariable_IntegrateElement( self, swarm, lElement_I );
		Journal_DPrintfL( self->debug, 3, "%s: Integrating element %d - Accumulated Integral = %g\n", self->name, lElement_I, integral );
	}
		
	/* Gather and sum integrals from other processors */
	MPI_Allreduce( &integral, &integralGlobal, 1, MPI_DOUBLE, MPI_SUM, self->communicator );

	return integralGlobal;
}

double FeVariable_AverageTopLayer( void* feVariable, void* swarm, Axis layerAxis ) {
	FeVariable*                self               = (FeVariable*)         feVariable;
	Grid*			elGrid;

	elGrid = *(Grid**)ExtensionManager_Get( self->feMesh->info, self->feMesh, 
						ExtensionManager_GetHandle( self->feMesh->info, "elementGrid" ) );

	return FeVariable_AverageLayer( self, swarm, layerAxis, elGrid->sizes[1] - 1 );
}

double FeVariable_AverageBottomLayer( void* feVariable, void* swarm, Axis layerAxis ) {
	FeVariable*                self               = (FeVariable*)         feVariable;

	return FeVariable_AverageLayer( self, swarm, layerAxis, 0 );
}

double FeVariable_AverageLayer( void* feVariable, void* swarm, Axis layerAxis, Index layerIndex ) {
	FeVariable*                self               = (FeVariable*)         feVariable;
	Axis                       aAxis              = ( layerAxis == I_AXIS ? J_AXIS : I_AXIS );
	Axis                       bAxis              = ( layerAxis == K_AXIS ? J_AXIS : K_AXIS );
	Dimension_Index            dim                = self->dim;
	double                     integral;
	double                     layerThickness     = 0.0;
	double			sendThickness;
	Grid*			vertGrid;
	unsigned*		inds;
	double			heights[2];
	unsigned		localInd[2], globalInd[2];
	double			*min, *max;
	unsigned		d_i;
	
	integral = FeVariable_IntegrateLayer( self, swarm, layerAxis, layerIndex );

	/* Calculate layer thickness.  This assumes the mesh is regular. */
	vertGrid = *(Grid**)ExtensionManager_Get( self->feMesh->info, self->feMesh, 
						  ExtensionManager_GetHandle( self->feMesh->info, "vertexGrid" ) );
	inds = Memory_Alloc_Array_Unnamed( unsigned, Mesh_GetDimSize( self->feMesh ) );
	for( d_i = 0; d_i < Mesh_GetDimSize( self->feMesh ); d_i++ ) {
		if( d_i != layerAxis )
			inds[d_i] = 0;
		else
			inds[d_i] = layerIndex;
	}
	globalInd[0] = Grid_Project( vertGrid, inds );
	inds[layerAxis]++;
	globalInd[1] = Grid_Project( vertGrid, inds );
	if( Mesh_GlobalToDomain( self->feMesh, MT_VERTEX, globalInd[0], &localInd[0] ) && 
	    Mesh_GlobalToDomain( self->feMesh, MT_VERTEX, globalInd[1], &localInd[1] ) )
	{
		heights[0] = Mesh_GetVertex( self->feMesh, localInd[0] )[layerAxis];
		heights[1] = Mesh_GetVertex( self->feMesh, localInd[1] )[layerAxis];
		sendThickness = heights[1] - heights[0];
	}
	else {
		sendThickness = 0.0;
	}
	MPI_Allreduce( &sendThickness, &layerThickness, 1, MPI_DOUBLE, MPI_MAX, self->communicator );
	FreeArray( inds );

	min = Memory_Alloc_Array_Unnamed( double, Mesh_GetDimSize( self->feMesh ) );
	max = Memory_Alloc_Array_Unnamed( double, Mesh_GetDimSize( self->feMesh ) );
	Mesh_GetGlobalCoordRange( self->feMesh, min, max );
	integral /= layerThickness * (max[aAxis] - min[aAxis]);
	if ( dim == 3 )
		integral /= max[ bAxis ] - min[ bAxis ];
	FreeArray( min );
	FreeArray( max );

	return integral;
}


double FeVariable_IntegrateLayer_AxisIndependent( 
		void* feVariable, void* _swarm,
		Axis layerAxis, Index layerIndex, Dimension_Index dim, 
		Axis axis0, Axis axis1, Axis axis2 ) 
{ 
	FeVariable*                self               = (FeVariable*)         feVariable;
	Swarm*                     swarm              = (Swarm*)              _swarm;
	Element_LocalIndex         lElement_I;
	Element_GlobalIndex        gElement_I;
	IJK                        elementIJK;
	double                     elementIntegral;
	double                     integral;
	double                     integralGlobal;

	Journal_DPrintf( self->debug, "In %s() for FeVariable \"%s\":\n", __func__, self->name );

	/* Initialise Sumation of Integral */
	integral = 0.0;

	Stream_Indent( self->debug );
	for ( gElement_I = 0 ; gElement_I < FeMesh_GetElementGlobalSize( self->feMesh ); gElement_I++ ) {
		RegularMeshUtils_Element_1DTo3D( self->feMesh, gElement_I, elementIJK );

		/* Check if element is in layer plane */
		if ( elementIJK[ layerAxis ] != layerIndex )
			continue;

		/* Check if element is local */
		if( !FeMesh_ElementGlobalToDomain( self->feMesh, gElement_I, &lElement_I ) || 
		    lElement_I >= FeMesh_GetElementLocalSize( self->feMesh ) )
		{
			continue;
		}

		elementIntegral = FeVariable_IntegrateElement_AxisIndependent( self, swarm, lElement_I, dim, axis0, axis1, axis2 );
		Journal_DPrintfL( self->debug, 2, "Integral of element %d was %f\n", lElement_I, elementIntegral );
		integral += elementIntegral;
	}
	Stream_UnIndent( self->debug );


	/* Gather and sum integrals from other processors */
	MPI_Allreduce( &integral, &integralGlobal, 1, MPI_DOUBLE, MPI_SUM, self->communicator );

	Journal_DPrintf( self->debug, "Calculated global integral of layer %d in Axis %d was %f\n", layerIndex, layerAxis, integralGlobal );
	return integralGlobal;
}


double FeVariable_AveragePlane( void* feVariable, Axis planeAxis, double planeHeight ) {
	FeVariable*                self               = (FeVariable*)         feVariable;
	double                     integral;
	Axis                       aAxis              = ( planeAxis == I_AXIS ? J_AXIS : I_AXIS );
	Axis                       bAxis              = ( planeAxis == K_AXIS ? J_AXIS : K_AXIS );
	Dimension_Index            dim                = self->dim;
	double				min[3], max[3];
	
	integral = FeVariable_IntegratePlane( self, planeAxis, planeHeight );

	Mesh_GetGlobalCoordRange( self->feMesh, min, max );

	integral /= max[ aAxis ] - min[ aAxis ];
	if ( dim == 3 )
		integral /= max[ bAxis ] - min[ bAxis ];

	return integral;
}

double FeVariable_IntegratePlane( void* feVariable, Axis planeAxis, double planeHeight ) {
	FeVariable*                self               = (FeVariable*)         feVariable;
	IJK                        planeIJK;
	Element_LocalIndex         lElement_I;
	Element_GlobalIndex        gElement_I;
	Element_LocalIndex         elementLocalCount  = FeMesh_GetElementLocalSize( self->feMesh );
	Axis                       aAxis              = ( planeAxis == I_AXIS ? J_AXIS : I_AXIS );
	Axis                       bAxis              = ( planeAxis == K_AXIS ? J_AXIS : K_AXIS );
	double                     integral;
	/* Swarm Stuff */
	Swarm*                     tmpSwarm;
	Bool                       dimExists[]        = { False, False, False };
	ExtensionManager_Register* extensionMgr_Register;
	SingleCellLayout*          singleCellLayout;
	GaussParticleLayout*       gaussParticleLayout;
	Particle_Index             lParticle_I;
	IntegrationPoint*          particle;
	/* Plane location stuff */
	double                     storedXi_J_AXIS;
	Coord                      planeCoord;
	double                     planeXi           = -1;
	double                     planeXiGlobal;
	Index                      planeLayer        = 0;
	Index                      planeLayerGlobal;
	Particle_InCellIndex       particlesPerDim[] = {2,2,2};

	/* Find Elements which plane cuts through */
	memcpy( planeCoord, Mesh_GetVertex( self->feMesh, 0 ), sizeof( Coord ) );
	planeCoord[ planeAxis ] = planeHeight;

	if( Mesh_Algorithms_SearchElements( self->feMesh->algorithms, planeCoord, &lElement_I ) && 
	    lElement_I < elementLocalCount )
	{
		Coord		planeXiCoord;

		gElement_I = FeMesh_ElementDomainToGlobal( self->feMesh, lElement_I );
		RegularMeshUtils_Element_1DTo3D( self->feMesh, gElement_I, planeIJK );
		planeLayer = planeIJK[ planeAxis ];
		
		/* Find Local Coordinate of plane */
		FeMesh_CoordGlobalToLocal( self->feMesh, lElement_I, planeCoord, planeXiCoord );
		planeXi = planeXiCoord[ planeAxis ];
	}
	
	/* Should be broadcast */
	MPI_Allreduce( &planeXi,    &planeXiGlobal, 1, MPI_DOUBLE, MPI_MAX, self->communicator );
	MPI_Allreduce( &planeLayer, &planeLayerGlobal, 1, MPI_UNSIGNED, MPI_MAX, self->communicator );

	/* Create Swarm in plane */
	extensionMgr_Register = ExtensionManager_Register_New();
	dimExists[ aAxis ] = True;
	if (self->dim == 3)
		dimExists[ bAxis ] = True;
	
	singleCellLayout = SingleCellLayout_New( "cellLayout", dimExists, NULL, NULL );
	particlesPerDim[ planeAxis ] = 1;
	gaussParticleLayout = GaussParticleLayout_New( "particleLayout", self->dim - 1, particlesPerDim );
	tmpSwarm = Swarm_New( 
			"tmpgaussSwarm",
			singleCellLayout, 
			gaussParticleLayout,
			self->dim,
			sizeof(IntegrationPoint), 
			extensionMgr_Register, 
			NULL,
			self->communicator,
		        NULL	);
	Stg_Component_Build( tmpSwarm, NULL, False );

	/* Change Positions of the particles */
	Stg_Component_Initialise( tmpSwarm, NULL, False );
	for ( lParticle_I = 0 ; lParticle_I < tmpSwarm->particleLocalCount ; lParticle_I++ ) {
		particle = (IntegrationPoint*) Swarm_ParticleAt( tmpSwarm, lParticle_I );

		storedXi_J_AXIS = particle->xi[ J_AXIS ];
		particle->xi[ aAxis ]     = particle->xi[ I_AXIS ];
		particle->xi[ bAxis ]     = storedXi_J_AXIS;
		particle->xi[ planeAxis ] = planeXiGlobal;
	}
	
	integral = FeVariable_IntegrateLayer_AxisIndependent( self, tmpSwarm, planeAxis, planeLayerGlobal, 
			self->dim - 1, aAxis, bAxis, planeAxis );

	/* Delete */
	Stg_Class_Delete( tmpSwarm );
	Stg_Class_Delete( gaussParticleLayout );
	Stg_Class_Delete( singleCellLayout );
	Stg_Class_Delete( extensionMgr_Register );
	
	return integral;
}


void FeVariable_ImportExportInfo_Delete( void* ptr ) {
	/* Nothing to do - the ObjectAdaptor will take care of deleting the actual struct itself */
}

void FeVariable_SaveToFile( void* feVariable, const char* filename, Bool saveCoords ) {
	FeVariable*       self = (FeVariable*)feVariable;
	Node_LocalIndex   lNode_I = 0;
	Node_GlobalIndex  gNode_I = 0;
	double*           coord;
	Dof_Index         dof_I;
	Dof_Index         dofAtEachNodeCount;
	double            variableValues[MAX_FIELD_COMPONENTS];	
	MPI_Comm	         comm = Comm_GetMPIComm( Mesh_GetCommTopology( self->feMesh, MT_VERTEX ) );
	int               myRank;
	int               nProcs;
	MPI_Status        status;
   Stream*           errorStr = Journal_Register( Error_Type, self->type );
   const int         FINISHED_WRITING_TAG = 100;
   int               confirmation = 0;   
   
#ifdef WRITE_HDF5
   hid_t             file, fileSpace, fileData;
   hid_t             memSpace;
   hsize_t           start[2], count[2], size[2];
   double*           buf;
#else
   FILE*             outputFile;
#endif 

	MPI_Comm_size( comm, (int*)&nProcs );
	MPI_Comm_rank( comm, (int*)&myRank );
	
	/* Note: assumes same number of dofs at each node */
	dofAtEachNodeCount = self->fieldComponentCount;
	
#ifdef WRITE_HDF5
	/* wait for go-ahead from process ranked lower than me, to avoid competition writing to file */
	if ( myRank != 0 ) {
		MPI_Recv( &confirmation, 1, MPI_INT, myRank - 1, FINISHED_WRITING_TAG, comm, &status );
	}	

   /* Open the file */
	if ( myRank == 0 ) {
      /* Open the HDF5 output file. */
      file = H5Fcreate( filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );
      Journal_Firewall( 
         file >= 0, 
         errorStr,
         "Error in %s for %s '%s' - Cannot create file %s.\n", 
         __func__, 
         self->type, 
         self->name, 
         filename );
       
      size[0] = Mesh_GetGlobalSize( self->feMesh, 0 );;
      size[1] = dofAtEachNodeCount;
      if( saveCoords )
         size[1] += self->dim;
      
      /* Create filespace */   
      fileSpace = H5Screate_simple( 2, size, NULL );         
      #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
      fileData  = H5Dcreate( file, "/data", H5T_NATIVE_DOUBLE, fileSpace, H5P_DEFAULT );
      #else
      fileData  = H5Dcreate( file, "/data", H5T_NATIVE_DOUBLE, fileSpace,
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

      /* get the filespace */   
      #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
      fileData  = H5Dopen( file, "/data" );
      #else
      fileData  = H5Dopen( file, "/data", H5P_DEFAULT );
      #endif
      /* get the filespace handle */
      fileSpace = H5Dget_space(fileData);
	}	
	
   /* get the section of fileSpace to write to... set start point to be the
      global index of first local node */
	count[0] = 1;
	count[1] = dofAtEachNodeCount;
   if( saveCoords )
      count[1] += self->dim;

   /* create memSpace */
   memSpace = H5Screate_simple( 2, count, NULL );
   H5Sselect_all( memSpace );

   buf = Memory_Alloc_Array( double, count[1], "fileBuffer" );
   
   for ( lNode_I = 0; lNode_I < FeMesh_GetNodeLocalSize( self->feMesh ); lNode_I++ ) {
		gNode_I = FeMesh_NodeDomainToGlobal( self->feMesh, lNode_I );

	   /* If required, add coords to array */
	   if( saveCoords ) {
	      coord = Mesh_GetVertex( self->feMesh, lNode_I );
	      buf[0] = coord[0];
	      buf[1] = coord[1];
	      if( self->dim == 3 )
	         buf[2] = coord[2]; 
	   }
	   
	   /* Add field value at current node to array */
      FeVariable_GetValueAtNode( self, lNode_I, variableValues );
		for ( dof_I = 0; dof_I < dofAtEachNodeCount; dof_I++ ) {
		   if( saveCoords )
			   buf[dof_I + self->dim] = variableValues[dof_I];
			else
			   buf[dof_I] = variableValues[dof_I];   
		}	

      /* select the region of dataspace to write to  */
      start[0] = FeMesh_NodeDomainToGlobal( self->feMesh, lNode_I );
      start[1] = 0;
      H5Sselect_hyperslab( fileSpace, H5S_SELECT_SET, start, NULL, count, NULL );
      /* Write the array to file */
      H5Dwrite( fileData, H5T_NATIVE_DOUBLE, memSpace, fileSpace, H5P_DEFAULT, buf );
   }   
   /* free memory! */
   Memory_Free( buf );
   
   /* Close all handles */
   H5Dclose( fileData );
   H5Sclose( memSpace );
   H5Sclose( fileSpace );
   H5Fclose( file );
   
	/* send go-ahead to process ranked above me, to avoid competition writing to file */
	if ( myRank != nProcs - 1 ) {
		MPI_Ssend( &confirmation, 1, MPI_INT, myRank + 1, FINISHED_WRITING_TAG, comm );
	}	

#else
	/* wait for go-ahead from process ranked lower than me, to avoid competition writing to file */
	if ( myRank != 0 ) {
		MPI_Recv( &confirmation, 1, MPI_INT, myRank - 1, FINISHED_WRITING_TAG, comm, &status );
	}	

   /* Open the file */
	if ( myRank == 0 ) {
		outputFile = fopen( filename, "w" );
	}
	else {
		outputFile = fopen( filename, "a" );
	}	
	
	Journal_Firewall( 
		outputFile != NULL, 
		errorStr,
		"Error in %s for %s '%s' - Cannot create file %s.\n", 
		__func__, 
		self->type, 
		self->name, 
		filename );
		
	for ( lNode_I = 0; lNode_I < FeMesh_GetNodeLocalSize( self->feMesh ); lNode_I++ ) {
		gNode_I = FeMesh_NodeDomainToGlobal( self->feMesh, lNode_I );
		fprintf( outputFile, "%u ", gNode_I );
		
		/* If required, write the node coords to file */		
      if( saveCoords ) {
		   coord = Mesh_GetVertex( self->feMesh, lNode_I );
         if(self->dim == 2)
            fprintf( outputFile, "%.15g %.15g 0 ", coord[0], coord[1]);
         else
            fprintf( outputFile, "%.15g %.15g %.15g ", coord[0], coord[1], coord[2] );
      }
		
		/* Add field value at current node to the file */            
		FeVariable_GetValueAtNode( self, lNode_I, variableValues );
		for ( dof_I = 0; dof_I < dofAtEachNodeCount; dof_I++ ) {
			fprintf( outputFile, "%.15g ", variableValues[dof_I] );
		}	
		fprintf( outputFile, "\n" );
	}
	
	/* Close the file */
	fclose( outputFile );
	
   /* send go-ahead to process ranked above me, to avoid competition writing to file */
	if ( myRank != nProcs - 1 ) {
		MPI_Ssend( &confirmation, 1, MPI_INT, myRank + 1, FINISHED_WRITING_TAG, comm );
	}	

#endif
}


#define MAX_LINE_LENGTH_DEFINE 1024

void FeVariable_ReadFromFile( void* feVariable, const char* filename ) {
	FeVariable*        self = (FeVariable*)feVariable;
	Node_LocalIndex    lNode_I = 0;
	Node_GlobalIndex   gNode_I = 0;
	Dof_Index          dof_I;
	Dof_Index          dofAtEachNodeCount;
	FILE*              inputFile;
	double             variableVal;
	Processor_Index    proc_I=0;
	MPI_Comm	          comm = Comm_GetMPIComm( Mesh_GetCommTopology( self->feMesh, MT_VERTEX ) );
	unsigned		       rank;
	unsigned		       nRanks;
	int                nDims;
	Bool               savedCoords = False;
	Stream*            errorStr = Journal_Register( Error_Type, self->type );
	
#ifdef READ_HDF5
   hid_t             file, fileSpace, fileData, error;
   unsigned          totalNodes, ii;
   hid_t             memSpace;
   hsize_t           start[2], count[2], size[2], maxSize[2];
   double*           buf; 
#else
   unsigned          uTemp;  
   double            temp[3];  
   int               count = 0;     
   char               lineString[MAX_LINE_LENGTH_DEFINE];
	const unsigned int MAX_LINE_LENGTH = MAX_LINE_LENGTH_DEFINE;
   int               offset, n;    
#endif   	

	MPI_Comm_rank( comm, (int*)&rank );
	MPI_Comm_size( comm, (int*)&nRanks );
	
	dofAtEachNodeCount = self->fieldComponentCount;
	
#ifdef READ_HDF5	
   
   /* Open the file and data set. */
	file = H5Fopen( filename, H5F_ACC_RDONLY, H5P_DEFAULT );
	
	Journal_Firewall( 
		file >= 0, 
		errorStr,
		"Error in %s for %s '%s' - Cannot open file %s.\n", 
		__func__, 
		self->type, 
		self->name, 
		filename );
		
	/* Prepare to read vertices from file */		
   #if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 8
	fileData = H5Dopen( file, "/data" );
   #else
	fileData = H5Dopen( file, "/data", H5P_DEFAULT );
   #endif
	fileSpace = H5Dget_space( fileData );
   
   /* Get size of dataspace to determine if coords are in file */
   H5Sget_simple_extent_dims( fileSpace, size, maxSize ); 

   if( maxSize[1] > dofAtEachNodeCount + 1 ) 
      savedCoords = True;
       
   start[1] = 0;
	count[0] = 1;
	count[1] = dofAtEachNodeCount + 1;
	if( savedCoords )  
	   count[1] += self->dim;
	   
   memSpace = H5Screate_simple( 2, count, NULL );
   totalNodes = Mesh_GetGlobalSize( self->feMesh, 0 );
   buf = Memory_Alloc_Array( double, count[1], "fileBuffer" );

   Journal_Firewall( 
      (maxSize[0] == totalNodes), 
      errorStr,
      "\n\nError in %s for %s '%s'\n"
      "Number of node values (%u) stored in %s does not correspond to total number of requested mesh nodes (%u).\n", 
      __func__, 
      self->type, 
      self->name, 
      (unsigned int)maxSize[0],
      filename,
      totalNodes);
   
         
   /* Read from HDF5 checkpint file */
   for( ii=0; ii<totalNodes; ii++ ) {   
	   start[0] = ii;
               
      H5Sselect_hyperslab( fileSpace, H5S_SELECT_SET, start, NULL, count, NULL );
      H5Sselect_all( memSpace );
         
      error = H5Dread( fileData, H5T_NATIVE_DOUBLE, memSpace, fileSpace, H5P_DEFAULT, buf );
      gNode_I = (int)buf[0];

      Journal_Firewall( 
         error >= 0, 
         errorStr,
         "\n\nError in %s for %s '%s' - Cannot read data in %s.\n", 
         __func__, 
         self->type, 
         self->name, 
         filename );
      
      if( Mesh_GlobalToDomain( self->feMesh, MT_VERTEX, gNode_I, &lNode_I ) && 
		   lNode_I < Mesh_GetLocalSize( self->feMesh, MT_VERTEX ) )
		{
		   for ( dof_I = 0; dof_I < dofAtEachNodeCount; dof_I++ ) {
		      if( savedCoords )
		         variableVal = buf[dof_I + self->dim + 1];
		      else
				   variableVal = buf[dof_I + 1];
				DofLayout_SetValueDouble( self->dofLayout, lNode_I, dof_I, variableVal );
			}  
		}
	}   
   Memory_Free( buf );
   
#else   

	/* This loop used to stop 2 processors trying to open the file at the same time, which
	  * seems to cause problems */
	for ( proc_I = 0; proc_I < nRanks; proc_I++ ) {
		MPI_Barrier( comm );
		if ( proc_I == rank ) {
			/* Do the following since in parallel on some systems, the file
			 * doesn't get re-opened at the start automatically. */
			inputFile = fopen( filename, "r" );

			if ( False == inputFile ) {
				Journal_Firewall( 0, errorStr, "Error- in %s(), for feVariable \"%s\": Couldn't import from file: "
					"\"%s\" - aborting.\n", __func__, self->name, filename );
					
			}
			rewind( inputFile );
	
		}
	}
   
   /* Need to determine whether checkpoint file contains coordinates */
   fgets( lineString, MAX_LINE_LENGTH, inputFile );
   
   sscanf( lineString, "%u%n ", &uTemp, &offset );
   while( sscanf( lineString + offset, "%lf%n ", &temp[0], &n ) )
   {
      offset += n;
      count ++;
      if( offset >= strlen( lineString ) - 2 )
         break;
   }   
 
   if( count > dofAtEachNodeCount + 1 ) 
      savedCoords = True;
    
   rewind( inputFile );
      
	/* Need to re-set the geometry here, in case we're loading from a checkpoint that had compression/squashing BCs,
		and hence ended up with a smaller mesh than the original */
	nDims = Mesh_GetDimSize( self->feMesh );
	while ( !feof(inputFile) ) {
		fscanf( inputFile, "%u ", &gNode_I );
		
		if( savedCoords ) 
		   fscanf( inputFile, "%lg %lg %lg ", &temp[0], &temp[1], &temp[2] );
		   
		if( FeMesh_NodeGlobalToDomain( self->feMesh, gNode_I, &lNode_I ) && 
		    lNode_I < FeMesh_GetNodeLocalSize( self->feMesh ) )
		{
			for ( dof_I = 0; dof_I < dofAtEachNodeCount; dof_I++ ) {
				fscanf( inputFile, "%lg ", &variableVal );
				DofLayout_SetValueDouble( self->dofLayout, lNode_I, dof_I, variableVal );
			}
		}
		else {
			fgets( lineString, MAX_LINE_LENGTH, inputFile );
		}
	}
	fclose( inputFile );
#endif	

	Mesh_DeformationUpdate( self->feMesh );

	/* Sync shadow values now, as all procs should have required input */
	FeVariable_SyncShadowValues( self );
}
