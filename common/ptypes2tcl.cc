#include <stdio.h>
#include <ctype.h>
#include "packet.h"

// The following copied from ~tclcl/tcl2c++.c
/*
 * Define TCL2C_INT if your compiler has problems with long strings.
 */
#if defined(WIN32) || defined(_WIN32) || defined(__alpha__) || defined(__hpux)
#define TCL2C_INT
#endif

char** p_info::name_;
unsigned int p_info::nPkt_ = 0;
PacketClassifier *p_info::pc_ = 0;
int p_info::addPacket(char *name)
{
	if(!nPkt_)
		initName();
	
	int newID = nPkt_-1;
	PT_NTYPE = nPkt_;
	initName();
	name_[newID] = name;
	return newID;
}

void
printLine(char *s) {
#ifdef TCL2C_INT
	for (unsigned int i = 0; i < strlen(s); i++) 
		if ((i > 0) && ((i % 20) == 0))
			printf("%u,\n", s[i]);
		else
			printf("%u,", s[i]);
	printf("%u,%u,\n", '\\', '\n');
#else
	printf("%s\\n\\\n", s);
#endif
}

char *
lcase(const char *s) {
	static char charbuf[512];
	char* to = charbuf;
	while ((*to++ = tolower(*s++)))
		/* NOTHING */;
	*to = '\0';
	return charbuf;
}

int main() {
	p_info pinfo;

#ifdef TCL2C_INT
	printf("static const char code[] = {\n");
#else
	printLine("static const char code[] = \"");
#endif
	printLine("global ptype pvals");
	printLine("set ptype(error) -1");
	printLine("set pvals(-1) error");
	char strbuf[512];
	for (unsigned int i = 0; i < PT_NTYPE; i++) {
		sprintf(strbuf, "set ptype(%s) %d", lcase(pinfo.name(packet_t(i))), i);
		printLine(strbuf);
		sprintf(strbuf, "set pvals(%d) %s", i, pinfo.name(packet_t(i)));
		printLine(strbuf);
	}
	printLine("proc ptype2val {str} {");
	printLine("global ptype");
	printLine("set str [string tolower $str]");
	printLine("if ![info exists ptype($str)] {");
	printLine("set str error");
	printLine("}");
	printLine("set ptype($str)");
	printLine("}");
	printLine("proc pval2type {val} {");
	printLine("global pvals");
	printLine("if ![info exists pvals($val)] {");
	printLine("set val -1");
	printLine("}");
	printLine("set pvals($val)");
	printLine("}");
#ifdef TCL2C_INT
	printf("0 };\n");
#else
	printf("\";\n");
#endif
	printf("#include \"config.h\"\n");
	printf("EmbeddedTcl et_ns_ptypes(code);\n");
	return 0;
}
