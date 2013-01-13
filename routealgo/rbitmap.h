
/*
 * rbitmap.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: rbitmap.h,v 1.4 2005/08/25 18:58:11 johnh Exp $
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

// Creates a bitmap object.  The 'entries' can be more than one bit,
// but default to one bit.  Bits per entry can't be more than 32.
// Entries DO NOT span words, for ease of coding

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <iostream>
#include <sys/types.h>

const unsigned  int BITS_ULONG = 32;

class BitMap {
  public :
    BitMap      ( );
    BitMap      ( u_long Size, u_long BitsPerEntry = 1);
    ~BitMap     () {if (m_Words > 1) delete [] m_pM; }
    void   Set  ( u_long Which, u_long Value = 1);
    void   Clear( u_long Which);
    u_long Get  ( u_long Which);  // Return the specified entry
    size_t Size ( void );         // Return storage used
    void   Log  ( ostream& os);   // Log to ostream
    void DBPrint();
    static u_long FindBPE( u_long );
    static size_t EstimateSize( u_long Size, u_long BitsPerEntry);
    //    friend ostream& operator<<(ostream&, const BitMap* ); // Output a bitmap
  private :
    u_long* GetWordAddress(u_long Which); // Get a pointer to the word needed
    u_long  GetBitMask(u_long Which);     // Get a bit mask to the needed bits
    short   GetShiftCount( u_long Which); // Get a shift count for position
    void    Validate(u_long Which);       // Validate which not too large
    u_long  m_Size; // how many entries
    u_long  m_BPE;  // Bits per entry
    u_long  m_Words;// Number of words needed for bitmap
    short   m_EPW;  // Entries per word
    u_long* m_pM; // Pointer to the actual map (or the data if < 32 bits)
};

#endif
