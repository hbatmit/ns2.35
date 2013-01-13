/*
 * Copyright (c) 1991-1994 Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and the Network Research Group at
 *	Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

 /*
 * SRR means Smoothed Round Robin. 
 * as compared with the DRR provided in NS, 
 * this version now can:
 * 1, assign multi flows to a queue
 * 2, assign same quantum to different queue
 * 3, use the flowid information in the packet to disdinguish 
 *    different packets.
 * 4, it has  a Weight Spread Sequence. the WSS with MAXWSSORDER is stored statically in the memory
 * 5, it has the Weight Matrix. the WM can be adjusted in the processing of SRR
 *
 * for details about SRR, see:  
 * " SRR: An O(1) Time Complexity Scheduler for Flows in Multi-Service Packet Networks", Chuanxiong Guo, sigcomm'01.
 */
// Ported by xuanc, 1/20/02

#include "config.h"   // for string.h
#include <stdlib.h>
#include "queue.h"
#include <math.h>
#include <assert.h>

#define MAXFLOW 100  // this version supports up to 1024 flows and queues.
#define MAXQUEUE 100 // if you want more, you can change it. 
#define MAXWSSORDER 16 // a 16th WSS needs pow(2, 16) space


//for debug 
#ifndef DEBUG_SRR
//#define DEBUG_SRR
#endif // DEBUG_SRR

// this struct is the basic element for the Weight Matrix
struct wm_node
{
	int queueid; // the queue the node belongs to 
	int weight;  // the weight of the node
	struct wm_node *next;
	struct wm_node *prev;
	struct wm_node *sibling;
};

#include "wss.h" 

class PacketSRR {
	PacketSRR(): pkts(0),bcount(0),deficitCounter(0),turn(0),head_(0), tail_(0), len_(0) {}
	friend class SRR;
protected: 
	int pkts;   //total packets in this Queue
	int bcount; //count of bytes in each flow to find the max flow;
	int deficitCounter; // concept borrowed from DRR 
	int turn;    // whether the queue is on service or not 
	int queueid; // mark the id of this packet queue

	Packet *head_, *tail_;
	int len_;

	inline void enque(Packet* p) {

		if (!tail_) head_= tail_= p;
		else {
			tail_->next_= p;
			tail_= p;
		}
		tail_->next_= 0;
		++len_;
	}
	inline Packet* deque() {
		if (!head_) return 0;
		Packet* p = head_;
		head_= p->next_; // 0 if p == tail_
		if (p == tail_) head_= tail_= 0;
		--len_;
		return p;
	}
	Packet* lookup(int n) {
		for (Packet* p = head_; p != 0; p = p->next_) {
			if (--n < 0)
				return (p);
		}
		return (0);
	}
};

class SRR : public Queue {
public :
	SRR();
	virtual int command(int argc, const char*const* argv);
	Packet *deque(void);
	void enque(Packet *pkt);
	void clear();

public:
	int maxqueuenumber_ ;	//total number of flows allowed
	int blimit_;			//total number of bytes allowed across all flows
	int bytecnt;			//cumulative sum of bytes across all flows
	int pktcnt;				// cumulative sum of packets across all flows
	int flwcnt;				//total number of active flows

	int last_queueid;		// the id of the previously served queue
	int last_size;			// the length of the last send packet
	
	PacketSRR srr[MAXFLOW]; //pointer to the entire srr struct

	int f2q[MAXFLOW];		// flow to queue mapping
	int private_rate[MAXFLOW];	// bandwidth for each queue
	int mtu_;					// it is used to mark the quantum adding to 
								// each queue
	int granularity_;			// the min rate that can be allocated to a queue
	int maxRate;
	int minRate; 

	class WSS wss;    // wss deals with the Weight Spread Sequence.
				
	/*
	 * wmHead[0] according to the least important weight,
   	 * wmHead[maxColumn] according to the most important weight
	 */
	struct wm_node wmHead[MAXWSSORDER];		// each column of WM has a head
	struct wm_node wmTail[MAXWSSORDER];		// each column of WM has a tail 
	struct wm_node *pwmCurr;				// points to the current scanning position

	struct wm_node *pRowHead[MAXFLOW];
	struct wm_node *pRowTail[MAXFLOW];


	int wmEmptyFlag;						// 0: WM is not empty, 1:WM is empty

	int maxColumn;		// the max column number of the WM, so there are total 
						// maxColumn+1 columns, the column number is numbered from 0 to 
						// maxColumn from left to right.
	int currMaxColumn;	// the current max number of the column
	
	int min_quantum;

	inline PacketSRR *getMaxflow () { //returns flow with max bytes 
		int i;
		PacketSRR *tmp=0;
		PacketSRR *maxflow=0;
		

		int bcount=0;

		for (i=0;i<maxqueuenumber_;i++) {
			tmp=&srr[i];
			if (tmp->bcount > bcount){
					bcount= tmp->bcount;
					maxflow=tmp;
			}
		}
		if(maxflow==0){
			fprintf(stderr, "getMaxflow, err");
			exit(1);
		}
		return maxflow;
	}
  
	//returns queuelength in packets
	inline int length () {
		return pktcnt;
	}

	//returns queuelength in bytes
	inline int blength () {
		return bytecnt;
	}

	int add_to_WM(int queueid, int weight);
	int del_from_WM(int queueid, int weight);
	struct wm_node * getNextNode( ); 
};

static class SRRClass : public TclClass {
public:
	SRRClass() : TclClass("Queue/SRR") {}
	TclObject* create(int, const char*const*) {
		return (new SRR);
	}
} class_srr;

SRR::SRR()
{	int i;

	maxqueuenumber_	= 10;
	blimit_ = 25000;
	mtu_= 1000; 		//1000 bytes quantum at default setting
	granularity_ = 1000; 	//default to 1K bit/s  

	last_queueid = -1; // -1 means that SRR does not have a previous deque operation	
	last_size = 0;

	flwcnt = 0;  // init 
	bytecnt = 0;
	pktcnt = 0;
	min_quantum = 1000;

	pwmCurr = 0; // at first, pwmCurr points to NULL

	for(i=0; i<MAXFLOW; i++)
	{
		private_rate[i] = granularity_; //default quantum value for each flow
		f2q[i]=0;		 // default queue id for all the flow
					 // or it will not works right at the default config
	}

	maxRate = 100000000; //100Mbps
	minRate = 1000; //1kbps	

	// init the WM double queues here too.
	for(i=0;i<MAXWSSORDER;i++){
		wmHead[i].prev=NULL;
		wmHead[i].next=&wmTail[i];
		wmTail[i].prev=&wmHead[i];
		wmTail[i].next=NULL;
		wmHead[i].queueid=wmTail[i].queueid=-1; // 
		wmHead[i].weight=wmTail[i].weight=i;
	}

	for (i=0;i<MAXFLOW; i++)	
		pRowHead[i]= pRowTail[i] = NULL;
	
	wmEmptyFlag=1; // it is empty at first
	wss.init(MAXWSSORDER); // create the Weight Spread Sequence

	currMaxColumn = -1;
	maxColumn=0;
	
	// allow the TCL scripts to change the following values
	bind("maxqueuenumber_",&maxqueuenumber_);  //it is the max queuenumber set in the TCL script
	bind("mtu_", &mtu_);      // set the Max Transfer Unit of the output link
	bind("granularity_", &granularity_); // set the rate allocation granularity of the 
				// all the flows. 
	bind("blimit_",&blimit_);
}


// enque and deque are two interface functions that provided by 
// NS 
void SRR::enque(Packet* pkt)
{
	PacketSRR *q;
	//PacketSRR *remq;
	int flowid, queueid;
	int weight;

	hdr_cmn *ch=hdr_cmn::access(pkt);
	hdr_ip *iph = hdr_ip::access(pkt);

	flowid= iph->flowid(); 		//get the flowid
	queueid= f2q[flowid]; 	// get the corresponding queue id

	if(queueid > maxqueuenumber_)
	{
		fprintf(stderr, "queueid too big\n");
		exit(1);
	}

#ifdef DEBUG_SRR
	printf("   in enque\n"); fflush(0);
#endif
	

// we drop packets from the longest queue
/*	if( bytecnt+ ch->size() > blimit_){

		drop(pkt);
		return;
	}
*/
	
	q=&srr[queueid];

	if(q->pkts==qlim_){

		drop(pkt);
		return;
	}

	q->enque(pkt);

	++q->pkts;
	++pktcnt;
	q->bcount += ch->size();
	bytecnt +=ch->size();


	if (q->pkts==1)
	{
			//add to the WM
			// adjust currMaxColumn
			// if it is a first active flow, put the pwmPtr
		        // all the above works are done in add_to_WM
			weight= private_rate[queueid];
			weight/=granularity_;
			
			add_to_WM(queueid, weight);
			
			q->deficitCounter=0;
	}

}

Packet *SRR::deque(void) 
{
	hdr_cmn *ch;
	hdr_ip *iph;
	Packet *pkt=0;
	int flowid;
	int queueid;
	PacketSRR *pP;
//	static int dcnt = 0;

//	if(dcnt <20){	
//	printf(" in dequeue: %lf \n",  Scheduler::instance().clock() ); 
//	dcnt++;
//	}
#ifdef DEBUG_SRR
printf("  in deque\n");
#endif

	if(last_queueid>=0){
//printf("last size=%d\n ", last_size);
			srr[last_queueid].bcount -= last_size;
			srr[last_queueid].pkts-=1;
			--pktcnt;
			bytecnt -= last_size;

			if (srr[last_queueid].pkts == 0) {
				srr[last_queueid].turn=0;
				srr[last_queueid].deficitCounter=0;

				// delete the queue from SRR
				del_from_WM(last_queueid, private_rate[last_queueid]/granularity_ );
			}

	}	

assert(pktcnt>=0);
	if (pktcnt==0) {
	//	fprintf (stderr,"No active flow\n");
		last_queueid=-1;
		return(0);
	}

	if(pwmCurr == NULL){
		printf("wrong, pwmCurr is NULL\n");
		exit(0);
	}

//printf("pktcnt=%d\n", pktcnt);

	while (!pkt) {

		if(pwmCurr->queueid==-1){ // it should never happen

			fprintf(stderr,"pwmCurr points to head or tail\n"); 
			fprintf(stderr, "weight:%d", pwmCurr->weight);
			exit(0);
		}
		pP=&srr[pwmCurr->queueid]; 

		pkt=pP->lookup(0);  

		if(pkt==0){
			fprintf(stderr, "wrong place, should never be here\n");
			exit(2);
	
		}
	
		iph = hdr_ip::access(pkt);

		flowid=iph->flowid();
		queueid= f2q[flowid]; // get the corresponding queue id

assert(queueid== pwmCurr->queueid);
	
		if (!pP->turn) {
			pP->deficitCounter+= mtu_; // each queue shares the same quantum!
					// this is the difference between DRR!
			pP->turn=1;
		}

		ch=hdr_cmn::access(pkt);

		if (pP->deficitCounter >= ch->size()) {
			pP->deficitCounter -= (ch->size());
			pkt=pP->deque();
			last_size=ch->size( );
			
//			pP->bcount -= ch->size();
//			--pP->pkts;
//			--pktcnt;
//			bytecnt -= ch->size();
//			if (pP->pkts == 0) {
//				pP->turn=0;
//				pP->deficitCounter=0;

				// delete the queue from SRR
//				del_from_WM(queueid, private_rate[queueid]/granularity_ );
				//getNextNode();
//			}
		
#ifdef DEBUG_SRR
printf("deque a packet, id=%d, size=%d\n", queueid, last_size);
#endif	
			last_queueid=queueid;

			return pkt;
		}
		else {
			pP->turn=0; 

 			// pwmCurr should be adjusted.
			getNextNode( );
			pkt=0;
		}
	}
	return 0;    // not reached
}

void SRR::clear()
{
	PacketSRR *q =srr;
	int i = maxqueuenumber_;

	if (!q)
		return;
	while (i--) {
		if (q->pkts) {
			fprintf(stderr, "Changing non-empty flow from srr\n");
			exit(1);
		}
		++q;
	}
}


// weight is the queues rate/granularity.
int SRR::add_to_WM(int queueid, int weight)
{
	struct wm_node *pNode;
	int i;
	//int j=maxColumn;
	//int temp=weight;
	//int flag=0;

	int old_colno = currMaxColumn;

	if(weight==0)
	{
		fprintf(stderr, "add_to_WM: weight should not be zero");
		exit(1);
	}

	if(weight > ( (1<<(maxColumn+1))-1) )
	{
		fprintf(stderr, "add_to_WM: weight too big");
		exit(1);
	} 

	// add to the WM
	// adjust currMaxColumn
	// if it is a first active flow, put the pwmPtr

	
	for(i=maxColumn; i>=0; i--)
	{

		if (weight & (1<<i) )
		{  
			// 
			// add to queueid= i; wmHead[queueid], wmTail[queueid]
			pNode=(struct wm_node*)malloc(sizeof(struct wm_node));

			if(pNode==NULL)
			{
				fprintf(stderr, "no memeory to create WM node");
				exit(2);
			}


			pNode->queueid = queueid;
			pNode->weight = i;
			pNode->sibling = NULL;	
			
			if(pRowTail[queueid] == NULL){
				pRowHead[queueid]= pRowTail[queueid] = pNode;
			}else{
				pRowTail[queueid]->sibling = pNode;
				pRowTail[queueid] = pNode;
			}

			if( pwmCurr && (pwmCurr->weight == i) ){
				pNode-> prev =  pwmCurr->prev;
				pNode-> next =  pwmCurr;
				(pwmCurr->prev)->next = pNode;
				pwmCurr->prev = pNode;

			}else {

				pNode->prev = wmTail[i].prev;
				pNode->next = &wmTail[i];
				(wmTail[i].prev)->next = pNode;
				wmTail[i].prev = pNode;
			}

			if(currMaxColumn < i)
				currMaxColumn = i; // adjust the current max column number
			
			if(wmEmptyFlag == 1)
			{
				wmEmptyFlag=0;
				if(pwmCurr == NULL) // we should let it points to the correct place.
					pwmCurr=pNode;
			}

		}
	}
	
	if (  old_colno < currMaxColumn )
	{
		if(old_colno >= 0){
	//	if(old_colno > 0){
			int pc = wss.get_ptr () + 1;
			pc = pc << (currMaxColumn - old_colno);
			wss.set_ptr ( pc -1);
		//	printf("set_ptr in add_to_wm: ptr:%d\n", pc-1);
		//	printf("old column no: %d %d\n", old_colno, currMaxColumn);
		}
	}

	//printf("in add_to_wm: k:%d, j:%d\n", currMaxColumn, old_colno);	
	++flwcnt;
	return 0;
}


//remove the wm_node from the links and free the memory
int SRR::del_from_WM(int queueid, int weight)
{
	struct wm_node *pNode, *pNode2;
	int i;
	int wss_term; 
	int temp; 


	if(pwmCurr->queueid==queueid)  // we adjust pwmCurr before we delete the row
	{ 
		if (pwmCurr->next != &wmTail[pwmCurr->weight])
			pwmCurr = pwmCurr->next ;
		else pwmCurr = NULL;
	}

	/*
	for(i=0;i<=currMaxColumn;i++) // travel all double links, and delete the node whose id is queueid
	{ 
		pNode=wmHead[i].next;

		while(pNode!=&wmTail[i]){
			if(pNode->queueid==queueid){ //yes, we get one.
				//remove it from the link, and free the node.
				(pNode->prev)->next=pNode->next;
				(pNode->next)->prev=pNode->prev;
				free(pNode);

				break;
			}
			else
				pNode=pNode->next;

		}
		
	} */

	pNode = pRowHead[queueid];
	while (pNode){
		(pNode->prev)->next=pNode->next;
		(pNode->next)->prev=pNode->prev;
		
		pNode2= pNode;
		pNode = pNode->sibling;
		free(pNode2);
	}

	pRowHead[queueid] = pRowTail[queueid] = NULL;

// we should adjust currMaxColumn, 
//and if currMaxColumn becomes less, we should also adjust WSS's pointer
	
	int old_colno = currMaxColumn;

	for(i=currMaxColumn;i>=0;i--)
	{
		if(wmHead[i].next != &wmTail[i])
		{
		  currMaxColumn=i;
		  break;
		}
	}

	if(i<0)
	{
		// it is empty now.
#ifdef DEBUG_SRR
//printf("WM empty\n");
#endif
		wmEmptyFlag=1;
		currMaxColumn=-1;
		pwmCurr=NULL;
		last_queueid=-1;
		wss.set_ptr (0); // reset the WSS sequence pointer to 0;
		goto rr;
	} else if ( currMaxColumn < old_colno)
	{
		int pc = wss.get_ptr () +1;
		//printf("pc = %d \n", pc);
		int mul = 1 << (old_colno - currMaxColumn ); 
		int tmpc = pc / mul;
		if (pc % mul){
			tmpc += 1;
			if (tmpc > ((1<<(currMaxColumn+1) )- 1) )
				tmpc = 1;

		}
		wss.set_ptr (tmpc -1);
		//printf("set_ptr in del_from__wm: ptr:%d\n", tmpc-1);
		//printf("k:%d j:%d\n", currMaxColumn, old_colno);

	}
	if (pwmCurr == NULL){
loop:
		wss_term=wss.get(currMaxColumn+1);
		wss.inc_ptr (currMaxColumn+1 );
		temp = currMaxColumn+1-wss_term;
		 
		// try to point pwmCurr to the right place.
		if( wmHead[temp].next != &wmTail[temp])	
		{
			pwmCurr=wmHead[temp].next;	

		}else
			goto loop;
	}

rr:
	--flwcnt;
	return 0;
}


/* should be checked:
 * whether it returns nothing when the WM is empty
 */
struct wm_node *SRR::getNextNode(){
  //struct wm_node *pNode;
  int weight;
  int wss_term;
  int temp;


	if(bytecnt==0){
		//	printf("getNextNode, pwmCurr = NULL, wmEmptyFlag=%d\n", wmEmptyFlag);
		return NULL;
	}

	weight= pwmCurr->weight;

	if(pwmCurr->next != &wmTail[weight]){ // not the tail 
		pwmCurr=pwmCurr ->next;
	} else{
		if (currMaxColumn==-1){
			fprintf(stderr, "getNextNode, empty WM");
			exit(0);
		}

again:
		wss_term=wss.get(currMaxColumn+1);
		wss.inc_ptr (currMaxColumn+1 );

		temp = currMaxColumn+1-wss_term;
		
		// wss_term according to currMaxColumn+1-wss_term queue
	 	if( wmHead[temp].next != &wmTail[temp])	{
			pwmCurr=wmHead[temp].next;	

		}else
			goto again;


	}

	return pwmCurr;
}


int getOrder(int i){
	int order=0;

	while( (1 << order)<= i)
		order++;
	return (order-1);	
}


/*
 *Allows one to change 
 *	blimit_  
 *	maxqueuenumber_ 
 *	mtu_
 *	granularity_
 *	for a particular srrQ :
 *
 *
 */
int SRR::command(int argc, const char*const* argv)
{


	if (argc==3) {

		if (strcmp(argv[1], "blimit") == 0) {
			blimit_ = atoi(argv[2]);
			if (bytecnt > blimit_)
				{
					fprintf (stderr,"More packets in buffer than the new limit");
					exit (1);
				}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "maxqueuenumber") == 0) {
			//clear();
			maxqueuenumber_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1],"mtu")==0) {
			mtu_= atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "granularity")==0) {
			granularity_ = atoi(argv[2]);
			return (TCL_OK);
		}
	}

	if (argc == 4) {
		if(!strcmp(argv[1],"setrate"))  {
			int rate;
			int queue,success=0;
			int temp;

			success += sscanf(argv[2],"%d",&queue);
			success += sscanf(argv[3],"%d",&rate);

			if(success!=2){
				fprintf(stderr, "SRR setrate ??"); exit(0);
				exit(1);
			}
		
			if ( queue>MAXQUEUE ) {
					fprintf(stderr,"queue id out of range"); exit(0);
			}
			min_quantum= min_quantum<rate ? min_quantum:rate;
			private_rate[queue]=rate;


			if(private_rate[queue]<minRate)
				private_rate[queue]=minRate;
			if(private_rate[queue]>maxRate){
				fprintf(stderr, "Rate too hight!\n");
				exit(1);
			}
				
				
			success=private_rate[queue]/granularity_;
			if(success==0) success=1;
			/*if(private_rate[queue]%granularity_)
				success++;*/
	 		temp=getOrder(success);  //now we have the order of the WSS
			if(maxColumn<temp){
				maxColumn=temp;

			}
	
#ifdef DEBUG_SRR
	printf("maxColumn=%d\n", maxColumn), fflush(0);
#endif
			if(maxColumn>(MAXWSSORDER-1)){ //the order of max band flow is too big!
				fprintf(stderr, "granularity too small or band too big!");	
				exit(2);
			}
	
			return (TCL_OK);
		}
	
	else if(!strcmp(argv[1],"setqueue")) {
			int queue,flow,success=0;
			success += sscanf(argv[2],"%d",&flow);
			success += sscanf(argv[3],"%d",&queue);
			if(success==2) {
				if ( !(queue<MAXQUEUE) ) {
                                        fprintf(stderr,"queue id out of range");exit(1);
                                }
				if ( !(flow<MAXFLOW) ) {
                                        fprintf(stderr,"flow id out of range"); exit(1);
                                }

				f2q[flow]=queue;

				return TCL_OK;
			}
		}
	}
	return (Queue::command(argc, argv));
}


