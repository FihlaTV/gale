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
** $Id: EigenvectorsCrossSection.c 791 2008-09-01 02:09:06Z JulianGiordani $
** 
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


#include <mpi.h>
#include <StGermain/StGermain.h>
#include <StgDomain/StgDomain.h>

#include <glucifer/Base/Base.h>
#include <glucifer/RenderingEngines/RenderingEngines.h>

#include "types.h"
#include "CrossSection.h"
#include "EigenvectorsCrossSection.h"

#include <assert.h>
#include <gl.h>
#include <glu.h>
#include <string.h>
#include <ctype.h>

#ifndef MASTER
	#define MASTER 0
#endif

/* Textual name of this class - This is a global pointer which is used for times when you need to refer to class and not a particular instance of a class */
const Type lucEigenvectorsCrossSection_Type = "lucEigenvectorsCrossSection";

/* Private Constructor: This will accept all the virtual functions for this class as arguments. */
lucEigenvectorsCrossSection* _lucEigenvectorsCrossSection_New(  LUCEIGENVECTORSCROSSSECTION_DEFARGS  ) 
{
	lucEigenvectorsCrossSection*					self;

	/* Call private constructor of parent - this will set virtual functions of parent and continue up the hierarchy tree. At the beginning of the tree it will allocate memory of the size of object and initialise all the memory to zero. */
	assert( _sizeOfSelf >= sizeof(lucEigenvectorsCrossSection) );
	self = (lucEigenvectorsCrossSection*) _lucCrossSection_New(  LUCCROSSSECTION_PASSARGS  );
	
	return self;
}

void _lucEigenvectorsCrossSection_Init( 
		lucEigenvectorsCrossSection*                                 self,
		Dimension_Index                                              dim,
		Name                                                         leastColourName,
		Name                                                         middleColourName,
		Name                                                         greatestColourName,
		IJK                                                          resolution,
		double                                                       arrowHeadSize,
		double                                                       lengthScale,
		float                                                        lineWidth,
		Bool 				                             useEigenValue,
		double                                                       notEigenValue,
		Bool 							     plotEigenVector,
		Bool 							     plotEigenValue,
		double 							     scaleEigenValue,
		Name                                                         leastColourForNegativeName,
		Name                                                         middleColourForNegativeName,
		Name                                                         greatestColourForNegativeName)
{
	Stream* errorStream         = Journal_MyStream( Error_Type, self );
	if ( dim == 2 ) {
		lucColour_FromString( &self->colours[0], leastColourName );
		lucColour_FromString( &self->colours[1], greatestColourName );
		lucColour_FromString( &self->colourForNegative[0], leastColourForNegativeName );
		lucColour_FromString( &self->colourForNegative[1], greatestColourForNegativeName );

	}
	else {
		lucColour_FromString( &self->colours[0], leastColourName );
		lucColour_FromString( &self->colours[1], middleColourName );
		lucColour_FromString( &self->colours[2], greatestColourName );

	        lucColour_FromString( &self->colourForNegative[0], leastColourForNegativeName );
		lucColour_FromString( &self->colourForNegative[1], middleColourForNegativeName );
		lucColour_FromString( &self->colourForNegative[2], greatestColourForNegativeName );
	}

	memcpy( self->resolution, resolution, sizeof(IJK) );
	self->arrowHeadSize = arrowHeadSize;
	Journal_Firewall( ( arrowHeadSize <= 1 && arrowHeadSize >= 0 ), errorStream,
			"Error in %s:\narrowHeadSize given for %s was not in the range [0, 1]. " 
			"Please use an arrowHeadSize within this range\n", __func__, self->name );
	self->lengthScale = lengthScale;
	self->lineWidth = lineWidth;
	
	
	self->useEigenValue = useEigenValue;
	self->notEigenValue = notEigenValue;

	self->plotEigenVector = plotEigenVector;
	self->plotEigenValue = plotEigenValue;
	self->scaleEigenValue = scaleEigenValue;
}

void _lucEigenvectorsCrossSection_Delete( void* drawingObject ) {
	lucEigenvectorsCrossSection*  self = (lucEigenvectorsCrossSection*)drawingObject;

	_lucCrossSection_Delete( self );
}

void _lucEigenvectorsCrossSection_Print( void* drawingObject, Stream* stream ) {
	lucEigenvectorsCrossSection*  self = (lucEigenvectorsCrossSection*)drawingObject;

	_lucCrossSection_Print( self, stream );
}

void* _lucEigenvectorsCrossSection_DefaultNew( Name name ) {
	/* Variables set in this function */
	SizeT                                                     _sizeOfSelf = sizeof(lucEigenvectorsCrossSection);
	Type                                                             type = lucEigenvectorsCrossSection_Type;
	Stg_Class_DeleteFunction*                                     _delete = _lucEigenvectorsCrossSection_Delete;
	Stg_Class_PrintFunction*                                       _print = _lucEigenvectorsCrossSection_Print;
	Stg_Class_CopyFunction*                                         _copy = NULL;
	Stg_Component_DefaultConstructorFunction*         _defaultConstructor = _lucEigenvectorsCrossSection_DefaultNew;
	Stg_Component_ConstructFunction*                           _construct = _lucEigenvectorsCrossSection_AssignFromXML;
	Stg_Component_BuildFunction*                                   _build = _lucEigenvectorsCrossSection_Build;
	Stg_Component_InitialiseFunction*                         _initialise = _lucEigenvectorsCrossSection_Initialise;
	Stg_Component_ExecuteFunction*                               _execute = _lucEigenvectorsCrossSection_Execute;
	Stg_Component_DestroyFunction*                               _destroy = _lucEigenvectorsCrossSection_Destroy;
	lucDrawingObject_SetupFunction*                                _setup = _lucOpenGLDrawingObject_Setup;
	lucDrawingObject_DrawFunction*                                  _draw = _lucCrossSection_Draw;
	lucDrawingObject_CleanUpFunction*                            _cleanUp = _lucOpenGLDrawingObject_CleanUp;
	lucOpenGLDrawingObject_BuildDisplayListFunction*    _buildDisplayList = _lucEigenvectorsCrossSection_BuildDisplayList;

	/* Variables that are set to ZERO are variables that will be set either by the current _New function or another parent _New function further up the hierachy */
	AllocationType  nameAllocationType = NON_GLOBAL /* default value NON_GLOBAL */;

	return (void*) _lucEigenvectorsCrossSection_New(  LUCEIGENVECTORSCROSSSECTION_PASSARGS  );
}

void _lucEigenvectorsCrossSection_AssignFromXML( void* drawingObject, Stg_ComponentFactory* cf, void* data ){
	lucEigenvectorsCrossSection* self = (lucEigenvectorsCrossSection*)drawingObject;
	Index            defaultResolution;
	IJK              resolution;

	/* Construct Parent */
	_lucCrossSection_AssignFromXML( self, cf, data );
   strcpy(self->fieldVariableName, "TensorField");

	defaultResolution = Stg_ComponentFactory_GetUnsignedInt( cf, self->name, (Dictionary_Entry_Key)"resolution", 8  );
	resolution[ I_AXIS ] = Stg_ComponentFactory_GetUnsignedInt( cf, self->name, (Dictionary_Entry_Key)"resolutionX", defaultResolution  );
	resolution[ J_AXIS ] = Stg_ComponentFactory_GetUnsignedInt( cf, self->name, (Dictionary_Entry_Key)"resolutionY", defaultResolution  );
	resolution[ K_AXIS ] = Stg_ComponentFactory_GetUnsignedInt( cf, self->name, (Dictionary_Entry_Key)"resolutionZ", defaultResolution  );
			
	_lucEigenvectorsCrossSection_Init( 
			self, 
			Stg_ComponentFactory_GetRootDictUnsignedInt( cf, (Dictionary_Entry_Key)"dim", 2  ),
			Stg_ComponentFactory_GetString( cf, self->name, (Dictionary_Entry_Key)"leastColour", "black"  ),
			Stg_ComponentFactory_GetString( cf, self->name, (Dictionary_Entry_Key)"middleColour", "black"  ),
			Stg_ComponentFactory_GetString( cf, self->name, (Dictionary_Entry_Key)"greatestColour", "black"  ),
			resolution,
			Stg_ComponentFactory_GetDouble( cf, self->name, (Dictionary_Entry_Key)"arrowHeadSize", 0.3  ),
			Stg_ComponentFactory_GetDouble( cf, self->name, (Dictionary_Entry_Key)"lengthScale", 0.3  ),
			(float) Stg_ComponentFactory_GetDouble( cf, self->name, (Dictionary_Entry_Key)"lineWidth", 1.0  ),
			Stg_ComponentFactory_GetBool( cf, self->name, (Dictionary_Entry_Key)"useEigenValue", True  ),
			Stg_ComponentFactory_GetDouble( cf, self->name, (Dictionary_Entry_Key)"notEigenValue", 0.3  ),
			Stg_ComponentFactory_GetBool( cf, self->name, (Dictionary_Entry_Key)"plotEigenVector", True  ),
			Stg_ComponentFactory_GetBool( cf, self->name, (Dictionary_Entry_Key)"plotEigenValue", False  ),
			Stg_ComponentFactory_GetDouble( cf, self->name, (Dictionary_Entry_Key)"scaleEigenValue", 1.0  ),
			Stg_ComponentFactory_GetString( cf, self->name, (Dictionary_Entry_Key)"leastColourForNegative", "black"  ),
			Stg_ComponentFactory_GetString( cf, self->name, (Dictionary_Entry_Key)"middleColourForNegative", "black"  ),
			Stg_ComponentFactory_GetString( cf, self->name, (Dictionary_Entry_Key)"greatestColourForNegative", "black" ) );
}

void _lucEigenvectorsCrossSection_Build( void* drawingObject, void* data ) {
   /* Build field variable in parent */
   _lucCrossSection_Build(drawingObject, data);
}

void _lucEigenvectorsCrossSection_Initialise( void* drawingObject, void* data ) {}
void _lucEigenvectorsCrossSection_Execute( void* drawingObject, void* data ) {}
void _lucEigenvectorsCrossSection_Destroy( void* drawingObject, void* data ) {}

void _lucEigenvectorsCrossSection_BuildDisplayList( void* drawingObject, void* _context ) {
	lucEigenvectorsCrossSection*       self            = (lucEigenvectorsCrossSection*)drawingObject;
	DomainContext*            context         = (DomainContext*) _context;

	_lucEigenvectorsCrossSection_DrawCrossSection( self, context->dim );
}

void _lucEigenvectorsCrossSection_DrawCrossSection( void* drawingObject, Dimension_Index dim ) {
	lucEigenvectorsCrossSection*  self           = (lucEigenvectorsCrossSection*)drawingObject;
	FieldVariable*    tensorField    = self->fieldVariable;
	Coord             pos;
	SymmetricTensor   tensor;
	Coord             globalMin;
	Coord             globalMax;
	Coord             localMin;
	Coord             localMax;
	double            dA, dB;
	Eigenvector       eigenvectorList[3];
	Dimension_Index   dim_I;
	
	FieldVariable_GetMinAndMaxGlobalCoords( tensorField, globalMin, globalMax );
	FieldVariable_GetMinAndMaxLocalCoords( tensorField, localMin, localMax );

	glLineWidth(self->lineWidth);
	
	dA = (globalMax[ self->axis1 ] - globalMin[ self->axis1 ])/(double)self->resolution[ self->axis1 ];
	dB = (globalMax[ self->axis2 ] - globalMin[ self->axis2 ])/(double)self->resolution[ self->axis2 ];
	
	pos[self->axis] = lucCrossSection_GetValue(self, globalMin[self->axis], globalMax[self->axis]);
	Journal_DPrintf( self->debugStream, "-- Drawing cross section on axis %d at value %lf\n", self->axis, pos[self->axis]);

	for ( pos[ self->axis1 ] = globalMin[ self->axis1 ] + dA * 0.5 ; pos[ self->axis1 ] < globalMax[ self->axis1 ] ; pos[ self->axis1 ] += dA ) {
		for ( pos[ self->axis2 ] = globalMin[ self->axis2 ] + dB * 0.5 ; pos[ self->axis2 ] < globalMax[ self->axis2 ] ; pos[ self->axis2 ] += dB ) {

			if ( pos[ self->axis1 ] < localMin[ self->axis1 ] || pos[ self->axis1 ] >= localMax[ self->axis1 ] )
				continue;
			if ( pos[ self->axis2 ] < localMin[ self->axis2 ] || pos[ self->axis2 ] >= localMax[ self->axis2 ] )
				continue;

			/* Get Value of Tensor at this point in space */
			if ( FieldVariable_InterpolateValueAt( tensorField, pos, tensor ) == LOCAL ) {
				SymmetricTensor_CalcAllEigenvectors( tensor, dim, eigenvectorList );

                                if(self->plotEigenVector){
					for ( dim_I = 0 ; dim_I < dim ; dim_I++ ) {
						
						lucColour_SetOpenGLColour( &self->colours[ dim_I ] );
						if(self->useEigenValue){
						     luc_DrawVector( dim, pos, eigenvectorList[ dim_I ].vector, 
								(eigenvectorList[ dim_I ].eigenvalue * self->scaleEigenValue), self->arrowHeadSize );
						}
						else{
							luc_DrawVector( dim, pos, eigenvectorList[ dim_I ].vector,
									 self->notEigenValue, self->arrowHeadSize );
						}
					}
				}
				if(self->plotEigenValue){
					GLfloat pointSize = 0;

					for ( dim_I = 0 ; dim_I < dim ; dim_I++ ) {
					        /* The EigenValue can be negative.... Got to attribute a potential */
						/* colour for negative values, one for each dim as well */
						if ( eigenvectorList[ dim_I ].eigenvalue >= 0) {
						        pointSize = eigenvectorList[ dim_I ].eigenvalue * self->scaleEigenValue;
							lucColour_SetOpenGLColour( &self->colours[ dim_I ] );
	                                        }
						else {
						        lucColour_SetOpenGLColour( &self->colourForNegative[ dim_I ] );
							pointSize = - eigenvectorList[ dim_I ].eigenvalue * self->scaleEigenValue;
	                                        }
						glPointSize( pointSize );
					
						glBegin(GL_POINTS);
							if (dim == 2)
					      			glVertex3f( (GLfloat)pos[0], (GLfloat)pos[1], 0.001 );
							else 
						    		glVertex3dv( pos );
						glEnd();
					
					}
				}

			}
		}
	}
}


