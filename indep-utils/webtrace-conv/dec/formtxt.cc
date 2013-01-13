#include <stdio.h>

#include "proxytrace.h"

void PrintHeader(FILE *out_file, int noURL) {

	fprintf(out_file, "           time   event    server client   ");
	fprintf(out_file, " size last mod status  meth. prot.   server  port   type  flags   path  query");

	if (noURL)
		fprintf(out_file, "\n");
	else
		fprintf(out_file, "  url #\n");
}
 
void PrintEntry_Text(FILE *out_file, TEntry *entry, int noURL) {

	static int entry_count = 0;

	if (!entry_count++)
		PrintHeader(out_file, noURL);

	fprintf(out_file, "%9d%06u %8d %8d %5d %7d %9d %6d %6s %6s %6d %6d %6s %6d %6d %6d",
		entry -> head.time_sec, entry -> head.time_usec, 
		entry -> head.event_duration, entry -> head.server_duration,
		entry -> head.client, entry -> head.size,  entry -> head.last_mod,
		entry -> tail.status,
		MethodStr(entry -> tail.method),
		ProtocolStr(entry -> tail.protocol),
		entry -> head.server, entry -> head.port, 
		ExtensionTypeStr(entry -> tail.type),
        entry -> tail.flags,
		entry -> head.path, entry -> head.query);

	if (noURL)
		fprintf(out_file, "\n");
	else
		fprintf(out_file, " %6d\n", entry -> url);
}
