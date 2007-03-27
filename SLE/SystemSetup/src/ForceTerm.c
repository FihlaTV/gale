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
** $Id: ForceTerm.c 656 2006-10-18 06:45:50Z SteveQuenette $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <mpi.h>
#include <StGermain/StGermain.h>
#include "StgFEM/Discretisation/Discretisation.h"
#include "StgFEM/SLE/LinearAlgebra/LinearAlgebra.h"

#include "units.h"
#include "types.h"
#include "shortcuts.h"
#include "ForceTerm.h"
#include "SolutionVector.h"
#include "ForceVector.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "EntryPoint.h"

/* Textual name of this class */
const Type ForceTerm_Type = "ForceTerm";

ForceTerm* ForceTerm_New(
		Name                                      name,
		ForceVector*                              forceVector,
		Swarm*                                    integrationSwarm,
		Stg_Component*                            extraInfo )		
{
	ForceTerm* self = (ForceTerm*) _ForceTerm_DefaultNew( name );

	ForceTerm_InitAll( self, forceVector, integrationSwarm, extraInfo );

	return self;
}

ForceTerm* _ForceTerm_New( 
		SizeT                                     _sizeOfSelf,
		Type                                      type,
		Stg_Class_DeleteFunction*                 _delete,
		Stg_Class_PrintFunction*                  _print,
		Stg_Class_CopyFunction*                   _copy, 
		Stg_Component_DefaultConstructorFunction* _defaultConstructor,
		Stg_Component_ConstructFunction*          _construct,
		Stg_Component_BuildFunction*              _build,
		Stg_Component_InitialiseFunction*         _initialise,
		Stg_Component_ExecuteFunction*            _execute,
		Stg_Component_DestroyFunction*            _destroy,
		ForceTerm_AssembleElementFunction*         _assembleElement,
		Name                                      name )
{
	ForceTerm*		self;
	
	/* Allocate memory */
	assert( _sizeOfSelf >= sizeof(ForceTerm) );
	self = (ForceTerm*)_Stg_Component_New( 
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
			NON_GLOBAL );

	self->_assembleElement = _assembleElement;
	
	return self;
}


void _ForceTerm_Init(
		void*                                     forceTerm,
		ForceVector*                              forceVector,
		Swarm*                                    integrationSwarm,
		Stg_Component*                            extraInfo )
{
	ForceTerm* self = (ForceTerm*)  forceTerm;
	
	self->isConstructed    = True;

	self->debug            = Stream_RegisterChild( StgFEM_SLE_SystemSetup_Debug, self->type );
	self->extraInfo        = extraInfo;	
	self->integrationSwarm = integrationSwarm;	

	ForceVector_AddForceTerm( forceVector, self );
}

void ForceTerm_InitAll(
		void*                                     forceTerm,
		ForceVector*                              forceVector,
		Swarm*                                    integrationSwarm,
		Stg_Component*                            extraInfo )
{
	ForceTerm* self = (ForceTerm*)forceTerm;
	_ForceTerm_Init( self, forceVector, integrationSwarm, extraInfo );
}


void _ForceTerm_Delete( void* forceTerm ) {
	ForceTerm* self = (ForceTerm*)forceTerm;
	
	Journal_DPrintf( self->debug, "In %s - for %s\n", __func__, self->name );

	/* Stg_Class_Delete parent*/
	_Stg_Component_Delete( self );
}


void _ForceTerm_Print( void* forceTerm, Stream* stream ) {
	ForceTerm* self = (ForceTerm*)forceTerm;

	/* General info */
	Journal_Printf( stream, "ForceTerm (ptr): %p\n", self );
	
	/* Print parent */
	_Stg_Component_Print( self, stream );
	
	/* ForceTerm info */
	Journal_Printf( stream, "\tintegrationSwarm (ptr): %p\n", self->integrationSwarm );
	Journal_Printf( stream, "\textraInfo (ptr): %p\n", self->extraInfo );
}


void* _ForceTerm_Copy( void* forceTerm, void* dest, Bool deep, Name nameExt, PtrMap* ptrMap ) {
	ForceTerm*	self = (ForceTerm*)forceTerm;
	ForceTerm*	newForceTerm;
	PtrMap*		map = ptrMap;
	Bool		ownMap = False;
	
	if( !map ) {
		map = PtrMap_New( 10 );
		ownMap = True;
	}
	
	newForceTerm = _Stg_Component_Copy( self, dest, deep, nameExt, map );
	
	newForceTerm->extraInfo = self->extraInfo;
	if( deep ) {
		newForceTerm->integrationSwarm = (Swarm*)Stg_Class_Copy( self->integrationSwarm, NULL, deep, nameExt, map );
	}
	else {
		newForceTerm->integrationSwarm = self->integrationSwarm;
	}
	
	if( ownMap ) {
		Stg_Class_Delete( map );
	}
	
	return (void*)newForceTerm;
}

void* _ForceTerm_DefaultNew( Name name ) {
	return _ForceTerm_New( 
			sizeof(ForceTerm), 
			ForceTerm_Type, 
			_ForceTerm_Delete,
			_ForceTerm_Print,
			_ForceTerm_Copy,
			_ForceTerm_DefaultNew, 
			_ForceTerm_Construct,
			_ForceTerm_Build, 
			_ForceTerm_Initialise,
			_ForceTerm_Execute, 
			_ForceTerm_Destroy,
			_ForceTerm_AssembleElement,
			name );
}

void _ForceTerm_Construct( void* forceTerm, Stg_ComponentFactory* cf, void* data ) {
	ForceTerm*      self               = (ForceTerm*)forceTerm;
	Swarm*          swarm              = NULL;
	Stg_Component*  extraInfo;
	ForceVector*    forceVector;


	forceVector =  Stg_ComponentFactory_ConstructByKey( cf, self->name, "ForceVector", ForceVector,   True,  data ) ;
	swarm       =  Stg_ComponentFactory_ConstructByKey( cf, self->name, "Swarm",       Swarm,         True,  data ) ;
	extraInfo   =  Stg_ComponentFactory_ConstructByKey( cf, self->name, "ExtraInfo",   Stg_Component, False, data ) ;
	_ForceTerm_Init( self, forceVector, swarm, extraInfo );
}

void _ForceTerm_Build( void* forceTerm, void* data ) {
	ForceTerm* self = (ForceTerm*)forceTerm;
	
	Journal_DPrintf( self->debug, "In %s - for %s\n", __func__, self->name );
	Stream_IndentBranch( StgFEM_Debug );
	
	/* ensure integrationSwarm is built */
	Stg_Component_Build( self->integrationSwarm, data, False );

	if ( self->extraInfo ) 
		Stg_Component_Build( self->extraInfo, data, False );
		
	Stream_UnIndentBranch( StgFEM_Debug );
}


void _ForceTerm_Initialise( void* forceTerm, void* data ) {
	ForceTerm* self = (ForceTerm*)forceTerm;
	
	Journal_DPrintf( self->debug, "In %s - for %s\n", __func__, self->name );
	Stream_IndentBranch( StgFEM_Debug );

	Stg_Component_Initialise( self->integrationSwarm, data, False );
	if ( self->extraInfo ) 
		Stg_Component_Initialise( self->extraInfo, data, False );
	
	Stream_UnIndentBranch( StgFEM_Debug );
}

void _ForceTerm_Execute( void* forceTerm, void* data ) {
}

void _ForceTerm_Destroy( void* forceTerm, void* data ) {
}

void ForceTerm_AssembleElement( 
			void*                             forceTerm, 
			ForceVector*                      forceVector, 
			Element_LocalIndex                lElement_I,
			double*                           elForceVecToAdd ) 
{
	ForceTerm* self = (ForceTerm*)forceTerm;

	self->_assembleElement( self, forceVector, lElement_I, elForceVecToAdd );
}
	
void _ForceTerm_AssembleElement( 
			void*                             forceTerm, 
			ForceVector*                      forceVector, 
			Element_LocalIndex                lElement_I,
			double*                           elForceVecToAdd ) 
{
	ForceTerm* self        = (ForceTerm*)forceTerm;
	Stream*    errorStream = Journal_Register( Error_Type, self->type );

	Journal_Printf( errorStream, "Error in func %s for %s '%s' - "
			"This function is the default function which should never be called - "
			"Please set this virtual function with appropriate application dependent function.\n",
			__func__, self->type, self->name );
	abort();
}	

void ForceTerm_SetAssembleElementFunction( void* forceTerm, ForceTerm_AssembleElementFunction* assembleElementFunction ) {
	ForceTerm* self        = (ForceTerm*)forceTerm;

	self->_assembleElement = assembleElementFunction;
}
