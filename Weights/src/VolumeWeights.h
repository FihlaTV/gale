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
** This file may be distributed under the terms of the VPAC Public License
** as defined by VPAC of Australia and appearing in the file
** LICENSE.VPL included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
*/
/** \file
**  Role:
**
** Assumptions:
**
** Comments:
**
** $Id: VolumeWeights.h 189 2005-10-20 00:39:29Z RobertTurnbull $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifndef __PICellerator_Weights_VolumeWeightsClass_h__
#define __PICellerator_Weights_VolumeWeightsClass_h__

	/* Textual name of this class */
	extern const Type VolumeWeights_Type;

	/* VolumeWeights information */
	#define __VolumeWeights \
		__WeightsCalculator \
		Stg_Shape*              shape; \
		FiniteElement_Mesh*     mesh; 

	struct VolumeWeights { __VolumeWeights };
	
	
	/*---------------------------------------------------------------------------------------------------------------------
	** Constructors
	*/
	VolumeWeights* VolumeWeights_New( Name name, Dimension_Index dim, Stg_Shape* shape, FiniteElement_Mesh* mesh );
	VolumeWeights* _VolumeWeights_New(
		SizeT                                 _sizeOfSelf, 
		Type                                  type,
		Stg_Class_DeleteFunction*             _delete,
		Stg_Class_PrintFunction*              _print,
		Stg_Class_CopyFunction*               _copy, 
		Stg_Component_DefaultConstructorFunction* _defaultConstructor,
		Stg_Component_ConstructFunction*      _construct,
		Stg_Component_BuildFunction*          _build,
		Stg_Component_InitialiseFunction*     _initialise,
		Stg_Component_ExecuteFunction*        _execute,
		Stg_Component_DestroyFunction*        _destroy,		
		WeightsCalculator_CalculateFunction*  _calculate,
		Name                                  name );

	void _VolumeWeights_Init( void* weights, Stg_Shape* shape, FiniteElement_Mesh* mesh ) ;
	void VolumeWeights_InitAll( void* weights, Dimension_Index dim, Stg_Shape* shape, FiniteElement_Mesh* mesh ) ;


	/* Stg_Class_Delete VolumeWeights implementation */
	void _VolumeWeights_Delete( void* weights );
	void _VolumeWeights_Print( void* weights, Stream* stream );
	#define VolumeWeights_Copy( self ) \
		(VolumeWeights*) Stg_Class_Copy( self, NULL, False, NULL, NULL )
	#define VolumeWeights_DeepCopy( self ) \
		(VolumeWeights*) Stg_Class_Copy( self, NULL, True, NULL, NULL )
	void* _VolumeWeights_Copy( void* weights, void* dest, Bool deep, Name nameExt, PtrMap* ptrMap );
	
	void* _VolumeWeights_DefaultNew( Name name ) ;
void _VolumeWeights_Construct( void* weights, Stg_ComponentFactory* cf, void* data ) ;
	void _VolumeWeights_Build( void* weights, void* data ) ;
	void _VolumeWeights_Initialise( void* weights, void* data ) ;
	void _VolumeWeights_Execute( void* weights, void* data );
	void _VolumeWeights_Destroy( void* weights, void* data ) ;
	
		
	void _VolumeWeights_Calculate( void* weights, void* _swarm, Cell_LocalIndex lCell_I ) ;
	
	
#endif 
