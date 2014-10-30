/*
 * Copyright (c) 2008, Marcello Caleffi, <marcello.caleffi@unina.it>,
 * http://wpage.unina.it/marcello.caleffi
 *
 * The AOMDV code has been developed at DIET, Department of Electronic
 * and Telecommunication Engineering, University of Naples "Federico II"
 *
 *
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
Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
Reserved. 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The AODV code developed by the CMU/MONARCH group was optimized and tuned by Samir Das and Mahesh Marina, University of Cincinnati. The work was partially done in Sun Microsystems.
*/



#include <assert.h>

#include <cmu-trace.h>
#include <aomdv/aomdv_rqueue.h>

#define CURRENT_TIME    Scheduler::instance().clock()
#define QDEBUG

/*
  Packet Queue used by AOMDV.
*/

aomdv_rqueue::aomdv_rqueue() {
  head_ = tail_ = 0;
  len_ = 0;
  limit_ = AOMDV_RTQ_MAX_LEN;
  timeout_ = AOMDV_RTQ_TIMEOUT;
}

void
aomdv_rqueue::enque(Packet *p) {
struct hdr_cmn *ch = HDR_CMN(p);

 /*
  * Purge any packets that have timed out.
  */
 purge();
 
 p->next_ = 0;
 ch->ts_ = CURRENT_TIME + timeout_;

 if (len_ == limit_) {
 Packet *p0 = remove_head();	// decrements len_

   assert(p0);
   if(HDR_CMN(p0)->ts_ > CURRENT_TIME) {
     drop(p0, DROP_RTR_QFULL);
   }
   else {
     drop(p0, DROP_RTR_QTIMEOUT);
   }
 }
 
 if(head_ == 0) {
   head_ = tail_ = p;
 }
 else {
   tail_->next_ = p;
   tail_ = p;
 }
 len_++;
#ifdef QDEBUG
   verifyQueue();
#endif // QDEBUG
}
                

Packet*
aomdv_rqueue::deque() {
Packet *p;

 /*
  * Purge any packets that have timed out.
  */
 purge();

 p = remove_head();
#ifdef QDEBUG
 verifyQueue();
#endif // QDEBUG
 return p;

}


Packet*
aomdv_rqueue::deque(nsaddr_t dst) {
Packet *p, *prev;

 /*
  * Purge any packets that have timed out.
  */
 purge();

 findPacketWithDst(dst, p, prev);
 assert(p == 0 || (p == head_ && prev == 0) || (prev->next_ == p));

 if(p == 0) return 0;

 if (p == head_) {
   p = remove_head();
 }
 else if (p == tail_) {
   prev->next_ = 0;
   tail_ = prev;
   len_--;
 }
 else {
   prev->next_ = p->next_;
   len_--;
 }

#ifdef QDEBUG
 verifyQueue();
#endif // QDEBUG
 return p;

}

char 
aomdv_rqueue::find(nsaddr_t dst) {
Packet *p, *prev;  
	
 findPacketWithDst(dst, p, prev);
 if (0 == p)
   return 0;
 else
   return 1;

}

	
	

/*
  Private Routines
*/

Packet*
aomdv_rqueue::remove_head() {
Packet *p = head_;
        
 if(head_ == tail_) {
   head_ = tail_ = 0;
 }
 else {
   head_ = head_->next_;
 }

 if(p) len_--;

 return p;

}

void
aomdv_rqueue::findPacketWithDst(nsaddr_t dst, Packet*& p, Packet*& prev) {
  
  p = prev = 0;
  for(p = head_; p; p = p->next_) {
	  //		if(HDR_IP(p)->dst() == dst) {
	       if(HDR_IP(p)->daddr() == dst) {
      return;
    }
    prev = p;
  }
}


void
aomdv_rqueue::verifyQueue() {
Packet *p, *prev = 0;
int cnt = 0;

 for(p = head_; p; p = p->next_) {
   cnt++;
   prev = p;
 }
 assert(cnt == len_);
 assert(prev == tail_);

}


/*
void
aomdv_rqueue::purge() {
Packet *p;

 while((p = head_) && HDR_CMN(p)->ts_ < CURRENT_TIME) {
   // assert(p == remove_head());     
   p = remove_head();     
   drop(p, DROP_RTR_QTIMEOUT);
 }

}
*/

bool
aomdv_rqueue::findAgedPacket(Packet*& p, Packet*& prev) {
  
  p = prev = 0;
  for(p = head_; p; p = p->next_) {
    if(HDR_CMN(p)->ts_ < CURRENT_TIME) {
      return true;
    }
    prev = p;
  }
  return false;
}

void
aomdv_rqueue::purge() {
Packet *p, *prev;

 while ( findAgedPacket(p, prev) ) {
 	assert(p == 0 || (p == head_ && prev == 0) || (prev->next_ == p));

 	if(p == 0) return;

 	if (p == head_) {
   		p = remove_head();
 	}
 	else if (p == tail_) {
   		prev->next_ = 0;
   		tail_ = prev;
   		len_--;
 	}
 	else {
   		prev->next_ = p->next_;
   		len_--;
 	}
#ifdef QDEBUG
 	verifyQueue();
#endif // QDEBUG

	p = prev = 0;
 }

}

