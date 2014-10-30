
/*
 * rbitmap.cc
 * Copyright (C) 2000 by the University of Southern California
 * $Id: rbitmap.cc,v 1.7 2010/05/11 04:53:03 tom_henderson Exp $
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
 * Define a bitmap object
 * contributed to ns
 * George Riley, Georgia Tech, Winter 2000
 */

#include "config.h"
#ifdef HAVE_STL

// Creates a bitmap object.  The 'entries' can be more than one bit,
// but default to one bit.  Bits per entry can't be more than 32.
// Entries DO NOT span words, for ease of coding

#include <stdio.h>
#include <assert.h>

#include "routealgo/rbitmap.h"

#ifndef TEST_BM
BitMap::BitMap() :  m_Size(0), m_BPE(0), m_Words(0), m_EPW(0), m_pM(0)
{ // Default constructor ...used only to input from log file
}

BitMap::BitMap( u_long Size, u_long BitsPerEntry)
  : m_Size(Size), m_BPE(BitsPerEntry)
{
  m_EPW = BITS_ULONG / BitsPerEntry; // Entries per word
  m_Words = (Size + m_EPW - 1) / m_EPW;
  if(0)printf("Hello from bitmap constructor size %ld bpe %ld epw %d m_Words %ld\n",
         Size, BitsPerEntry, m_EPW, m_Words);
  if (m_Words > 1)
    {
      m_pM = new u_long[m_Words];
      for (u_long i = 0; i < m_Words; i++) m_pM[i] = 0;
    }
  else
    {
      m_pM = (u_long*)(0);
    }
  if(0)printf("Exiting bitmap constructor\n");
}

/*BitMap::~BitMap()
{
  if (m_Words > 1) delete [] m_pM;
}*/

void BitMap::Set(u_long Which, u_long Value)
{
u_long* pW;
u_long  Mask;
short   Shift;

  pW    = GetWordAddress(Which);
  Mask  = GetBitMask(Which);
  Shift = GetShiftCount(Which);
  *pW  &= (~Mask); // Clear existing bits
  *pW  |= (Value << Shift);
}

void BitMap::Clear(u_long Which)
{
u_long* pW;
u_long  Mask;

  pW    = GetWordAddress(Which);
  Mask  = GetBitMask(Which);
  *pW  &= (~Mask); // Clear existing bits
}

u_long BitMap::Get(u_long Which)
{
u_long* pW;
u_long  Mask;
short   Shift;
u_long  r;

  pW    = GetWordAddress(Which);
  Mask  = GetBitMask(Which);
  Shift = GetShiftCount(Which);
  if(0)printf("Get which %ld Mask %08lx Shift %d\n", Which, Mask, Shift);
  r = *pW;

  r  &= (Mask); // get existing bits
  return (r >> Shift);
}


// Private subroutines

u_long* BitMap::GetWordAddress(
    u_long Which) // Get a pointer to the word needed
{
u_long ind;

 Validate(Which);
 ind = Which / m_EPW; 
 if (m_Words == 1)
   { // not indirected
     return((u_long*)&m_pM);
   }
 return(&m_pM[ind]);
}

u_long BitMap::GetBitMask( // Get a bit mask to the needed bits
    u_long Which)
{
long  m;
short c;
u_long um;

  m = 0x80000000;
  m >>= (m_BPE - 1);
  c = GetShiftCount(Which);
  um = m;
  um = um >> (32 - (c + m_BPE));
  if(0)printf("Get Bit Mask m %08lx shifted m %08lx\n", m, um);
  return(um);
}


short BitMap::GetShiftCount( // Get a shift count for position
    u_long Which)  
{
u_long ind;
short  left;

 Validate(Which);
 ind = Which / m_EPW; 
 left = Which - (ind * m_EPW);  // Entry number in the actual word
 return(left * m_BPE);
}

void    BitMap::Validate(u_long Which)// Validate which not too large
{
  assert (Which < m_Size);
}


size_t BitMap::Size( void )
{
  size_t r;

  r = sizeof(u_long*) + // m_pM
      sizeof(u_long) + // m_Size
      sizeof(u_long) + // m_BPE
      sizeof(u_long) + // m_Words
      sizeof(short); // m_EPW
  if (m_Size * m_BPE > BITS_ULONG)
    r += ((m_Size * m_BPE) + BITS_ULONG - 1 / BITS_ULONG) * 
         sizeof(u_long) ; // Account for the actual map
  return(r);
}

void BitMap::DBPrint()
{
  printf("Size %ld BPE %ld Words %ld EPW %d\n", m_Size, m_BPE, m_Words, m_EPW);
  if (m_Words == 1)
    {
      printf("Word 0 %08lx\n", (u_long)m_pM);
    }
  else
    {
      for (u_long i = 0; i < m_Words; i++)
        printf("Word %ld %08lx\n", i, m_pM[i]);
    }
}

u_long BitMap::FindBPE( u_long m ) // Find how many bits/entry we need
{
  u_long bpe = 32;
  u_long k = 0x80000000;

  while(k)
    {
      if (m & k) return(bpe);
      bpe--;
      k >>= 1;
    }
  return(0);
}

size_t BitMap::EstimateSize( u_long Size, u_long BitsPerEntry)
{
  size_t r;

  r = sizeof(u_long*) + // m_pM
      sizeof(u_long) + // m_Size
      sizeof(u_long) + // m_BPE
      sizeof(u_long) + // m_Words
      sizeof(short); // m_EPW
  if (Size * BitsPerEntry > BITS_ULONG)
    r += ((Size * BitsPerEntry) + BITS_ULONG - 1 / BITS_ULONG) * 
         sizeof(u_long) ; // Account for the actual map
  return (r);
}

void BitMap::Log( ostream& os)
{
  os << " " << m_Size;
  os << " " << m_BPE;
  os << " " << m_Words;
  os << " " << m_EPW;
  if (m_Words == 1)
    { // Not a pointer, but the actual map
      os << " " << hex << (unsigned long)m_pM << dec;
    }
  else
    {
      for (unsigned int i = 0; i < m_Words; i++)
        os << " " << hex << m_pM[i] << dec;
    }
}

#endif

#ifdef TEST_BM
int main()
{
BitMap B(64);
BitMap B2(64, 2);
BitMap B3(64, 3);

  B.DBPrint();
  B.Set(0);
  B.DBPrint();
  printf("Entry 0 is %ld\n", B.Get(0));
  B.Set(31);
  B.DBPrint();
  B.Set(32);
  B.DBPrint();
  B.Set(63);
  B.DBPrint();

  B2.DBPrint();
  B2.Set(0, 1);
  B2.DBPrint();
  B2.Set(31, 2);
  B2.DBPrint();
  B2.Set(32, 2);
  B2.DBPrint();
  B2.Set(63, 3);
  B2.DBPrint();

  B3.DBPrint();
  B3.Set(0, 1);
  B3.DBPrint();
  B3.Set(31, 2);
  B3.DBPrint();
  B3.Set(32, 2);
  B3.DBPrint();
  B3.Set(63, 7);
  B3.DBPrint();

}
#endif

#endif /* STL */
