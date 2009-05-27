/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
** Copyright (C), 2003, Victorian Partnership for Advanced Computing (VPAC) Ltd, 110 Victoria Street, Melbourne, 3053, Australia.
**
** Authors:
**   Stevan M. Quenette, Senior Software Engineer, VPAC. (steve@vpac.org)
**   Patrick D. Sunter, Software Engineer, VPAC. (pds@vpac.org)
**   Luke J. Hodkinson, Computational Engineer, VPAC. (lhodkins@vpac.org)
**   Siew-Ching Tan, Software Engineer, VPAC. (siew@vpac.org)
**   Alan H. Lo, Computational Engineer, VPAC. (alan@vpac.org)
**   Raquibul Hassan, Computational Engineer, VPAC. (raq@vpac.org)
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
**
** $Id: testJournal-Dictionary.c 2745 2005-03-05 08:12:18Z SteveQuenette $
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include <stdio.h>
#include <stdlib.h>

#include "pcu/pcu.h"
#include "StGermain/Base/Foundation/Foundation.h"
#include "StGermain/Base/IO/IO.h"
#include "StGermain/Base/Container/Container.h"
#include "StGermain/Base/Automation/Automation.h"
#include "CommSuite.h"

typedef struct {
} CommSuiteData;


void CommSuite_Setup( CommSuiteData* data ) {
}

void CommSuite_Teardown( CommSuiteData* data ) {
}

   
void CommSuite_TestConstruct( CommSuiteData* data ) {
   Comm* comm;

   pcu_check_true( 0 == "The Comm macro-style component seems to be broken currently - aborting test." ); return;
   comm = Comm_New();
   pcu_check_true( comm );
   pcu_check_true( comm->mpiComm == MPI_COMM_WORLD );
   pcu_check_true( comm->recvs == NULL );
   pcu_check_true( comm->sends == NULL );
   pcu_check_true( comm->stats == NULL );

   NewClass_Delete( comm );
}


void CommSuite_TestSetMPIComm( CommSuiteData* data ) {
   Comm* comm;

   pcu_check_true( 0 == "The Comm macro-style component seems to be broken currently - aborting test." ); return;
   comm = Comm_New();
   Comm_SetMPIComm( comm, MPI_COMM_WORLD );
   pcu_check_true( comm->mpiComm == MPI_COMM_WORLD );

   NewClass_Delete( comm );
}


void CommSuite_TestSetNbrs( CommSuiteData* data ) {
   Comm* comm;
   int nRanks, rank;
   int nNbrs, nbrs[2];

   pcu_check_true( 0 == "The Comm macro-style component seems to be broken currently - aborting test." ); return;
   MPI_Comm_size( MPI_COMM_WORLD, &nRanks );
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );
   if( nRanks > 1 ) {
      if( rank == 0 ) {
	 nNbrs = 1;
	 nbrs[0] = rank + 1;
      }
      else if( rank == nRanks - 1 ) {
	 nNbrs = 1;
	 nbrs[0] = rank - 1;
      }
      else {
	 nNbrs = 2;
	 nbrs[0] = rank - 1;
	 nbrs[1] = rank + 1;
      }
   }
   else
      nNbrs = 0;

   comm = Comm_New();
   Comm_SetMPIComm( comm, MPI_COMM_WORLD );
   Comm_SetNeighbours( comm, nNbrs, nbrs );

   if( nRanks > 1 ) {
      if( rank == 0 ) {
	 pcu_check_true( comm->nbrs.size == 1 );
	 pcu_check_true( comm->nbrs.ptr[0] == rank + 1 );
	 pcu_check_true( 1 );
      }
      else if( rank == nRanks - 1 ) {
	 pcu_check_true( comm->nbrs.size == 1 );
	 pcu_check_true( comm->nbrs.ptr[0] == rank - 1 );
	 pcu_check_true( 1 );
      }
      else {
	 pcu_check_true( comm->nbrs.size == 2 );
	 pcu_check_true( comm->nbrs.ptr[0] == rank - 1 );
	 pcu_check_true( comm->nbrs.ptr[1] == rank + 1 );
      }
      pcu_check_true( comm->recvs != NULL );
      pcu_check_true( comm->sends != NULL );
      pcu_check_true( comm->stats != NULL );
   }
   else {
      pcu_check_true( comm->nbrs.size == 0 );
      pcu_check_true( comm->nbrs.ptr == NULL );
      pcu_check_true( comm->recvs == NULL );
      pcu_check_true( comm->sends == NULL );
      pcu_check_true( comm->stats == NULL );
   }

   NewClass_Delete( comm );
}


void CommSuite_TestAddNbrs( CommSuiteData* data ) {
   Comm* comm;
   int nRanks, rank;
   int nNbrs, nbrs[2];

   pcu_check_true( 0 == "The Comm macro-style component seems to be broken currently - aborting test." ); return;
   MPI_Comm_size( MPI_COMM_WORLD, &nRanks );
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );
   if( nRanks > 1 ) {
      if( rank == 0 ) {
	 nNbrs = 1;
	 nbrs[0] = rank + 1;
      }
      else if( rank == nRanks - 1 ) {
	 nNbrs = 1;
	 nbrs[0] = rank - 1;
      }
      else {
	 nNbrs = 1;
	 nbrs[0] = rank - 1;
      }
   }
   else
      nNbrs = 0;

   comm = Comm_New();
   Comm_SetMPIComm( comm, MPI_COMM_WORLD );
   Comm_SetNeighbours( comm, nNbrs, nbrs );
   if( rank > 0 && rank < nRanks - 1 ) {
      nNbrs = 1;
      nbrs[0] = rank + 1;
   }
   else
      nNbrs = 0;
   Comm_AddNeighbours( comm, nNbrs, nbrs );

   if( nRanks > 1 ) {
      if( rank == 0 ) {
	 pcu_check_true( comm->nbrs.size == 1 );
	 pcu_check_true( comm->nbrs.ptr[0] == rank + 1 );
	 pcu_check_true( 1 );
      }
      else if( rank == nRanks - 1 ) {
	 pcu_check_true( comm->nbrs.size == 1 );
	 pcu_check_true( comm->nbrs.ptr[0] == rank - 1 );
	 pcu_check_true( 1 );
      }
      else {
	 pcu_check_true( comm->nbrs.size == 2 );
	 pcu_check_true( comm->nbrs.ptr[0] == rank - 1 );
	 pcu_check_true( comm->nbrs.ptr[1] == rank + 1 );
      }
      pcu_check_true( comm->recvs != NULL );
      pcu_check_true( comm->sends != NULL );
      pcu_check_true( comm->stats != NULL );
   }
   else {
      pcu_check_true( comm->nbrs.size == 0 );
      pcu_check_true( comm->nbrs.ptr == NULL );
      pcu_check_true( comm->recvs == NULL );
      pcu_check_true( comm->sends == NULL );
      pcu_check_true( comm->stats == NULL );
   }

   NewClass_Delete( comm );
}


void CommSuite_TestRemNbrs( CommSuiteData* data ) {
   Comm* comm;
   int nRanks, rank;
   int nNbrs, nbrs[2];
   IMap mapObj, *map = &mapObj;

   pcu_check_true( 0 == "The Comm macro-style component seems to be broken currently - aborting test." ); return;
   MPI_Comm_size( MPI_COMM_WORLD, &nRanks );
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );
   if( nRanks > 1 ) {
      if( rank == 0 ) {
	 nNbrs = 1;
	 nbrs[0] = rank + 1;
      }
      else if( rank == nRanks - 1 ) {
	 nNbrs = 1;
	 nbrs[0] = rank - 1;
      }
      else {
	 nNbrs = 2;
	 nbrs[0] = rank - 1;
	 nbrs[1] = rank + 1;
      }
   }
   else
      nNbrs = 0;

   comm = Comm_New();
   Comm_SetMPIComm( comm, MPI_COMM_WORLD );
   Comm_SetNeighbours( comm, nNbrs, nbrs );
   if( rank > 0 && rank < nRanks - 1 ) {
      nNbrs = 1;
      nbrs[0] = 0; /* local index */
   }
   else
      nNbrs = 0;
   IMap_Construct( map );
   Comm_RemoveNeighbours( comm, nNbrs, nbrs, map );

   if( nRanks > 1 ) {
      if( rank == 0 ) {
	 pcu_check_true( comm->nbrs.size == 1 );
	 pcu_check_true( comm->nbrs.ptr[0] == rank + 1 );
      }
      else if( rank == nRanks - 1 ) {
	 pcu_check_true( comm->nbrs.size == 1 );
	 pcu_check_true( comm->nbrs.ptr[0] == rank - 1 );
      }
      else {
	 pcu_check_true( comm->nbrs.size == 1 );
	 pcu_check_true( comm->nbrs.ptr[0] == rank + 1 );
      }
      pcu_check_true( comm->recvs != NULL );
      pcu_check_true( comm->sends != NULL );
      pcu_check_true( comm->stats != NULL );
   }
   else {
      pcu_check_true( comm->nbrs.size == 0 );
      pcu_check_true( comm->nbrs.ptr == NULL );
      pcu_check_true( comm->recvs == NULL );
      pcu_check_true( comm->sends == NULL );
      pcu_check_true( comm->stats == NULL );
   }

   NewClass_Delete( comm );
   IMap_Destruct( map );
}


void CommSuite_TestAllgather( CommSuiteData* data ) {
   Comm* comm;
   int nRanks, rank;
   int nNbrs, nbrs[2];

   pcu_check_true( 0 == "The Comm macro-style component seems to be broken currently - aborting test." ); return;
   MPI_Comm_size( MPI_COMM_WORLD, &nRanks );
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );
   if( nRanks > 1 ) {
      if( rank == 0 ) {
	 nNbrs = 1;
	 nbrs[0] = rank + 1;
      }
      else if( rank == nRanks - 1 ) {
	 nNbrs = 1;
	 nbrs[0] = rank - 1;
      }
      else {
	 nNbrs = 2;
	 nbrs[0] = rank - 1;
	 nbrs[1] = rank + 1;
      }
   }
   else
      nNbrs = 0;

   comm = Comm_New();
   Comm_SetMPIComm( comm, MPI_COMM_WORLD );
   Comm_SetNeighbours( comm, nNbrs, nbrs );
   /* TODO */

   NewClass_Delete( comm );
}


void CommSuite( pcu_suite_t* suite ) {
   pcu_suite_setData( suite, CommSuiteData );
   pcu_suite_setFixtures( suite, CommSuite_Setup, CommSuite_Teardown );
   pcu_suite_addTest( suite, CommSuite_TestConstruct );
   pcu_suite_addTest( suite, CommSuite_TestSetMPIComm );
   pcu_suite_addTest( suite, CommSuite_TestSetNbrs );
   pcu_suite_addTest( suite, CommSuite_TestAddNbrs );
   pcu_suite_addTest( suite, CommSuite_TestRemNbrs );
   pcu_suite_addTest( suite, CommSuite_TestAllgather );
}
