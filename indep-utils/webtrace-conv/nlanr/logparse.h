#ifndef LOG_PARSE_H
#define LOG_PARSE_H

// Parsing NLANR root cache traces

#include <stdlib.h>
#include "config.h"

struct lf_entry {
	double rt;		/* request arrival time */
	u_int32_t cid;		/* client address */
	char *sid;
	u_int32_t size; 	/* page size */
	char* url;		/* url. NEED to be freed by caller */
};

const int MAXBUF = 4096;

int lf_get_next_entry(FILE *fp, lf_entry &ne);

#endif // LOG_PARSE_H
