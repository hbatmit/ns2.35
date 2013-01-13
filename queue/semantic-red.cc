/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * A RED queue that allows certain operations to be done in a way
 * that depends on higher-layer semantics. The only such operation
 * at present is pickPacketToDrop(), which invokes the corresponding
 * member function in SemanticPacketQueue.
 */

#include "red.h"
#include "semantic-packetqueue.h"

class SemanticREDQueue : public REDQueue {
public:
	SemanticREDQueue() : REDQueue() {}
	Packet* pickPacketToDrop() {
		return(((SemanticPacketQueue*) pq_)->pickPacketToDrop());
	}
	Packet* pickPacketForECN(Packet *pkt) {
		return(((SemanticPacketQueue*) pq_)->pickPacketForECN(pkt));
	}
};

static class SemanticREDClass : public TclClass {
public:
	SemanticREDClass() : TclClass("Queue/RED/Semantic") {}
	TclObject* create(int, const char*const*) {
		return (new SemanticREDQueue);
	}
} class_semantic_red;

