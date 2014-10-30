#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "logparse.h"

// Read next line into file
int lf_get_next_entry(FILE *fp, lf_entry &ne)
{
	char buf[MAXBUF]; // must be large enough for a cache log

	if ((fgets(buf, MAXBUF, fp) == NULL) || feof(fp) || ferror(fp)) {
		return 1;
	}

	// Parse a line and fill an lf_entry
	char *p = buf, *q, *tmp1, *tmp2, *ret_code;
	u_int32_t lapse;

	// first two entries: <TimeStamp> and <Elapsed Time>
	q = strtok(p, " ");
	ne.rt = strtod(q, NULL);
	q = strtok(NULL, " ");
	lapse = strtoul(q, NULL, 10);
	ne.rt -= (double)lapse/1000.0;

	// Client address
	q = strtok(NULL, " ");
	ne.cid = (u_int32_t)inet_addr(q);

	// Log tags, do not store them but use it to filter entries
	ret_code = strtok(NULL, " ");
	if (ret_code == NULL) { abort(); } 
	// XXX Have to handle this return code in the end because we are using
	// strtok() and it cannot interleave two strings :( STUPID!!

	// Page size
	q = strtok(NULL, " "); 
	ne.size = strtoul(q, NULL, 10);

	// Request method, GET only
	q = strtok(NULL, " ");
	if (strcmp(q, "GET") != 0) 
		return -1;

	// URL
	q = strtok(NULL, " ");
	if (q == NULL) abort(); 
	if (strchr(q, '?') != NULL) 
		// Do not accept any URL containing '?'
		return -1;
	ne.url = new char[strlen(q) + 1];
	strcpy(ne.url, q);
	// Try to locate server name from the URL
	// XXX no more parsing from the original string!!!!
	tmp1 = strtok(q, "/");
	if (strcmp(tmp1, "http:") != 0) {
		// How come this isn't a http request???
		delete []ne.url;
		return -1;
	}
	tmp1 = strtok(NULL, "/"); 
	if (tmp1 == NULL) abort();
	ne.sid = new char[strlen(tmp1) + 1];
	strcpy(ne.sid, tmp1);

	// Now check return codes
	if (ret_code == NULL) abort();
	tmp1 = new char[strlen(ret_code)+1];
	strcpy(tmp1, ret_code);
	tmp2 = strtok(tmp1, "/");
	tmp2 += 4; // Ignore the first 4 char "TCP_"
	if ((strcmp(tmp2, "MISS") == 0) || 
	    (strcmp(tmp2, "CLIENT_REFRESH_MISS") == 0) || 
	    (strcmp(tmp2, "IMS_MISS") == 0) || 
	    (strcmp(tmp2, "DENIED") == 0)) {
		delete []ne.url;
		delete []ne.sid;
		delete []tmp1;
		return -1;	// Return negative to discard this entry
	}
	delete []tmp1;

	// All the rest are useless, do not parse them
	return 0;
}



