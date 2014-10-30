//* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 2000  International Computer Science Institute
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by ACIRI, the AT&T 
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "ident-tree.h"
#include "rate-limit.h"

// ############################ PrefixTree Methods ####################

PrefixTree::PrefixTree() {

  for (int i=0; i<=getLastIndex(); i++) {
    countArray[i]=0;
  }
} 

void
PrefixTree::reset() {

  for (int i=0; i<=getLastIndex(); i++) {
    countArray[i]=0;
  }
}

void
PrefixTree::traverse() {
  printf("Traversal \n");
  for (int i=0; i<=getLastIndex(); i++) {
    printf("%d/%d %d\n",getPrefixFromIndex(i), getNoBitsFromIndex(i), countArray[i]);
  }
}

void
PrefixTree::registerDrop(int address, int size) {

  if (address > getMaxAddress()) {
    printf("ERROR: Address out of Range\n");
    exit(EXIT_FAILURE);
  }

  for (int i=0; i<=NO_BITS; i++) {
    int index = getIndexFromAddress(address, i);
    countArray[index]+=size;
  }
}

AggReturn *
PrefixTree::calculateLowerBound() {
  
  //bulk of this code is taken from identifyAggregate.
	// bad idea - but quick.
	// better way - to make the common code into a separate function.
  int sum = 0; int count=0; int i;
  for (i=getFirstIndexOfBit(NO_BITS); i<=getLastIndexOfBit(NO_BITS); i++) {
    if (countArray[i]!=0) {
      sum+=countArray[i];
      count++;
    }
  }

  //  printf("CLB: sum: %d count: %d\n",sum, count);

  if (count < 2) return NULL;

  cluster *clusterList = (cluster *)malloc(sizeof(cluster)*MAX_CLUSTER);
  
  for (i=0; i < MAX_CLUSTER; i++) {
    clusterList[i].prefix_=-1;
    clusterList[i].count_=0;
  }
  
  double mean = sum/count;
  for (i=getFirstIndexOfBit(NO_BITS); i<=getLastIndexOfBit(NO_BITS); i++) {
    if (countArray[i] >= mean/2) { //using mean/2 helps in trivial simulations.
      insertCluster(clusterList, i, countArray[i], CLUSTER_LEVEL);
    }
  }
  
  for (i=0; i<MAX_CLUSTER; i++) {
    if (clusterList[i].prefix_==-1) {
      break;
    }
    goDownCluster(clusterList, i);
  }
  int lastIndex = i-1;
  
  sortCluster(clusterList, lastIndex);
  
  return new AggReturn(clusterList, 0, lastIndex, countArray[0]);
}

AggReturn *  
PrefixTree::identifyAggregate(double arrRate, double linkBW) {
  
  int sum = 0; int count=0; int i;
  for (i=getFirstIndexOfBit(NO_BITS); i<=getLastIndexOfBit(NO_BITS); i++) {
    if (countArray[i]!=0) {
      sum+=countArray[i];
      count++;
    }
  }

  if (count == 0) return NULL;

  cluster *clusterList = (cluster *)malloc(sizeof(cluster)*MAX_CLUSTER);
  
  for (i=0; i < MAX_CLUSTER; i++) {
    clusterList[i].prefix_=-1;
    clusterList[i].count_=0;
  }
  
  double mean = sum/count;
  for (i=getFirstIndexOfBit(NO_BITS); i<=getLastIndexOfBit(NO_BITS); i++) {
    if (countArray[i] >= mean/2) { //using mean/2 helps in trivial simulations.
      insertCluster(clusterList, i, countArray[i], CLUSTER_LEVEL);
    }
  }
  
  for (i=0; i<MAX_CLUSTER; i++) {
    if (clusterList[i].prefix_==-1) {
      break;
    }
    goDownCluster(clusterList, i);
  }
  int lastIndex = i-1;
  
  sortCluster(clusterList, lastIndex);
  
  //check for natural rifts here, if you want to.
  
  double targetRate = linkBW/(1 - TARGET_DROPRATE);
  double excessRate = arrRate - targetRate;
  ////////////////
  //printf("arrRate: %5.2f targetRate: %5.2f excessRate %5.2f\n",
  //   arrRate, targetRate, excessRate);
  /////////////////
  double rateTillNow = 0;
  double requiredBottom;
  int id=0;
  for (; id<=lastIndex; id++) {
    rateTillNow+=clusterList[id].count_*(arrRate/countArray[0]);
    requiredBottom = (rateTillNow - excessRate)/(id+1);
    //printf("id: %d excessRate: %5.2f rateTillNow: %5.2f requiredBottom: %5.2f\n",
    //id, excessRate, rateTillNow, requiredBottom);
    if (clusterList[id+1].prefix_==-1) {
      // I think that this means that no viable set of aggregates was found.
      // Shouldn't it return failure in this case?  - Sally 
      break;
    }
    if (clusterList[id+1].count_* (arrRate/countArray[0]) < requiredBottom) break;
  }

  return new AggReturn(clusterList, requiredBottom, id, countArray[0]);
}
    
void
PrefixTree::insertCluster(cluster * clusterList, int index, int count, int noBits) {
  
  int address = getPrefixFromIndex(index);
  int prefix = (address >> (NO_BITS - noBits)) << (NO_BITS - noBits);
  int breakCode=0;
  for (int i=0;i<MAX_CLUSTER; i++) {
    if (clusterList[i].prefix_ == prefix && clusterList[i].bits_ == noBits) {
      clusterList[i].count_+=count;
      breakCode=1;
      break;
    }
    if (clusterList[i].prefix_ == -1) {
      clusterList[i].prefix_ = prefix;
      clusterList[i].bits_ = noBits;
      clusterList[i].count_=count;
      breakCode=2;
      break;
    }
  }
  
  if (breakCode==0) {
    printf("Error: Too Small MAX_CLUSTER. Increase and Recompile\n");
    exit(-1);
  }
}

void
PrefixTree::goDownCluster(cluster * clusterList, int index) {
  
  int noBits = clusterList[index].bits_;
  int prefix = clusterList[index].prefix_;
  
  int treeIndex = getIndexFromPrefix(prefix, noBits);
  int maxIndex = treeIndex;
  while (1) {
    int leftIndex = 2*maxIndex+1;
    if (leftIndex > getLastIndex()) break;
    int leftCount = countArray[leftIndex];
    int rightCount = countArray[leftIndex+1];
    if (leftCount > 9*rightCount) {
      maxIndex = leftIndex;
    } 
    else if (rightCount > 9*leftCount) {
      maxIndex = leftIndex+1;
    }
    else {
      break;
    }
  }

  clusterList[index].prefix_=getPrefixFromIndex(maxIndex);
  clusterList[index].bits_=getNoBitsFromIndex(maxIndex);
  clusterList[index].count_=countArray[maxIndex];
}

void PrefixTree::sortCluster(cluster * clusterList, int lastIndex) 
{
  int i, j;

  for (i=0; i<=lastIndex; i++) {
    for (j=i+1; j<=lastIndex; j++) {
      if (clusterList[i].count_ < clusterList[j].count_) {
	swapCluster(clusterList, i, j);
      }
    }
  }
}
 
void 
PrefixTree::swapCluster(cluster * clusterList, int id1, int id2) {
  
  int tempP = clusterList[id1].prefix_;
  int tempB = clusterList[id1].bits_;
  int tempC = clusterList[id1].count_;

  clusterList[id1].prefix_ = clusterList[id2].prefix_;
  clusterList[id1].bits_   = clusterList[id2].bits_;
  clusterList[id1].count_  = clusterList[id2].count_;

  clusterList[id2].prefix_ = tempP;
  clusterList[id2].bits_   = tempB;
  clusterList[id2].count_  = tempC;
}

int
PrefixTree::getMaxAddress() {
  return (1 << NO_BITS) - 1;
}

int
PrefixTree::getBitI(int address, int i) {

  int andAgent = 1 << (NO_BITS - i);
  int bitI = address & andAgent;
  if (bitI) 
    return 1;
  else 
    return 0;
}

int
PrefixTree::getIndexFromPrefix(int prefix, int noBits) {
  int base = (1 << noBits) - 1;
  return base + (prefix >> (NO_BITS - noBits));
}

int
PrefixTree::getIndexFromAddress(int address, int noBits) {
  
  int base = (1 << noBits) - 1;
//   int andAgent = address >> (NO_BITS - noBits);
//   int additive = base & andAgent;
  int additive = address >> (NO_BITS - noBits);
    
  return base + additive;
}

int 
PrefixTree::getPrefixFromIndex(int index) {
  
  int noBits = getNoBitsFromIndex(index);
  int base = (1 << noBits) - 1;
  int prefix = (index - base) << (NO_BITS - noBits);
    
  return prefix;
}


int 
PrefixTree::getPrefixBits(int prefix, int noBits) {
  return (prefix >> (NO_BITS - noBits)) << (NO_BITS - noBits);
}
  
int 
PrefixTree::getNoBitsFromIndex(int index) {
 
  //using 1.2 is an ugly hack to get precise floating point calculation.
  int noBits = (int) floor(log(index+1.2)/log(2));
  return noBits;
}

int 
PrefixTree::getFirstIndexOfBit(int noBits) {
  return ( 1 << noBits) - 1;
}

int 
PrefixTree::getLastIndexOfBit(int noBits) {
  return ( 1 << (noBits+1)) - 2;
}

// ######################## IdentStruct Methods ########################

IdentStruct::IdentStruct() {
  dstTree_ = new PrefixTree();
  srcTree_ = new PrefixTree();
  dropHash_ = new DropHashTable();
  lowerBound_ = 0;
}

void
IdentStruct::reset() {
  dstTree_->reset();
  //  srcTree_->reset();
  //  dropHash_->reset();
}
  
void 
IdentStruct::traverse() {
  dstTree_->traverse();
  //  srcTree_->traverse();
  //  dropHash_->traverse();
}

void 
IdentStruct::registerDrop(Packet *p) {
   
  hdr_ip * iph = hdr_ip::access(p);
  //  ns_addr_t src = iph->src();
  ns_addr_t dst = iph->dst();
  int fid = iph->flowid();
  
  hdr_cmn* hdr  = HDR_CMN(p);
  int size = hdr->size();

 if (AGGREGATE_CLASSIFICATION_MODE_FID)
   dstTree_->registerDrop(fid, size);
 else 
  dstTree_->registerDrop(dst.addr_, size);
  
  //  srcTree_->registerDrop(src.addr_, size);
  //  dropHash_->insert(p, size);
}

AggReturn * 
IdentStruct::identifyAggregate(double arrRate, double linkBW) {
  return dstTree_->identifyAggregate(arrRate, linkBW);
}

AggReturn * 
IdentStruct::calculateLowerBound() {
   return dstTree_->calculateLowerBound();
}

void
IdentStruct::setLowerBound(double bound, int averageIt) {
       
	double alpha = 0.5;
	if (lowerBound_ == 0) 
		lowerBound_ = bound;
	else if (averageIt == 0) {
		if (bound < lowerBound_) 
			lowerBound_ = bound;
		else
			lowerBound_ = alpha * lowerBound_ + (1 - alpha) * bound;
	}
	else {
	   lowerBound_ = alpha * lowerBound_ + (1 - alpha) * bound;
	}

	//printf("lower bound: new = %g avg = %g\n", bound, lowerBound_);
	//fflush(stdout);
}

