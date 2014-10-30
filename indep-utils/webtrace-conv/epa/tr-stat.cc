// Generate statistics from EPA traces 
//   http://ita.ee.lbl.gov/html/contrib/EPA-HTTP.html
//
// All we need to know: 
// 
// (1) client request streams: 
//     <time> <clientID> <serverID> <URL_ID> 
// (2) server page mod stream(s):
//     <serverID> <URL_ID> <PageSize>
//
// Part of the code comes from Steven Gribble's UCB trace parse codes
// 
// $Header: /cvsroot/nsnam/ns-2/indep-utils/webtrace-conv/epa/tr-stat.cc,v 1.3 2005/09/18 23:33:32 tomh Exp $

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

struct URL {
	URL(int i, int sd, int sz) : access(1), id(i), sid(sd), size(sz) {}
	int access;	// access counts
	int id;
	int sid, size;
};

struct ReqLog {
	ReqLog() {}
	ReqLog(double t, unsigned int c, unsigned int s, unsigned int u) :
		time(t), cid(c), sid(s), url(u) {}
	double time;
	unsigned int cid, sid, url;
};

FILE *cf, *sf;
double initTime = -1;
double duration = -1;
double startTime = -1;

Tcl_HashTable cidHash;  // Client id (IP, port) hash
static int client = 0;	// client sequence number

static int server = 1;

Tcl_HashTable urlHash;	// URL id hash
static int url = 0;	// URL sequence number
static int* umap;	// URL mapping table, used for url sort

ReqLog* rlog = NULL;
unsigned int num_rlog = 0, sz_rlog = 0;

struct Entry {
	char *client;
	unsigned int time;
	char *url;
	int size;
};

static int compare(const void *a1, const void *b1)
{
	const ReqLog *a, *b;
	a = (const ReqLog*)a1, b = (const ReqLog*)b1;
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
	const URL **a, **b;
	a = (const URL**)a1, b = (const URL**)b1;
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

double lf_analyze(Entry& lfe)
{
	double time;
	int ne, cid, sid, uid;
	Tcl_HashEntry *he;

	time = (double)lfe.time;

	if (initTime < 0) {
		initTime = time;
		time = 0;
	} else 
		time -= initTime;

	// If a trace start time is required, don't do anything
	if ((startTime > 0) && (time < startTime)) 
		return -1;

	// If page size is 0, ignore it
	if (lfe.size == 0) 
		return -1;

	// check client id
	if (!(he = Tcl_FindHashEntry(&cidHash, (const char *)lfe.client))) {
		// new client, allocate a client id
		he = Tcl_CreateHashEntry(&cidHash, (const char *)lfe.client, &ne);
		client++;
		long clientValue = client;
		Tcl_SetHashValue(he, clientValue);
		cid = client;
	} else {
		// existing entry, find its client seqno
		cid = (long)Tcl_GetHashValue(he);
	}

	// only a single server for EPA trace
	sid = 0;

	// check url id
	if (!(he = Tcl_FindHashEntry(&urlHash, (const char*)lfe.url))) {
		// new client, allocate a client id
		he = Tcl_CreateHashEntry(&urlHash, (const char*)lfe.url, &ne);
		URL* u = new URL(++url, sid, lfe.size);
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

int get_next_entry(Entry& lfe) 
{
	char buf[1024];

	if (feof(stdin)) 
		return 0;

	fgets(buf, 1024, stdin);
	if (feof(stdin) || ferror(stdin))
		return 0;

	char *tmp = buf, *code, *method, *date;
	lfe.client = strtok(tmp, " ");
	date = strtok(NULL, " "); 
	method = strtok(NULL, " "); 	// GET/POST
	*(method++) = 0;
	if (strcmp(method, "GET") != 0) 
		// Only take GET requests
		return -1;

	lfe.url = strtok(NULL, " "); 
	if (strchr(lfe.url, '?') != NULL) 
		// Do not take any url that contains '?'
		return -1;
	strtok(NULL, " "); 		// HTTP/1.0
	code = strtok(NULL, " "); 	// return code
	if ((atoi(code) != 200) && (atoi(code) != 304)) 
		return -1;
	// last element: size
	tmp = strtok(NULL, " ");
	lfe.size = atoi(tmp);

	// parse date
	// date is from internal string of strtok(), we have to copy it. 
	// What a stupid strtok()!!!!
	tmp = new char[strlen(date)+1];
	strcpy(tmp, date);	
	date = tmp + 1;
	lfe.time = 0;
	date = strtok(date, ":"); // day
	lfe.time = atoi(date);
	date = strtok(NULL, ":"); // hour
	lfe.time = lfe.time*24 + atoi(date);
	date = strtok(NULL, ":"); // minute
	lfe.time = lfe.time*60 + atoi(date);
	date = strtok(NULL, "]");
	lfe.time = lfe.time*60 + atoi(date);
	delete []tmp;

	return 1;
}

int main(int argc, char**argv)
{
	Entry lfntree;
	int      ret;
	double   ctime;

	// Init tcl
	Tcl_Interp *interp = Tcl_CreateInterp();
	if (Tcl_Init(interp) == TCL_ERROR) {
		printf("%s\n", interp->result);
		abort();
	}
	Tcl_InitHashTable(&cidHash, TCL_STRING_KEYS);
	Tcl_InitHashTable(&urlHash, TCL_STRING_KEYS);

	if ((cf = fopen("reqlog", "w")) == NULL) {
		printf("cannot open request log.\n");
		exit(1);
	}
	if ((sf = fopen("pglog", "w")) == NULL) {
		printf("cannot open page log.\n");
		exit(1);
	}

	if ((argc > 4) || (argc < 2)) {
		printf("Usage: %s <trace size> [<time duration>] [<start_time>]\n", argv[0]);
		return 1;
	}
	if (argc >= 3) {
		duration = strtod(argv[2], NULL);
		if (argc == 4) {
			startTime = strtod(argv[3], NULL);
			printf("start time = %f\n", startTime);
		}
	}

	sz_rlog = strtoul(argv[1], NULL, 10);
	rlog = new ReqLog[sz_rlog];

	while((ret = get_next_entry(lfntree)) != 0) {
		// Analyse one log entry
		if (ret < 0)
			continue;
		ctime = lf_analyze(lfntree);
		if ((duration > 0) && (ctime > duration))
			break;
	}
	Tcl_DeleteHashTable(&cidHash);

	fprintf(stderr, "sort url\n");
	sort_url();
	fclose(sf);

	fprintf(stderr, "sort requests\n");
	sort_rlog();
	fclose(cf);

	fprintf(stderr, 
		"%d unique clients, %d unique servers, %d unique urls.\n", 
		client, server, url);
	return 0;
}
