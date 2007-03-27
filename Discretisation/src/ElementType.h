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
**	Handles the Finite Element Method's requirements of an element,
**	such as evaluating shape(basis) functions.
**
** Assumptions:
**
** Comments:
**	Is there a better name for this class? IE at least FeElementType.
**	Perhaps FE_ElementDiscretisation?
**
** $Id: ElementType.h 656 2006-10-18 06:45:50Z SteveQuenette $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifndef __StgFEM_Discretisation_ElementType_h__
#define __StgFEM_Discretisation_ElementType_h__
	
	/** Type of this classes */
	extern const Type ElementType_Type;
	
	/* Child classes must define these abstract functions */
	typedef void	(ElementType_EvaluateShapeFunctionsAtFunction)			( void* elementType, const double localCoord[],
		double* const evaluatedValues );

	typedef void	(ElementType_EvaluateShapeFunctionLocalDerivsAtFunction)	( void* elementType, const double localCoord[],
		double** const evaluatedDerivatives );

	typedef void	(ElementType_ConvertGlobalCoordToElLocalFunction)		( void* elementType,
		ElementLayout* elementLayout, const Coord** globalNodeCoordPtrsInElement, const Coord globalCoord,
		Coord elLocalCoord );

	typedef void	(ElementType_BuildFunction)					( void* elementType, void *arg );
	
	/* ElementType information */
	#define __ElementType  \
		/* General info */ \
		__Stg_Component \
		\
		/* Virtual info */ \
		ElementType_EvaluateShapeFunctionsAtFunction*			_evaluateShapeFunctionsAt; \
		ElementType_EvaluateShapeFunctionLocalDerivsAtFunction*		_evaluateShapeFunctionLocalDerivsAt; \
		ElementType_ConvertGlobalCoordToElLocalFunction*		_convertGlobalCoordToElLocal; \
		\
		/* ElementType info */ \
		Index								nodeCount; \
		Stream*								debug;

	struct ElementType { __ElementType };
	
	
	/* No "ElementType_New" and "ElementType_Init" as this is an abstract class */
	
	
	/* Creation implementation / Virtual constructor */
	ElementType* _ElementType_New(
		SizeT								_sizeOfSelf,
		Type								type,
		Stg_Class_DeleteFunction*						_delete,
		Stg_Class_PrintFunction*						_print,
		Stg_Class_CopyFunction*						_copy, 
		Stg_Component_DefaultConstructorFunction*	_defaultConstructor,
		Stg_Component_ConstructFunction*			_construct,
		Stg_Component_BuildFunction*		_build,
		Stg_Component_InitialiseFunction*		_initialise,
		Stg_Component_ExecuteFunction*		_execute,
		Stg_Component_DestroyFunction*		_destroy,
		Name							name,
		Bool							initFlag,
		ElementType_EvaluateShapeFunctionsAtFunction*			_evaluateShapeFunctionsAt,
		ElementType_EvaluateShapeFunctionLocalDerivsAtFunction*		_evaluateShapeFunctionLocalDerivsAt,
		ElementType_ConvertGlobalCoordToElLocalFunction*		_convertGlobalCoordToElLocal,
		Index								nodeCount );
	
	/* Initialise implementation */
	void _ElementType_Init( 
		ElementType*				self,
		Index					nodeCount );
	
	/* Stg_Class_Delete a ElementType construst */
	void _ElementType_Delete( void* elementType );
	
	/* Print the contents of an ElementType construct */
	void _ElementType_Print( void* elementType, Stream* stream );
	
	/** Build the element type */
	void ElementType_Build( void* elementType, void *data );
	
	/** Evaluate the value of the shape functions at a local coordinate.
	Note that in Hughes FEM notation, localCoord -> Xi, and evaluatedValues -> Ni */
	void ElementType_EvaluateShapeFunctionsAt( void* elementType, const double localCoord[], double* const evaluatedValues );
	
	/** Evaluate the value of the shape function derivatives at a local coordinate.
	Note that in Hughes FEM notation, localCoord -> Xi, and evaluatedValues -> GNi */
	void ElementType_EvaluateShapeFunctionLocalDerivsAt( void* elementType, const double localCoord[], double** const evaluatedDerivatives );
	
	/** Convert a co-ordinate from the global (x,y,z etc) to elementLocal (Xi,Eta,Zeta etc) coordinate system.
	The elementLayout is passed in since the function may be able to take shortcuts given the information in it
	(eg if its a "brick" element). */
	void ElementType_ConvertGlobalCoordToElLocal(
		void*		elementType,
		ElementLayout*	elementLayout,
		const Coord**	globalNodeCoordPtrsInElement,
		const Coord	globalCoord,
		Coord		elLocalCoord );
	
	/** General implementation of ElementType_ConvertGlobalCoordToElLocal(). Solves the system of FE elLocal->global
	coordinate equations. */
	void _ElementType_ConvertGlobalCoordToElLocal(
		void*		elementType,
		ElementLayout*	elementLayout,
		const Coord**	globalNodeCoordPtrsInElement,
		const Coord	globalCoord,
		Coord		elLocalCoord );
	
	/** Calculate the shape function global derivatives for all degrees of freedom for all nodes */
	void ElementType_ShapeFunctionsGlobalDerivs( 
		void*			elementType,
		void*			_mesh,
		Element_DomainIndex	elId, 
		double*			xi, 
		int			dim, 
		double*			detJac, 
		double**		GNx );

	#define ElementType_Jacobian( elementType, mesh, elId, xi, dim, jacobian, GNi ) \
		ElementType_Jacobian_AxisIndependent( elementType, mesh, elId, xi, dim, jacobian, GNi, I_AXIS, J_AXIS, K_AXIS )

	void ElementType_Jacobian_AxisIndependent( 
		void*               _elementType, 
		void*               _mesh, 
		Element_DomainIndex	elId, 
		double*             xi, 
		Dimension_Index     dim, 
		double**            jacobian, 
		double**            _GNi, 
		Coord_Index         A_axis, 
		Coord_Index         B_axis, 
		Coord_Index         C_axis );

	#define ElementType_JacobianDeterminant( elementType, mesh, elId, xi, dim ) \
		ElementType_JacobianDeterminant_AxisIndependent( elementType, mesh, elId, xi, dim, I_AXIS, J_AXIS, K_AXIS )

	double ElementType_JacobianDeterminant_AxisIndependent( 
		void*               _elementType, 
		void*               _mesh, 
		Element_DomainIndex	elId, 
		double*             xi, 
		Dimension_Index     dim, 
		Coord_Index         A_axis, 
		Coord_Index         B_axis, 
		Coord_Index         C_axis );

#endif /* __StgFEM_Discretisation_ElementType_h__ */
