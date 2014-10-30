/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * A RIO queue that allows certain operations to be done in a way
 * that depends on higher-layer semantics. The only such operation
 * at present is pickPacketToDrop(), which invokes the corresponding
 * member function in SemanticPacketQueue.
 */

#include "rio.h"
#include "semantic-packetqueue.h"

class SemanticRIOQueue : public RIOQueue {
public:
	SemanticRIOQueue() : RIOQueue() {}
	Packet* pickPacketToDrop() {
		return(((SemanticPacketQueue*) pq_)->pickPacketToDrop());
	}
	Packet* pickPacketForECN(Packet *pkt) {
		return(((SemanticPacketQueue*) pq_)->pickPacketForECN(pkt));
	}
};

static class SemanticRIOClass : public TclClass {
public:
	SemanticRIOClass() : TclClass("Queue/RED/RIO/Semantic") {}
	TclObject* create(int, const char*const*) {
		return (new SemanticRIOQueue);
	}
} class_semantic_rio;

