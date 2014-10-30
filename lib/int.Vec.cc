// This may look like C code, but it is really -*- C++ -*-
/* 
Copyright (C) 1988 Free Software Foundation
    written by Doug Lea (dl@rocky.oswego.edu)

This file is part of the GNU C++ Library.  This library is free
software; you can redistribute it and/or modify it under the terms of
the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.  This library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

Linking this file statically or dynamically with other modules is making
a combined work based on this file.  Thus, the terms and conditions of
the GNU General Public License cover the whole combination.

In addition, as a special exception, the copyright holders of this file
give you permission to combine this file with free software programs or
libraries that are released under the GNU LGPL and with code included in
the standard release of ns-2 under the Apache 2.0 license or under
otherwise-compatible licenses with advertising requirements (or modified
versions of such code, with unchanged license).  You may copy and
distribute such a system following the terms of the GNU GPL for this
file and the licenses of the other code concerned, provided that you
include the source code of that other code when and as the GNU GPL
requires distribution of source code.

Note that people who make modified versions of this file are not
obligated to grant this special exception for their modified versions;
it is their choice whether to do so.  The GNU General Public License
gives permission to release a modified version without this exception;
this exception also makes it possible to release a modified version
which carries forward this exception.
*/

#ifdef __GNUG__
#pragma implementation
#endif
// #include <stream.h>
#include <stdlib.h>
#include "lib/builtin.h"
#include "lib/int.Vec.h"

// error handling


void default_intVec_error_handler(const char* msg)
{
#if 0
  cerr << "Fatal intVec error. " << msg << "\n";
#else
  // ns doesn't use streams
  fprintf(stderr, "Fatal intVec error. %s\n", msg);
#endif
  exit(1);
}

one_arg_error_handler_t intVec_error_handler = default_intVec_error_handler;

one_arg_error_handler_t set_intVec_error_handler(one_arg_error_handler_t f)
{
  one_arg_error_handler_t old = intVec_error_handler;
  intVec_error_handler = f;
  return old;
}

void intVec::error(const char* msg)
{
  (*intVec_error_handler)(msg);
}

void intVec::range_error()
{
  (*intVec_error_handler)("Index out of range.");
}

intVec::intVec(const intVec& v)
{
  s = new int [len = v.len];
  int* top = &(s[len]);
  int* t = s;
  const int* u = v.s;
  while (t < top) *t++ = *u++;
}

intVec::intVec(int l, int  fill_value)
{
  s = new int [len = l];
  int* top = &(s[len]);
  int* t = s;
  while (t < top) *t++ = fill_value;
}


intVec& intVec::operator = (const intVec& v)
{
  if (this != &v)
  {
    delete [] s;
    s = new int [len = v.len];
    int* top = &(s[len]);
    int* t = s;
    const int* u = v.s;
    while (t < top) *t++ = *u++;
  }
  return *this;
}

void intVec::apply(intProcedure f)
{
  int* top = &(s[len]);
  int* t = s;
  while (t < top) (*f)(*t++);
}

// can't just realloc since there may be need for constructors/destructors
void intVec::resize(int newl)
{
  int* news = new int [newl];
  int* p = news;
  int minl = (len < newl)? len : newl;
  int* top = &(s[minl]);
  int* t = s;
  while (t < top) *p++ = *t++;
  delete [] s;
  s = news;
  len = newl;
}

intVec concat(intVec & a, intVec & b)
{
  int newl = a.len + b.len;
  int* news = new int [newl];
  int* p = news;
  int* top = &(a.s[a.len]);
  int* t = a.s;
  while (t < top) *p++ = *t++;
  top = &(b.s[b.len]);
  t = b.s;
  while (t < top) *p++ = *t++;
  return intVec(newl, news);
}


intVec combine(intCombiner f, intVec& a, intVec& b)
{
  int newl = (a.len < b.len)? a.len : b.len;
  int* news = new int [newl];
  int* p = news;
  int* top = &(a.s[newl]);
  int* t = a.s;
  int* u = b.s;
  while (t < top) *p++ = (*f)(*t++, *u++);
  return intVec(newl, news);
}

int intVec::reduce(intCombiner f, int  base)
{
  int r = base;
  int* top = &(s[len]);
  int* t = s;
  while (t < top) r = (*f)(r, *t++);
  return r;
}

intVec reverse(intVec& a)
{
  int* news = new int [a.len];
  if (a.len != 0)
  {
    int* lo = news;
    int* hi = &(news[a.len - 1]);
    while (lo < hi)
    {
      int tmp = *lo;
      *lo++ = *hi;
      *hi-- = tmp;
    }
  }
  return intVec(a.len, news);
}

void intVec::reverse()
{
  if (len != 0)
  {
    int* lo = s;
    int* hi = &(s[len - 1]);
    while (lo < hi)
    {
      int tmp = *lo;
      *lo++ = *hi;
      *hi-- = tmp;
    }
  }
}

int intVec::index(int  targ)
{
  for (int i = 0; i < len; ++i) if (intEQ(targ, s[i])) return i;
  return -1;
}

intVec map(intMapper f, intVec& a)
{
  int* news = new int [a.len];
  int* p = news;
  int* top = &(a.s[a.len]);
  int* t = a.s;
  while(t < top) *p++ = (*f)(*t++);
  return intVec(a.len, news);
}

int operator == (intVec& a, intVec& b)
{
  if (a.len != b.len)
    return 0;
  int* top = &(a.s[a.len]);
  int* t = a.s;
  int* u = b.s;
  while (t < top) if (!(intEQ(*t++, *u++))) return 0;
  return 1;
}

void intVec::fill(int  val, int from, int n)
{
  int to;
  if (n < 0)
    to = len - 1;
  else
    to = from + n - 1;
  if ((unsigned)from > (unsigned)to)
    range_error();
  int* t = &(s[from]);
  int* top = &(s[to]);
  while (t <= top) *t++ = val;
}

intVec intVec::at(int from, int n)
{
  int to;
  if (n < 0)
  {
    n = len - from;
    to = len - 1;
  }
  else
    to = from + n - 1;
  if ((unsigned)from > (unsigned)to)
    range_error();
  int* news = new int [n];
  int* p = news;
  int* t = &(s[from]);
  int* top = &(s[to]);
  while (t <= top) *p++ = *t++;
  return intVec(n, news);
}

intVec merge(intVec & a, intVec & b, intComparator f)
{
  int newl = a.len + b.len;
  int* news = new int [newl];
  int* p = news;
  int* topa = &(a.s[a.len]);
  int* as = a.s;
  int* topb = &(b.s[b.len]);
  int* bs = b.s;

  for (;;)
  {
    if (as >= topa)
    {
      while (bs < topb) *p++ = *bs++;
      break;
    }
    else if (bs >= topb)
    {
      while (as < topa) *p++ = *as++;
      break;
    }
    else if ((*f)(*as, *bs) <= 0)
      *p++ = *as++;
    else
      *p++ = *bs++;
  }
  return intVec(newl, news);
}

static int gsort(int*, int, intComparator);
 
void intVec::sort (intComparator compar)
{
  gsort(s, len, compar);
}


// An adaptation of Schmidt's new quicksort

static inline void SWAP(int* A, int* B)
{
  int tmp = *A; *A = *B; *B = tmp;
}

/* This should be replaced by a standard ANSI macro. */
#define BYTES_PER_WORD 8
#define BYTES_PER_LONG 4

/* The next 4 #defines implement a very fast in-line stack abstraction. */

#define STACK_SIZE (BYTES_PER_WORD * BYTES_PER_LONG)
#define PUSH(LOW,HIGH) do {top->lo = LOW;top++->hi = HIGH;} while (0)
#define POP(LOW,HIGH)  do {LOW = (--top)->lo;HIGH = top->hi;} while (0)
#define STACK_NOT_EMPTY (stack < top)                

/* Discontinue quicksort algorithm when partition gets below this size.
   This particular magic number was chosen to work best on a Sun 4/260. */
#define MAX_THRESH 4


/* Order size using quicksort.  This implementation incorporates
   four optimizations discussed in Sedgewick:
   
   1. Non-recursive, using an explicit stack of pointer that
      store the next array partition to sort.  To save time, this
      maximum amount of space required to store an array of
      MAX_INT is allocated on the stack.  Assuming a 32-bit integer,
      this needs only 32 * sizeof (stack_node) == 136 bits.  Pretty
      cheap, actually.

   2. Chose the pivot element using a median-of-three decision tree.
      This reduces the probability of selecting a bad pivot value and 
      eliminates certain extraneous comparisons.

   3. Only quicksorts TOTAL_ELEMS / MAX_THRESH partitions, leaving
      insertion sort to order the MAX_THRESH items within each partition.  
      This is a big win, since insertion sort is faster for small, mostly
      sorted array segements.
   
   4. The larger of the two sub-partitions is always pushed onto the
      stack first, with the algorithm then concentrating on the
      smaller partition.  This *guarantees* no more than log (n)
      stack size is needed! */
      
static int gsort (int *base_ptr, int total_elems, intComparator cmp)
{
/* Stack node declarations used to store unfulfilled partition obligations. */
  struct stack_node {  int *lo;  int *hi; };
  int   pivot_buffer;
  int   max_thresh   = MAX_THRESH;

  if (total_elems > MAX_THRESH)
    {
      int       *lo = base_ptr;
      int       *hi = lo + (total_elems - 1);
      int       *left_ptr;
      int       *right_ptr;
      stack_node stack[STACK_SIZE]; /* Largest size needed for 32-bit int!!! */
      stack_node *top = stack + 1;

      while (STACK_NOT_EMPTY)
        {
          {
            int *pivot = &pivot_buffer;
            {
              /* Select median value from among LO, MID, and HI. Rearrange
                 LO and HI so the three values are sorted. This lowers the 
                 probability of picking a pathological pivot value and 
                 skips a comparison for both the LEFT_PTR and RIGHT_PTR. */

              int *mid = lo + ((hi - lo) >> 1);

              if ((*cmp) (*mid, *lo) < 0)
                SWAP (mid, lo);
              if ((*cmp) (*hi, *mid) < 0)
              {
                SWAP (mid, hi);
                if ((*cmp) (*mid, *lo) < 0)
                  SWAP (mid, lo);
              }
              *pivot = *mid;
              pivot = &pivot_buffer;
            }
            left_ptr  = lo + 1;
            right_ptr = hi - 1; 

            /* Here's the famous ``collapse the walls'' section of quicksort.  
               Gotta like those tight inner loops!  They are the main reason 
               that this algorithm runs much faster than others. */
            do 
              {
                while ((*cmp) (*left_ptr, *pivot) < 0)
                  left_ptr += 1;

                while ((*cmp) (*pivot, *right_ptr) < 0)
                  right_ptr -= 1;

                if (left_ptr < right_ptr) 
                  {
                    SWAP (left_ptr, right_ptr);
                    left_ptr += 1;
                    right_ptr -= 1;
                  }
                else if (left_ptr == right_ptr) 
                  {
                    left_ptr += 1;
                    right_ptr -= 1;
                    break;
                  }
              } 
            while (left_ptr <= right_ptr);

          }

          /* Set up pointers for next iteration.  First determine whether
             left and right partitions are below the threshold size. If so, 
             ignore one or both.  Otherwise, push the larger partition's
             bounds on the stack and continue sorting the smaller one. */

          if ((right_ptr - lo) <= max_thresh)
            {
              if ((hi - left_ptr) <= max_thresh) /* Ignore both small partitions. */
                POP (lo, hi); 
              else              /* Ignore small left partition. */  
                lo = left_ptr;
            }
          else if ((hi - left_ptr) <= max_thresh) /* Ignore small right partition. */
            hi = right_ptr;
          else if ((right_ptr - lo) > (hi - left_ptr)) /* Push larger left partition indices. */
            {                   
              PUSH (lo, right_ptr);
              lo = left_ptr;
            }
          else                  /* Push larger right partition indices. */
            {                   
              PUSH (left_ptr, hi);
              hi = right_ptr;
            }
        }
    }

  /* Once the BASE_PTR array is partially sorted by quicksort the rest
     is completely sorted using insertion sort, since this is efficient 
     for partitions below MAX_THRESH size. BASE_PTR points to the beginning 
     of the array to sort, and END_PTR points at the very last element in
     the array (*not* one beyond it!). */


  {
    int *end_ptr = base_ptr + 1 * (total_elems - 1);
    int *run_ptr;
    int *tmp_ptr = base_ptr;
    int *thresh  = (end_ptr < (base_ptr + max_thresh))? 
      end_ptr : (base_ptr + max_thresh);

    /* Find smallest element in first threshold and place it at the
       array's beginning.  This is the smallest array element,
       and the operation speeds up insertion sort's inner loop. */

    for (run_ptr = tmp_ptr + 1; run_ptr <= thresh; run_ptr += 1)
      if ((*cmp) (*run_ptr, *tmp_ptr) < 0)
        tmp_ptr = run_ptr;

    if (tmp_ptr != base_ptr)
      SWAP (tmp_ptr, base_ptr);

    /* Insertion sort, running from left-hand-side up to `right-hand-side.' 
       Pretty much straight out of the original GNU qsort routine. */

    for (run_ptr = base_ptr + 1; (tmp_ptr = run_ptr += 1) <= end_ptr; )
      {

        while ((*cmp) (*run_ptr, *(tmp_ptr -= 1)) < 0)
          ;

        if ((tmp_ptr += 1) != run_ptr)
          {
            int *trav;

            for (trav = run_ptr + 1; --trav >= run_ptr;)
              {
                int c = *trav;
                int *hi, *lo;

                for (hi = lo = trav; (lo -= 1) >= tmp_ptr; hi = lo)
                  *hi = *lo;
                *hi = c;
              }
          }

      }
  }
  return 1;
}
