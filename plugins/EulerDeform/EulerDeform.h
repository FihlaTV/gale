/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
** Victorian Partnership for Advanced Computing (VPAC) Ltd, Australia
** (C) 2003-2005 All Rights Reserved
**
** Authors:
**	Stevan M. Quenette, Senior Software Engineer, VPAC. (steve@vpac.org)
**	Patrick D. Sunter, Software Engineer, VPAC. (pds@vpac.org)
**	David May, PhD Student Monash University, VPAC. (david.may@sci.maths.monash.edu.au)
**	Luke J. Hodkinson, Computational Engineer, VPAC. (lhodkins@vpac.org)
**	Alan H. Lo, Computational Engineer, VPAC. (alan@vpac.org)
**	Raquibul Hassan, Computational Engineer, VPAC. (raq@vpac.org)
**
**
** Role:
**	Defines any header info, such as new structures, needed by this plugin
**
** Assumptions:
**
** Comments:
**
** $Id $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifndef __StgFEM_plugins_EulerDeform_EulerDeform_h__
#define __StgFEM_plugins_EulerDeform_EulerDeform_h__

	extern const char*	EULERDEFORM_PLUGIN_TAG;


	Index _StgFEM_EulerDeform_Register( PluginsManager* pluginsMgr );

	void* _StgFEM_EulerDeform_DefaultNew( Name name );

	void _StgFEM_EulerDeform_Construct( void* component, Stg_ComponentFactory* cf, void* data );

	void _StgFEM_EulerDeform_Build( void* component, void* data );

	void _StgFEM_EulerDeform_Destroy( void* component, void* data );

	void EulerDeform_IntegrationSetup( void* _timeIntegrator, void* _velField );

	Bool EulerDeform_TimeDeriv( void* crdAdvector, Index arrayInd, double* timeDeriv );

	void EulerDeform_Remesh( TimeIntegratee* crdAdvector, EulerDeform_Context* edCtx );

	void EulerDeform_InterpVar( FieldVariable* field, Variable* var, Mesh* mesh, Coord* newCrds );

	void EulerDeform_WrapTopSurface( EulerDeform_System* sys, Coord* oldCrds );

	void EulerDeform_WrapBottomSurface( EulerDeform_System* sys, Coord* oldCrds );

	void EulerDeform_WrapLeftSurface( EulerDeform_System* sys, Coord* oldCrds );

	void EulerDeform_TopInternalLoop( EulerDeform_System* sys, GRM* grm, Coord* oldCrds, unsigned* ijk, unsigned curDim );

	void EulerDeform_BottomInternalLoop( EulerDeform_System* sys, GRM* grm, Coord* oldCrds, unsigned* ijk, unsigned curDim );

	void EulerDeform_LeftInternalLoop( EulerDeform_System* sys, GRM* grm, Coord* oldCrds, unsigned* ijk, unsigned curDim );

#endif
