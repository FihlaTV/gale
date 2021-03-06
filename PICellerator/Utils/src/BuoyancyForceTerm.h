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
*/


#ifndef __PICellerator_Utils_BuoyancyForceTerm_h__
#define __PICellerator_Utils_BuoyancyForceTerm_h__

	typedef void (BuoyancyForceTerm_CalcGravityFunction) (BuoyancyForceTerm *self, FeMesh *mesh, Element_DomainIndex dElement_I, double* xi, double *gravity);

	/** Textual name of this class */
	extern const Type BuoyancyForceTerm_Type;

	typedef struct {
          double alpha;
          double density;
          Name alpha_equation;
          Name density_equation;
	} BuoyancyForceTerm_MaterialExt;

	/** BuoyancyForceTerm class contents */
	#define __BuoyancyForceTerm \
		/* General info */ \
		__ForceTerm \
		\
		/* Virtual info */ \
		BuoyancyForceTerm_CalcGravityFunction*	_calcGravity; \
		\
		/* BuoyancyForceTerm info */ \
		FeVariable*										temperatureField; \
		FeVariable*										pressureField; \
		double											gravity; \
		double*											gHat; \
		Bool												adjust; \
		Bool												damping; \
		Materials_Register*							materials_Register; \
		ExtensionInfo_Index							materialExtHandle; \
		MaterialSwarmVariable**						densitySwarmVariables; \
		MaterialSwarmVariable**						alphaSwarmVariables; \
		MaterialSwarmVariable**						densityEquationSwarmVariables; \
		MaterialSwarmVariable**						alphaEquationSwarmVariables; \
		Index												materialSwarmCount;\
                HydrostaticTerm*                                    hydrostaticTerm;

	struct BuoyancyForceTerm { __BuoyancyForceTerm };

	BuoyancyForceTerm* BuoyancyForceTerm_New( 
		Name							name,
		FiniteElementContext*	context,
		ForceVector*				forceVector,
		Swarm*						integrationSwarm,
		FeVariable*					temperatureField,
		FeVariable*					pressureField,
		double						gravity,
		double*						gHat,
		Bool							adjust,
		Bool							damping,
		Materials_Register*		materials_Register,
                HydrostaticTerm*                                    hydrostaticTerm);

	
	#ifndef ZERO
	#define ZERO 0
	#endif

	#define BUOYANCYFORCETERM_DEFARGS \
                FORCETERM_DEFARGS, \
                BuoyancyForceTerm_CalcGravityFunction*  _calcGravity

	#define BUOYANCYFORCETERM_PASSARGS \
                FORCETERM_PASSARGS, \
	        _calcGravity

	BuoyancyForceTerm* _BuoyancyForceTerm_New(  BUOYANCYFORCETERM_DEFARGS  );

	void _BuoyancyForceTerm_Init(
		void*                forceTerm,
		FeVariable*          temperatureField,
		FeVariable*          pressureField,
		double               gravity,
		double*              gHat,
		Bool                 adjust,
		Bool                 damping,
		Materials_Register*  materials_Register,
                HydrostaticTerm*     hydrostaticTerm );
	
	void _BuoyancyForceTerm_Delete( void* forceTerm );

	void _BuoyancyForceTerm_Print( void* forceTerm, Stream* stream );

	void* _BuoyancyForceTerm_DefaultNew( Name name ) ;

	void _BuoyancyForceTerm_AssignFromXML( void* forceTerm, Stg_ComponentFactory* cf, void* data ) ;

	void _BuoyancyForceTerm_Build( void* forceTerm, void* data ) ;

	void _BuoyancyForceTerm_Initialise( void* forceTerm, void* data ) ;

	void _BuoyancyForceTerm_Execute( void* forceTerm, void* data ) ;

	void _BuoyancyForceTerm_Destroy( void* forceTerm, void* data ) ;

	void _BuoyancyForceTerm_AssembleElement( void* forceTerm, ForceVector* forceVector, Element_LocalIndex lElement_I, double* elForceVec ) ;

	void _BuoyancyForceTerm_CalcGravity(BuoyancyForceTerm *self,
                                            FeMesh *mesh,
                                            Element_DomainIndex dElement_I,
                                            double* xi, double *gravity);

        #define BuoyancyForceTerm_CalcGravity( forceTerm, mesh, dElement_I, xi, gravity ) \
        (( (BuoyancyForceTerm*) forceTerm )->_calcGravity( forceTerm, mesh, dElement_I, xi, gravity ) )

        void BuoyancyForceTerm_average_density_alpha(BuoyancyForceTerm *self,
                                                     IntegrationPointsSwarm* input_swarm,
                                                     FeMesh* mesh,
                                                     const Element_LocalIndex &lElement_I,
                                                     double *density, double *alpha);

        double BuoyancyForceTerm_density_alpha(BuoyancyForceTerm *self,
                                               FeMesh* mesh,
                                               const Element_LocalIndex &lElement_I,
                                               const Cell_Index &cell_I,
                                               IntegrationPointMapper* mapper,
                                               IntegrationPoint* particle,
                                               const int &type);

        inline double BuoyancyForceTerm_density(BuoyancyForceTerm *self,
                                                FeMesh* mesh,
                                                const Element_LocalIndex &lElement_I,
                                                const Cell_Index &cell_I,
                                                IntegrationPointMapper* mapper,
                                                IntegrationPoint* particle)
        {
          return BuoyancyForceTerm_density_alpha(self,mesh,lElement_I,cell_I,
                                                 mapper,particle,0);
        }

        inline double BuoyancyForceTerm_alpha(BuoyancyForceTerm *self,
                                              FeMesh* mesh,
                                              const Element_LocalIndex &lElement_I,
                                              const Cell_Index &cell_I,
                                              IntegrationPointMapper* mapper,
                                              IntegrationPoint* particle)
        {
          return BuoyancyForceTerm_density_alpha(self,mesh,lElement_I,cell_I,
                                                 mapper,particle,1);
        }

#endif

