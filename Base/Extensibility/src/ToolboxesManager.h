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
*/
/** \file
**  Role:
**	Handles the loading of "Toolbox" modules, which can extend the functionality or data structures of 
**	a main StGermain program.
**
** Assumptions:
**
** Comments:
**
** $Id: ToolboxesManager.h 4014 2007-02-23 02:15:16Z KathleenHumble $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifndef __Base_Extensibility_ToolboxesManager_h__
#define __Base_Extensibility_ToolboxesManager_h__
	

	/* Textual name of this class */
	extern const Type ToolboxesManager_Type;

	
	/* Toolboxes info */
	#define __ToolboxesManager \
		/* General info */ \
		__ModulesManager \
		\
		/* Virtual info */ \
		\
		/* Toolboxes info */ \
		int*       argc; \
		char***    argv; 
		
	struct ToolboxesManager { __ToolboxesManager };
	
    /** Define a global list of plugin directories*/
     extern Stg_ObjectList*  pluginDirectories;	

	/* Create a new Toolboxes */
	ToolboxesManager* ToolboxesManager_New( int* argc, char*** argv );
	
	/* Creation implementation / Virtual constructor */
	ToolboxesManager* _ToolboxesManager_New( 
		SizeT                                   _sizeOfSelf,
		Type                                    type,
		Stg_Class_DeleteFunction*               _delete,
		Stg_Class_PrintFunction*                _print,
		Stg_Class_CopyFunction*                 _copy, 
		ModulesManager_GetModulesListFunction*  _getModulesList,
		ModulesManager_LoadModuleFunction*	_loadModule,
		ModulesManager_UnloadModuleFunction*	_unloadModule,
		ModulesManager_ModuleFactoryFunction*   _moduleFactory,
		int*					argc,
		char***					argv );

	/* Initialisation implementation */
	void _ToolboxesManager_Init( void* toolboxesManager, int* argc, char*** argv );
	
	/* Stg_Class_Delete implementation */
	void _ToolboxesManager_Delete( void* toolboxesManager );
	
	/* Print implementation */
	void _ToolboxesManager_Print( void* toolboxesManager, Stream* stream );
	
	/** Get the plugins list from the dictionary */
	Dictionary_Entry_Value* _ToolboxesManager_GetToolboxesList(  void* toolboxesManager, void* dictionary );
	
	/** Exactly what to do to load the toolbox */
	Bool _ToolboxesManager_LoadToolbox( void* toolboxesManager, Module* toolbox );

	/** Exactly what to do to unload the toolbox */
	Bool _ToolboxesManager_UnloadToolbox( void* toolboxesManager, Module* toolbox );

	#define ToolboxesManager_Submit ModulesManager_Submit

#endif /* __Base_Extensibility_ToolboxesManager_h__ */
