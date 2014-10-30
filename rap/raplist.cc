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
 * $Header: /cvsroot/nsnam/ns-2/rap/raplist.cc,v 1.4 2005/09/18 23:33:34 tomh Exp $
 * 
 * This is taken from UCB Nachos project
 * 
 * list.cc 
 *
 *      Routines to manage a singly-linked list of "things".
 *
 *      A "ListElement" is allocated for each item to be put on the
 *      list; it is de-allocated when the item is removed. This means
 *      we don't need to keep a "next" pointer in every object we
 *      want to put on a list.
 */

#include "utilities.h"
#include "raplist.h"

//----------------------------------------------------------------------
// ListElement::ListElement
// 	Initialize a List element, so it can be added somewhere on a List.
//
//	"itemPtr" is the item to be put on the List.  It can be a pointer
//		to anything.
//	"sortKey" is the priority of the item, if any.
//----------------------------------------------------------------------

ListElement::ListElement(void *itemPtr, float sortKey)
{
  item = itemPtr;
  key = sortKey;
  next = NULL;	// assume we'll put it at the end of the List 
}

//----------------------------------------------------------------------
// List::List
//	Initialize a List, empty to start with.
//	Elements can now be added to the List.
//----------------------------------------------------------------------

List::List()
{
  size  = 0;
  first = last = NULL; 
}

//----------------------------------------------------------------------
// List::~List
//	Prepare a List for deallocation.  If the List still contains any 
//	ListElements, de-allocate them.  However, note that we do *not*
//	de-allocate the "items" on the List -- this module allocates
//	and de-allocates the ListElements to keep track of each item,
//	but a given item may be on multiple Lists, so we can't
//	de-allocate them here.
//----------------------------------------------------------------------

List::~List()
{ 
  while (Remove() != NULL)
    ;	 // delete all the List elements
}

//----------------------------------------------------------------------
// List::Append
//      Append an "item" to the end of the List.
//      
//	Allocate a ListElement to keep track of the item.
//      If the List is empty, then this will be the only element.
//	Otherwise, put it at the end.
//
//	"item" is the thing to put on the List, it can be a pointer to 
//		anything.
//----------------------------------------------------------------------

void List::Append(void *item)
{
  ListElement *element = new ListElement(item, 0);
  if (IsEmpty()) 
    {				// List is empty
      first = element;
      last = element;
    } 
  else 
    {				// else put it after last
      last->next = element;
      last = element;
    }
  
  size++;
}

//----------------------------------------------------------------------
// List::Prepend
//      Put an "item" on the front of the List.
//      
//	Allocate a ListElement to keep track of the item.
//      If the List is empty, then this will be the only element.
//	Otherwise, put it at the beginning.
//
//	"item" is the thing to put on the List, it can be a pointer to 
//		anything.
//----------------------------------------------------------------------

void List::Prepend(void *item)
{
  ListElement *element = new ListElement(item, 0);
  
  if (IsEmpty()) 
    {				// List is empty
      first = element;
      last = element;
    } 
  else 
    {				// else put it before first
      element->next = first;
      first = element;
    }

  size++;
}

//----------------------------------------------------------------------
// List::Remove
//      Remove the first "item" from the front of the List.
// 
// Returns:
//	Pointer to removed item, NULL if nothing on the List.
//----------------------------------------------------------------------

void *List::Remove()
{
  return SortedRemove(NULL);  // Same as SortedRemove, but ignore the key
}

//----------------------------------------------------------------------
// List::Mapcar
//	Apply a function to each item on the List, by walking through  
//	the List, one element at a time.
//
//	Unlike LISP, this mapcar does not return anything!
//
//	"func" is the procedure to apply to each element of the List.
//----------------------------------------------------------------------

void List::Mapcar(VoidFunctionPtr func)
{
  for (ListElement *ptr = first; ptr != NULL; ptr = ptr->next)
    (*func)((long)ptr->item);
}

//----------------------------------------------------------------------
// List::IsEmpty
//      Returns TRUE if the List is empty (has no items).
//----------------------------------------------------------------------

int List::IsEmpty() 
{
  if (first == NULL)
    return TRUE;
  else
    return FALSE;
}

//----------------------------------------------------------------------
// List::SortedInsert
//      Insert an "item" into a List, so that the List elements are
//	sorted in increasing order by "sortKey".
//      
//	Allocate a ListElement to keep track of the item.
//      If the List is empty, then this will be the only element.
//	Otherwise, walk through the List, one element at a time,
//	to find where the new item should be placed.
//
//	"item" is the thing to put on the List, it can be a pointer to 
//		anything.
//	"sortKey" is the priority of the item.
//----------------------------------------------------------------------

void List::SortedInsert(void *item, float sortKey)
{
  ListElement *element = new ListElement(item, sortKey);
  ListElement *ptr;	        // keep track
  
  if (IsEmpty()) 
    {				// List is empty, put in front
      first = element;
      last = element;
    } 
  else if (sortKey < first->key) 
    {				// item goes on front of List	
      element->next = first;
      first = element;
    } 
  else 
    {				// look for first elt in List bigger than item
      for (ptr = first; ptr->next != NULL; ptr = ptr->next) {
	if (sortKey < ptr->next->key) 
	  {
	    element->next = ptr->next;
	    ptr->next = element;
	    return;
	  }	
      }
      last->next = element;	// item goes at end of List
      last = element;
    }
  
  size++;
}

//----------------------------------------------------------------------
// List::SortedRemove
//      Remove the first "item" from the front of a sorted List.
// 
// Returns:
//	Pointer to removed item, NULL if nothing on the List.
//	Sets *keyPtr to the priority value of the removed item
//	(this is needed by interrupt.cc, for instance).
//
//	"keyPtr" is a pointer to the location in which to store the 
//		priority of the removed item.
//----------------------------------------------------------------------

void *List::SortedRemove(float *keyPtr)
{
  ListElement *element = first;
  void *thing;
  
  if (IsEmpty()) 
    return NULL;
  
  thing = first->item;
  if (first == last) 
    {				// List had one item, now has none 
      first = NULL;
      last = NULL;
    } 
  else 
    first = element->next;

  if (keyPtr != NULL)
    *keyPtr = element->key;
  delete element;
  
  size--;
  return thing;
}

//----------------------------------------------------------------------
// List::MinKey
//      Return the key of the item in the front of a sorted List.
// 
// Returns :
//	-1 if List is empty
//----------------------------------------------------------------------

float List::MinKey()
{
  if (IsEmpty())
    return -1;
  else
    return first->key;
}

//----------------------------------------------------------------------
// List::SetInsert
//      Insert "key" into a List, so that the List elements are unique.
//
// Returns :
//      TRUE if "key" inserted, FALSE o/w
//
// Caveat :
//      Does not allocate space for key! Provided by calling function.
//      If FALSE, calling function should delete allocated space.
//
//	"key" is the thing to put on the List.
//      "eq" is the comparison function used to compare two items.
//----------------------------------------------------------------------

int List::SetInsert(void *key, CompareFunction eq)
{
  if (IsPresent(key, eq))
    return FALSE;

  Prepend(key);
  return TRUE;
}

//----------------------------------------------------------------------
// List::SetRemove
//      Remove "key" from List.
// 
// Returns :
//      handle to "key" in list if removed, NULL o/w
//
// Caveat :
//      if not NULL, Calling function should delete allocated space.
//
//	"key" is the item to be removed.
//      "eq" is the comparison function used to compare two items.
//----------------------------------------------------------------------

void *List::SetRemove(void *key, CompareFunction eq)
{
  ListElement *prev, *curr;
  void *thing;

  if (!IsPresent(key, eq))
    return NULL;
 
  for (prev = NULL, curr = first; 
       curr != NULL; 
       prev = curr, curr = curr->next)
    if ((*eq)(key, curr->item))
      break;

  assert(curr != NULL);		// Since its present we'd better find it

  if (curr == first)
    first = curr->next;
  else
    prev->next = curr->next;
  
  if (curr == last)
    last = prev;

  thing = curr->item;
  delete curr;

  size--;
  return thing;
}

//----------------------------------------------------------------------
// List::IsPresent
//      Is "key" there in the Set
//
// Returns :
//      handle to the "key" in list (NULL if not found)
//      
//	"key" is the item to be searched.
//      "eq" is the comparison function used to compare two items.
//----------------------------------------------------------------------

void *List::IsPresent(void *key, CompareFunction eq)
{
  ListElement *ptr;

  for (ptr = first; ptr != NULL; ptr = ptr->next) // Check all items
    if ((*eq)(key, ptr->item))
      return ptr->item;

  return NULL;
}

//----------------------------------------------------------------------
// List::Purge
//      Remove all elements with (key eq "key") from the Set
//
//      "key" is the item to be searched and deleted.
//      "eq" is the comparison function used to compare two items.
//      "destroy" is the function used to delete the purged "key".
//----------------------------------------------------------------------

void List::Purge(void *key, CompareFunction eq, VoidFunctionPtr destroy)
{
  ListElement *prev, *curr, *temp;
  void *thing;

  for (prev = NULL, curr = first; curr != NULL; ) // Check all items
    if ((*eq)(key, curr->item))
      {
        if (curr == first)
          first = curr->next;
        else
          prev->next = curr->next;

	if (curr == last)
          last = prev;

        thing = curr->item;
	temp = curr;

	curr = curr->next;

        (* destroy)((long) thing);
        delete temp;
        size--;
      }
  else
    {
      prev = curr;
      curr = curr->next;
    }
}
