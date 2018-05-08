#include "elliptic.h"

void ellipticStartHaloExchange(elliptic_t *elliptic, occa::memory &o_q, int Nentries, dfloat *sendBuffer, dfloat *recvBuffer){

  mesh_t *mesh = elliptic->mesh;
  
  // count size of halo for this process
  dlong haloBytes = mesh->totalHaloPairs*Nentries*sizeof(dfloat);
 
  // extract halo on DEVICE
  if(haloBytes){

    // make sure compute device is ready to perform halo extract
    mesh->device.finish();

    // switch to data stream
    mesh->device.setStream(elliptic->dataStream);

    // extract halo on data stream
    mesh->haloExtractKernel(mesh->totalHaloPairs, Nentries, mesh->o_haloElementList,
          o_q, mesh->o_haloBuffer);

    // queue up async copy of halo on data stream
    mesh->o_haloBuffer.asyncCopyTo(sendBuffer);

    mesh->device.setStream(elliptic->defaultStream);
  }
}

void ellipticInterimHaloExchange(elliptic_t *elliptic, occa::memory &o_q, int Nentries, dfloat *sendBuffer, dfloat *recvBuffer){

  mesh_t *mesh = elliptic->mesh;

  // count size of halo for this process
  dlong haloBytes = mesh->totalHaloPairs*Nentries*sizeof(dfloat);
  
  // extract halo on DEVICE
  if(haloBytes){
    
    // copy extracted halo to HOST
    mesh->device.setStream(elliptic->dataStream);

    // make sure async copy finished
    mesh->device.finish(); 
    
    // start halo exchange HOST<>HOST
    meshHaloExchangeStart(mesh,
        Nentries*sizeof(dfloat),
        sendBuffer,
        recvBuffer);
    
    mesh->device.setStream(elliptic->defaultStream);

  }
}
    

void ellipticEndHaloExchange(elliptic_t *elliptic, occa::memory &o_q, int Nentries, dfloat *recvBuffer){

  mesh_t *mesh = elliptic->mesh;
  
  // count size of halo for this process
  dlong haloBytes = mesh->totalHaloPairs*Nentries*sizeof(dfloat);
  dlong haloOffset = mesh->Nelements*Nentries*sizeof(dfloat);
  
  // extract halo on DEVICE
  if(haloBytes){
    // finalize recv on HOST
    meshHaloExchangeFinish(mesh);
    
    // copy into halo zone of o_r  HOST>DEVICE
    mesh->device.setStream(elliptic->dataStream);
    o_q.asyncCopyFrom(recvBuffer, haloBytes, haloOffset);
    mesh->device.finish();
    
    mesh->device.setStream(elliptic->defaultStream);
    mesh->device.finish();
  }
}