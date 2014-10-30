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
 *
 * $Header: /cvsroot/nsnam/ns-2/routing/alloc-address.cc,v 1.10 2000/09/14 18:19:25 haoboy Exp $
 */

/* functions invoked to allocate bits to the ns-address space */

#include <stdlib.h>
#include <assert.h>
#include "config.h"
#include <tclcl.h>


class AllocAddr : public TclObject {
public:
	AllocAddr();
	~AllocAddr();
	int command(int argc, const char*const* argv);
protected:
	void get_mask(nsmask_t *mask, int fieldsize);
	void alloc(int n);
	bool check_size(int n);
	bool find_free(int len, int *pos);
	bool test(int which);
	void set_field(int offset, int len, nsmask_t *mask, int *shift);
	void free_field(int len);
	void mark(int i);
	int size_;
	int *bitmap_;
};


class AllocAddrClass : public TclClass {
public:
	AllocAddrClass() : TclClass("AllocAddr") {}
	TclObject* create(int, const char*const*) {
		return (new AllocAddr());
	}
} AllocAddr_class;



int AllocAddr::command(int argc, const char*const* argv)
{
	int offset,
		addrsize,
		shift,
		fieldlen;
	nsmask_t mask;

	Tcl& tcl = Tcl::instance();

	if (argc == 3) {
		if (strcmp(argv[1], "freebit") == 0) {
			fieldlen = atoi(argv[2]);
			assert(fieldlen > 0);
			free_field(fieldlen);
			return (TCL_OK);
		}
	}


	else if (argc == 4) {
		if (strcmp(argv[1], "setbit") == 0) {
			fieldlen = atoi(argv[2]);
			addrsize = atoi(argv[3]);
			if (!check_size(addrsize)) {
				tcl.result("setbit: Size_ increased: Reallocate bits");
				return (TCL_ERROR);
			}
			if (!find_free(fieldlen, &offset)) {
				tcl.result("setbit: no contiguous space found\n");
				return (TCL_ERROR);
			}
			set_field(offset, fieldlen, &mask, &shift);
			// TESTING
			tcl.resultf("%d %d", mask, shift);
			return (TCL_OK);
		}
	}
	else if (argc == 5) {
		int oldfldlen;
		if (strcmp(argv[1], "expand-port") == 0) {
			fieldlen = atoi(argv[2]);
			addrsize = atoi(argv[3]);
			oldfldlen = atoi(argv[4]);
			if (!check_size(addrsize)) {
				tcl.result("expand-port: Size_ increased: Reallocate bits");
				return (TCL_ERROR);
			}
			if (!find_free(fieldlen, &offset)) {
				tcl.result("expand-port: no contiguous space found\n");
				return (TCL_ERROR);
			}
			int i, k;
			for (i = offset, k = 0; k < fieldlen; k++, i--) {
				bitmap_[i] = 1;
			}
			shift = offset - (fieldlen - 1);
			get_mask(&mask, fieldlen + oldfldlen);
			// TESTING
			tcl.resultf("%d %d", mask, shift);
			return (TCL_OK);
		}
	}
	return TclObject::command(argc, argv);
}


AllocAddr::AllocAddr()
{
	size_ = 0;
	bitmap_ = 0;

}

AllocAddr::~AllocAddr()
{
	delete [] bitmap_;

}

void AllocAddr::alloc(int n)
{
	size_ = n;
	bitmap_ = new int[n];
	for (int i=0; i < n; i++)
		bitmap_[i] = 0;
}


bool AllocAddr::check_size(int n)
{
	if (n <= size_)
		return 1;
	assert (n > 0);
	if (size_ == 0) {
		alloc(n);
		return 1;
	}
	if (n > size_) 
		return 0;
	return 1;

	// this check is no longer needed, as now bits are re-allocated every time
	// the size changes.
	//     int *old = bitmap_;
	//     int osize = size_;
	//     alloc(n);
	//     for (int i = 0; i < osize; i++) 
	// 	bitmap_[i] = old[i];
	//     delete [] old;
}

void AllocAddr::mark(int i)
{
	bitmap_[i] = 1;
}


void AllocAddr::get_mask(nsmask_t *mask, int fieldsize)
{
	// int temp = (int)(pow(2, (double)fieldsize));
	*mask = (1 << fieldsize) - 1;
}


bool AllocAddr::test(int which)
{
	assert(which <= size_);
	if (bitmap_[which] == 1)
		return TRUE;
	else
		return FALSE;
}


bool AllocAddr::find_free(int len, int *pos) 
{
	int count = 0;
	int temp = 0;

	for (int i = (size_ - 1); i >= 0; i--)
		if (!test(i)) {
			/**** check if n contiguous bits are free ****/
			temp = i;
			for (int k = 0; (k < len) && (i >=0); k++, i--){
				if(test(i)) {
					count = 0;
					break;
				}
				count++;
			}
			if (count == len) {
				*pos = temp;
				return 1;
			}
		}
	return 0;
}


void AllocAddr::set_field(int offset, int len, nsmask_t *mask, int *shift)
{
	int i, k;

	for (k = 0, i = offset; k < len; k++, i--) {
		bitmap_[i] = 1; 
	}
	*shift = offset - (len-1);
	get_mask(mask, len);
}


void AllocAddr::free_field(int len)
{
	int count = 0;

	for (int i = 0; i < size_; i++) {
		if (test(i)) {
			bitmap_[i] = 0;
			count++;
		}
		if (count == len)
			break;
	}
}
