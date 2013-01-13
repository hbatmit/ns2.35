
/*
 * Copyright (c) 2007 Regents of the SIGNET lab, University of Padova.
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
 * 3. Neither the name of the University of Padova (SIGNET lab) nor the 
 *    names of its contributors may be used to endorse or promote products 
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */



/**
 * This file is used when ns is built into two separate components: 
 * 1) a shared library which includes all the code of the simulator
 * 2) a tiny executable (built from this file) which has the only purpose of 
 *    loading the library described above
 * This setup is needed to support the loading of dynamic libraries on 
 * windows systems (e.g. cygwin), in order to overcome some well-known limitations
 * of the DLL format (in particular, the fact that an executable cannot export 
 * its simbols to the libraries that it loads).
 * For most other operating systems, common/main-monolithic.cc is used instead 
 * of this file.
 * The selection of the main file used for the build is made in the Makefile, 
 * depending on the operating system (as detected by ./configure)
 */


#include<stdio.h>
#include <dlfcn.h>


int
main(int argc, char** argv)
{
  void *handle = dlopen(NSLIBNAME, RTLD_LAZY);
  if(!handle)
    {
      fprintf(stderr,"Error: Cannot open shared library: %s\n", dlerror());
      return 1;
    }

  typedef int (*nslibmain_t)(int,char**);
  dlerror();
  nslibmain_t nslibmain = (nslibmain_t)dlsym(handle, "nslibmain");
  const char* dlsym_error=dlerror();
  if(dlsym_error) 
    {
      fprintf(stderr,"Cannot load symbol libmain: %s\n", dlsym_error);
      dlclose(handle);
      return 1;
    }

  nslibmain(argc,argv);

  dlclose(handle);
  return 0;			/* Needed only to prevent compiler warning. */
}

