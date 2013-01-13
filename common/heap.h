/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: heap.h,v 1.9 2005/08/25 18:58:02 johnh Exp $
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
 * @(#) $Header: /cvsroot/nsnam/ns-2/common/heap.h,v 1.9 2005/08/25 18:58:02 johnh Exp $ (USC/ISI)
 */

#ifndef ns_heap_h
#define	ns_heap_h

#define	HEAP_DEFAULT_SIZE	32

// This code has a long and rich history.  It was stolen from the MaRS-2.0
// distribution, that had in its turn acquired it from the NetSim
// distribution (or so I have been told).  It has been customised for
// event queue handling.  I, Kannan, added prolific comments to it in order
// to better understand the data structure for myself, and turned into
// a standalone C++ class that you see in this version now.

typedef double heap_key_t;
typedef unsigned long heap_secondary_key_t;

// This code has (atleast?) one flaw:  It does not check for memory allocated,
// especially in the constructor, and in heap_insert() when trying
// to resize the h_elems[] array.

class Heap 
{
	struct Heap_elem {
		heap_key_t he_key;
		heap_secondary_key_t he_s_key;
		void*		he_elem;
	} *h_elems;
	unsigned int    h_s_key;
	unsigned int	h_size;
	unsigned int	h_maxsize;
	unsigned int	h_iter;

	unsigned int	parent(unsigned int i)	{ return ((i - 1) / 2); }
	unsigned int	left(unsigned int i)	{ return ((i * 2) + 1); }
	unsigned int	right(unsigned int i)	{ return ((i + 1) * 2); }
	void swap(unsigned int i, unsigned int j) {
		// swap elems[i] with elems[j] in this
		Heap_elem __he = h_elems[i];
		h_elems[i] = h_elems[j];
		h_elems[j] = __he;
		return;
	};
	unsigned int	KEY_LESS_THAN(heap_key_t k1, heap_secondary_key_t ks1,
				      heap_key_t k2, heap_secondary_key_t ks2) {
		return (k1 < k2) || ((k1==k2)&&(ks1<ks2));
	};
	unsigned int	KEY_LESS_OR_EQUAL_THAN(heap_key_t k1, heap_key_t k2) {
		return (k1 <= k2);
	};

public:
	Heap(int size=HEAP_DEFAULT_SIZE);
	~Heap();

	/*
	 * int	heap_member(Heap *h, void *elem):		O(n) algorithm.
	 */
	int	heap_member(void* elem);

	/*
	 * int	heap_delete(Heap *h, void *elem):		O(n) algorithm
	 *
	 *	Returns 1 for success, 0 otherwise.
	 */
	int heap_delete(void* elem);

	/*
	 * Couple of functions to support iterating through all things on the
	 * heap without having to know what a heap looks like.  To be used as
	 * follows:
	 *
	 *	for (elem = heap_iter_init(h); elem; elem = heap_iter(h))
	 *		Do whatever to the elem.
	 *
	 * You cannot use heap_iter to do anything but inspect the heap.  If
	 * heap_insert(), heap_extract_min(), or heap_delete() are called
	 * while iterating through the heap, all bets are off.
	 *
	 * Tue Nov 14 20:17:59 PST 1995 -- kva
	 *	Actually now, heap_create() initialises h_iter correctly,
	 *	and heap_iter() correctly resets h_iter
	 *	when the heap is traversed, So, we do not really need
	 *	heap_iter_init() anymore, except to break off a current
	 *	iteration and restart.
	 */
	void*	heap_iter_init() {
		h_iter = 1;
		return heap_min();
	};
	void*	heap_iter() {
		if (h_iter >= h_size) {
			h_iter = 0;
			return 0;
		} else {
			return h_elems[h_iter++].he_elem;
		}
	};

	/*
	 * void	heap_insert(Heap *h, heap_key_t *key, void *elem)
	 *
	 * Insert <key, elem> into heap h.
	 * Adjust heap_size if we hit the limit.
	 */
	void heap_insert(heap_key_t key, void* elem);

	/*
	 * void *heap_min(Heap *h)
	 *
	 *	Returns the smallest element in the heap, if it exists,
	 *	NULL otherwise.   The element is still in the heap.
	 */
	void* heap_min() {
		return (h_size > 0 ? h_elems[0].he_elem : 0);
	};
			
	/*
	 * void *heap_extract_min(Heap *h)
	 *
	 *	Returns the smallest element in the heap, if it exists.
	 *	NULL otherwise.
	 */
	void* heap_extract_min();
};

#endif /* ns_heap_h */






