#include <stdlib.h>
#include "mesh2D.h"

typedef struct {
  iint localId;
  iint baseId;
  iint baseRank;
}parallelGatherNode_t;

void meshParallelGatherScatterSetupQuad2D(mesh2D *mesh){

  // set up local-global number pairs
  parallelGatherNode_t *parallelGatherNodes =
    (parallelGatherNode_t*) calloc(mesh->Np*mesh->Nelements, sizeof(parallelGatherNode_t));
  
  iint *gatherSendCounts = (iint*) calloc(size, sizeof(iint));
  
  int cnt = 0;
  for(iint n=0;n<mesh->Np*mesh->Nelements;++n){
    if(r!=rank){
	parallelGatherNodes[lnode].localId       = n+e*mesh->Np;
	parallelGatherNodes[lnode].baseLocalId = gid-cumulativeNlocalNodes[r];
	parallelGatherNodes[lnode].baseRank    = r;
	++gatherSendCounts[r];
	++cnt;
      }
    }
  }
  
  int NgatherAllSend = cnt;
  
  // sort based on parallelGatherNode index
  qsort(parallelGatherNodes, NgatherAllSend,
	sizeof(parallelGatherNode_t), compareParallelGatherNodes);

  // extract: ordered list of local node indices for gather
  iint *gatherBaseIds = (iint*) calloc(NgatherAllSend, sizeof(iint));
  iint *gatherSendIds  = (iint*) calloc(NgatherAllSend, sizeof(iint));
  for(iint n=0;n<NgatherAllSend;++n){
    // ids of nodes to send
    gatherSendIds[n] = parallelGatherNodes[n].localId;
    
    // ids of destination nodes 
    gatherBaseIds[n] = parallelGatherNodes[n].baseId;
  }
  
  // all processes tell all processes how many nodes to receive
  iint *gatherRecvCounts = (iint*) calloc(size,sizeof(iint));
  MPI_Alltoall(gatherSendCounts, 1, MPI_IINT,
	       gatherRecvCounts, 1, MPI_IINT,
	       MPI_COMM_WORLD);

  // form arrays for all to all (variable length)
  iint *gatherRecvDispls = (iint*) calloc(size,sizeof(iint));
  iint *gatherSendDispls = (iint*) calloc(size,sizeof(iint));
  for(iint r=1;r<size;++r){
    gatherRecvDispls[r] = gatherRecvDispls[r-1]+gatherRecvCounts[r-1];
    gatherSendDispls[r] = gatherSendDispls[r-1]+gatherSendCounts[r-1];
  }

  iint NgatherAllRecv = 0;
  for(iint r=0;r<size;++r)
    NgatherAllRecv += gatherRecvCounts[r];
  
  iint *gatherRecvIds = (iint*) calloc(NgatherAllRecv, sizeof(iint));
  MPI_Alltoallv(gatherBaseIds, gatherBaseCounts, gatherSendDispls, MPI_IINT,
		gatherRecvIds,   gatherRecvCounts,   gatherRecvDispls, MPI_IINT, MPI_COMM_WORLD);

  mesh->gatherNsend = NgatherAllSend;
  mesh->gatherNrecv = NgatherAllRecv;
  mesh->gatherSendIds = gatherSendIds;
  mesh->gatherRecvIds = gatherRecvIds;
  mesh->gatherSendCounts = gatherSendCounts;
  mesh->gatherRecvCounts = gatherRecvCounts;
  mesh->gatherSendDispls = gatherSendDispls;
  mesh->gatherRecvDispls = gatherRecvDispls;

  iint cnt = 0, sendMessage = 0, recvMessage = 0;
  for(iint r=0;r<size;++r){
    if(r!=rank){
      // send 
      if(mesh->gatherNsend[r]>0){
	for(iint n=0;n<mesh->gatherNsend[r];++n){
	  iint id = mesh->gatherSendIds[cnt];
	  memcpy(((char*)sendBuffer)+cnt*Nbytes, ((char*)sourceBuffer)+id*Nbytes, Nbytes);
	  ++cnt;
	}
	MPI_Isend(sendBuffer+Nbytes*mesh->gatherSendDispls[r],
		  Nbytes*mesh->gatherSendCounts[r],
		  MPI_CHAR, r, tag, MPI_COMM_WORLD, mesh->gatherSendRequests+sendMessage);
	++sendMessage;
      }
      // recv
      if(mesh->gatherNrecv[r]>0){
	MPI_Irecv(recvBuffer+Nbytes*mesh->gatherRecvDispls[r],
		  Nbytes*mesh->gatherRecvCountrs[r],
		  MPI_CHAR, r, tag, MPI_COMM_WORLD, mesh->gatherRecvRequests+recvMessage);
	++recvMessage;
      }
    }
  }

  mesh->gatherNsendMessages = sendMessage;
  mesh->gatherNrecvMessages = recvMessage;

  MPI_Status *recvStatuses = (MPI_Status*) calloc(size, sizeof(MPI_Status));
  MPI_Status *sendStatuses = (MPI_Status*) calloc(size, sizeof(MPI_Status));
  MPI_Waitall(mesh->gatherNrecvMessages, mesh->gatherRecvRequests, recvStatuses);
  MPI_Waitall(mesh->gatherNsendMessages, mesh->gatherSendRequests, sendStatuses);

  // add incoming to sourceBuffer
  cnt = 0;
  for(iint r=0;r<size;++r){
    if(r!=rank){
      for(iint n=0;n<mesh->gatherNrecv[r];++n){
	iint id = mesh->gatherRecvIds[cnt];
	sourceBuffer[id] += recvBuffer[cnt];
	++cnt;
      }
    }
  }

  // now send back
  iint cnt = 0, sendMessage = 0, recvMessage = 0;
  for(iint r=0;r<size;++r){
    if(r!=rank){
      // send 
      if(mesh->gatherNsend[r]>0){
	for(iint n=0;n<mesh->gatherNsend[r];++n){
	  iint id = mesh->gatherSendIds[cnt];
	  memcpy(((char*)sendBuffer)+cnt*Nbytes, ((char*)sourceBuffer)+id*Nbytes, Nbytes);
	  ++cnt;
	}
	MPI_Isend(sendBuffer+Nbytes*mesh->gatherSendDispls[r],
		  Nbytes*mesh->gatherSendCounts[r],
		  MPI_CHAR, r, tag, MPI_COMM_WORLD, mesh->gatherSendRequests+sendMessage);
	++sendMessage;
      }
      // recv
      if(mesh->gatherNrecv[r]>0){
	MPI_Irecv(recvBuffer+Nbytes*mesh->gatherRecvDispls[r],
		  Nbytes*mesh->gatherRecvCountrs[r],
		  MPI_CHAR, r, tag, MPI_COMM_WORLD, mesh->gatherRecvRequests+recvMessage);
	++recvMessage;
      }
    }
  }  
  
  free(parallelGatherNodes);
  free(gatherBaseIds);
}
