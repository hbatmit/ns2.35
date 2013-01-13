
/*
 * nixvec.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: nixvec.cc,v 1.6 2009/12/30 22:06:34 tom_henderson Exp $
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
 *
 */

#include "config.h"
#ifdef HAVE_STL

#include <string.h> // for memcpy
#include <stdio.h>
#include "nix/nixvec.h"

#ifndef TEST_NIXVEC

// Constructor from existing
NixVec::NixVec(NixVec* p) : m_used(0), m_alth(p->m_alth)
{
  if (Lth() > NIX_BPW)
    { // Need to allocate storage
      m_pNV = new Nix_t[Lth() / NIX_BPW];
      memcpy(m_pNV, p->m_pNV, Lth()/8);
    }
  else
    { // Just use the pointer
      m_pNV = p->m_pNV;
    }
}

NixVec::~NixVec()
{ // Destructor 
  if(0)printf("Hello from nixvec destructor\n");
  if (Lth() > NIX_BPW) delete [] m_pNV;  // Delete the allocated memory
}

void NixVec::Add( NixPair_t p)
{ // Add some bits to the nix vector
  Nixl_t newused;
  Nix_t  newbits;
  Nixl_t newlth;

  if (p.second == 0) return; // Not really possible!

#ifdef NEED_DEBUG
  // debug follows
  Nix_t db;

  if (Lth() == NIX_BPW)
    db = (Nix_t)m_pNV;
  else
    db = m_pNV[0];
  db = ((db & 0xfffffff0) ^ 0x00038520);
  if (!db)
    {
      printf("Adding %ld lth %ld\n", p.first, p.second);
      DBDump();
    }
  // end debug
#endif

  newused = m_used + p.second;
  if (newused > Lth())
    { // Need more room
      newlth = Lth() + NIX_BPW;
      Nix_t* pNew = new Nix_t[newlth / NIX_BPW];
      if (Lth() == NIX_BPW)
        { // Old is not pointer, just transfer
          pNew[0] = (Nix_t)m_pNV;
        }
      else
        {
          for (unsigned long k = 0; k < Lth() / NIX_BPW; k++)
            pNew[k] = m_pNV[k];
        }
      pNew[newlth / NIX_BPW - 1] = 0;
      if (Lth() != NIX_BPW)
        delete [] m_pNV;
      m_pNV = pNew;
      if (m_used == Lth())
        { // No overlap, just use the new entry
          newbits = p.first << (NIX_BPW - m_used + (newlth - 32)  - p.second);
          Nixl_t i = (m_used / NIX_BPW);
          m_pNV[i] |= newbits;
        }
      else
        {
          // Fill in remaining of previous word
          Nixl_t left = Lth() - m_used;
          newbits = p.first >> (p.second - left);
          Nixl_t i = (m_used / NIX_BPW);
          m_pNV[i] |= newbits;
          // And the next word
          newbits = p.first << (NIX_BPW - (p.second - left));
          i = (m_used / NIX_BPW) + 1;
          m_pNV[i] |= newbits;
        }
      //     Lth() = newlth;
    }
  else
    {
      newbits = p.first << (NIX_BPW - m_used + (Lth() - 32)  - p.second);
      if (Lth() == NIX_BPW)
        { // Not a pointer, just the vector
          Nix_t n = (Nix_t)m_pNV;
          n |= newbits;
          m_pNV = (Nix_t*)n;
        }
      else
        { // Pointer, find the correct index
          Nixl_t i = (m_used / NIX_BPW);
          m_pNV[i] |= newbits;
        }
    }
  m_used = newused;
  if (m_used > m_alth) m_alth = m_used;
#ifdef NEED_DEBUG
  if (!db)
    {
      printf("Added  %ld lth %ld\n", p.first, p.second);
      DBDump();
    }
#endif
}

Nix_t NixVec::Extract(Nixl_t n)
{ // Extract using built in "m_used"
  return(Extract(n, &m_used));
}

Nix_t NixVec::Extract(Nixl_t n, Nixl_t* pUsed)
{ // Get the next "n" bits from the vec
  Nixl_t used = *pUsed;
  
  Nixl_t word = used / NIX_BPW;
  Nixl_t bit  = used - (word * NIX_BPW);
  long  m     = 0x80000000; // n bit mask
  Nix_t  w;

  if(0)printf("Extracting %ld bits, used %ld lth %ld alth %ld\n", 
         n, used, Lth(), m_alth);
   if ((used + n) > m_alth) return(NIX_NONE); // Overflow
   if ((bit + n) <= NIX_BPW)
     { // Simple case
       if (Lth() == NIX_BPW)
         w = (long)m_pNV;
       else
         w = m_pNV[word];
       m >>= (n-1); // n bit mask
       Nix_t u = (Nix_t)m;
       u >>= bit;   // position mask
       w &= u;      // extract the bits
       w >>= (NIX_BPW - bit - n);
     }
   else
     { // spans a word
       if (Lth() == NIX_BPW)
         w = (long)m_pNV;
       else
         w = m_pNV[word];
       Nixl_t t = NIX_BPW - bit; // Number bits in first word
       m >>= (t-1); // n bit mask
       Nix_t u = (Nix_t)m;
       u >>= bit;   // position mask
       w &= u;
       t = n - t;   // number bits in second word
       w <<= t;
       m = 0x80000000; // n bit mask
       m >>= (t-1);
       w |= (m_pNV[word+1] >> (NIX_BPW - t));
     }
   used += n;
   *pUsed = used; // Return advanced
   return(w);
}

void NixVec::DBDump()
{
  printf("Lth %3ld ActualLth %3ld Used %3ld ", Lth(), ALth(), m_used);
  if (Lth() == NIX_BPW)
    { // print the lone value
      printf("val[0] %08lx\n", (unsigned long)m_pNV);
    }
  else
    {
      printf("val[0] %08lx\n", m_pNV[0]);
      for (unsigned long i = 1; i < Lth() / NIX_BPW; i++)
        printf("                 val[%ld] %08lx\n", i, m_pNV[i]);
    }
}

void NixVec::Reset()
{
  m_used = 0; // Reset to beginning
}

// Static functions
Nixl_t NixVec::GetBitl( Nixl_t l)
{ // Find out how many bits needed
  int h = 31; // highest bit number set
  // Find a good starting point
  if ((l & 0xFFFF0000) == 0)
    {
      if ((l & 0xFFFFFF00) == 0)
        {
          h = 7;
        }
      else
        {
          h = 15;
        }
    }
  while(h > 0)
    {
      Nixl_t m = 0x1L << h;
      if (m & l)
        { // Found highest bit
          return(h+1);
        }
      h--;
    }
  return(1);
}

Nixl_t NixVec::Lth()
{ // Compute the length in bits of maximum allocated size
  if (m_alth == 0) return (NIX_BPW); // Empty vector, can use up to 32
  return((((m_alth - 1) / NIX_BPW) + 1) * NIX_BPW);
}

#endif

#ifdef TEST_NIXVEC

int main()
{
  NixVec n;
  NixVec n1;
  NixVec n2;
  int    i;

  // test the nixvector generator
  for (i = 0; i < NIX_BPW-1; i++)
    {
      n.Add(NixPair_t(1, 1));
    } 
  n.Add(NixPair_t(2, 2));
  n.Add(NixPair_t(1, 1));
  n.DBDump();
  //  exit(1);
  n.Reset();
  for (i = 0; i < NIX_BPW; i++)
    {
      n.Add(NixPair_t(1, 1));
      n.Add(NixPair_t(0, 1));
      n.DBDump();
    } 
  n.Reset();
  for (i = 0; i < NIX_BPW*2; i++)
    {
      Nix_t v = n.Extract(1);
      printf("V0 = %lx\n", v);
    }
  for (i = 0; i < 16; i++)
    {
      n1.Add(NixPair_t(i, 4));
      n1.DBDump();
    }
  n1.Reset();
  for (i = 0; i < 16; i++)
    {
      Nix_t v = n1.Extract(4);
      printf("V1 = %lx\n", v);
    }
  for (i = 0; i < 8; i++)
    {
      n2.Add(NixPair_t(0xfe0 + i, 12));
      n2.DBDump();
    }
  n2.Reset();
  while(1)
    {
      Nix_t v = n2.Extract(12);
      if (v == NIX_NONE) break;
      printf("V2 = %lx\n", v);
    }
}

#endif

#endif /* STL */
