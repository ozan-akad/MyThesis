#include <stdio.h>
#include <math.h>
#include <mpi.h>
#include "mesh3D.h"

void meshAcousticsError3D(mesh3D *mesh, dfloat time){

  dfloat maxErrorP = 0;
  for(iint e=0;e<mesh->Nelements;++e){
    for(int n=0;n<mesh->Np;++n){

      iint id = n+e*mesh->Np;
      dfloat x = mesh->x[id];
      dfloat y = mesh->y[id];
      dfloat z = mesh->z[id];

      dfloat u,v,w,p;
      acousticsCavitySolution3D(x, y, z, time, &u, &v, &w, &p);

      maxErrorP = mymax(maxErrorP, fabs(p-mesh->q[id*mesh->Nfields+3]));
    }
  }

  // compute maximum over all processes
  dfloat globalMaxErrorP;
  MPI_Allreduce(&maxErrorP, &globalMaxErrorP, 1, MPI_DFLOAT, MPI_MAX, MPI_COMM_WORLD);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if(rank==0)
    printf("%g, %g (time,maxError(pressure)\n", time, globalMaxErrorP);
  
}
