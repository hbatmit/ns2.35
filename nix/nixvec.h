
/*
 * nixvec.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: nixvec.h,v 1.3 2005/08/25 18:58:10 johnh Exp $
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

/*
 * Support for NixVector routing
 * contributed to ns from 
 * George F. Riley, Georgia Tech, Spring 2000
 */

#ifndef __NIXVEC_H__
#define __NIXVEC_H__

#include <utility>  // for pair
#ifdef WIN32
#include <pair.h>   // for MSVC 6.0 that doens't have a proper <utility>
#endif /* WIN32 */

// Define a type for the neighbor index
typedef unsigned long Nix_t;
typedef unsigned long Nixl_t; // Length of a NV
const   Nix_t NIX_NONE = 0xffffffff;    // If not a neighbor
const   Nixl_t NIX_BPW = 32;            // Bits per long word
typedef pair<Nix_t,  Nixl_t> NixPair_t; // Index, bits needed
typedef pair<Nix_t*, Nixl_t> NixpPair_t;// NV Pointer, length


// The variable length neighbor index routing vector
class NixVec {
  public :
    NixVec () : m_pNV(0), m_used(0), m_alth(0) { };
    NixVec(NixVec*);              // Construct from existing
    ~NixVec();                    // Destructor
    void   Add(NixPair_t);        // Add bits to the nix vector
    Nix_t  Extract(Nixl_t);       // Extract the specified number of bits
    Nix_t  Extract(Nixl_t, Nixl_t*); // Extract using external "used"
    NixpPair_t Get(void);         // Get the entire nv
    void   Reset();               // Reset used to 0
    Nixl_t Lth();                 // Get length in bits of allocated
    void   DBDump();              // Debug..print it out
    Nixl_t ALth() { return m_alth;} // Debug...how many bits actually used
    static Nixl_t GetBitl(Nix_t); // Find out how many bits needed
  private :
    Nix_t* m_pNV;  // Points to variable lth nixvector (or actual if l == 32)
  //    Nixl_t m_lth;  // Length of this nixvector (computed from m_alth)
    Nixl_t m_used; // Used portion of this nixvector
    Nixl_t m_alth; // Actual length (largest used)
};

#endif
