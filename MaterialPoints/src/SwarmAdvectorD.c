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
** $Id:  $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StgDomain/StgDomain.h>
#include <StgFEM/StgFEM.h>

#include <PICellerator/PopulationControl/PopulationControl.h>
#include <PICellerator/Weights/Weights.h>

#include "types.h"
#include "SwarmAdvector.h"
#include "SwarmAdvectorD.h"

#include "MaterialPointsSwarm.h"
#include "PeriodicBoundariesManager.h"
#include <assert.h>
#include <string.h>
#include <math.h>

/*I am generalising this so that I can use it- Wendy S*/


/* Textual name of this class */
const Type SwarmAdvectorD_Type = "SwarmAdvectorD";

/*-------------------------------------------------------------------------------------------------------------------------
** Constructors
*/
SwarmAdvectorD* SwarmAdvectorD_New(
		Name                                       name,
		TimeIntegrator*                            timeIntegrator,
		FeVariable*                                velocityField,
		Bool                                       allowFallbackToFirstOrder,
		MaterialPointsSwarm*                       swarm,
		PeriodicBoundariesManager*                 periodicBCsManager )
{
	SwarmAdvectorD* self = (SwarmAdvectorD*) _SwarmAdvectorD_DefaultNew( name );
	int whichaxis;
	
	/* 	SwarmAdvectorD_InitAll */
	_TimeIntegratee_Init( self, timeIntegrator, swarm->particleCoordVariable->variable, 0, NULL,
		allowFallbackToFirstOrder );
	_SwarmAdvector_Init( (SwarmAdvector*)self, velocityField, swarm, periodicBCsManager);
	_SwarmAdvectorD_Init( self, whichaxis );

	return self;
}

SwarmAdvectorD* _SwarmAdvectorD_New(
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
	SwarmAdvectorD* self;
	
	/* Allocate memory */
	assert( _sizeOfSelf >= sizeof(SwarmAdvectorD) );
	self = (SwarmAdvectorD*)_SwarmAdvector_New( 
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

void _SwarmAdvectorD_Init( SwarmAdvectorD* self, int whichaxis )
{
	/* Adding parameter which axis as the user can choose to prevent the advection of a tracer in the X (0), Y (1) or Z (2) directions */
	self->whichaxis = whichaxis;
}


/*------------------------------------------------------------------------------------------------------------------------
** Virtual functions
*/
void _SwarmAdvectorD_Delete( void* swarmAdvector ) {
	SwarmAdvectorD* self = (SwarmAdvectorD*)swarmAdvector;

	/* Delete parent */
	_SwarmAdvector_Delete( self );
}


void _SwarmAdvectorD_Print( void* swarmAdvector, Stream* stream ) {
	SwarmAdvectorD* self = (SwarmAdvectorD*)swarmAdvector;
	
	/* Print parent */
	_SwarmAdvector_Print( self, stream );
}


void* _SwarmAdvectorD_Copy( void* swarmAdvector, void* dest, Bool deep, Name nameExt, PtrMap* ptrMap ) {
	SwarmAdvectorD*	self = (SwarmAdvectorD*)swarmAdvector;
	SwarmAdvectorD*	newSwarmAdvectorD;
	
	newSwarmAdvectorD = (SwarmAdvectorD*)_SwarmAdvector_Copy( self, dest, deep, nameExt, ptrMap );

	newSwarmAdvectorD->velocityField = self->velocityField;
	newSwarmAdvectorD->swarm         = self->swarm;
	newSwarmAdvectorD->periodicBCsManager = self->periodicBCsManager;
	
	return (void*)newSwarmAdvectorD;
}

void* _SwarmAdvectorD_DefaultNew( Name name ) {
	return (void*) _SwarmAdvectorD_New(
			sizeof(SwarmAdvectorD),
			SwarmAdvectorD_Type,
			_SwarmAdvectorD_Delete,
			_SwarmAdvectorD_Print,
			_SwarmAdvectorD_Copy,
			_SwarmAdvectorD_DefaultNew,
			_SwarmAdvectorD_Construct,
			/* Just use the normal parent's implementation for next few, apart from new TimeDeriv function */
			_SwarmAdvector_Build,
			_SwarmAdvector_Initialise,
			_SwarmAdvector_Execute,
			_SwarmAdvector_Destroy,
			_SwarmAdvectorD_TimeDeriv,
			_SwarmAdvector_Intermediate,
			name );
}


void _SwarmAdvectorD_Construct( void* swarmAdvector, Stg_ComponentFactory* cf, void* data) {
	SwarmAdvectorD*	            self          = (SwarmAdvectorD*) swarmAdvector;
	int							whichaxis;
	_SwarmAdvector_Construct( self, cf, data );
	/* Everything except whichaxis constructed by parent already */
	whichaxis = Stg_ComponentFactory_GetInt( cf, self->name, "whichaxis",  1 );
	_SwarmAdvectorD_Init( self, whichaxis );
}


Bool _SwarmAdvectorD_TimeDeriv( void* swarmAdvector, Index array_I, double* timeDeriv ) {
	SwarmAdvectorD*      self          = (SwarmAdvectorD*) swarmAdvector;
	FieldVariable*      velocityField = (FieldVariable*) self->velocityField;
	double*             coord;
	InterpolationResult result;

	/* Get Coordinate of Object using Variable */
	coord = Variable_GetPtrDouble( self->variable, array_I );

	result = FieldVariable_InterpolateValueAt( velocityField, coord, timeDeriv );
	
	
	/* This prevents advection in the X, Y or Z direction */
	timeDeriv[ self->whichaxis ] = 0.0;


	if ( result == OTHER_PROC || result == OUTSIDE_GLOBAL || isinf(timeDeriv[0]) || isinf(timeDeriv[1]) || 
			( self->swarm->dim == 3 && isinf(timeDeriv[2]) ) ) 
	{
		#if 0
		Journal_Printf( Journal_Register( Error_Type, self->type ),
			"Error in func '%s' for particle with index %u.\n\tPosition (%g, %g, %g)\n\tVelocity here is (%g, %g, %g)."
			"\n\tInteropolation result is %u.\n",
			__func__, array_I, coord[0], coord[1], coord[2], 
			timeDeriv[0], timeDeriv[1], ( self->swarm->dim == 3 ? timeDeriv[2] : 0.0 ),
			result );
		abort();
		#endif

		return False;
	}

	return True;
}

/*-------------------------------------------------------------------------------------------------------------------------
** Private Functions
*/
/*---------------------------------------------------------------------------------------------------------------------
** Entry Point Hooks
*/
/*-------------------------------------------------------------------------------------------------------------------------
** Public Functions
*/


