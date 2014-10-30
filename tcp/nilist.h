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

#ifndef NILIST
#define NILIST

class slink {
public:
    slink *next_;
    int key_;
    slink(int key=0) {next_=0; key_ = key;}
    slink(slink *p, int key=0) {next_ = p; key_ = key;}
};

class slist_base {
    slink *last_;
    int count_;
	int append_count_;
	int remove_count_;
public:
    slist_base() {last_ = 0;count_=0;append_count_=0;remove_count_=0;}
    slist_base(slink *a) {last_ = a->next_ = a;}

    void insert(slink *a);	// add at head
    void append(slink *a);	// add at tail
    void remove(slink *a, slink *prev);
    
    slink *get();		// remove from head
    slink *find(int key);	// return to matching key

    void clear() {last_ = 0;}
    int count() {return count_;}	
    slink *last() {return last_;}    /* XXX */
    int ac() {return append_count_;} /* XXX */
    int rc() {return remove_count_;} /* XXX */
    friend class slist_base_iter;
};

template<class T> class Islist_iter;

template<class T>
class Islist : private slist_base {
	friend class Islist_iter<T>;
  public:
	void insert(T* a) {slist_base::insert(a);}
	void append(T* a) {slist_base::append(a);}
	void remove(T* a, T* prev) {slist_base::remove(a,prev);}
	T *get() {return (T* ) slist_base::get();}
	T *find(int key) {return (T*) slist_base::find(key);}
	int count() {return slist_base::count();}
};

template<class T>
struct Tlink : public slink {
    T info;
    Tlink(const T& a) : info(a) { }
};

template<class T> class Slist_iter;

template<class T>
class Slist: private slist_base {
    friend class Slist_iter<T>;
public:
    void insert(const T& a)
         { slist_base::insert(new Tlink<T>(a));}
    void append(const T& a)
         { slist_base::append(new Tlink<T>(a));}
    T get();
};

class slist_base_iter 
{
    slink *ce;			
    slist_base *cs;
public:
    slist_base_iter(slist_base &s);
    void set_cur(slink *cur) {ce = cur;}
    slink *get_cur() {return ce;}
    slink *get_last() {return cs->last_;}
    slink *operator() ();
    int count() {return cs->count();}
};

template <class T>
class Islist_iter : public slist_base_iter {
public:
    Islist_iter(Islist<T> &s) : slist_base_iter(s) { }
    T *operator() ()
       { return (T *) slist_base_iter::operator() (); }
    T *get_last() {return (T *) slist_base_iter::get_last(); }
    T *get_cur() {return (T *) slist_base_iter::get_cur(); }
};

template <class T>
class Slist_iter : public slist_base_iter {
public:
    Slist_iter(Slist<T> &s) : slist_base_iter(s) { }
    inline T *operator() ();
};



#endif
