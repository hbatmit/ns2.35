/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 * 	This product includes software developed by the Daedalus Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 */

#include <stdlib.h>
/*#include <unistd.h>*/
#include "nilist.h"

template<class T>
T Slist<T>::get()
{
    Tlink<T>* lnk = (Tlink<T>*) slist_base::get();
    T i = lnk->info;
    delete lnk;
    return i;
}

void slist_base::insert(slink *a)
{
    count_++;
    if (last_)
	a->next_ = last_->next_;
    else
	last_ = a;
    last_->next_ = a;
}
#include <stdio.h>

void slist_base::remove(slink *a, slink *prev)
{
    remove_count_++; /* XXX */
    count_--;
    if (prev && prev->next_ != a)
	    printf("In remove(): Error: prev->next!=a  prev=%p  a=%p\n", 
		   reinterpret_cast<void *>(prev),
		   reinterpret_cast<void *>(a) );
    if (last_ == NULL) 
	return;
    if (prev == NULL)
    {
	if (last_->next_ == a)
	{
	    prev = last_;
	}
	else 
	{
	    return;
	}
    }

    prev->next_ = a->next_;
    if (last_ == a)		// a was last in list
	last_ = prev;
    if (last_->next_ == a)	// a was only one in list
	last_ = NULL;
}

void slist_base::append(slink *a)
{
    append_count_++; /* XXX */
    count_++;
    if (last_) {
	a->next_ = last_->next_;
	last_ = last_->next_ = a;
    }
    else
	last_ = a->next_ = a;
}

slink *slist_base::get()
{
    count_--;
    if (last_ == 0) return NULL;
    slink * f = last_->next_;
    if (f == last_)
	last_ = 0;
    else 
	last_->next_ = f->next_;
    return f;
}

slink *slist_base::find(int key)
{
    slink *cur = last_;
    
    if (last_ == 0) return NULL;
    do {
	cur = cur->next_;
	if (cur->key_ == key)
	    return cur;
    } while (cur != last_);
    return NULL;
}

slist_base_iter::slist_base_iter(slist_base &s)
{
    cs = &s;
    ce = cs->last_;
}

slink *
slist_base_iter::operator() ()
    // return 0 to indicate end of iteration
{
    slink *ret = ce ? (ce=ce->next_) : 0;
    if (ce == cs->last_) ce = 0;
    return ret;
}

template<class T> T* Slist_iter<T>::operator() ()
{
    Tlink<T> *lnk = (Tlink<T> *) slist_base_iter::operator() ();
    return lnk ? &lnk->info : 0;
}

