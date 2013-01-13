#ifndef ident_tree_h
#define ident_tree_h

#include <stdlib.h>
#include <stdio.h>
#include "address.h"
#include "packet.h"
#include "ip.h"
#include "tcl.h"
#include "math.h"

#include "pushback-constants.h"

class AggSpec;

struct cluster {
  int prefix_;
  int bits_;
  int count_;
};

class AggReturn {
 public:
  cluster * clusterList_;
  double limit_;
  int finalIndex_;
  int totalCount_;

  AggReturn(cluster * clusterList, double bottom, int finalIndex, int totalCount) {
    clusterList_ = clusterList;
    limit_ = bottom;
    finalIndex_ = finalIndex;
    totalCount_=totalCount;
  }

  ~AggReturn() {
    free(clusterList_);
  }
};

class DropHashTable {

 public:
  Tcl_HashTable * hashTable_;
  int count_;
  char key[200];   //generous space for key

  DropHashTable():count_(0) {
    hashTable_ = new Tcl_HashTable;
    Tcl_InitHashTable(hashTable_, TCL_STRING_KEYS);
  }
  
  void 
  packetToKey(Packet *p) {
    hdr_ip * iph = hdr_ip::access(p);
    ns_addr_t src = iph->src();
    ns_addr_t dst = iph->dst();
    int fid = iph->flowid();
    sprintf(key,"%d.%d %d.%d %d",src.addr_, src.port_, dst.addr_, dst.port_, fid);
  }
    
  void 
  insert(Packet *p, int size) {
    count_+=size;

    int new_entry;
    long oldValue;
    packetToKey(p);
    Tcl_HashEntry* he = Tcl_CreateHashEntry(hashTable_, (char*) key, &new_entry);
    if (new_entry) 
      oldValue = 0;
    else 
      oldValue = (long)Tcl_GetHashValue(he);
    
    //printf("old value = %d", oldValue);
    oldValue+=size;
    Tcl_SetHashValue(he, oldValue);
  }
   
  void 
  traverse() {
    printf("DropHash count = %d\n", count_);
    Tcl_HashSearch searchPtr;
    Tcl_HashEntry * he = Tcl_FirstHashEntry(hashTable_, &searchPtr);
    while (he != NULL) {
      char * key = Tcl_GetHashKey(hashTable_, he);
      long value = (long)Tcl_GetHashValue(he);
      printf("%s = %ld\n", key, value);
      he = Tcl_NextHashEntry(&searchPtr);
    }
  }
   
  void
  reset() {
    count_=0;
    Tcl_DeleteHashTable(hashTable_);
    Tcl_InitHashTable(hashTable_, TCL_STRING_KEYS);
  }

};

  
class PrefixTree {

 public:
  int countArray[(1 << (NO_BITS+1)) - 1];
    
  PrefixTree();
  void reset();
  void traverse();
  void registerDrop(int address, int size);
  
  static int getLastIndex() {return (1 << (NO_BITS+1)) - 2;}
  static int getMaxAddress();
  static int getBitI(int address, int i);
  static int getIndexFromPrefix(int prefix, int noBits);
  static int getIndexFromAddress(int address, int noBits);
  static int getPrefixFromIndex(int index);
  static int getNoBitsFromIndex(int index);
  static int getFirstIndexOfBit(int noBits);
  static int getLastIndexOfBit(int noBits);
  static int getPrefixBits(int prefix, int noBits);

  AggReturn * identifyAggregate(double arrRate, double linkBW);
  void insertCluster(cluster * clusterList, int index, int count, int noBits);
  void goDownCluster(cluster * clusterList, int index);
  void sortCluster(cluster * clusterList, int lastIndex);
  void swapCluster(cluster * clusterList, int id1, int id2);

  AggReturn * calculateLowerBound(); 
};

class IdentStruct {

 public:
  PrefixTree * dstTree_;
  PrefixTree * srcTree_;
  DropHashTable* dropHash_;
  
  double lowerBound_;
  void setLowerBound(double newBound, int averageIt);
  AggReturn * calculateLowerBound();
  
  IdentStruct();
  
  void reset();
  void traverse();
  
  void registerDrop(Packet *);
  
  static unsigned int getMaxAddress();
  static int getBitI(unsigned int, int);
 
  AggReturn * identifyAggregate(double arrRate, double linkBW);
  
};

#endif
