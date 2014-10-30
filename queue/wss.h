/*
 * Copyright (c) 1991-1994 Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and the Network Research Group at
 *	Lawrence Berkeley Laboratory.
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
 */


/* it is used to generate the Weight Spread Sequence of SRR 
 * WSS and SRR are named by Dr. W. Qi 
 */

/*
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
*/

class PacketSRR;
class SRR;

int power( int base, int j)
{
	int r=1; int i;
	if(j==0) return 1;
	else
	{
		for(i=0;i<j;i++)
		r*=base;
	}

	return r;
}

void rawScan( int i, int j, int N, int *p)
{
	if(j==N)
	{
		*p=N;
		return;
	}

	if(i%(int)power(2,j))
	{
		*p=j;
		return;
	}
	else
		rawScan(i, j+1, N, p);
}

class WSS{
	public:WSS(): currOrder(1), items(0), ptr(0), pwss(0){ }
	friend class SRR;
public:	
	int maxOrder; // the order of the WSS
	int currOrder; // current order of WSS
	int items; //how many items are in the WSS
	unsigned int ptr;
	int *pwss;  // 
	void init(int i);

	int get_ptr()
	{
	  return ptr;
	}

	void set_ptr (int val){
		ptr = val;
	}

	void inc_ptr ( int order)
	{
		ptr += 1;
		if((int)ptr > ((1<<order)-2))
		{
			ptr = 0;
		}
	}

	int get(int order);

	void print(); // for debug purpose

};

void WSS::init(int i){
	int j;
	maxOrder=i;
	items= (1<<maxOrder)-1;
	ptr=0;
	pwss=(int*)malloc(sizeof(int)*items);
	
	for(j=1;j<=items; j++)
		rawScan(j, 1, maxOrder,  (int*)(pwss+j-1));
}

int WSS::get(int order)  // it should also tells the WSS the order
{  
	int value;

	currOrder=order;

	//printf("get wss\n");
	int tmp = 1 << order;
	if((int)ptr > (tmp-2))
	{
		printf("tmp :%d \n", tmp);
		printf("error, too large ptr:%d, order:%d\n", (int)ptr, order);
		exit(0);
	}

	value= *(pwss+ptr);
	return value;
}

void WSS::print(){
	int i;
	for(i=0;i<items;i++)
		printf("%4d", *(pwss+i));
	printf("\n");
}

/*
main(int argc,  char **argv)
{
	WSS wss;
	if(argc!=2)
	{
		printf("usage:%s number\n", argv[0]);
		exit(0);
	}
	wss.init(atoi(argv[1]));
	wss.print();
}
*/
