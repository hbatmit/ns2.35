/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1993 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
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
 * Author:
 *   Mohit Talwar (mohit@catarina.usc.edu)
 *
 * $Header: /cvsroot/nsnam/ns-2/rap/raplist.h,v 1.4 2005/09/18 23:33:34 tomh Exp $
 * 
 * This is taken from UCB Nachos project
 * 
 * list.h 
 *      Data structures to manage LISP-like lists.  
 *
 *      As in LISP, a list can contain any type of data structure
 *      as an item on the list: IP addresses, pending interrupts etc.  
 *      That is why each item is a "void *", or in other words, 
 *      a "pointers to anything".
 */

#ifndef RAPLIST_H
#define RAPLIST_H

typedef void (*VoidFunctionPtr)(long arg); 
typedef int (*CompareFunction)(void *item, void *key);
 
// The following class defines a "List element" -- which is
// used to keep track of one item on a List.  It is equivalent to a
// LISP cell, with a "car" ("next") pointing to the next element on the List,
// and a "cdr" ("item") pointing to the item on the List.

class ListElement 
{
public:
  ListElement(void *itemPtr, float sortKey); // initialize a List element
  
  ListElement *next;		// next element on List, 
                                // NULL if this is the last
  float key;		    	// priority (+ve), for a sorted List
				// identification, for some
  void *item;			// pointer to item on the List
};

// The following class defines a "List" -- a singly linked List of
// List elements, each of which points to a single item on the List.
//
// By using the "Sorted" functions, the List can be kept in sorted
// in increasing order by "key" in ListElement.

class List 
{
public:
  List();			// Initialize the List
  ~List();			// Deallocate the List
  
  void Prepend(void *item); 	// Put item at the beginning of the List
  void Append(void *item); 	// Put item at the end of the List
  void *Remove(); 	 	// Take item off the front of the List
  
  void Mapcar(VoidFunctionPtr func); // Apply "func" to every element 
                                     // on the List
  int IsEmpty();		// Is the List empty? 
  int Size() {return size;}	// Return the number of elements

  // Routines to put/get items on/off List in order (sorted by key)
  void SortedInsert(void *item, float sortKey);	// Put item into List
  void *SortedRemove(float *keyPtr); 	  	// Remove first item from List
  float MinKey();		                // sortKey of item at front

  // Routines to put/get items on/off a Set
  int SetInsert(void *key, CompareFunction eq);   // Put key into set
  void *SetRemove(void *key, CompareFunction eq);  // Remove key from set
  void *IsPresent(void *key, CompareFunction eq);  // Is key there in set
  void Purge(void *key, CompareFunction eq, VoidFunctionPtr destroy);
  
private:
  ListElement *first;  	        // Head of the List, NULL if List is empty
  ListElement *last;		// Last element of List
  int size;			// Number of elements in the List 
};

#endif // RAPLIST_H
