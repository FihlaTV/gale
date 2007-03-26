/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
** Copyright (C), 2003-2006, Victorian Partnership for Advanced Computing (VPAC) Ltd, 110 Victoria Street,
**	Melbourne, 3053, Australia.
** Copyright (c) 2005-2006, Monash Cluster Computing, Building 28, Monash University Clayton Campus,
**	Victoria, 3800, Australia
**
** Primary Contributing Organisations:
**	Victorian Partnership for Advanced Computing Ltd, Computational Software Development - http://csd.vpac.org
**	Australian Computational Earth Systems Simulator - http://www.access.edu.au
**	Monash Cluster Computing - http://www.mcc.monash.edu.au
**
** Contributors:
**	Robert Turnbull, Research Assistant, Monash University. (robert.turnbull@sci.monash.edu.au)
**	Patrick D. Sunter, Software Engineer, VPAC. (patrick@vpac.org)
**	Alan H. Lo, Computational Engineer, VPAC. (alan@vpac.org)
**	Stevan M. Quenette, Senior Software Engineer, VPAC. (steve@vpac.org)
**	David May, PhD Student, Monash University (david.may@sci.monash.edu.au)
**	Vincent Lemiale, Postdoctoral Fellow, Monash University. (vincent.lemiale@sci.monash.edu.au)
**	Julian Giordani, Research Assistant, Monash University. (julian.giordani@sci.monash.edu.au)
**	Louis Moresi, Associate Professor, Monash University. (louis.moresi@sci.monash.edu.au)
**	Luke J. Hodkinson, Computational Engineer, VPAC. (lhodkins@vpac.org)
**	Raquibul Hassan, Computational Engineer, VPAC. (raq@vpac.org)
**	David Stegman, Postdoctoral Fellow, Monash University. (david.stegman@sci.monash.edu.au)
**	Wendy Sharples, PhD Student, Monash University (wendy.sharples@sci.monash.edu.au)
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
** $Id: SwarmAdvector.c 376 2006-10-18 06:58:41Z SteveQuenette $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StgFEM/StgFEM.h>

#include <PICellerator/Voronoi/Voronoi.h>
#include <PICellerator/PopulationControl/PopulationControl.h>
#include <PICellerator/Weights/Weights.h>

#include "types.h"
#include "SwarmAdvector.h"

#include "MaterialPointsSwarm.h"
#include "PeriodicBoundariesManager.h"
#include <assert.h>
#include <string.h>
#include <math.h>


/* Textual name of this class */
const Type SwarmAdvector_Type = "SwarmAdvector";

/*-------------------------------------------------------------------------------------------------------------------------
** Constructors
*/
SwarmAdvector* SwarmAdvector_New(
		Name                                       name,
		TimeIntegrator*                            timeIntegrator,
		FeVariable*                                velocityField,
		Bool                                       allowFallbackToFirstOrder,
		MaterialPointsSwarm*                       swarm,
		PeriodicBoundariesManager*                 periodicBCsManager )
{
	SwarmAdvector* self = (SwarmAdvector*) _SwarmAdvector_DefaultNew( name );

	/* 	SwarmAdvector_InitAll */
	_TimeIntegratee_Init( self, timeIntegrator, swarm->particleCoordVariable->variable, 0, NULL,
		allowFallbackToFirstOrder );
	_SwarmAdvector_Init( self, velocityField, swarm, periodicBCsManager );

	return self;
}

SwarmAdvector* _SwarmAdvector_New(
		SizeT                                      _sizeOfSelf, 
		Type                                       type,
		Stg_Class_DeleteFunction*                  _delete,
		Stg_Class_PrintFunction*                   _print,
		Stg_Class_CopyFunction*                    _copy, 
		Stg_Component_DefaultConstructorFunction*  _defaultConstructor,
		Stg_Component_ConstructFunction*           _construct,
		Stg_Component_BuildFunction*               _build,
		Stg_Component_InitialiseFunction*          _initialise,
		Stg_Component_ExecuteFunction*             _execute,
		Stg_Component_DestroyFunction*             _destroy,		
		TimeIntegratee_CalculateTimeDerivFunction* _calculateTimeDeriv,
		TimeIntegratee_IntermediateFunction*       _intermediate,
		Name                                       name )
{
	SwarmAdvector* self;
	
	/* Allocate memory */
	assert( _sizeOfSelf >= sizeof(SwarmAdvector) );
	self = (SwarmAdvector*)_TimeIntegratee_New( 
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
			_calculateTimeDeriv,
			_intermediate,
			name );
	
	/* General info */

	/* Virtual Info */
	
	return self;
}

void _SwarmAdvector_Init( 
		SwarmAdvector*                             self,
		FeVariable*                                velocityField,
		MaterialPointsSwarm*                       swarm,
		PeriodicBoundariesManager*                 periodicBCsManager )
{
	// TODO - commented out by Pat Sunter 20060428 since we need to use gauss layout initially for testing sometimes
	/*
	Journal_Firewall(
		swarm->particleLayout->coordSystem == GlobalCoordSystem,
		Journal_MyStream( Error_Type, self ),
		"In func - %s, swarm %s does not use a global coordinate system because of particle layout %s is of type %s\n",
		__func__,
		swarm->name,
		swarm->particleLayout->name,
		swarm->particleLayout->type );
	*/
	
	self->velocityField = velocityField;
	self->swarm = swarm;
	self->swarm->swarmAdvector = self;	/* Attach ourselves to the swarm */
	self->variable = swarm->particleCoordVariable->variable;
	self->periodicBCsManager = periodicBCsManager;

	TimeIntegrator_AppendSetupEP( self->timeIntegrator,  
			"SwarmAdvector_AdvectionSetup", SwarmAdvector_AdvectionSetup,  self->name, self );
	TimeIntegrator_PrependFinishEP( self->timeIntegrator,
			"SwarmAdvector_AdvectionFinish", SwarmAdvector_AdvectionFinish,  self->name, self );
	TimeIntegrator_InsertBeforeFinishEP( 
		self->timeIntegrator,
		"IntegrationPointsSwarm_Update", /* Must before this */
		"MaterialPointsSwarm_Update", 
		_MaterialPointsSwarm_UpdateHook, 
		swarm->name, 
		swarm );
			
}


/*------------------------------------------------------------------------------------------------------------------------
** Virtual functions
*/

void _SwarmAdvector_Delete( void* swarmAdvector ) {
	SwarmAdvector* self = (SwarmAdvector*)swarmAdvector;

	/* Delete parent */
	_TimeIntegratee_Delete( self );
}


void _SwarmAdvector_Print( void* swarmAdvector, Stream* stream ) {
	SwarmAdvector* self = (SwarmAdvector*)swarmAdvector;
	
	/* Print parent */
	_TimeIntegratee_Print( self, stream );
}


void* _SwarmAdvector_Copy( void* swarmAdvector, void* dest, Bool deep, Name nameExt, PtrMap* ptrMap ) {
	SwarmAdvector*	self = (SwarmAdvector*)swarmAdvector;
	SwarmAdvector*	newSwarmAdvector;
	
	newSwarmAdvector = (SwarmAdvector*)_TimeIntegratee_Copy( self, dest, deep, nameExt, ptrMap );

	newSwarmAdvector->velocityField = self->velocityField;
	newSwarmAdvector->swarm         = self->swarm;
	newSwarmAdvector->periodicBCsManager = self->periodicBCsManager;
	
	return (void*)newSwarmAdvector;
}

void* _SwarmAdvector_DefaultNew( Name name ) {
	return (void*) _SwarmAdvector_New(
			sizeof(SwarmAdvector),
			SwarmAdvector_Type,
			_SwarmAdvector_Delete,
			_SwarmAdvector_Print,
			_SwarmAdvector_Copy,
			_SwarmAdvector_DefaultNew,
			_SwarmAdvector_Construct,
			_SwarmAdvector_Build,
			_SwarmAdvector_Initialise,
			_SwarmAdvector_Execute,
			_SwarmAdvector_Destroy,
			_SwarmAdvector_TimeDeriv,
			_SwarmAdvector_Intermediate,
			name );
}


void _SwarmAdvector_Construct( void* swarmAdvector, Stg_ComponentFactory* cf, void* data ) {
	SwarmAdvector*	            self          = (SwarmAdvector*) swarmAdvector;
	FeVariable*                 velocityField;
	MaterialPointsSwarm*        swarm;
	PeriodicBoundariesManager*  periodicBCsManager;

	_TimeIntegratee_Construct( self, cf, data );

	velocityField      = Stg_ComponentFactory_ConstructByKey( cf, self->name, "VelocityField", FeVariable,  True, data );
	swarm              = Stg_ComponentFactory_ConstructByKey( cf, self->name, "Swarm",  MaterialPointsSwarm, True, data );
	periodicBCsManager = Stg_ComponentFactory_ConstructByKey( cf, self->name, "PeriodicBCsManager", PeriodicBoundariesManager, False, data );

	_SwarmAdvector_Init( self, velocityField, swarm, periodicBCsManager );
}

void _SwarmAdvector_Build( void* swarmAdvector, void* data ) {
	SwarmAdvector*	self = (SwarmAdvector*) swarmAdvector;

	_TimeIntegratee_Build( self, data );
}
void _SwarmAdvector_Initialise( void* swarmAdvector, void* data ) {
	SwarmAdvector*	self = (SwarmAdvector*) swarmAdvector;
	
	_TimeIntegratee_Initialise( self, data );
}
void _SwarmAdvector_Execute( void* swarmAdvector, void* data ) {
	SwarmAdvector*	self = (SwarmAdvector*)swarmAdvector;
	
	_TimeIntegratee_Execute( self, data );
}
void _SwarmAdvector_Destroy( void* swarmAdvector, void* data ) {
	SwarmAdvector*	self = (SwarmAdvector*)swarmAdvector;
	
	_TimeIntegratee_Destroy( self, data );
}

Bool _SwarmAdvector_TimeDeriv( void* swarmAdvector, Index array_I, double* timeDeriv ) {
	SwarmAdvector*      self          = (SwarmAdvector*) swarmAdvector;
	FieldVariable*      velocityField = (FieldVariable*) self->velocityField;
	double*             coord;
	InterpolationResult result;

	/* Get Coordinate of Object using Variable */
	coord = Variable_GetPtrDouble( self->variable, array_I );

	result = FieldVariable_InterpolateValueAt( velocityField, coord, timeDeriv );

	if ( result == OTHER_PROC || result == OUTSIDE_GLOBAL || isinf(timeDeriv[0]) || isinf(timeDeriv[1]) || 
			( self->swarm->dim == 3 && isinf(timeDeriv[2]) ) ) 
	{
		#if 0
		Journal_Printf( Journal_Register( Error_Type, self->type ),
			"Error in func '%s' for particle with index %u.\n\tPosition (%g, %g, %g)\n\tVelocity here is (%g, %g, %g)."
			"\n\tInterpolation result is %s.\n",
			__func__, array_I, coord[0], coord[1], coord[2], 
			timeDeriv[0], timeDeriv[1], ( self->swarm->dim == 3 ? timeDeriv[2] : 0.0 ),
			InterpolationResultToStringMap[result]  );
		#endif	
		return False;	
	}

	return True;
}


void _SwarmAdvector_Intermediate( void* swarmAdvector, Index lParticle_I ) {
	SwarmAdvector*      self          = (SwarmAdvector*) swarmAdvector;

	if ( self->periodicBCsManager ) {
		PeriodicBoundariesManager_UpdateParticle( self->periodicBCsManager, lParticle_I );
	}
}
/*-------------------------------------------------------------------------------------------------------------------------
** Private Functions
*/
/*---------------------------------------------------------------------------------------------------------------------
** Entry Point Hooks
*/
void SwarmAdvector_AdvectionSetup( TimeIntegrator* timeIntegrator, SwarmAdvector* self ) {
	FeVariable_SyncShadowValues( self->velocityField );
}

void SwarmAdvector_AdvectionFinish( TimeIntegrator* timeIntegrator, SwarmAdvector* self ) {
	#if DEBUG
		Swarm_CheckCoordsAreFinite( self->swarm );
	#endif
	
	/* Move particles across processors because they've just been advected */
	Swarm_UpdateAllParticleOwners( self->swarm );
}


/*-------------------------------------------------------------------------------------------------------------------------
** Public Functions
*/


