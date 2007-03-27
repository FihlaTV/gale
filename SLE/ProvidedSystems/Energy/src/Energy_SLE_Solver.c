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
** $Id: Energy_SLE_Solver.c 656 2006-10-18 06:45:50Z SteveQuenette $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <mpi.h>
#include <StGermain/StGermain.h>
#include "StgFEM/Discretisation/Discretisation.h"
#include "StgFEM/SLE/LinearAlgebra/LinearAlgebra.h"
#include "StgFEM/SLE/SystemSetup/SystemSetup.h"
#include "types.h"
#include "Energy_SLE_Solver.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/* Textual name of this class */
const Type Energy_SLE_Solver_Type = "Energy_SLE_Solver";

void* Energy_SLE_Solver_DefaultNew( Name name ) {
	return _Energy_SLE_Solver_New( 
		sizeof(Energy_SLE_Solver), 
		Energy_SLE_Solver_Type, 
		_Energy_SLE_Solver_Delete, 
		_Energy_SLE_Solver_Print, 
		_Energy_SLE_Solver_Copy,
		Energy_SLE_Solver_DefaultNew,
		_Energy_SLE_Solver_Construct,
		_Energy_SLE_Solver_Build, 
		_Energy_SLE_Solver_Initialise,
		_SLE_Solver_Execute,
		_SLE_Solver_Destroy,
		_Energy_SLE_Solver_SolverSetup, 
		_Energy_SLE_Solver_Solve, 
		_Energy_SLE_GetResidual, 
		_SLE_Solver_MG_SetupSmoother, 
		name );
}

Energy_SLE_Solver* Energy_SLE_Solver_New( Name name, Bool useStatSolve, int statReps ) {
	Energy_SLE_Solver* self = (Energy_SLE_Solver*) Energy_SLE_Solver_DefaultNew( name );

	Energy_SLE_Solver_InitAll( self, useStatSolve, statReps ) ;

	return self;
}

/* Creation implementation / Virtual constructor */
Energy_SLE_Solver* _Energy_SLE_Solver_New( 
		SizeT                                   sizeOfSelf,
		Type                                    type,
		Stg_Class_DeleteFunction*               _delete,
		Stg_Class_PrintFunction*                _print,
		Stg_Class_CopyFunction*                 _copy, 
		Stg_Component_DefaultConstructorFunction*   _defaultConstructor,
		Stg_Component_ConstructFunction*        _construct,
		Stg_Component_BuildFunction*            _build,
		Stg_Component_InitialiseFunction*       _initialise,
		Stg_Component_ExecuteFunction*          _execute,
		Stg_Component_DestroyFunction*          _destroy,
		SLE_Solver_SolverSetupFunction*         _solverSetup,
		SLE_Solver_SolveFunction*               _solve,
		SLE_Solver_GetResidualFunc*             _getResidual, 
		SLE_Solver_MG_SetupSmootherFunc*        _mgSetupSmoother,
		Name									name )
{
	Energy_SLE_Solver* self;

	/* Allocate memory */
	assert( sizeOfSelf >= sizeof(Energy_SLE_Solver) );
	self = (Energy_SLE_Solver*) _SLE_Solver_New( 
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
		_solverSetup,
		_solve,
		_getResidual, 
		_mgSetupSmoother,
		name );
	
	/* Virtual info */
	return self;
}
	
/* Initialisation implementation */
void _Energy_SLE_Solver_Init( Energy_SLE_Solver* self ) {
	self->isConstructed = True;
	self->residual = NULL;
}
void Energy_SLE_Solver_InitAll( Energy_SLE_Solver* solver, Bool useStatSolve, int statReps ) {
	Energy_SLE_Solver* self = (Energy_SLE_Solver*)solver;

	SLE_Solver_InitAll( self, useStatSolve, statReps );
}

void _Energy_SLE_Solver_Delete( void* sle ) {
	Energy_SLE_Solver* self = (Energy_SLE_Solver*)sle;

	MatrixSolver_Destroy( self->matrixSolver );

	if( self->residual ) {
		Vector_Destroy( self->residual );
	}
}

void _Energy_SLE_Solver_Print( void* solver, Stream* stream ) {
	Energy_SLE_Solver* self = (Energy_SLE_Solver*)solver;
	
	/* Set the Journal for printing informations */
	Stream* standard_SLE_SolverStream = stream;
	
	/* General info */
	Journal_Printf( standard_SLE_SolverStream, "Energy_SLE_Solver (ptr): %p\n", self );

	/* other info */
	Journal_Printf( standard_SLE_SolverStream, "\tmatrixSolver (ptr): %p", self->matrixSolver );
}


void* _Energy_SLE_Solver_Copy( void* standardSleSolver, void* dest, Bool deep, Name nameExt, PtrMap* ptrMap ) {
	Energy_SLE_Solver*	self = (Energy_SLE_Solver*)standardSleSolver;
	Energy_SLE_Solver*	newEnergySleSolver;
	
	newEnergySleSolver = _SLE_Solver_Copy( self, dest, deep, nameExt, ptrMap );
	
	newEnergySleSolver->matrixSolver = self->matrixSolver;
	
	return (void*)newEnergySleSolver;
}

void _Energy_SLE_Solver_Construct( void* sleSolver, Stg_ComponentFactory* cf, void* data ) {
	Energy_SLE_Solver *self = (Energy_SLE_Solver*)sleSolver;

	_SLE_Solver_Construct( self, cf, data );

	_Energy_SLE_Solver_Init( self );
}

/* Build */
void _Energy_SLE_Solver_Build( void* sleSolver, void* standardSLE ) {
	Energy_SLE_Solver*	self = (Energy_SLE_Solver*)sleSolver;
	SystemLinearEquations*	sle = (SystemLinearEquations*) standardSLE;
	StiffnessMatrix*	stiffMat = (StiffnessMatrix*)sle->stiffnessMatrices->data[0];

	Journal_DPrintf( self->debug, "In %s()\n", __func__ );
	Stream_IndentBranch( StgFEM_SLE_ProvidedSystems_Energy_Debug );
	Journal_DPrintf( self->debug, "building a standard L.A. solver for the \"%s\" matrix.\n", stiffMat->name );
	self->matrixSolver = MatrixSolver_Build( sle->comm, stiffMat->matrix );
	Stream_UnIndentBranch( StgFEM_SLE_ProvidedSystems_Energy_Debug );

	Vector_Duplicate( ((ForceVector**)sle->forceVectors->data)[0]->vector, &self->residual );
}


void _Energy_SLE_Solver_Initialise( void* sleSolver, void* standardSLE ) {
	Energy_SLE_Solver*	self = (Energy_SLE_Solver*)sleSolver;
	SystemLinearEquations*	sle = (SystemLinearEquations*) standardSLE;
	
	/* Initialise parent. */
	_SLE_Solver_Initialise( self, sle );
	
	/* MG: If multi-grid is enabled, enable it on the velocity matrix solver. */
	if( sle->mgEnabled ) {
		MultiGrid_InitMatrixSolver( sle->mgHandles[0], self->matrixSolver );
		
		MultiGrid_BuildGridOps( sle->mgHandles[0] );
		MultiGrid_BuildWorkVectors( sle->mgHandles[0] );
	}
}

void _Energy_SLE_Solver_Execute( void* sleSolver, void* data ) {
	
}
	
void _Energy_SLE_Solver_Destroy( void* sleSolver, void* data ) {
	
}

void _Energy_SLE_Solver_SolverSetup( void* sleSolver, void* standardSLE ) {
	Energy_SLE_Solver* self = (Energy_SLE_Solver*)sleSolver;
	SystemLinearEquations* sle = (SystemLinearEquations*) standardSLE;
	StiffnessMatrix*	stiffMat = (StiffnessMatrix*)sle->stiffnessMatrices->data[0];
	
	Journal_DPrintf( self->debug, "In %s()\n", __func__ );
	Stream_IndentBranch( StgFEM_SLE_ProvidedSystems_Energy_Debug );
	
	Journal_DPrintf( self->debug, "Initialising the L.A. solver for the \"%s\" matrix.\n", stiffMat->name );
	MatrixSolver_Setup( self->matrixSolver, stiffMat->matrix );
	Stream_UnIndentBranch( StgFEM_SLE_ProvidedSystems_Energy_Debug );
	
	if( !sle->mgEnabled ) {
		if( self->maxIterations > 0 ) {
			MatrixSolver_SetMaxIts( self->matrixSolver, self->maxIterations );
		}
	}
}


void _Energy_SLE_Solver_Solve( void* sleSolver, void* standardSLE ) {
	Energy_SLE_Solver*     self       = (Energy_SLE_Solver*)sleSolver;
	SystemLinearEquations* sle        = (SystemLinearEquations*) standardSLE;
	Iteration_Index        iterations;

	
	Journal_DPrintf( self->debug, "In %s - for standard SLE solver\n", __func__ );
	Stream_IndentBranch( StgFEM_SLE_ProvidedSystems_Energy_Debug );
	
	if ( (sle->stiffnessMatrices->count > 1 ) ||
		(sle->forceVectors->count > 1 ) )
	{
		Stream* warning = Stream_RegisterChild( StgFEM_Warning, self->type );
		Journal_Printf( warning, "Warning: Energy solver unable to solve more that one matrix/vector.\n" );
	}

	if ( sle->solutionVectors->count > 1 ) {
		Stream* warning = Stream_RegisterChild( StgFEM_Warning, self->type );
		Journal_Printf( warning, "Warning: More than 1 solution vector provided to standard sle. Ignoring second and subsequent"
			" solution vectors.\n" );
	}
	
	/* Handle MG. */
	if( sle->mgEnabled ) {
		MultiGrid_BuildSmoothers( sle->mgHandles[0] );
		MultiGrid_UpdateMatrixSolver( sle->mgHandles[0], self->matrixSolver, (SLE_Solver*)self );
	}
	
	/* If a stat solve was specified then do so. */
	if( self->useStatSolve ) {
		iterations = MatrixSolver_StatSolve( self->matrixSolver,
									  ((SolutionVector*) sle->solutionVectors->data[0])->vector,
									  ((ForceVector*) sle->forceVectors->data[0])->vector, 
									  self->nStatReps );
	}
	else {
		iterations = MatrixSolver_Solve( self->matrixSolver, 
								   ((SolutionVector*) sle->solutionVectors->data[0])->vector,
								   ((ForceVector*) sle->forceVectors->data[0])->vector );
	}
	Journal_DPrintf( self->debug, "Solved after %u iterations.\n", iterations );
	Stream_UnIndentBranch( StgFEM_SLE_ProvidedSystems_Energy_Debug );
	
	/* calculate the residual */
	/* TODO: need to make this optional */
	MatrixMultiply( ((StiffnessMatrix**)sle->stiffnessMatrices->data)[0]->matrix, ((SolutionVector**)sle->solutionVectors->data)[0]->vector, self->residual );
	Vector_ScaleAndAddVector( self->residual, -1.0, ((ForceVector**)sle->forceVectors->data)[0]->vector );
}


Vector* _Energy_SLE_GetResidual( void* sleSolver, Index fv_I ) {
	Energy_SLE_Solver*	self = (Energy_SLE_Solver*)sleSolver;

	return self->residual;
}
