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
** $Id: CosineHillRotate.c 968 2007-10-23 07:53:39Z JulianGiordani $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <StGermain/StGermain.h>
#include <StgDomain/StgDomain.h>
#include <StgFEM/StgFEM.h>

const Type CosineHillRotate_Type = "CosineHillRotate";

typedef struct { 
	__FieldTest
	FeVariable* temperatureField;
	double      hillHeight;
	double      hillDiameter;
	Coord       rotationCentre;
} CosineHillRotate;

void CosineHillRotate_TemperatureFunction( void* analyticSolution, const double* coord, double* temperature ) {
	CosineHillRotate *self = (CosineHillRotate*)analyticSolution;
	double distanceFromCentre = StGermain_DistanceBetweenPoints( self->rotationCentre, coord, 2 );
	
	if (distanceFromCentre < self->hillDiameter ) 
		*temperature = self->hillHeight * (0.5 + 0.5 * cos( 2.0 * M_PI/self->hillDiameter * distanceFromCentre + M_PI ) );
	else
		*temperature = 0.0;
}

void CosineHillRotate_TemperatureBC(const double *coord, void* _context, void* _result ) {
	DomainContext*	context    = (DomainContext*)_context;
	CosineHillRotate*  self       = Stg_ComponentFactory_ConstructByName( context->CF, (Name)CosineHillRotate_Type, CosineHillRotate, True, 0 );
	double*                 result     = (double*) _result;
	
	CosineHillRotate_TemperatureFunction( self, coord, result );
}

void _CosineHillRotate_AssignFromXML( void* analyticSolution, Stg_ComponentFactory* cf, void* data ) {
	CosineHillRotate* self = (CosineHillRotate*)analyticSolution;
	ConditionFunction*     condFunc;

	_FieldTest_AssignFromXML( self, cf, data );

	/* Read values from dictionary */
	self->hillHeight       = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"CosineHillHeight"  , 1.0  );
	self->hillDiameter     = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"CosineHillDiameter", 1.0  );
	self->rotationCentre[ I_AXIS ] = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"SolidBodyRotationCentreX" , 0.0  );
	self->rotationCentre[ J_AXIS ] = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"SolidBodyRotationCentreY" , 0.0  );
	self->rotationCentre[ K_AXIS ] = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"SolidBodyRotationCentreZ" , 0.0  );

	/* Create Condition Functions */
	condFunc = ConditionFunction_New( CosineHillRotate_TemperatureBC, (Name)"Temperature_CosineHill"  );
	ConditionFunction_Register_Add( condFunc_Register, condFunc );
}

void _CosineHillRotate_Build( void* analyticSolution, void* data ) {
	CosineHillRotate* self = (CosineHillRotate*)analyticSolution;	

	_FieldTest_Build( self, data );

	/* here we assign the memory and the func ptr for analytic sols */
	self->_analyticSolutionList = Memory_Alloc_Array_Unnamed( FieldTest_AnalyticSolutionFunc*, 1 );
	/* this order MUST be consistent with the xml file definition */
	self->_analyticSolutionList[0] = CosineHillRotate_TemperatureFunction;
}

void* _CosineHillRotate_DefaultNew( Name name ) {
	/* Variables set in this function */
	SizeT                                              _sizeOfSelf = sizeof(CosineHillRotate);
	Type                                                      type = CosineHillRotate_Type;
	Stg_Class_DeleteFunction*                              _delete = _FieldTest_Delete;
	Stg_Class_PrintFunction*                                _print = _FieldTest_Print;
	Stg_Class_CopyFunction*                                  _copy = _FieldTest_Copy;
	Stg_Component_DefaultConstructorFunction*  _defaultConstructor = _CosineHillRotate_DefaultNew;
	Stg_Component_ConstructFunction*                    _construct = _CosineHillRotate_AssignFromXML;
	Stg_Component_BuildFunction*                            _build = _CosineHillRotate_Build;
	Stg_Component_InitialiseFunction*                  _initialise = _FieldTest_Initialise;
	Stg_Component_ExecuteFunction*                        _execute = _FieldTest_Execute;
	Stg_Component_DestroyFunction*                        _destroy = _FieldTest_Destroy;

	/* Variables that are set to ZERO are variables that will be set either by the current _New function or another parent _New function further up the hierachy */
	AllocationType  nameAllocationType = NON_GLOBAL /* default value NON_GLOBAL */;

	return (void*) _FieldTest_New(  FIELDTEST_PASSARGS  );
}

/* This function is automatically run by StGermain when this plugin is loaded. The name must be "<plugin-name>_Register". */
Index StgFEM_CosineHillRotate_Register( PluginsManager* pluginsManager ) {
	/* A plugin is only properly registered once it returns the handle provided when submitting a codelet to StGermain. */
	return PluginsManager_Submit( pluginsManager, CosineHillRotate_Type, (Name)"0", _CosineHillRotate_DefaultNew  );
}



