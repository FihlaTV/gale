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
*+		Robert Turnbull
*+		Vincent Lemiale
*+		Louis Moresi
*+		David May
*+		David Stegman
*+		Mirko Velic
*+		Patrick Sunter
*+		Julian Giordani
*+
** $Id: BuoyancyIntegrals.c 487 2007-06-07 05:48:32Z LukeHodkinson $
** 
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StgFEM/StgFEM.h>
#include <PICellerator/PICellerator.h>
#include <Underworld/Underworld.h>
#include <StgFEM/FrequentOutput/FrequentOutput.h>


/* prototypes */
Index Underworld_BuoyancyIntegrals_Register( PluginsManager *pluginsManager );
void* _Underworld_BuoyancyIntegrals_DefaultNew( Name name );
void _Underworld_BuoyancyIntegrals_CTX_Delete( void *component );
void _Underworld_BuoyancyIntegrals_Construct( void *component, Stg_ComponentFactory *cf, void *data );
void Underworld_BuoyancyIntegrals_Output( UnderworldContext *context );
void Underworld_BuoyancyIntegrals_Setup( void* _context );

typedef struct {
        __Codelet
	double int_w_bar_dt;
	double beta;
	double gravity;
	int dim;
} Underworld_BuoyancyIntegrals_CTX;



/*---------------------------------- PLUGIN SOURCE CODE --------------------------------------*/

const Type Underworld_BuoyancyIntegrals_Type = "Underworld_BuoyancyIntegrals";

Index Underworld_BuoyancyIntegrals_Register( PluginsManager *pluginsManager ) 
{
	return PluginsManager_Submit( pluginsManager, Underworld_BuoyancyIntegrals_Type, "0", _Underworld_BuoyancyIntegrals_DefaultNew );
}

void* _Underworld_BuoyancyIntegrals_DefaultNew( Name name ) 
{

        return _Codelet_New(
                sizeof(Underworld_BuoyancyIntegrals_CTX),
                Underworld_BuoyancyIntegrals_Type,
                _Underworld_BuoyancyIntegrals_CTX_Delete,
                /*_Codelet_Delete, */
                _Codelet_Print,
                _Codelet_Copy,
                _Underworld_BuoyancyIntegrals_DefaultNew,
                _Underworld_BuoyancyIntegrals_Construct,
                _Codelet_Build,
                _Codelet_Initialise,
                _Codelet_Execute,
                _Codelet_Destroy,
                name );
}

void _Underworld_BuoyancyIntegrals_CTX_Delete( void *component ) 
{
        Underworld_BuoyancyIntegrals_CTX  *ctx = (Underworld_BuoyancyIntegrals_CTX*)component;

        _Codelet_Delete( ctx );
}


void _Underworld_BuoyancyIntegrals_Construct( void *component, Stg_ComponentFactory *cf, void *data ) 
{
	UnderworldContext* context;

	context = Stg_ComponentFactory_ConstructByName( cf, "context", UnderworldContext, True, data ); 

	/* Add functions to entry points */
	ContextEP_Append( context, AbstractContext_EP_ConstructExtensions, Underworld_BuoyancyIntegrals_Setup );
	ContextEP_Append( context, AbstractContext_EP_FrequentOutput, Underworld_BuoyancyIntegrals_Output );
}

void Underworld_BuoyancyIntegrals_Setup( void *_context )
{
	UnderworldContext *context = (UnderworldContext*) _context;
	Underworld_BuoyancyIntegrals_CTX *ctx;


	/* allocate memory */
	ctx = (Underworld_BuoyancyIntegrals_CTX*)LiveComponentRegister_Get(
                                        context->CF->LCRegister,
                                        Underworld_BuoyancyIntegrals_Type );

	/* init values */
        ctx->dim = Stg_ComponentFactory_GetRootDictInt( context->CF, "dim", -1 );
        if( (int)ctx->dim == -1 ) {
                printf("******************** ERROR dim IS UNINITIALISED ******************************** \n");
        }


	ctx->beta = Stg_ComponentFactory_GetRootDictDouble( context->CF, "alpha", -1 );
	if( (int)ctx->beta == -1 ) {
		printf("******************** ERROR ALPHA IS UNINITIALISED ******************************** \n");
	}

	ctx->gravity = Stg_ComponentFactory_GetRootDictDouble( context->CF, "gravity", -1 );
	if( (int)ctx->gravity == -1 ) {
		printf("******************** ERROR GRAVITY IS UNINITIALISED ******************************** \n");
	}

	ctx->int_w_bar_dt = 0.0;


#if 0
       /* Create Some FeVariables to calculate nusselt number */
        temperatureField          = FieldVariable_Register_GetByName( fV_Register, "TemperatureField" );
        velocityField             = FieldVariable_Register_GetByName( fV_Register, "VelocityField" );
	

	/* To compute B */
	/* perturbed_temperatre := T - T_0 */
	/*
        self->perturbed_temperatre_field = OperatorFeVariable_NewUnary(
                        "AdvectiveHeatFluxField",
                        temperatureField,
                        "VectorScale" );
	*/
	/* 
	Does not appear to be anything to shift fields by a scalar. 
	Will just assume that T0 = 0
	*/
	self->perturbed_temperatre_field = temperatureField;


	/* To compute w_bar */
	/* grab the second component of the velocity field ez \dot u */
        self->vertical_velocity_field = (FeVariable*) OperatorFeVariable_NewUnary(
                        "VerticalVelocityField",
                        velocityField,
                        "TakeSecondComponent" );
        self->vertical_velocity_field->feMesh = ((FeVariable*)velocityField)->feMesh;

        self->vT_field = OperatorFeVariable_NewBinary(
                        "VerticalAdvectiveTemperatureField",
                        temperatureField,
                        self->vertical_velocity_field,
                        "VectorScale" );
	self->vT_field->feMesh = ((FeVariable*)velocityField)->feMesh;
#endif



	/* Add names to the integrals in the frequent output */
	StgFEM_FrequentOutput_PrintString( context, "B" );
	StgFEM_FrequentOutput_PrintString( context, "w_bar" );
	StgFEM_FrequentOutput_PrintString( context, "y_b" );
	StgFEM_FrequentOutput_PrintString( context, "int_w_bar_dt" );
	StgFEM_FrequentOutput_PrintString( context, "temp_b" );

}


void perform_integrals( UnderworldContext *context, double *B, double *w_bar, double *y_b, double *int_w_bar_dt )
{
	Underworld_BuoyancyIntegrals_CTX *ctx;
	IntegrationPoint *ip;
	double *xi;
	double weight;
	Particle_InCellIndex p, ngp;
	Cell_Index cell_I;
	double velocity[3], global_coord[3];
	Element_LocalIndex e;
	
	double i_T, i_v, i_y, i_vT; /* interpolated quantity */
	double sum_T, sum_vT, sum_yT; /* integral sum */
	double _sum_T, _sum_vT, _sum_yT; /* element sums */
	double g_sum_T, g_sum_vT, g_sum_yT; /* element sums */
	int n_elements;
        ElementType *elementType;
        Node_Index elementNodeCount;
	Dimension_Index dim;
	double det_jac, dt;
	double **GNx;
	double g_sum_vol, _sum_vol, sum_vol;


	ctx = (Underworld_BuoyancyIntegrals_CTX*)LiveComponentRegister_Get(
			context->CF->LCRegister,
			Underworld_BuoyancyIntegrals_Type );

	/* initialise values to compute */
	*B = *w_bar = *y_b = -1.0;
	*int_w_bar_dt = ctx->int_w_bar_dt;



#if 0
	/* test 1 - get beta */
	*B = ctx->beta;	
	/* test 2 - get gravity */
	*w_bar = ctx->gravity;
	/* test 3 get dt */
	*y_b = context->dt;
#endif	

	/* evaluate some integrals */

#if 0
	ngp = context->gaussSwarm->particleLocalCount;
	cell_I = 0;
	for( p = 0; p<ngp; p++ ) {
		ip = (IntegrationPoint*)Swarm_ParticleInCellAt( context->gaussSwarm, cell_I, p );
		xi = ip->xi;
		weight = ip->weight;
		///*
		printf("[%d] w = %f : xi = %f %f \n", p, weight, xi[0], xi[1] );
		//*/
	}
#endif

	/* assuming all elements are the same */
	dim = ctx->dim;
	elementType = FeMesh_GetElementType( context->velocityField->feMesh, 0 );
	elementNodeCount = elementType->nodeCount;
	GNx = Memory_Alloc_2DArray( double, dim, elementNodeCount, "Global Shape Function Derivatives for mayhem" );


	_sum_T = _sum_vT = _sum_yT = 0.0;
	_sum_vol = 0.0;

	ngp = context->gaussSwarm->particleLocalCount;
	n_elements = FeMesh_GetElementLocalSize( context->velocityField->feMesh );
//	printf("n_elements = %d \n", n_elements );

	for( e=0; e<n_elements; e++ ) {
		cell_I = CellLayout_MapElementIdToCellId( context->gaussSwarm->cellLayout, e );
		elementType = FeMesh_GetElementType( context->velocityField->feMesh, e );

		sum_T  = 0.0;
		sum_vT = 0.0;
		sum_yT = 0.0;
		sum_vol = 0.0;

		i_T = i_v = i_vT = i_y = 0.0;

		for( p=0; p<ngp; p++ ) {
			ip = (IntegrationPoint*)Swarm_ParticleInCellAt( context->gaussSwarm, cell_I, p );
			xi = ip->xi;
			weight = ip->weight;

	                ElementType_ShapeFunctionsGlobalDerivs(
        	                elementType,
                	        context->velocityField->feMesh, e,
                        	xi, dim, &det_jac, GNx );
			
			_FeVariable_InterpolateNodeValuesToElLocalCoord( context->temperatureField, e, xi, &i_T );
			
			_FeVariable_InterpolateNodeValuesToElLocalCoord( context->velocityField, e, xi, velocity );
			i_v = velocity[1];
			i_vT = i_v * i_T;
			
			FeMesh_CoordLocalToGlobal( context->velocityField->feMesh, e, xi, global_coord );
			i_y = global_coord[1];
			
			//printf("%f %f %f %f J=%f \n", i_T, i_v, i_vT, i_y, det_jac );
			
			sum_T  = sum_T  + weight * i_T * det_jac;
			sum_vT = sum_vT + weight * i_vT * det_jac;
			sum_yT = sum_yT + weight * i_y * i_T * det_jac;
			sum_vol = sum_vol + weight * det_jac;
		}

		_sum_T = _sum_T + sum_T;
		_sum_vT = _sum_vT + sum_vT;
		_sum_yT = _sum_yT + sum_yT;
		_sum_vol = _sum_vol + sum_vol;		

	}
	//printf("%f %f %f \n", _sum_T, _sum_vT, _sum_yT );


	
	/* all reduce */
	MPI_Allreduce ( &_sum_T, &g_sum_T, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
	MPI_Allreduce ( &_sum_vT, &g_sum_vT, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
	MPI_Allreduce ( &_sum_yT, &g_sum_yT, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
/*
	MPI_Allreduce ( &_sum_vol, &g_sum_vol, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
	printf("l_sum_vol = %f \n", _sum_vol );
	printf("g_sum_vol = %f \n", g_sum_vol );
*/

	*B     = ctx->gravity * ctx->beta * g_sum_T;
	*w_bar = (ctx->gravity * ctx->beta/ *B) * g_sum_vT;
	*y_b   = (ctx->gravity * ctx->beta/ *B) * g_sum_yT;

	dt = context->dt;
	*int_w_bar_dt = *int_w_bar_dt + (*w_bar) * dt;
	ctx->int_w_bar_dt = *int_w_bar_dt;

	Memory_Free( GNx );

}



void eval_temperature( UnderworldContext *context, double y_b, double *temp_b )
{
        Underworld_BuoyancyIntegrals_CTX *ctx;
        IntegrationPoint *ip;
        double *xi;
        double weight;
        Particle_InCellIndex p, ngp;
        Cell_Index cell_I;
        double velocity[3], global_coord[3];
        Element_LocalIndex e;
	double x_b, z_b;
	Stg_Shape *shape;

	*temp_b = 66.99;
	return;

        ctx = (Underworld_BuoyancyIntegrals_CTX*)LiveComponentRegister_Get(
                        context->CF->LCRegister,
                        Underworld_BuoyancyIntegrals_Type );

	/* Get x_b, and z_b from xml */
	/* "cylinder" z_b = CentreZ (0.5), x_b = CentreX (1.0) */
	
	shape = (Stg_Shape*)Stg_ComponentFactory_ConstructByName( context->CF, "cylinder", Stg_Shape, False, 0 /* dummy */ );

	global_coord[0] = x_b;
	global_coord[1] = y_b;
	global_coord[2] = z_b;




}


void Underworld_BuoyancyIntegrals_Output( UnderworldContext *context ) 
{
	double B, w_bar, y_b, int_w_bar_dt;
	double temp_b; /* the temperature at (x_b,y_b,z_b) */

	perform_integrals( context, &B, &w_bar, &y_b, &int_w_bar_dt );
	eval_temperature( context, y_b, &temp_b );

	StgFEM_FrequentOutput_PrintValue( context, B );
	StgFEM_FrequentOutput_PrintValue( context, w_bar );
	StgFEM_FrequentOutput_PrintValue( context, y_b );
	StgFEM_FrequentOutput_PrintValue( context, int_w_bar_dt );
	StgFEM_FrequentOutput_PrintValue( context, temp_b );
}
