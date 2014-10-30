/********************************************/
/*     NS2 Simulator for IEEE 802.15.4      */
/*           (per P802.15.4/D18)            */
/*------------------------------------------*/
/* by:        Jianliang Zheng               */
/*        (zheng@ee.ccny.cuny.edu)          */
/*              Myung J. Lee                */
/*          (lee@ccny.cuny.edu)             */
/*        ~~~~~~~~~~~~~~~~~~~~~~~~~         */
/*           SAIT-CUNY Joint Lab            */
/********************************************/

// File:  p802_15_4fail.cc
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4fail.cc,v 1.2 2011/06/22 04:03:26 tom_henderson Exp $

/*
 * Copyright (c) 2003-2004 Samsung Advanced Institute of Technology and
 * The City University of New York. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Samsung Advanced Institute of Technology nor of 
 *    The City University of New York may be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE JOINT LAB OF SAMSUNG ADVANCED INSTITUTE
 * OF TECHNOLOGY AND THE CITY UNIVERSITY OF NEW YORK ``AS IS'' AND ANY EXPRESS 
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN 
 * NO EVENT SHALL SAMSUNG ADVANCED INSTITUTE OR THE CITY UNIVERSITY OF NEW YORK 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "p802_15_4fail.h"

LFAILLINK *lfailLink1 = NULL;
LFAILLINK *lfailLink2 = NULL;
NFAILLINK *nfailLink1 = NULL;
NFAILLINK *nfailLink2 = NULL;

int addLFailLink(int s,int d)
{
	LFAILLINK *tmp;
	if(lfailLink2 == NULL)		//not exist yet
	{
		lfailLink2 = new LFAILLINK(s,d);
		if (lfailLink2 == NULL) return 1;
		lfailLink1 = lfailLink2;
	}
	else
	{
		tmp=new LFAILLINK(s,d);
		if (tmp == NULL) return 1;
		tmp->last = lfailLink2;
		(lfailLink2)->next = tmp;
		lfailLink2 = tmp;
	}
	return 0;
}

int updateLFailLink(int oper,int s,int d)
{
	LFAILLINK *tmp;
	int rt;

	rt = 1;

	tmp = lfailLink1;
	while(tmp != NULL)
	{
		if ((tmp->src == s)&&(tmp->dst == d))
		{
			if (oper == fl_oper_del)	//delete an element
			{
				if(tmp->last != NULL)
				{
					tmp->last->next = tmp->next;
					if(tmp->next != NULL)
						tmp->next->last = tmp->last;
					else
						lfailLink2 = tmp->last;
				}
				else if (tmp->next != NULL)
				{
					lfailLink1 = tmp->next;
					tmp->next->last = NULL;
				}
				else
				{
					lfailLink1 = NULL;
					lfailLink2 = NULL;
				}
				delete tmp;
			}
			rt = 0;
			break;
		}
		tmp = tmp->next;
	}
	return rt;
}

int chkAddLFailLink(int s,int d)
{
        int i;

        i = updateLFailLink(fl_oper_est,s,d);
        if (i == 0) return 1;
        i = addLFailLink(s,d);
        if (i == 0) return 0;
        else return 2;
}

int addNFailLink(int a)
{
	NFAILLINK *tmp;
	if(nfailLink2 == NULL)		//not exist yet
	{
		nfailLink2 = new NFAILLINK(a);
		if (nfailLink2 == NULL) return 1;
		nfailLink1 = nfailLink2;
	}
	else
	{
		tmp=new NFAILLINK(a);
		if (tmp == NULL) return 1;
		tmp->last = nfailLink2;
		(nfailLink2)->next = tmp;
		nfailLink2 = tmp;
	}
	return 0;
}

int updateNFailLink(int oper,int a)
{
	NFAILLINK *tmp;
	int rt;

	rt = 1;

	tmp = nfailLink1;
	while(tmp != NULL)
	{
		if (tmp->addr == a)
		{
			if (oper == fl_oper_del)	//delete an element
			{
				if(tmp->last != NULL)
				{
					tmp->last->next = tmp->next;
					if(tmp->next != NULL)
						tmp->next->last = tmp->last;
					else
						nfailLink2 = tmp->last;
				}
				else if (tmp->next != NULL)
				{
					nfailLink1 = tmp->next;
					tmp->next->last = NULL;
				}
				else
				{
					nfailLink1 = NULL;
					nfailLink2 = NULL;
				}
				delete tmp;
			}
			rt = 0;
			break;
		}
		tmp = tmp->next;
	}
	return rt;
}

int chkAddNFailLink(int a)
{
        int i;

        i = updateNFailLink(fl_oper_est,a);
        if (i == 0) return 1;
        i = addNFailLink(a);
        if (i == 0) return 0;
        else return 2;
}

// End of file: p802_15_4fail.cc
