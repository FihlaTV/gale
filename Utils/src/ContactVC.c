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
** $Id: ContactVC.c 4153 2007-07-26 02:25:22Z LukeHodkinson $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <mpi.h>
#include <StGermain/StGermain.h>

#include <StgDomain/Geometry/Geometry.h>
#include <StgDomain/Shape/Shape.h>
#include <StgDomain/Mesh/Mesh.h>

#include "types.h"
#include "WallVC.h"
#include "ContactVC.h"
#include "RegularMeshUtils.h"

#include <string.h>
#include <assert.h>


const Type ContactVC_Type = "ContactVC";
const Name defaultContactVCName = "defaultContactVCName";


/*--------------------------------------------------------------------------------------------------------------------------
** Constructor
*/

VariableCondition* ContactVC_Factory(
	Variable_Register*				variable_Register, 
	ConditionFunction_Register*			conFunc_Register, 
	Dictionary*					dictionary,
	void*						data )
{
	return (VariableCondition*)ContactVC_New( defaultContactVCName, NULL, variable_Register, conFunc_Register, dictionary, (Mesh*)data );
}


ContactVC*	ContactVC_DefaultNew( Name name )
{
	return _ContactVC_New(
		sizeof(ContactVC), 
		ContactVC_Type, 
		_ContactVC_Delete, 
		_WallVC_Print, 
		_WallVC_Copy,
		(Stg_Component_DefaultConstructorFunction*)ContactVC_DefaultNew,
		_ContactVC_Construct,	
		_ContactVC_Build,
		_VariableCondition_Initialise,
		_VariableCondition_Execute,
		_VariableCondition_Destroy,
		name,
		False,
		_WallVC_BuildSelf, 
		_WallVC_PrintConcise,
		_ContactVC_ReadDictionary,
		_ContactVC_GetSet, 
		_WallVC_GetVariableCount, 
		_WallVC_GetVariableIndex, 
		_WallVC_GetValueIndex, 
		_WallVC_GetValueCount, 
		_WallVC_GetValue,
		_VariableCondition_Apply, 
		NULL,
		NULL, 
		NULL, 
		NULL, 
		NULL);
}

ContactVC*	ContactVC_New(
	Name						name,
	Name						_dictionaryEntryName, 
	Variable_Register*				variable_Register, 
	ConditionFunction_Register*			conFunc_Register, 
	Dictionary*					dictionary,
	void*						_mesh )
{
	return _ContactVC_New(
		sizeof(ContactVC), 
		ContactVC_Type, 
		_ContactVC_Delete, 
		_WallVC_Print, 
		_WallVC_Copy,
		(Stg_Component_DefaultConstructorFunction*)ContactVC_DefaultNew,
		_ContactVC_Construct,	
		_ContactVC_Build,
		_VariableCondition_Initialise,
		_VariableCondition_Execute,
		_VariableCondition_Destroy,
		name,
		True,
		_WallVC_BuildSelf, 
		_WallVC_PrintConcise,
		_ContactVC_ReadDictionary,
		_ContactVC_GetSet, 
		_WallVC_GetVariableCount, 
		_WallVC_GetVariableIndex, 
		_WallVC_GetValueIndex, 
		_WallVC_GetValueCount, 
		_WallVC_GetValue,
		_VariableCondition_Apply, 
		_dictionaryEntryName,
		variable_Register, 
		conFunc_Register, 
		dictionary, 
		_mesh );
}


ContactVC* _ContactVC_New( 
	SizeT						_sizeOfSelf, 
	Type						type,
	Stg_Class_DeleteFunction*				_delete,
	Stg_Class_PrintFunction*				_print,
	Stg_Class_CopyFunction*				_copy, 
	Stg_Component_DefaultConstructorFunction*	_defaultConstructor,
	Stg_Component_ConstructFunction*			_construct,
	Stg_Component_BuildFunction*			_build,
	Stg_Component_InitialiseFunction*			_initialise,
	Stg_Component_ExecuteFunction*			_execute,
	Stg_Component_DestroyFunction*			_destroy,
	Name								name, 
	Bool								initFlag,
	VariableCondition_BuildSelfFunc*		_buildSelf, 
	VariableCondition_PrintConciseFunc*		_printConcise,
	VariableCondition_ReadDictionaryFunc*		_readDictionary,
	VariableCondition_GetSetFunc*			_getSet,
	VariableCondition_GetVariableCountFunc*		_getVariableCount,
	VariableCondition_GetVariableIndexFunc*		_getVariableIndex,
	VariableCondition_GetValueIndexFunc*		_getValueIndex,
	VariableCondition_GetValueCountFunc*		_getValueCount,
	VariableCondition_GetValueFunc*			_getValue,
	VariableCondition_ApplyFunc*			_apply, 
	Name						_dictionaryEntryName, 
	Variable_Register*				variable_Register, 
	ConditionFunction_Register*			conFunc_Register, 
	Dictionary*					dictionary,
	void*						_mesh)
{
	ContactVC*	self;
	
	/* Allocate memory/General info */
	assert(_sizeOfSelf >= sizeof(ContactVC));
	self = (ContactVC*)_WallVC_New(
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
		_buildSelf, 
		_printConcise,	
		_readDictionary,
		_getSet, 
		_getVariableCount, 
		_getVariableIndex, 
		_getValueIndex, 
		_getValueCount, 
		_getValue, 
		_apply, 
		_dictionaryEntryName,
		variable_Register, 
		conFunc_Register,
		dictionary,
		_mesh );
	
	/* Virtual info */
	
	/* Stg_Class info */
	if( initFlag ){
		_ContactVC_Init( self, _dictionaryEntryName, _mesh );
	}
	
	return self;
}


void _ContactVC_Init(
	void*						wallVC, 
	Name						_dictionaryEntryName, 
	void*						_mesh )
{
	ContactVC*			self = (ContactVC*)wallVC;

	self->depth = 0;
	self->includeTop = False;
}


/*--------------------------------------------------------------------------------------------------------------------------
** General virtual functions
*/

void _ContactVC_ReadDictionary( void* variableCondition, void* dictionary ) {
	ContactVC* self = (ContactVC*)variableCondition;
	Dictionary_Entry_Value *vcDictVal, _vcDictVal, *entryVal;

	_WallVC_ReadDictionary( variableCondition, dictionary );

	/* Find dictionary entry */
	if (self->_dictionaryEntryName)
		vcDictVal = Dictionary_Get(dictionary, self->_dictionaryEntryName);
	else
	{
		vcDictVal = &_vcDictVal;
		Dictionary_Entry_Value_InitFromStruct(vcDictVal, dictionary);
	}

	if (vcDictVal) {
	   self->depth = Dictionary_Entry_Value_AsInt(
	      Dictionary_Entry_Value_GetMember( vcDictVal, "depth" ) );
	   entryVal = Dictionary_Entry_Value_GetMember( vcDictVal, "includeTop" );
	   if( entryVal )
	      self->includeTop = Dictionary_Entry_Value_AsBool( entryVal );
	}
	else {
	   self->depth = 0;
	   self->includeTop = False;
	}
}


void _ContactVC_Delete(void* wallVC)
{
	ContactVC*	self = (ContactVC*)wallVC;
	
	/* Stg_Class_Delete parent */
	_WallVC_Delete(self);
}

void _ContactVC_Build(  void* wallVC, void* data ) {
	ContactVC*			self = (ContactVC*)wallVC;
	
	_WallVC_Build( self, data );
}
	

/*--------------------------------------------------------------------------------------------------------------------------
** Macros
*/


/*--------------------------------------------------------------------------------------------------------------------------
** Virtual functions
*/

void _ContactVC_Construct( void* wallVC, Stg_ComponentFactory* cf, void* data )
{
	
}

IndexSet* _ContactVC_GetSet(void* variableCondition) {
	ContactVC*		self = (ContactVC*)variableCondition;
	IndexSet*	set = NULL;
	Stream*		warningStr = Journal_Register( Error_Type, self->type );
	unsigned	nDims;
	Grid*		vertGrid;

	nDims = Mesh_GetDimSize( self->_mesh );
	vertGrid = *(Grid**)ExtensionManager_Get( self->_mesh->info, self->_mesh, 
						  ExtensionManager_GetHandle( self->_mesh->info, 
									      "vertexGrid" ) );
	
	switch (self->_wall) {
	case WallVC_Wall_Front:
		if ( nDims < 3 || !vertGrid->sizes[2] ) {
			Journal_Printf( warningStr, "Warning - in %s: Can't build a %s wall VC "
					"when mesh has no elements in the %s axis. Returning an empty set.\n", __func__,
					WallVC_WallEnumToStr[self->_wall], "K" );
			set = IndexSet_New( Mesh_GetDomainSize( self->_mesh, MT_VERTEX ) );
		}
		else {
		   abort();
		   /*set = RegularMeshUtils_CreateContactFrontSet( self->_mesh );*/
		}
		break;
			
	case WallVC_Wall_Back:
		if ( nDims < 3 || !vertGrid->sizes[2] ) {
			Journal_Printf( warningStr, "Warning - in %s: Can't build a %s wall VC "
					"when mesh has no elements in the %s axis. Returning an empty set.\n", __func__,
					WallVC_WallEnumToStr[self->_wall], "K" );
			set = IndexSet_New( Mesh_GetDomainSize( self->_mesh, MT_VERTEX ) );
		}
		else {
		   abort();
		   /*set = RegularMeshUtils_CreateContactBackSet( self->_mesh );*/
		}	
		break;
			
	case WallVC_Wall_Top:
		if ( nDims < 2 || !vertGrid->sizes[1] ) {
			Journal_Printf( warningStr, "Warning - in %s: Can't build a %s wall VC "
					"when mesh has no elements in the %s axis. Returning an empty set.\n", __func__,
					WallVC_WallEnumToStr[self->_wall], "J" );
			set = IndexSet_New( Mesh_GetDomainSize( self->_mesh, MT_VERTEX ) );
		}
		else {
		   abort();
		   /*set = RegularMeshUtils_CreateContactTopSet(self->_mesh);*/
		}	
		break;
			
	case WallVC_Wall_Bottom:
		if ( nDims < 2 || !vertGrid->sizes[1] ) {
			Journal_Printf( warningStr, "Warning - in %s: Can't build a %s wall VC "
					"when mesh has no elements in the %s axis. Returning an empty set.\n", __func__,
					WallVC_WallEnumToStr[self->_wall], "J" );
			set = IndexSet_New( Mesh_GetDomainSize( self->_mesh, MT_VERTEX ) );
		}
		else {
		   set = RegularMeshUtils_CreateContactBottomSet(self->_mesh, self->depth);
		}	
		break;
			
	case WallVC_Wall_Left:
		if ( nDims < 1 ) {
			Journal_Printf( warningStr, "Warning - in %s: Can't build a %s wall VC "
					"when mesh has no elements in the %s axis. Returning an empty set.\n", __func__,
					WallVC_WallEnumToStr[self->_wall], "I" );
			set = IndexSet_New( Mesh_GetDomainSize( self->_mesh, MT_VERTEX ) );
		}
		else {
		   set = RegularMeshUtils_CreateContactLeftSet(self->_mesh, self->depth, self->includeTop);
		}	
		break;
			
	case WallVC_Wall_Right:
		if( nDims < 1 ) {
			Journal_Printf( warningStr, "Warning - in %s: Can't build a %s wall VC "
					"when mesh has no elements in the %s axis. Returning an empty set.\n", __func__,
					WallVC_WallEnumToStr[self->_wall], "I" );
			set = IndexSet_New( Mesh_GetDomainSize( self->_mesh, MT_VERTEX ) );
		}
		else {
		   set = RegularMeshUtils_CreateContactRightSet(self->_mesh, self->depth, self->includeTop);
		}
		break;
			
	case WallVC_Wall_Size:
	default:
		assert(0);
		break;
	}
	
	return set;
}

/*--------------------------------------------------------------------------------------------------------------------------
** Build functions
*/


/*--------------------------------------------------------------------------------------------------------------------------
** Functions
*/
