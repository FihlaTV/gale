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
*%		Cecile Duboz - Cecile.Duboz@sci.monash.edu.au
*%
** Contributors:
*+		Cecile Duboz
*+		Robert Turnbull
*+		Alan Lo
*+		Louis Moresi
*+		David Stegman
*+		David May
*+		Stevan Quenette
*+		Patrick Sunter
*+		Greg Watson
*+
** $Id: SwarmVectors.c 791 2008-09-01 02:09:06Z JulianGiordani $
** 
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StgDomain/StgDomain.h>

#include <glucifer/Base/Base.h>
#include <glucifer/RenderingEngines/RenderingEngines.h>

#include "types.h"
#include "OpenGLDrawingObject.h"
#include "SwarmViewerBase.h"
#include "SwarmViewer.h"
#include "SwarmVectors.h"

#include <assert.h>
#include <gl.h>
#include <glu.h>
#include <string.h>

/* Textual name of this class - This is a global pointer which is used for times when you need to refer to class and not a particular instance of a class */
const Type lucSwarmVectors_Type = "lucSwarmVectors";

/* Private Constructor: This will accept all the virtual functions for this class as arguments. */
lucSwarmVectors* _lucSwarmVectors_New(  LUCSWARMVECTORS_DEFARGS  ) 
{
	lucSwarmVectors*					self;

	/* Call private constructor of parent - this will set virtual functions of parent and continue up the hierarchy tree. At the beginning of the tree it will allocate memory of the size of object and initialise all the memory to zero. */
	assert( _sizeOfSelf >= sizeof(lucSwarmVectors) );
	self = (lucSwarmVectors*) _lucSwarmViewerBase_New(  LUCSWARMVIEWERBASE_PASSARGS  );
	
	return self;
}

void _lucSwarmVectors_Init( 
		lucSwarmVectors*                                   self,
		Name                                               directionVariableName,
		double                                             arrowHeadSize,
		Name                                               thicknessVariableName,
		double                                             thickness,
		Name                                               lengthVariableName,
		double                                             length )
{
	Stream* errorStream         = Journal_MyStream( Error_Type, self );
	
	self->directionVariableName = directionVariableName;
	self->arrowHeadSize         = arrowHeadSize;
	Journal_Firewall( ( arrowHeadSize <= 1 && arrowHeadSize >= 0 ), errorStream,
			"Error in %s:\narrowHeadSize given for %s was not in the range [0, 1]. " 
			"Please use an arrowHeadSize within this range\n", __func__, self->name );
	self->thicknessVariableName = thicknessVariableName;
	self->thickness             = thickness;
	self->lengthVariableName    = lengthVariableName;
	self->length                = length;
}

void _lucSwarmVectors_Delete( void* drawingObject ) {
	lucSwarmVectors*  self = (lucSwarmVectors*)drawingObject;

	_lucSwarmViewerBase_Delete( self );
}

void _lucSwarmVectors_Print( void* drawingObject, Stream* stream ) {
	lucSwarmVectors*  self = (lucSwarmVectors*)drawingObject;

	_lucSwarmViewerBase_Print( self, stream );
}

void* _lucSwarmVectors_Copy( void* drawingObject, void* dest, Bool deep, Name nameExt, PtrMap* ptrMap) {
	lucSwarmVectors*  self = (lucSwarmVectors*)drawingObject;
	lucSwarmVectors* newDrawingObject;

	newDrawingObject = _lucSwarmViewerBase_Copy( self, dest, deep, nameExt, ptrMap );

	/* TODO */
	abort();

	return (void*) newDrawingObject;
}


void* _lucSwarmVectors_DefaultNew( Name name ) {
	/* Variables set in this function */
	SizeT                                                     _sizeOfSelf = sizeof(lucSwarmVectors);
	Type                                                             type = lucSwarmVectors_Type;
	Stg_Class_DeleteFunction*                                     _delete = _lucSwarmVectors_Delete;
	Stg_Class_PrintFunction*                                       _print = _lucSwarmVectors_Print;
	Stg_Class_CopyFunction*                                         _copy = NULL;
	Stg_Component_DefaultConstructorFunction*         _defaultConstructor = _lucSwarmVectors_DefaultNew;
	Stg_Component_ConstructFunction*                           _construct = _lucSwarmVectors_AssignFromXML;
	Stg_Component_BuildFunction*                                   _build = _lucSwarmVectors_Build;
	Stg_Component_InitialiseFunction*                         _initialise = _lucSwarmVectors_Initialise;
	Stg_Component_ExecuteFunction*                               _execute = _lucSwarmVectors_Execute;
	Stg_Component_DestroyFunction*                               _destroy = _lucSwarmVectors_Destroy;
	lucDrawingObject_SetupFunction*                                _setup = _lucSwarmVectors_Setup;
	lucDrawingObject_DrawFunction*                                  _draw = _lucSwarmVectors_Draw;
	lucDrawingObject_CleanUpFunction*                            _cleanUp = _lucSwarmVectors_CleanUp;
	lucOpenGLDrawingObject_BuildDisplayListFunction*    _buildDisplayList = _lucSwarmVectors_BuildDisplayList;
	lucSwarmViewerBase_PlotParticleFunction*                _plotParticle = _lucSwarmVectors_PlotParticle;
	lucSwarmViewerBase_SetParticleColourFunction*      _setParticleColour = _lucSwarmViewerBase_SetParticleColourDefault;

	/* Variables that are set to ZERO are variables that will be set either by the current _New function or another parent _New function further up the hierachy */
	AllocationType  nameAllocationType = NON_GLOBAL /* default value NON_GLOBAL */;

	return (void*) _lucSwarmVectors_New(  LUCSWARMVECTORS_PASSARGS  );
}

void _lucSwarmVectors_AssignFromXML( void* drawingObject, Stg_ComponentFactory* cf, void* data ){
	lucSwarmVectors*  self = (lucSwarmVectors*)drawingObject;

	/* Construct Parent */
	_lucSwarmViewerBase_AssignFromXML( self, cf, data );

	_lucSwarmVectors_Init( 
			self,
			Stg_ComponentFactory_GetString( cf, self->name, (Dictionary_Entry_Key)"DirectionVariable", ""  ),
			Stg_ComponentFactory_GetDouble( cf, self->name, (Dictionary_Entry_Key)"arrowHeadSize", 0.5  ),
			Stg_ComponentFactory_GetString( cf, self->name, (Dictionary_Entry_Key)"ThicknessVariable", ""  ),
			Stg_ComponentFactory_GetDouble( cf, self->name, (Dictionary_Entry_Key)"thickness", 1.0  ),
			Stg_ComponentFactory_GetString( cf, self->name, (Dictionary_Entry_Key)"LengthVariable", ""  ),
			Stg_ComponentFactory_GetDouble( cf, self->name, (Dictionary_Entry_Key)"length", 0.2 )  );
}

void _lucSwarmVectors_Build( void* drawingObject, void* data ) {
	lucSwarmVectors*         self                   = (lucSwarmVectors*)drawingObject;

	_lucSwarmViewerBase_Build( self, data );
}
void _lucSwarmVectors_Initialise( void* drawingObject, void* data ) {
	lucSwarmVectors*         self                   = (lucSwarmVectors*)drawingObject;
	SwarmVariable_Register*  swarmVariable_Register = self->swarm->swarmVariable_Register;
	Stream*                  errorStream            = Journal_MyStream( Error_Type, self );

	_lucSwarmViewerBase_Initialise( self, data );

	if ( 0 != strcmp( self->directionVariableName, "" ) ) {
		self->directionVariable    = SwarmVariable_Register_GetByName( swarmVariable_Register, self->directionVariableName );
		Journal_Firewall( self->directionVariable != NULL, errorStream, 
				"Error in func %s for %s '%s' - Cannot find SwarmVariable %s to be variable for direction vector.\n", 
				__func__, self->type, self->name, self->directionVariableName );
		Stg_Component_Build( self->directionVariable, data, False );
		Stg_Component_Initialise( self->directionVariable, data, False );
	}

	if ( 0 != strcmp( self->thicknessVariableName, "" ) ) {
		self->thicknessVariable    = SwarmVariable_Register_GetByName( swarmVariable_Register, self->thicknessVariableName );
		Journal_Firewall( self->thicknessVariable != NULL, errorStream,
			"Error - for gLucifer drawing object \"%s\" - in %s(): thickness Variable name given was \"%s\", "
			"but no corresponding SwarmVariable found in the register for swarm \"%s\".\n",
			self->name, __func__, self->thicknessVariableName, self->swarm->name );
		
		Stg_Component_Build( self->thicknessVariable, data, False );
		Stg_Component_Initialise( self->thicknessVariable, data, False );
	}
	if ( 0 != strcmp( self->lengthVariableName, "" ) ) {
		self->lengthVariable       = SwarmVariable_Register_GetByName( swarmVariable_Register, self->lengthVariableName );
		Journal_Firewall( self->lengthVariable != NULL, errorStream,
			"Error - for gLucifer drawing object \"%s\" - in %s(): length Variable name given was \"%s\", "
			"but no corresponding SwarmVariable found in the register for swarm \"%s\".\n",
			self->name, __func__, self->lengthVariableName, self->swarm->name );
		
		Stg_Component_Build( self->lengthVariable, data, False );
		Stg_Component_Initialise( self->lengthVariable, data, False );
	}
}
void _lucSwarmVectors_Execute( void* drawingObject, void* data ) {}
void _lucSwarmVectors_Destroy( void* drawingObject, void* data ) {}

void _lucSwarmVectors_Setup( void* drawingObject, void* _context ) {
	_lucSwarmViewerBase_Setup( drawingObject, _context );
	lucSwarmVectors_UpdateVariables( drawingObject );
}
	
void _lucSwarmVectors_Draw( void* drawingObject, lucWindow* window, lucViewportInfo* viewportInfo, void* _context ) {
	_lucSwarmViewerBase_Draw( drawingObject, window, viewportInfo, _context );
}

void _lucSwarmVectors_CleanUp( void* drawingObject, void* _context ) {
	_lucSwarmViewerBase_CleanUp( drawingObject, _context );
}

void _lucSwarmVectors_BuildDisplayList( void* drawingObject, void* _context ) {
	lucSwarmVectors*         self                = (lucSwarmVectors*)drawingObject;
	
	_lucSwarmViewerBase_BuildDisplayList( self, _context );
}

void _lucSwarmVectors_PlotParticle( void* drawingObject, void* _context, Particle_Index lParticle_I ) {
	lucSwarmVectors*        self                = (lucSwarmVectors*)drawingObject;
	DomainContext*   		context             = (DomainContext*) _context;
	GlobalParticle*         particle            = (GlobalParticle*)Swarm_ParticleAt( self->swarm, lParticle_I );
	SwarmVariable*          lengthVariable      = self->lengthVariable;
	SwarmVariable*          thicknessVariable   = self->thicknessVariable;
	double*                 coord               = particle->coord;
	double                  length              = self->length;
	double                  thickness           = self->thickness;
	XYZ                     direction           = { 0, 0, 0 };

	if ( self->directionVariable )
		SwarmVariable_ValueAt( self->directionVariable, lParticle_I, direction );

	if ( lengthVariable )
		SwarmVariable_ValueAt( lengthVariable, lParticle_I, &length );

	if ( thicknessVariable )
		SwarmVariable_ValueAt( thicknessVariable, lParticle_I, &thickness );

	glLineWidth( (float) thickness );

	luc_DrawVector( context->dim, coord, direction, length, self->arrowHeadSize );
}

void lucSwarmVectors_UpdateVariables( void* drawingObject ) {
	lucSwarmVectors*          self                = (lucSwarmVectors*)drawingObject;
	lucSwarmViewerBase_UpdateVariables( drawingObject ) ;

	if ( self->directionVariable && self->directionVariable->variable ) {
		Variable_Update( self->directionVariable->variable );
	}
	if ( self->thicknessVariable && self->thicknessVariable->variable ) {
		Variable_Update( self->thicknessVariable->variable );
	}
	if ( self->lengthVariable && self->lengthVariable->variable ) {
		Variable_Update( self->lengthVariable->variable );
	}
}


