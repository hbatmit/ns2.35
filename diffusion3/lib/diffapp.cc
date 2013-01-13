//
// diffapp.cc     : Base Class for Diffusion Apps and Filters
// author         : Fabio Silva and Padma Haldar
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: diffapp.cc,v 1.11 2005/09/13 04:53:49 tomh Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
// Linking this file statically or dynamically with other modules is making
// a combined work based on this file.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//
// In addition, as a special exception, the copyright holders of this file
// give you permission to combine this file with free software programs or
// libraries that are released under the GNU LGPL and with code included in
// the standard release of ns-2 under the Apache 2.0 license or under
// otherwise-compatible licenses with advertising requirements (or modified
// versions of such code, with unchanged license).  You may copy and
// distribute such a system following the terms of the GNU GPL for this
// file and the licenses of the other code concerned, provided that you
// include the source code of that other code when and as the GNU GPL
// requires distribution of source code.
//
// Note that people who make modified versions of this file are not
// obligated to grant this special exception for their modified versions;
// it is their choice whether to do so.  The GNU General Public License
// gives permission to release a modified version without this exception;
// this exception also makes it possible to release a modified version
// which carries forward this exception.
//

#include "diffapp.hh"

#ifdef NS_DIFFUSION
int DiffApp::command(int argc, const char*const* argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "start") == 0) {
      start();
      return TCL_OK;
    }
  }
  if (argc == 3) {
    if (strcmp(argv[1], "dr") == 0) {
      DiffAppAgent *agent;
      agent = (DiffAppAgent *)TclObject::lookup(argv[2]);
      dr_ = agent->dr();
      return TCL_OK;
    }
  }
  return Application::command(argc, argv);
}

#else
void DiffApp::usage(char *s){
  DiffPrint(DEBUG_ALWAYS, "Usage: %s [-d debug] [-p port] [-f file] [-h]\n\n", s);
  DiffPrint(DEBUG_ALWAYS, "\t-d - Sets debug level (0-10)\n");
  DiffPrint(DEBUG_ALWAYS, "\t-p - Uses port 'port' to talk to diffusion\n");
  DiffPrint(DEBUG_ALWAYS, "\t-f - Specifies a config file\n");
  DiffPrint(DEBUG_ALWAYS, "\t-h - Prints this information\n");
  DiffPrint(DEBUG_ALWAYS, "\n");
  exit(0);
}

void DiffApp::parseCommandLine(int argc, char **argv)
{
  u_int16_t diff_port = DEFAULT_DIFFUSION_PORT;
  int debug_level;
  int opt;

  config_file_ = NULL;
  opterr = 0;

  while (1){
    opt = getopt(argc, argv, "f:hd:p:");
    switch (opt){

    case 'p':

      diff_port = (u_int16_t) atoi(optarg);
      if ((diff_port < 1024) || (diff_port >= 65535)){
	DiffPrint(DEBUG_ALWAYS, "Error: Diffusion port must be between 1024 and 65535 !\n");
	exit(-1);
      }

      break;

    case 'h':

      usage(argv[0]);

      break;

    case 'd':

      debug_level = atoi(optarg);

      if (debug_level < 1 || debug_level > 10){
	DiffPrint(DEBUG_ALWAYS, "Error: Debug level outside range or missing !\n");
	usage(argv[0]);
      }

      global_debug_level = debug_level;

      break;

    case 'f':

      if (!strncasecmp(optarg, "-", 1)){
	DiffPrint(DEBUG_ALWAYS, "Error: Parameter missing !\n");
	usage(argv[0]);
      }

      config_file_ = strdup(optarg);

      break;

    case '?':

      DiffPrint(DEBUG_ALWAYS,
		"Error: %c isn't a valid option or its parameter is missing !\n", optopt);
      usage(argv[0]);

      break;

    case ':':

      DiffPrint(DEBUG_ALWAYS, "Parameter missing !\n");
      usage(argv[0]);

      break;

    }

    if (opt == -1)
      break;
  }

  diffusion_port_ = diff_port;
}

#endif // NS_DIFFUSION
