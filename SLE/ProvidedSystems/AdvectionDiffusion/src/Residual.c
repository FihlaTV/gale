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
** $Id: Residual.c 985 2007-11-21 00:20:24Z MirkoVelic $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StgDomain/StgDomain.h>
#include <StgFEM/Discretisation/Discretisation.h>
#include <StgFEM/SLE/SystemSetup/SystemSetup.h>
#include <PICellerator/PICellerator.h>
#include <Underworld/Underworld.h>

#include "types.h"
#include "AdvectionDiffusionSLE.h"
#include "Residual.h"
#include "UpwindParameter.h"

#include <assert.h>
#include <string.h>

/* Textual name of this class */
const Type AdvDiffResidualForceTerm_Type = "AdvDiffResidualForceTerm";

AdvDiffResidualForceTerm* AdvDiffResidualForceTerm_New( 
	Name							name,
	FiniteElementContext*	context,
	ForceVector*				forceVector,
	Swarm*						integrationSwarm,
	Stg_Component*				sle, 
	FeVariable*					velocityField,
	double						defaultDiffusivity,
        Swarm*						picSwarm,
	void*		materials_Register,
	AdvDiffResidualForceTerm_UpwindParamFuncType upwindFuncType )
{
	AdvDiffResidualForceTerm* self = (AdvDiffResidualForceTerm*) _AdvDiffResidualForceTerm_DefaultNew( name );

	self->isConstructed = True;
	_ForceTerm_Init( self, context, forceVector, integrationSwarm, sle );
	_AdvDiffResidualForceTerm_Init( self, velocityField, defaultDiffusivity,
                                        picSwarm,
                                        materials_Register, upwindFuncType );

	return self;
}

void* _AdvDiffResidualForceTerm_DefaultNew( Name name ) {
	/* Variables set in this function */
	SizeT                                                  _sizeOfSelf = sizeof(AdvDiffResidualForceTerm);
	Type                                                          type = AdvDiffResidualForceTerm_Type;
	Stg_Class_DeleteFunction*                                  _delete = _AdvDiffResidualForceTerm_Delete;
	Stg_Class_PrintFunction*                                    _print = _AdvDiffResidualForceTerm_Print;
	Stg_Class_CopyFunction*                                      _copy = NULL;
	Stg_Component_DefaultConstructorFunction*      _defaultConstructor = _AdvDiffResidualForceTerm_DefaultNew;
	Stg_Component_ConstructFunction*                        _construct = _AdvDiffResidualForceTerm_AssignFromXML;
	Stg_Component_BuildFunction*                                _build = _AdvDiffResidualForceTerm_Build;
	Stg_Component_InitialiseFunction*                      _initialise = _AdvDiffResidualForceTerm_Initialise;
	Stg_Component_ExecuteFunction*                            _execute = _AdvDiffResidualForceTerm_Execute;
	Stg_Component_DestroyFunction*                            _destroy = _AdvDiffResidualForceTerm_Destroy;
	ForceTerm_AssembleElementFunction*                _assembleElement = _AdvDiffResidualForceTerm_AssembleElement;
	AdvDiffResidualForceTerm_UpwindParamFunction*         _upwindParam = _AdvDiffResidualForceTerm_UpwindParam;

	/* Variables that are set to ZERO are variables that will be set either by the current _New function or another parent _New function further up the hierachy */
	AllocationType  nameAllocationType = NON_GLOBAL /* default value NON_GLOBAL */;

	return (void*)_AdvDiffResidualForceTerm_New(  ADVDIFFRESIDUALFORCETERM_PASSARGS  );
}

/* Creation implementation / Virtual constructor */
AdvDiffResidualForceTerm* _AdvDiffResidualForceTerm_New(  ADVDIFFRESIDUALFORCETERM_DEFARGS  )
{
	AdvDiffResidualForceTerm* self;
	
	/* Allocate memory */
	assert( _sizeOfSelf >= sizeof(AdvDiffResidualForceTerm) );
	/* The following terms are parameters that have been passed into this function but are being set before being passed onto the parent */
	/* This means that any values of these parameters that are passed into this function are not passed onto the parent function
	   and so should be set to ZERO in any children of this class. */
	nameAllocationType = NON_GLOBAL;

	self = (AdvDiffResidualForceTerm*) _ForceTerm_New(  FORCETERM_PASSARGS  );
	
	/* Virtual info */
	self->_upwindParam = _upwindParam;
	
	return self;
}

/*******************************************************************************
  The following function scans all the elements of the mesh associated with
  the residual forceterm phi to find the element with the most nodes.
  Once the maximum number of nodes is found we then may allocate memory for
  GNx etc that now live on the AdvDiffResidualForceTerm struct. This way we 
  do not reallocate memory for these arrays for every element.
 *******************************************************************************/
void __AdvDiffResidualForceTerm_UpdateLocalMemory( AdvectionDiffusionSLE* sle ){
	FeVariable*				phiField = sle->phiField;
	FeMesh*					phiMesh = phiField->feMesh;
	Dimension_Index 		dim = phiField->dim;
	Element_LocalIndex	e, n_elements;
	Node_Index				max_elementNodeCount;

	n_elements = FeMesh_GetElementLocalSize(phiMesh);//returns number of elements in a mesh

	/* Scan all elements in Mesh to get max node count */
	max_elementNodeCount = 0;

	for(e=0;e<n_elements;e++){
		ElementType *elementType = FeMesh_GetElementType( phiMesh, e );
		Node_Index elementNodeCount = elementType->nodeCount;
	
		if( elementNodeCount > max_elementNodeCount){
			max_elementNodeCount = elementNodeCount;
		}
	}
       
	sle->advDiffResidualForceTerm->GNx = Memory_Alloc_2DArray( double, dim, max_elementNodeCount, (Name)"(SUPG): Global Shape Function Derivatives" );
	sle->advDiffResidualForceTerm->phiGrad = Memory_Alloc_Array(double, dim, "(SUPG): Gradient of the Advected Scalar");
	sle->advDiffResidualForceTerm->Ni = Memory_Alloc_Array(double, max_elementNodeCount, "(SUPG): Gradient of the Advected Scalar");
	sle->advDiffResidualForceTerm->SUPGNi = Memory_Alloc_Array(double, max_elementNodeCount, "(SUPG): Upwinded Shape Function");
	sle->advDiffResidualForceTerm->incarray=IArray_New();
}

void __AdvDiffResidualForceTerm_FreeLocalMemory( AdvectionDiffusionSLE* sle ){
	Memory_Free(sle->advDiffResidualForceTerm->GNx);
	Memory_Free(sle->advDiffResidualForceTerm->phiGrad);
	Memory_Free(sle->advDiffResidualForceTerm->Ni);
	Memory_Free(sle->advDiffResidualForceTerm->SUPGNi);
  
	NewClass_Delete(sle->advDiffResidualForceTerm->incarray);
}

void _AdvDiffResidualForceTerm_Init(
	void*			residual,
	FeVariable*		velocityField,
	double			defaultDiffusivity,
        Swarm*			picSwarm,
	void*	                materials_Register,
	AdvDiffResidualForceTerm_UpwindParamFuncType	upwindFuncType ) //WHY IS THIS LINE HERE???
{
	AdvDiffResidualForceTerm* self = (AdvDiffResidualForceTerm*)residual;

	self->velocityField = velocityField;
	self->defaultDiffusivity = defaultDiffusivity;
        self->picSwarm = picSwarm;
	self->materials_Register  = materials_Register;
	self->upwindParamType = upwindFuncType;
}

void _AdvDiffResidualForceTerm_Delete( void* residual ) {
	AdvDiffResidualForceTerm* self = (AdvDiffResidualForceTerm*)residual;

	_ForceTerm_Delete( self );
}

void _AdvDiffResidualForceTerm_Print( void* residual, Stream* stream ) {
	AdvDiffResidualForceTerm* self = (AdvDiffResidualForceTerm*)residual;
	
	_ForceTerm_Print( self, stream );

	Journal_Printf( stream, "self->calculateUpwindParam = %s\n", 
		self->_upwindParam == AdvDiffResidualForceTerm_UpwindXiExact ? 
			"AdvDiffResidualForceTerm_UpwindXiExact" :
		self->_upwindParam == AdvDiffResidualForceTerm_UpwindXiDoublyAsymptoticAssumption ? 
			"AdvDiffResidualForceTerm_UpwindXiDoublyAsymptoticAssumption" :
		self->_upwindParam == AdvDiffResidualForceTerm_UpwindXiCriticalAssumption ? 
		"AdvDiffResidualForceTerm_UpwindXiCriticalAssumption" : "Unknown"  );

	/* General info */
	Journal_PrintPointer( stream, self->velocityField );
	Journal_PrintDouble( stream, self->defaultDiffusivity );
}

void _AdvDiffResidualForceTerm_AssignFromXML( void* residual, Stg_ComponentFactory* cf, void* data ) {
	AdvDiffResidualForceTerm*							self = (AdvDiffResidualForceTerm*)residual;
	FeVariable*												velocityField;
	Name														upwindParamFuncName;
	double													defaultDiffusivity;
	Materials_Register*		materials_Register;
	AdvDiffResidualForceTerm_UpwindParamFuncType	upwindFuncType = 0;
        Swarm *picSwarm;

	/* Construct Parent */
	_ForceTerm_AssignFromXML( self, cf, data );

	velocityField = Stg_ComponentFactory_ConstructByKey( cf, self->name, (Dictionary_Entry_Key)"VelocityField", FeVariable, True, data  );
	upwindParamFuncName = Stg_ComponentFactory_GetString( cf, self->name, (Dictionary_Entry_Key)"UpwindXiFunction", "Exact"  );

	if ( strcasecmp( upwindParamFuncName, "DoublyAsymptoticAssumption" ) == 0 )
		upwindFuncType = DoublyAsymptoticAssumption;
	else if ( strcasecmp( upwindParamFuncName, "CriticalAssumption" ) == 0 )
		upwindFuncType = CriticalAssumption;
	else if ( strcasecmp( upwindParamFuncName, "Exact" ) == 0 )
		upwindFuncType = Exact;
	else 
		Journal_Firewall( False, Journal_Register( Error_Type, (Name)self->type  ), "Cannot understand '%s'\n", upwindParamFuncName );

	defaultDiffusivity = Stg_ComponentFactory_GetDouble( cf, self->name, (Dictionary_Entry_Key)"defaultDiffusivity", 1.0  );
	picSwarm       = Stg_ComponentFactory_ConstructByKey( cf, self->name, (Dictionary_Entry_Key)"picSwarm", Swarm, True, data  ) ;
	materials_Register = ((PICelleratorContext*)(self->context))->materials_Register;

	_AdvDiffResidualForceTerm_Init( self, velocityField, defaultDiffusivity,
                                        picSwarm, materials_Register,
                                        upwindFuncType );
}

void _AdvDiffResidualForceTerm_Build( void* residual, void* data ) {
	AdvDiffResidualForceTerm* self = (AdvDiffResidualForceTerm*)residual;
	AdvDiffResidualForceTerm_MaterialExt*   materialExt;
	Material_Index                   material_I;
	Material*                        material;
	Materials_Register*              materials_Register = self->materials_Register;
	IntegrationPointsSwarm*          swarm              = (IntegrationPointsSwarm*)self->picSwarm;
	MaterialPointsSwarm**            materialSwarms;
	Index                            materialSwarm_I;
	Stg_ComponentFactory*            cf;
	Name                             name;

	cf = self->context->CF;

	_ForceTerm_Build( self, data );

	Stg_Component_Build( self->velocityField, data, False );

	/* Sort out material extension stuff */
	self->materialExtHandle = Materials_Register_AddMaterialExtension( 
			self->materials_Register, 
			self->type, 
			sizeof(AdvDiffResidualForceTerm_MaterialExt) );
	for ( material_I = 0 ; material_I < Materials_Register_GetCount( materials_Register ) ; material_I++) {
		material = Materials_Register_GetByIndex( materials_Register, material_I );
		materialExt = ExtensionManager_GetFunc( material->extensionMgr, material, self->materialExtHandle );

		materialExt->diffusivity = Stg_ComponentFactory_GetDouble( cf, material->name,
                                                                           (Dictionary_Entry_Key)"diffusivity",
                                                                           self->defaultDiffusivity );
	}
	
	/* Create Swarm Variables of each material swarm this ip swarm is mapped against */
	materialSwarms = IntegrationPointMapper_GetMaterialPointsSwarms( swarm->mapper, &(self->materialSwarmCount) );
	self->diffusivitySwarmVariables = Memory_Alloc_Array( MaterialSwarmVariable*, self->materialSwarmCount, "DiffusivityVariables" );
	
	for ( materialSwarm_I = 0; materialSwarm_I < self->materialSwarmCount; ++materialSwarm_I ) {
		name = Stg_Object_AppendSuffix( materialSwarms[materialSwarm_I], (Name)"Diffusivity"  );
		self->diffusivitySwarmVariables[materialSwarm_I] = MaterialSwarmVariable_New( 
				name,
				(AbstractContext*)self->context,
				materialSwarms[materialSwarm_I], 
				1, 
				self->materials_Register, 
				self->materialExtHandle, 
				GetOffsetOfMember( *materialExt, diffusivity ) );
		Memory_Free( name );

		/* Build new Swarm Variables */
		Stg_Component_Build( self->diffusivitySwarmVariables[materialSwarm_I], data, False );
	}
}

void _AdvDiffResidualForceTerm_Initialise( void* residual, void* data ) {
	AdvDiffResidualForceTerm* self = (AdvDiffResidualForceTerm*)residual;
	Index                          i;

	_ForceTerm_Initialise( self, data );

	Stg_Component_Initialise( self->velocityField, data, False );

	for ( i = 0; i < self->materialSwarmCount; ++i ) {
		Stg_Component_Initialise( self->diffusivitySwarmVariables[i], data, False );
	}
}

void _AdvDiffResidualForceTerm_Execute( void* residual, void* data ) {
	AdvDiffResidualForceTerm* self = (AdvDiffResidualForceTerm*)residual;

	_ForceTerm_Execute( self, data );
}

void _AdvDiffResidualForceTerm_Destroy( void* residual, void* data ) {
	AdvDiffResidualForceTerm* self = (AdvDiffResidualForceTerm*)residual;
	Index                          i;

	for ( i = 0; i < self->materialSwarmCount; ++i ) {
		_Stg_Component_Delete( self->diffusivitySwarmVariables[i] );
	}
	Memory_Free( self->diffusivitySwarmVariables );

	_ForceTerm_Destroy( self, data );
}

void _AdvDiffResidualForceTerm_AssembleElement( void* forceTerm, ForceVector* forceVector, Element_LocalIndex lElement_I, double* elementResidual ) {
	AdvDiffResidualForceTerm*  self               = Stg_CheckType( forceTerm, AdvDiffResidualForceTerm );
	AdvectionDiffusionSLE*     sle                = Stg_CheckType( self->extraInfo, AdvectionDiffusionSLE );
	Swarm*                     swarm              = self->picSwarm;
	Particle_Index             lParticle_I;
	Particle_Index             cParticle_I;
	Particle_Index             cellParticleCount;
	Cell_Index                 cell_I;    
	IntegrationPoint*          particle;
	FeVariable*                phiField           = sle->phiField;
	Dimension_Index            dim                = forceVector->dim;
	double                     velocity[3];
	double                     phi, phiDot;
	double                     detJac;
	double*                    xi;
	double                     totalDerivative, diffusionTerm;
	double                     diffusivity         = self->defaultDiffusivity;
	ElementType*               elementType         = FeMesh_GetElementType( phiField->feMesh, lElement_I );
	Node_Index                 elementNodeCount    = elementType->nodeCount;
	Node_Index                 node_I;
	double                     factor;

	double**                   GNx;
	double*                    phiGrad;
	double*                    Ni;
	double*                    SUPGNi;
	double                     supgfactor;
	double                     udotu, perturbation;
	double                     upwindDiffusivity;

	GNx     = self->GNx;
	phiGrad = self->phiGrad;
	Ni = self->Ni;
	SUPGNi = self->SUPGNi;
	
	upwindDiffusivity  = AdvDiffResidualForceTerm_UpwindDiffusivity( self, sle, swarm, phiField->feMesh, lElement_I, dim );

	/* Determine number of particles in element */
	cell_I = CellLayout_MapElementIdToCellId( swarm->cellLayout, lElement_I );
	cellParticleCount = swarm->cellParticleCountTbl[ cell_I ];
	
	for ( cParticle_I = 0 ; cParticle_I < cellParticleCount ; cParticle_I++ ) {
		lParticle_I     = swarm->cellParticleTbl[cell_I][cParticle_I];

		particle        = (IntegrationPoint*) Swarm_ParticleAt( swarm, lParticle_I );
		xi              = particle->xi;
		
		/* Evaluate Shape Functions */
		ElementType_EvaluateShapeFunctionsAt(elementType, xi, Ni);

		/* Calculate Global Shape Function Derivatives */
		ElementType_ShapeFunctionsGlobalDerivs( 
			elementType,
			phiField->feMesh, lElement_I,
			xi, dim, &detJac, GNx );
		
		/* Calculate Velocity */
		FeVariable_InterpolateFromMeshLocalCoord( self->velocityField, phiField->feMesh, lElement_I, xi, velocity );

		/* Build the SUPG shape functions */
		udotu = velocity[I_AXIS]*velocity[I_AXIS] + velocity[J_AXIS]*velocity[J_AXIS];
		if(dim == 3) udotu += velocity[ K_AXIS ] * velocity[ K_AXIS ];

		supgfactor = upwindDiffusivity / udotu;
		for ( node_I = 0 ; node_I < elementNodeCount ; node_I++ ) {
			/* In the case of per diffusion - just build regular shape functions */
			if ( fabs(upwindDiffusivity) < SUPG_MIN_DIFFUSIVITY ) {
				SUPGNi[node_I] = Ni[node_I];
				continue;
			}
			
			perturbation = velocity[ I_AXIS ] * GNx[ I_AXIS ][ node_I ] + velocity[ J_AXIS ] * GNx[ J_AXIS ][ node_I ];
			if (dim == 3)
					perturbation = perturbation + velocity[ K_AXIS ] * GNx[ K_AXIS ][ node_I ];
			
			/* p = \frac{\bar \kappa \hat u_j w_j }{ ||u|| } -  Eq. 3.2.25 */
			perturbation = supgfactor * perturbation;
			
			SUPGNi[node_I] = Ni[node_I] + perturbation;
		}  
		
		/* Calculate phi on particle */
		_FeVariable_InterpolateNodeValuesToElLocalCoord( phiField, lElement_I, xi, &phi );

		/* Calculate Gradients of Phi */
		FeVariable_InterpolateDerivatives_WithGNx( phiField, lElement_I, GNx, phiGrad );

		/* Calculate time derivative of phi */
		_FeVariable_InterpolateNodeValuesToElLocalCoord( sle->phiDotField, lElement_I, xi, &phiDot );
		
		/* Calculate total derivative (i.e. Dphi/Dt = \dot \phi + u . \grad \phi) */
		totalDerivative = phiDot + StGermain_VectorDotProduct( velocity, phiGrad, dim );

		/* Get Diffusivity */
		diffusivity = IntegrationPointMapper_GetDoubleFromMaterial(((IntegrationPointsSwarm *)swarm)->mapper, particle, self->materialExtHandle,
		    offsetof(AdvDiffResidualForceTerm_MaterialExt, diffusivity));

		/* Add to element residual */
		factor = particle->weight * detJac;
		for ( node_I = 0 ; node_I < elementNodeCount ; node_I++ ) {
			/* Calculate Diffusion Term */
			diffusionTerm = diffusivity * ( GNx[0][node_I] * phiGrad[0] + GNx[1][node_I] * phiGrad[1] );
			if (dim == 3)
				diffusionTerm += diffusivity * GNx[2][ node_I ] * phiGrad[2] ;
			
			elementResidual[ node_I ] -=  factor * ( SUPGNi[ node_I ] * totalDerivative + diffusionTerm );
		}
	}
	
}

/* Virtual Function Implementations */
double _AdvDiffResidualForceTerm_UpwindParam( void* residual, double pecletNumber ) {
	AdvDiffResidualForceTerm* self = (AdvDiffResidualForceTerm*)residual;

	switch ( self->upwindParamType ) {
		case Exact:
			self->_upwindParam = AdvDiffResidualForceTerm_UpwindXiExact; break;
		case DoublyAsymptoticAssumption:
			self->_upwindParam = AdvDiffResidualForceTerm_UpwindXiDoublyAsymptoticAssumption; break;
		case CriticalAssumption:
			self->_upwindParam = AdvDiffResidualForceTerm_UpwindXiCriticalAssumption; break;
	}

	return AdvDiffResidualForceTerm_UpwindParam( self, pecletNumber );
}


