
/*
 * path.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: path.h,v 1.7 2005/08/25 18:58:05 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Ported from CMU/Monarch's code, appropriate copyright applies.  
/* -*- c++ -*- 
   path.h

   handles source routes
   
*/
#ifndef _path_h
#define _path_h

extern "C" {
#include <stdio.h>
#include <assert.h>
}

#include <packet.h>
#include "hdr_sr.h"

class Path;			// forward declaration

// state used for tracing the performance of the caches
enum Link_Type {LT_NONE = 0, LT_TESTED = 1, LT_UNTESTED = 2};
enum Log_Status {LS_NONE = 0, LS_UNLOGGED = 1, LS_LOGGED = 2};

// some type conversion between exisiting NS code and old DSR sim
typedef double Time;
enum ID_Type {NONE = NS_AF_NONE, MAC = NS_AF_ILINK, IP = NS_AF_INET };

struct ID {
  friend class Path; 
  ID() : type(NONE), t(-1), link_type(LT_NONE), log_stat(LS_NONE) {}
  //  ID():addr(0),type(NONE) {}	// remove for speed? -dam 1/23/98
  //ID(unsigned long name, ID_Type t):addr(name),type(t), t(-1), link_type(LT_NONE),log_stat(LS_NONE)
  //{
  //assert(type == NONE || type == MAC || type == IP);
  //}
  ID(unsigned long name, ID_Type t):addr(name), type(t), t(-1), 
    link_type(LT_NONE),log_stat(LS_NONE)
	{
		assert(type == NONE || type == MAC || type == IP);
	}
  inline ID(const struct sr_addr &a): addr(a.addr), 
    type((enum ID_Type) a.addr_type), t(-1), link_type(LT_NONE),
	  log_stat(LS_NONE)
	{
		assert(type == NONE || type == MAC || type == IP);
	}
  inline void fillSRAddr(struct sr_addr& a) {
	  a.addr_type = (int) type;
	  a.addr = addr;
  }    
  inline nsaddr_t getNSAddr_t() const {
	  assert(type == IP); return addr;
  }
  inline bool operator == (const ID& id2) const {
	  return (type == id2.type) && (addr == id2.addr);
  }
  inline bool operator != (const ID& id2) const {return !operator==(id2);}
  inline int size() const {return (type == IP ? 4 : 6);} 
  void unparse(FILE* out) const;
  char* dump() const;

  unsigned long addr;
  ID_Type type;

  Time t;			// when was this ID added to the route
  Link_Type link_type;
  Log_Status log_stat;
};

extern ID invalid_addr;
extern ID IP_broadcast;

class Path {
friend void compressPath(Path& path);
friend void CopyIntoPath(Path& to, const Path& from, int start, int stop);
public:
  Path();
  Path(int route_len, const ID *route = NULL);
  Path(const Path& old);
  Path(const struct sr_addr *addrs, int len);
  Path(struct hdr_sr *srh);

  ~Path();

  void fillSR(struct hdr_sr *srh);

  inline ID& next() {assert(cur_index < len); return path[cur_index++];}
  inline void resetIterator() {  cur_index = 0;}
  inline void reset() {len = 0; cur_index = 0;}

  inline void setIterator(int i) {assert(i>=0 && i<len); cur_index = i;}
  inline void setLength(int l) {assert(l>=0 && l<=MAX_SR_LEN); len = l;}
  inline ID& operator[] (int n) const {  
    assert(n < len && n >= 0);
    return path[n];}
  void operator=(const Path& rhs);
  bool operator==(const Path& rhs);
  inline void appendToPath(const ID& id) { 
    assert(len < MAX_SR_LEN); 
    path[len++] = id;}
  void appendPath(Path& p);
  bool member(const ID& id) const;
  bool member(const ID& net_id, const ID& MAC_id) const;
  Path copy() const;
  void copyInto(Path& to) const;
  Path reverse() const;
  void reverseInPlace();
  void removeSection(int from, int to);
  // the elements at indices from -> to-1 are removed from the path

  inline bool full() const {return (len >= MAX_SR_LEN);}
  inline int length() const {return len;}
  inline int index() const {return cur_index;}
  inline int &index() {return cur_index;}
  int size() const; // # of bytes needed to hold path in packet
  void unparse(FILE *out) const;
  char *dump() const;
  inline ID &owner() {return path_owner;}

  void checkpath(void) const;
private:
  int len;
  int cur_index;
  ID* path;
  ID path_owner;
};

void compressPath(Path& path);
// take a path and remove any double backs from it
// eg:  A B C B D --> A B D

void CopyIntoPath(Path& to, const Path& from, int start, int stop);
// sets to[0->(stop-start)] = from[start->stop]

#endif // _path_h
