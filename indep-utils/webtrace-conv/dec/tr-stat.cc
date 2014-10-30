// Generate statistics from UCB traces
// All we need to know: 
// 
// (1) client request streams: 
//     <time> <clientID> <serverID> <URL_ID> 
// (2) server page mod stream(s):
//     <serverID> <URL_ID> <PageSize>
//
// Part of the code comes from Steven Gribble's UCB trace parse codes
// 
// $Header: /cvsroot/nsnam/ns-2/indep-utils/webtrace-conv/dec/tr-stat.cc,v 1.3 2005/09/18 23:33:32 tomh Exp $

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <tcl.h>

#include "proxytrace.h"

FILE *cf, *sf;
double initTime = -1;
double duration = -1;
double startTime = -1;

Tcl_HashTable cidHash;  // Client id (IP, port) hash
static int client = 0;	// client sequence number

Tcl_HashTable sidHash;	// server id (IP, port) hash
static int server = 0;	// server sequence number

Tcl_HashTable urlHash;	// URL id hash
static int url = 0;	// URL sequence number
static int* umap;	// URL mapping table, used for url sort

ReqLog* rlog = NULL;
unsigned int num_rlog = 0, sz_rlog = 0;

static int compare(const void *a1, const void *b1)
{
	const ReqLog *a = (const ReqLog*)a1, *b = (const ReqLog*)b1;
	return (a->time > b->time) ? 1 : 
		(a->time == b->time) ? 0 : -1;
}

void sort_rlog()
{
	qsort((void *)rlog, num_rlog, sizeof(ReqLog), compare);
	double t = rlog[0].time;
	for (unsigned int i = 0; i < num_rlog; i++) {
		rlog[i].time -= t;
		fprintf(cf, "%f %d %d %d\n", rlog[i].time, 
			rlog[i].cid, rlog[i].sid, umap[rlog[i].url]);
	}
	// Record trace duration and # of unique urls
	fprintf(cf, "i %f %u\n", rlog[num_rlog-1].time, url);

	fprintf(stderr, 
		"%d unique clients, %d unique servers, %d unique urls.\n", 
		client, server, url);
}

static int compare_url(const void* a1, const void* b1)
{
	const URL **a = (const URL**)a1, **b = (const URL**)b1;
	return ((*a)->access > (*b)->access) ? -1:
		((*a)->access == (*b)->access) ? 0 : 1;
}

void sort_url()
{
	// XXX use an interval member of Tcl_HashTable
	URL** tbl = new URL*[urlHash.numEntries];
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	int i = 0, sz = urlHash.numEntries;
	for (he = Tcl_FirstHashEntry(&urlHash, &hs);
	     he != NULL;
	     he = Tcl_NextHashEntry(&hs))
		tbl[i++] = (URL*)Tcl_GetHashValue(he);
	Tcl_DeleteHashTable(&urlHash);

	// sort using access frequencies
	qsort((void *)tbl, sz, sizeof(URL*), compare_url);
	umap = new int[url];
	// write sorted url to page table
	for (i = 0; i < sz; i++) {
		umap[tbl[i]->id] = i;
		fprintf(sf, "%d %d %d %u\n", tbl[i]->sid, i,
			tbl[i]->size, tbl[i]->access);
		delete tbl[i];
	}
	delete []tbl;
}

const unsigned long MAX_FILESIZE = 10000000;

double lf_analyze(TEntry& lfe)
{
	double time;
	int ne, cid, sid, uid;
	Tcl_HashEntry *he;

	// Filter out entries with 'post', 'head' etc. only keep 'get'
	// Also filter out 
	if (lfe.tail.method != METHOD_GET)
		return -1;
	if ((lfe.tail.flags & QUERY_FOUND_FLAG) || 
	    (lfe.tail.flags & CGI_BIN_FLAG))
		return -1;
	if ((lfe.tail.status != 200) && (lfe.tail.status != 304))
		return -1;

	// We don't consider pages with size 0
	if (lfe.head.size == 0)
		return -1;
	// We don't consider file size larger than 10MB
	if (lfe.head.size > MAX_FILESIZE)
		return -1;

	time = (double)lfe.head.time_sec + (double)lfe.head.time_usec/(double)1000000.0;

	if (initTime < 0) {
		initTime = time;
		time = 0;
	} else 
		time -= initTime;

	// If a trace start time is required, don't do anything
	if ((startTime > 0) && (time < startTime)) 
		return -1;

	// check client id
	long clientKey = lfe.head.client;
	if (!(he = Tcl_FindHashEntry(&cidHash, (const char *)clientKey))) {
		// new client, allocate a client id
		he = Tcl_CreateHashEntry(&cidHash, (const char *)clientKey, &ne);
		client++;
		long clientValue = client;
		Tcl_SetHashValue(he, clientValue);
		cid = client;
	} else {
		// existing entry, find its client seqno
		cid = (long)Tcl_GetHashValue(he);
	}

	// check server id
	long serverKey = lfe.head.server;
	if (!(he = Tcl_FindHashEntry(&sidHash, (const char *)serverKey))) {
		// new client, allocate a client id
		he = Tcl_CreateHashEntry(&sidHash, (const char *)serverKey, &ne);
		server++;
		long serverValue = server;
		Tcl_SetHashValue(he, serverValue);
		sid = server;
	} else {
		// existing entry, find its client seqno
		sid = (long)Tcl_GetHashValue(he);
	}

	// check url id
	long urlKey = lfe.url;
	if (!(he = Tcl_FindHashEntry(&urlHash, (const char*)urlKey))) {
		// new client, allocate a client id
		he = Tcl_CreateHashEntry(&urlHash, (const char*)urlKey, &ne);
		URL* u = new URL(++url, sid, lfe.head.size);
		Tcl_SetHashValue(he, (const char*)u);
		uid = u->id;
	} else {
		// existing entry, find its client seqno
		URL* u = (URL*)Tcl_GetHashValue(he);
		u->access++;
		uid = u->id;
	}

	rlog[num_rlog++] = ReqLog(time, cid, sid, uid);
	//fprintf(cf, "%f %d %d %d\n", time, cid, sid, uid);

	if (startTime > 0) 
		return time - startTime;
	else 
		return time;
}
