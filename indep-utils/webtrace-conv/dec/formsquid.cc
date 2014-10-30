#include <stdio.h>

#include "proxytrace.h"


/* emulates a query part of a URL */
const char *make_query(u_4bytes query) {
	static char buf[128];

	sprintf(buf, "?%d", query);

	return buf;
}

/* emulates a port part of a URL */
const char *make_port(u_4bytes port) {
	static char buf[128];

	sprintf(buf, ":%d", port);

	return buf;
}

void PrintEntry_Squid(FILE *out_file, TEntry *entry, int swap) {

	u_4bytes duration = ((entry -> head.event_duration+500)/1000); /* milliseconds */

	fprintf(stdout, "%9d.%06u %d %d %s/%d %d %s %s://%d%s/%s%d%s%s %s %s/%d\n",
		entry -> head.time_sec, entry -> head.time_usec,
		(( swap) ? entry -> head.client : duration),
		((!swap) ? entry -> head.client : duration),
		"NONE", /* Log Tag is missing */
		entry -> tail.status,
		entry -> head.size,
		MethodStr(entry -> tail.method),
        ProtocolStr(entry -> tail.protocol),
		entry -> head.server,
		((entry -> tail.flags & PORT_SPECIFIED_FLAG) ? make_port(entry -> head.port) : ""),
		((entry -> tail.flags & CGI_BIN_FLAG) ? "cgi_bin/" : ""),
		entry -> head.path,
		((entry -> tail.flags & EXTENSION_SPECIFIED_FLAG) ? ExtensionStr(entry -> tail.type) : ""),
		((entry -> tail.flags & QUERY_FOUND_FLAG) ? make_query(entry -> head.query) : ""),
		"-", /* Ident is missing */
		"DIRECT", /* assuming direct retrieval */
		entry -> head.server);
}
