/*
 * Copyright (c) 1996 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 * This module contributed by John Brezak <brezak@apollo.hp.com>.
 * January 31, 1996
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/win32.c,v 1.6 2006/02/21 15:20:18 mahrenho Exp $ (LBL)
 */

#ifdef WIN32

#include <assert.h>
#include <io.h>
#include <process.h>
#include <fcntl.h>
#include <windows.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <winsock.h>
#include <tcl.h>
#include "config.h"
#include <locale.h>

/* forward declarations */
int WinGetUserName(ClientData, Tcl_Interp*, int ac, char**av);
int WinGetHostName(ClientData, Tcl_Interp*, int ac, char**av);
int WinPutRegistry(ClientData, Tcl_Interp*, int ac, char**av);
int WinGetRegistry(ClientData, Tcl_Interp*, int ac, char**av);
void TkConsoleCreate();
int TkConsoleInit(Tcl_Interp* interp);
        
int
uname(struct utsname *ub)
{
    char *ptr;
    DWORD version;
    SYSTEM_INFO sysinfo;
    char hostname[4096];
    
    version = GetVersion();
    GetSystemInfo(&sysinfo);
    
    switch (sysinfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL:
	(void)strcpy(ub->machine, "ix86");
	break;
    case PROCESSOR_ARCHITECTURE_MIPS :
	(void)strcpy(ub->machine, "mips");
	break;
    case PROCESSOR_ARCHITECTURE_ALPHA:
	(void)strcpy(ub->machine, "alpha");
	break;
    case PROCESSOR_ARCHITECTURE_PPC:
	(void)strcpy(ub->machine, "ppc");
	break;
    default:
	(void)strcpy(ub->machine, "unknown");
	break;
    }
    
    if (version < 0x80000000) {
	(void)strcpy(ub->version, "NT");
    }
    else if (LOBYTE(LOWORD(version))<4) {
	(void)strcpy(ub->version, "Win32s");
    }
    else				/* Win95 */ {
	(void)strcpy(ub->version, "Win95");
    }
    (void)sprintf(ub->release, "%u.%u",
		  (DWORD)(LOBYTE(LOWORD(version))),
		  (DWORD)(HIBYTE(LOWORD(version))));
    (void)strcpy(ub->sysname, "Windows");
    if (gethostname(hostname, sizeof(hostname)) == 0) {
	if (ptr = strchr(hostname, '.'))
	    *ptr = '\0';
    }
    else {
	perror("uname: gethostname failed");
	strcpy(hostname, "FAILURE");
    }
    strncpy(ub->nodename, hostname, sizeof(ub->nodename));
    ub->nodename[_SYS_NMLN - 1] = '\0';
    return 0;
}

int strcasecmp(const char *s1, const char *s2)
{
    return stricmp(s1, s2);
}

uid_t getuid(void) 
{ 
    return 1;
    
}

gid_t getgid(void)
{
    return 0;
}

int gethostid(void)
{
    /*XXX*/
    return 0;
}

__inline int nice(int pri)
{
    return 0;
}

extern void TkWinXInit(HINSTANCE hInstance);
extern int main(int argc, const char *argv[]);
extern int __argc;
extern char **__argv;

static char argv0[255];		/* Buffer used to hold argv0. */

char *__progname = "mash";

void
ShowMessage(int level, char *msg)
{
    MessageBeep(level);
    MessageBox(NULL, msg, __progname,
	       level | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
}

int SetupConsole()
{
    /*
     * stuff from knowledge base Q105305 (see that for details)
     * open a console and do the work around to get the console to work in all
     * cases
     */
    int hCrt;
    FILE *hf=0;
    const COORD screenSz = {80, 5000}; /* size of console buffer */

    AllocConsole();

    hf=0;
    hCrt = _open_osfhandle(
            (long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
    if (hCrt!=-1) hf = _fdopen(hCrt, "w");
    if (hf!=0) *stdout = *hf;
    if (hCrt==-1 || hf==0 || 0!=setvbuf(stdout, NULL, _IONBF, 0)) {
            ShowMessage(MB_ICONINFORMATION,
                        "unable to mount reroute stdout");
            return FALSE;
    }
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), screenSz);

    hf=0;
    hCrt = _open_osfhandle(
        (long)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);
    if (hCrt!=-1) hf = _fdopen(hCrt, "w");
    if (hf!=0) *stderr = *hf;
    if (hCrt==-1 || hf==0 || 0!=setvbuf(stderr, NULL, _IONBF, 0)) {
            ShowMessage(MB_ICONINFORMATION,
                        "reroute stderr failed in SetupConsole");
            return FALSE;
    }

    hf=0;
    hCrt = _open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), _O_TEXT);
    if (hCrt!=-1) hf = _fdopen(hCrt, "r");
    if (hf!=0) *stdin = *hf;
    if (hCrt==-1 || hf==0 || 0!=setvbuf(stdin, NULL, _IONBF, 0)) {
            ShowMessage(MB_ICONINFORMATION,
                        "reroute stdin failed in SetupConsole");
            return FALSE;
    }
    return TRUE;
}
     

int APIENTRY
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpszCmdLine,
    int nCmdShow)
{
    char *p;
    WSADATA WSAdata;
    int retcode;

    setlocale(LC_ALL, "C");

    /* XXX
     * initialize our socket interface plus the tcl 7.5 socket
     * interface (since they redefine some routines we call).
     * eventually we should just call the tcl sockets but at
     * the moment that's hard to set up since they only support
     * tcp in the notifier.
     */
    if (WSAStartup(MAKEWORD (1, 1), &WSAdata)) {
    	perror("Windows Sockets init failed");
	abort();
    }
/*    TclHasSockets(NULL);

    TkWinXInit(hInstance); */

    /*
     * Increase the application queue size from default value of 8.
     * At the default value, cross application SendMessage of WM_KILLFOCUS
     * will fail because the handler will not be able to do a PostMessage!
     * This is only needed for Windows 3.x, since NT dynamically expands
     * the queue.
     */
    SetMessageQueue(64);

    GetModuleFileName(NULL, argv0, 255);
    p = argv0;
    __progname = strrchr(p, '/');
    if (__progname != NULL) {
	__progname++;
    }
    else {
	__progname = strrchr(p, '\\');
	if (__progname != NULL) {
	    __progname++;
	} else {
	    __progname = p;
	}
    }

    if (__argc>1) {            
            SetupConsole();
    }

    retcode=main(__argc, (const char**)__argv);
    if (retcode!=0) {
            assert(FALSE);      /* don't die without letting user know why */
    }
    return retcode;
}

#if 0

static char szTemp[4096];

int
printf(const char *fmt, ...)
{
    int retval;
    
    va_list ap;
    va_start (ap, fmt);
    retval = vsprintf(szTemp, fmt, ap);
    OutputDebugString(szTemp);
    ShowMessage(MB_ICONINFORMATION, szTemp);
    va_end (ap);

    return(retval);
}

int
fprintf(FILE *f, const char *fmt, ...)
{
    int retval;
    
    va_list ap;
    va_start (ap, fmt);
    if (f == stderr) {
	retval = vsprintf(szTemp, fmt, ap);
	OutputDebugString(szTemp);
	ShowMessage(MB_ICONERROR, szTemp);
	va_end (ap);
    }
    else
	retval = vfprintf(f, fmt, ap);
    
    return(retval);
}

void
perror(const char *msg)
{
    DWORD cMsgLen;
    CHAR *msgBuf;
    DWORD dwError = GetLastError();
    
    cMsgLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
			    FORMAT_MESSAGE_ALLOCATE_BUFFER | 40, NULL,
			    dwError,
			    MAKELANGID(0, SUBLANG_ENGLISH_US),
			    (LPTSTR) &msgBuf, 512,
			    NULL);
    if (!cMsgLen)
	fprintf(stderr, "%s%sError code %lu\n",
		msg?msg:"", msg?": ":"", dwError);
    else {
	fprintf(stderr, "%s%s%s\n", msg?msg:"", msg?": ":"", msgBuf);
	LocalFree((HLOCAL)msgBuf);
    }
}

int
WinPutsCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* ConsoleInfo pointer. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    int i, newline;
    char *fileId;

    i = 1;
    newline = 1;
    if ((argc >= 2) && (strcmp(argv[1], "-nonewline") == 0)) {
	newline = 0;
	i++;
    }
    if ((i < (argc-3)) || (i >= argc)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" ?-nonewline? ?fileId? string\"", (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * The code below provides backwards compatibility with an old
     * form of the command that is no longer recommended or documented.
     */

    if (i == (argc-3)) {
	if (strncmp(argv[i+2], "nonewline", strlen(argv[i+2])) != 0) {
	    Tcl_AppendResult(interp, "bad argument \"", argv[i+2],
		    "\": should be \"nonewline\"", (char *) NULL);
	    return TCL_ERROR;
	}
	newline = 0;
    }
    if (i == (argc-1)) {
	fileId = "stdout";
    } else {
	fileId = argv[i];
	i++;
    }

    if (strcmp(fileId, "stdout") == 0 || strcmp(fileId, "stderr") == 0) {
	char *result;
	int level;
	
	if (newline) {
	    int len = strlen(argv[i]);
	    result = ckalloc(len+2);
	    memcpy(result, argv[i], len);
	    result[len] = '\n';
	    result[len+1] = 0;
	} else {
	    result = argv[i];
	}
	if (strcmp(fileId, "stdout") == 0) {
	    level = MB_ICONINFORMATION;
	} else {
	    level = MB_ICONERROR;
	}
	OutputDebugString(result);
	ShowMessage(level, result);
	if (newline)
	    ckfree(result);
	return TCL_OK;
    } else {
	extern int Tcl_PutsCmd(ClientData clientData, Tcl_Interp *interp,
			       int argc, char **argv);

	return (Tcl_PutsCmd(clientData, interp, argc, argv));
    }
}
#endif

int
WinGetUserName(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char *argv[];			/* Argument strings. */
{
    char user[256];
    int size = sizeof(user);
    
    if (!GetUserName(user, &size)) {
	Tcl_AppendResult(interp, "GetUserName failed", NULL);
	return TCL_ERROR;
    }
    Tcl_AppendResult(interp, user, NULL);
    return TCL_OK;
}

int WinGetHostName(clientData, interp, argc, argv)
                   ClientData clientData;
                   Tcl_Interp *interp; /* Current interpreter. */
                   int argc; /* Number of arguments. */
                   char *argv[]; /* Argument strings. */               
{
        char hostname[MAXGETHOSTSTRUCT];
        if (SOCKET_ERROR == gethostname(hostname, MAXGETHOSTSTRUCT)) {
                Tcl_AddErrorInfo(interp, "gethostname failed!");
        }
        Tcl_AppendResult(interp, hostname, NULL);
        return TCL_OK;
}

static HKEY
regroot(root)
    char *root;
{
    if (strcasecmp(root, "HKEY_LOCAL_MACHINE") == 0)
	return HKEY_LOCAL_MACHINE;
    else if (strcasecmp(root, "HKEY_CURRENT_USER") == 0)
	return HKEY_CURRENT_USER;
    else if (strcasecmp(root, "HKEY_USERS") == 0)
	return HKEY_USERS;
    else if (strcasecmp(root, "HKEY_CLASSES_ROOT") == 0)
	return HKEY_CLASSES_ROOT;
    else
	return NULL;
}

int
WinGetRegistry(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    HKEY hKey, hRootKey;
    DWORD dwType;
    DWORD len, retCode;
    CHAR *regRoot, *regPath, *keyValue, *keyData;
    int retval = TCL_ERROR;
    
    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		"key value\"", (char *) NULL);
	return TCL_ERROR;
    }
    regRoot = argv[1];
    keyValue = argv[2];

    regPath = strchr(regRoot, '\\');
    *regPath++ = '\0';
    
    if ((hRootKey = regroot(regRoot)) == NULL) {
	Tcl_AppendResult(interp, "Unknown registry root \"",
			 regRoot, "\"", NULL);
	return (TCL_ERROR);
    }
    
    retCode = RegOpenKeyEx(hRootKey, regPath, 0,
			   KEY_READ, &hKey);
    if (retCode == ERROR_SUCCESS) {
	retCode = RegQueryValueEx(hKey, keyValue, NULL, &dwType,
				  NULL, &len);
	if (retCode == ERROR_SUCCESS &&
	    dwType == REG_SZ && len) {
	    keyData = (CHAR *) ckalloc(len);
	    retCode = RegQueryValueEx(hKey, keyValue, NULL, NULL,
				      keyData, &len);
	    if (retCode == ERROR_SUCCESS) {
		Tcl_AppendResult(interp, keyData, NULL);
		free(keyData);
		retval = TCL_OK;
	    }
	}
	RegCloseKey(hKey);
    }
    if (retval == TCL_ERROR) {
	Tcl_AppendResult(interp, "Cannot find registry entry \"", regRoot,
			 "\\", regPath, "\\", keyValue, "\"", NULL);
    }
    return (retval);
}

int
WinPutRegistry(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    HKEY hKey, hRootKey;
    DWORD retCode;
    CHAR *regRoot, *regPath, *keyValue, *keyData;
    DWORD new;
    int result = TCL_OK;
    
    if (argc != 4) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		"key value data\"", (char *) NULL);
	return TCL_ERROR;
    }
    regRoot = argv[1];
    keyValue = argv[2];
    keyData = argv[3];
    
    regPath = strchr(regRoot, '\\');
    *regPath++ = '\0';
    
    if ((hRootKey = regroot(regRoot)) == NULL) {
	Tcl_AppendResult(interp, "Unknown registry root \"",
			 regRoot, "\"", NULL);
	return (TCL_ERROR);
    }

    retCode = RegCreateKeyEx(hRootKey, regPath, 0,
			     "",
			     REG_OPTION_NON_VOLATILE,
			     KEY_ALL_ACCESS,
			     NULL,
			     &hKey, &new);
    if (retCode == ERROR_SUCCESS) {
	retCode = RegSetValueEx(hKey, keyValue, 0, REG_SZ, keyData, strlen(keyData));
	if (retCode != ERROR_SUCCESS) {
	    Tcl_AppendResult(interp, "unable to set key \"", regRoot, "\\",
			     regPath, "\" with value \"", keyValue, "\"",
			     (char *) NULL);
	    result = TCL_ERROR;
	}
	RegCloseKey(hKey);
    }
    else {
	Tcl_AppendResult(interp, "unable to create key \"", regRoot, "\\",
			 regPath, "\"", (char *) NULL);
	result = TCL_ERROR;
    }
    return (result);
}

int platformInit(Tcl_Interp* interp)
{
        /* tcl.CreateCommand("puts", WinPutsCmd, (ClientData)0); */
        Tcl_CreateCommand(interp, "getusername", WinGetUserName,
                          (ClientData)0, (Tcl_CmdDeleteProc*)0);
        Tcl_CreateCommand(interp, "gethostname", WinGetHostName,
                          (ClientData)0, (Tcl_CmdDeleteProc*)0);
        Tcl_CreateCommand(interp, "putregistry", WinPutRegistry,
                          (ClientData)0, (Tcl_CmdDeleteProc*)0);
        Tcl_CreateCommand(interp, "getregistry", WinGetRegistry,
                          (ClientData)0, (Tcl_CmdDeleteProc*)0);
        
#ifndef NO_TK
        /*
         * Initialize the console only if we are running as an interactive
         * application.
         */    
        if (0==strcmp(Tcl_GetVar(interp, "tcl_interactive",
                                 TCL_GLOBAL_ONLY), "1")) {
                /*
                 * Create the console channels and install them as the standard
                 * channels.  All I/O will be discarded until TkConsoleInit is
                 * called to attach the console to a text widget.
                 */            
                TkConsoleCreate();
                if (TkConsoleInit(interp) == TCL_ERROR) {
                        fprintf(stderr, "error calling TkConsoleInit\n");
                }
        }
#endif
        return TCL_OK;
}

#endif

static const char ISO_C_forbids_an_empty_source_file; /* don't know why ... */
