/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
** Copyright (c) 2005, Monash Cluster Computing 
** All rights reserved.
** Redistribution and use in source and binary forms, with or without modification,
** are permitted provided that the following conditions are met:
**
** 		* Redistributions of source code must retain the above copyright notice, 
** 			this list of conditions and the following disclaimer.
** 		* Redistributions in binary form must reproduce the above copyright 
**			notice, this list of conditions and the following disclaimer in the 
**			documentation and/or other materials provided with the distribution.
** 		* Neither the name of the Monash University nor the names of its contributors 
**			may be used to endorse or promote products derived from this software 
**			without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
** THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS 
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
** OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**
** Contact:
*%		Louis Moresi - Louis.Moresi@sci.monash.edu.au
*%
** Contributors:
*+		Mirko Velic
*+		Julian Giordani
*+		Robert Turnbull
*+		Vincent Lemiale
*+		Louis Moresi
*+		David May
*+		David Stegman
*+		Patrick Sunter
** $Id: solJ.c 636 2006-09-08 03:07:06Z JulianGiordani $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "Analytic_solJ.h"

const Type Velic_solJ_Type = "Underworld_Velic_solJ";

void Velic_solJ_PressureFunction( void* analyticSolution, FeVariable* analyticFeVariable, const double *coord, double* pressure ) {
	Velic_solJ* self = (Velic_solJ*) analyticSolution;
	
	_Velic_solJ( coord, self->sigmaB, self->sigmaA, self->etaB, self->etaA, self->dxB, self->dxA,
		       self->x0B, self->x0A, self->zc,
	       	       NULL, pressure, NULL, NULL );
}

void Velic_solJ_VelocityFunction( void* analyticSolution, FeVariable* analyticFeVariable, const double *coord, double* velocity ) {
	Velic_solJ* self = (Velic_solJ*) analyticSolution;
	
	_Velic_solJ( coord, self->sigmaB, self->sigmaA, self->etaB, self->etaA, self->dxB, self->dxA,
		       self->x0B, self->x0A, self->zc,
	       	       velocity, NULL, NULL, NULL );
}

void Velic_solJ_StressFunction( void* analyticSolution, FeVariable* analyticFeVariable, const double *coord, double* stress ) {
	Velic_solJ* self = (Velic_solJ*) analyticSolution;
	
	_Velic_solJ( coord, self->sigmaB, self->sigmaA, self->etaB, self->etaA, self->dxB, self->dxA,
		       self->x0B, self->x0A, self->zc,
	       	       NULL, NULL, stress, NULL );
}


void Velic_solJ_StrainRateFunction( void* analyticSolution, FeVariable* analyticFeVariable, const double *coord, double* strainRate ) {
	Velic_solJ* self = (Velic_solJ*) analyticSolution;
	
	_Velic_solJ( coord, self->sigmaB, self->sigmaA, self->etaB, self->etaA, self->dxB, self->dxA,
		       self->x0B, self->x0A, self->zc,
	       	       NULL, NULL, NULL, strainRate );
}

void _Velic_solJ_Init( Velic_solJ* self, double sigmaA, double sigmaB, double etaB, double etaA, double dxB, double dxA, double x0B, double x0A, double zc ) {
// TODO	Bool                     correctInput = True;
	// Set the general AnalyticSolution (parent class) field function pointers to the specific analytic sols to use
        self->_getAnalyticVelocity    = Velic_solJ_VelocityFunction;
	self->_getAnalyticPressure    = Velic_solJ_PressureFunction;
	self->_getAnalyticTotalStress = Velic_solJ_StressFunction;
	self->_getAnalyticStrainRate  = Velic_solJ_StrainRateFunction;
	
	self->sigmaA = sigmaA;
	self->sigmaB = sigmaB;
	self->etaB = etaB;
	self->etaA = etaA;
	self->dxB = dxB;
	self->dxA = dxA;
	self->x0B = x0B;
	self->x0A = x0A;
	self->zc = zc;
}

void _Velic_solJ_AssignFromXML( void* analyticSolution, Stg_ComponentFactory* cf, void* data ) {
	Velic_solJ* self = (Velic_solJ*) analyticSolution;
	FeVariable*              velocityField;
	FeVariable*              pressureField;
	FeVariable*              stressField;
	FeVariable*              strainRateField;
	FeVariable*              recoveredStrainRateField;
	FeVariable*              recoveredStressField;
	double                   sigmaA, sigmaB, etaB, etaA, dxB, dxA, x0B, x0A, zc;

	/* Construct Parent */
	_AnalyticSolution_AssignFromXML( self, cf, data );

	/* Create Analytic Fields */
	velocityField = Stg_ComponentFactory_ConstructByName( cf, (Name)"VelocityField", FeVariable, True, data  );
	AnalyticSolution_RegisterFeVariableWithAnalyticFunction( self, velocityField, Velic_solJ_VelocityFunction );

	pressureField = Stg_ComponentFactory_ConstructByName( cf, (Name)"PressureField", FeVariable, True, data  );
	AnalyticSolution_RegisterFeVariableWithAnalyticFunction( self, pressureField, Velic_solJ_PressureFunction );

	stressField = Stg_ComponentFactory_ConstructByName( cf, (Name)"StressField", FeVariable, False, data );
	if ( stressField  )
		AnalyticSolution_RegisterFeVariableWithAnalyticFunction( self, stressField, Velic_solJ_StressFunction );

	strainRateField = Stg_ComponentFactory_ConstructByName( cf, (Name)"StrainRateField", FeVariable, False, data );
	if ( strainRateField   ) {
		AnalyticSolution_RegisterFeVariableWithAnalyticFunction( self, strainRateField, Velic_solJ_StrainRateFunction );
	}

	recoveredStrainRateField = Stg_ComponentFactory_ConstructByName( cf, (Name)"recoveredStrainRateField", FeVariable, False, data );
	if ( recoveredStrainRateField  )
		AnalyticSolution_RegisterFeVariableWithAnalyticFunction( self, recoveredStrainRateField, Velic_solJ_StrainRateFunction );

	recoveredStressField = Stg_ComponentFactory_ConstructByName( cf, (Name)"recoveredStressField", FeVariable, False, data );
	if ( recoveredStressField  )
		AnalyticSolution_RegisterFeVariableWithAnalyticFunction( self, recoveredStressField, Velic_solJ_StressFunction );

	sigmaA = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"solJ_sigmaA", 1.0  );
	sigmaB = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"solJ_sigmaB", 3.0  );
	etaB = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"solJ_etaB", 2.0  );
	etaA = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"solJ_etaA", 1.0  );
	dxB = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"solJ_dxB", 0.4  );
	dxA = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"solJ_dxA", 0.3  );
	x0B = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"solJ_x0B", 0.3  );
	x0A = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"solJ_x0A", 0.6  );
	zc = Stg_ComponentFactory_GetRootDictDouble( cf, (Dictionary_Entry_Key)"solJ_zc", 0.8  );

	_Velic_solJ_Init( self, sigmaA, sigmaB, etaB, etaA, dxB, dxA, x0B, x0A, zc );
}

void* _Velic_solJ_DefaultNew( Name name ) {
	/* Variables set in this function */
	SizeT                                              _sizeOfSelf = sizeof(Velic_solJ);
	Type                                                      type = Velic_solJ_Type;
	Stg_Class_DeleteFunction*                              _delete = _AnalyticSolution_Delete;
	Stg_Class_PrintFunction*                                _print = _AnalyticSolution_Print;
	Stg_Class_CopyFunction*                                  _copy = _AnalyticSolution_Copy;
	Stg_Component_DefaultConstructorFunction*  _defaultConstructor = _Velic_solJ_DefaultNew;
	Stg_Component_ConstructFunction*                    _construct = _Velic_solJ_AssignFromXML;
	Stg_Component_BuildFunction*                            _build = _AnalyticSolution_Build;
	Stg_Component_InitialiseFunction*                  _initialise = _AnalyticSolution_Initialise;
	Stg_Component_ExecuteFunction*                        _execute = _AnalyticSolution_Execute;
	Stg_Component_DestroyFunction*                        _destroy = _AnalyticSolution_Destroy;

	/* Variables that are set to ZERO are variables that will be set either by the current _New function or another parent _New function further up the hierachy */
	AllocationType  nameAllocationType = NON_GLOBAL /* default value NON_GLOBAL */;

	return _AnalyticSolution_New(  ANALYTICSOLUTION_PASSARGS  );
}

Index Underworld_Velic_solJ_Register( PluginsManager* pluginsManager ) {
	return PluginsManager_Submit( pluginsManager, Velic_solJ_Type, (Name)"0", _Velic_solJ_DefaultNew  );
}


