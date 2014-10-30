#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tcl.h>

#include "proxytrace.h"
#include "my-endian.h"
 
#ifdef PC
#include <io.h>
#endif

void PrintEntry_Text(FILE *out_file, TEntry *entry, int noURL);
void PrintEntry_Squid(FILE *out_file, TEntry *entry, int swap);

static enum { ifNone = 0, ifDECV1_0, ifDECV1_2 } InputFormat = ifNone;
static enum { ofNone = 0, ofDECText, ofSquid, ofSquidSwapped } OutputFormat = ofNone;

int getInputFormat(char* argv[]) {

	if (strncmp("-i", argv[0], 3) != 0)
		return ifNone;

	if (strncmp("v1.0", argv[1], 5) == 0)
		InputFormat = ifDECV1_0;
	else
	if (strncmp("v1.2", argv[1], 5) == 0)
		InputFormat = ifDECV1_2;
	else
		InputFormat = ifNone;

	return InputFormat;
}

int getOutputFormat(char* argv[]) {

	if (strncmp("-o", argv[0], 3) != 0)
		return ofNone;

	if (strncmp("txt", argv[1], 4) == 0)
		OutputFormat = ofDECText;
	else
	if (strncmp("squid", argv[1], 6) == 0)
		OutputFormat = ofSquid;
	else
	if (strncmp("squidsw", argv[1], 8) == 0)
		OutputFormat = ofSquidSwapped;
	else
		OutputFormat = ofNone;

	return OutputFormat;
}

extern FILE *cf, *sf;
extern double initTime;
extern double duration;
extern double startTime;
extern ReqLog* rlog;
extern unsigned int num_rlog, sz_rlog;
extern Tcl_HashTable cidHash, sidHash, urlHash;

double lf_analyze(TEntry& lfe);
void sort_url();
void sort_rlog();

int main (int argc, char* argv[]) 
{
	int    is_little_endian = 0;
	TEntry entry;
	double   ctime;

	// Init tcl
	Tcl_Interp *interp = Tcl_CreateInterp();
	if (Tcl_Init(interp) == TCL_ERROR) {
		printf("%s\n", interp->result);
		abort();
	}
	Tcl_InitHashTable(&cidHash, TCL_ONE_WORD_KEYS);
	Tcl_InitHashTable(&sidHash, TCL_ONE_WORD_KEYS);
	Tcl_InitHashTable(&urlHash, TCL_ONE_WORD_KEYS);

	if ((cf = fopen("reqlog", "w")) == NULL) {
		printf("cannot open request log.\n");
		exit(1);
	}
	if ((sf = fopen("pglog", "w")) == NULL) {
		printf("cannot open page log.\n");
		exit(1);
	}

	/* parse command line */
	if ((argc < 2) || (argc > 4)) {
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

	/* determine endian-ness */
	is_little_endian = IsLittleEndian();

	/* read trace header */
	ReadHeader(stdin, 0);

	/* read entries untill EOF */
	while (ReadEntry(stdin, &entry)) {
		if ( !is_little_endian )
			ToOtherEndian(&entry);
		// Analyse one log entry
		ctime = lf_analyze(entry);
		if ((duration > 0) && (ctime > duration))
			break;
	}

	Tcl_DeleteHashTable(&cidHash);
	Tcl_DeleteHashTable(&sidHash);

	fprintf(stderr, "sort url\n");
	sort_url();
	fclose(sf);

	fprintf(stderr, "sort requests\n");
	sort_rlog();
	fclose(cf);

	return 0;
}
