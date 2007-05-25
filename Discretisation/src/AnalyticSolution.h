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
*/
/** \file
**  Role:
**
** Assumptions:
**
** Comments:
**
** $Id $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifndef __StgFEM_Discretisation_AnalyticSolution_h__
#define __StgFEM_Discretisation_AnalyticSolution_h__
	
	typedef void (AnalyticSolution_FeVariableSolutionFunction) (void* analyticSolution, FeVariable* analyticFeVariable, double* coord, double* value );
	typedef void (AnalyticSolution_GetVelocity) (void* analyticSolution, FeVariable* analyticFeVariable, double* coord, double* value );
	typedef void (AnalyticSolution_GetPressure) (void* analyticSolution, FeVariable* analyticFeVariable, double* coord, double* value );
	typedef void (AnalyticSolution_GetTotalStress) (void* analyticSolution, FeVariable* analyticFeVariable, double* coord, double* value );
	typedef void (AnalyticSolution_GetStrainRate) (void* analyticSolution, FeVariable* analyticFeVariable, double* coord, double* value );

	/** Textual name of this class */
	extern const Type AnalyticSolution_Type;
	
	/** AnalyticSolution information */
	#define __AnalyticSolution \
		/* Parent info */ \
		__Stg_Component \
		/* Virtual info */ \
		/* AnalyticSolution info */ \
		Stg_ObjectList*        feVariableList;             \
		Stg_ObjectList*        analyticFeVariableList;     \
		Stg_ObjectList*        analyticFeVariableFuncList; \
		Stg_ObjectList*        errorMagnitudeFieldList;    \
		Stg_ObjectList*        relativeErrorMagnitudeFieldList;    \
		Stg_ObjectList*        streamList;                 \
		double*                toleranceList;              \
		Swarm*                 integrationSwarm;           \
		LiveComponentRegister* LC_Register;                \
		AbstractContext*       context;                    \
		AnalyticSolution_GetVelocity* _getAnalyticVelocity; \
		AnalyticSolution_GetPressure* _getAnalyticPressure; \
		AnalyticSolution_GetTotalStress* _getAnalyticTotalStress; \
		AnalyticSolution_GetStrainRate* _getAnalyticStrainRate; \
		
	/** Brings together and manages the life-cycle of a a mesh and all the 
	info relevant to it for the Finite Element Method. See Mesh.h for more. */
	struct AnalyticSolution { __AnalyticSolution };
	
	/* --- Constructors / Destructor --- */
	
	/** Create a new AnalyticSolution and initialise */

	void* _AnalyticSolution_DefaultNew( Name name );
	
	/* Creation implementation / Virtual constructor */
	AnalyticSolution* _AnalyticSolution_New(
		SizeT                                       _sizeOfSelf,
		Type                                        type,
		Stg_Class_DeleteFunction*                   _delete,
		Stg_Class_PrintFunction*                    _print,
		Stg_Class_CopyFunction*                     _copy, 
		Stg_Component_DefaultConstructorFunction*   _defaultConstructor,
		Stg_Component_ConstructFunction*            _construct,
		Stg_Component_BuildFunction*                _build,
		Stg_Component_InitialiseFunction*           _initialise,
		Stg_Component_ExecuteFunction*              _execute,
		Stg_Component_DestroyFunction*              _destroy,
		Name                                        name );			

	/* Stg_Class_Delete a AnalyticSolution construst */
	void _AnalyticSolution_Delete( void* analyticSolution );

	/* --- Virtual Function implementations --- */
	
	/* Print the contents of an AnalyticSolution construct */
	void _AnalyticSolution_Print( void* analyticSolution, Stream* stream );
	
	/* Copy */
	#define AnalyticSolution_Copy( self ) \
		(AnalyticSolution*)Stg_Class_Copy( self, NULL, False, NULL, NULL )
	#define AnalyticSolution_DeepCopy( self ) \
		(AnalyticSolution*)Stg_Class_Copy( self, NULL, True, NULL, NULL )
	
	void* _AnalyticSolution_Copy( void* analyticSolution, void* dest, Bool deep, Name nameExt, PtrMap* ptrMap );
	
	/* Build implementation */
	void _AnalyticSolution_Build( void* analyticSolution, void* data );
	
	/* Construct implementation */
	void _AnalyticSolution_Construct( void* analyticSolution, Stg_ComponentFactory* cf, void* data );
	
	/* Initialisation implementation */
	void _AnalyticSolution_Initialise( void* analyticSolution, void* data );
	
	/* Execution implementation */
	void _AnalyticSolution_Execute( void* analyticSolution, void* data );
	
	/* Destruct Implementation */
	void _AnalyticSolution_Destroy( void* analyticSolution, void* data );

	/* --- Public Functions --- */
	void AnalyticSolution_Test( void* analyticSolution, Index analyticFeVariable_I ) ;
	void AnalyticSolution_TestAll( void* analyticSolution, void* data ) ;
	
	void AnalyticSolution_PutAnalyticSolutionOntoNodes( void* analyticSolution, Index analyticFeVariable_I ) ;

	void AnalyticSolution_RegisterFeVariableWithAnalyticFunction( void* analyticSolution, FeVariable* feVariable, AnalyticSolution_FeVariableSolutionFunction* solutionFunction );
	void AnalyticSolution_BuildAllAnalyticFields( void* analyticSolution, void* data );

	FeVariable* AnalyticSolution_CreateAnalyticField( void* analyticSolution, FeVariable* feVariable ) ;
	FeVariable* AnalyticSolution_CreateAnalyticVectorField( void* analyticSolution, FeVariable* vectorField, AnalyticSolution_FeVariableSolutionFunction* solutionFunction ) ;
	FeVariable* AnalyticSolution_CreateAnalyticSymmetricTensorField( void* analyticSolution, FeVariable* vectorField ) ;

	FeVariable* AnalyticSolution_GetFeVariableFromAnalyticFeVariable( void* analyticSolution, FeVariable* analyticFeVariable ) ;
	InterpolationResult AnalyticSolution_InterpolateValueFromNormalFeVariable( void* analyticSolution, FeVariable* analyticFeVariable, double* coord, double* value ) ;
	
#endif /* __StgFEM_Discretisation_AnalyticSolution_h__ */
