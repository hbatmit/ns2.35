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

// File:  p802_15_4hlist.cc
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4hlist.cc,v 1.3 2011/06/22 04:03:26 tom_henderson Exp $

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


#include <scheduler.h>
#include "p802_15_4hlist.h"


int addHListLink(HLISTLINK **hlistLink1, HLISTLINK **hlistLink2, UINT_16 hostid, UINT_8 sn)
{
	HLISTLINK *tmp;
	if(*hlistLink2 == NULL)		//not exist yet
	{
		*hlistLink2 = new HLISTLINK(hostid, sn);
		if (*hlistLink2 == NULL) return 1;
		*hlistLink1 = *hlistLink2;
	}
	else
	{
		tmp=new HLISTLINK(hostid, sn);
		if (tmp == NULL) return 1;
		tmp->last = *hlistLink2;
		(*hlistLink2)->next = tmp;
		*hlistLink2 = tmp;
	}
	return 0;
}

int updateHListLink(int oper, HLISTLINK **hlistLink1, HLISTLINK **hlistLink2, UINT_16 hostid, UINT_8 sn)
{
	HLISTLINK *tmp;
	int ok;

	ok = 1;

	tmp = *hlistLink1;
	while(tmp != NULL)
	{
		if(tmp->hostID == hostid)
		{
			if (oper == hl_oper_del)	//delete an element
			{
				if(tmp->last != NULL)
				{
					tmp->last->next = tmp->next;
					if(tmp->next != NULL)
						tmp->next->last = tmp->last;
					else
						*hlistLink2 = tmp->last;
				}
				else if (tmp->next != NULL)
				{
					*hlistLink1 = tmp->next;
					tmp->next->last = NULL;
				}
				else
				{
					*hlistLink1 = NULL;
					*hlistLink2 = NULL;
				}
				delete tmp;
			}
			if (oper == hl_oper_rpl)	//replace
			{
				if (tmp->SN != sn)
					tmp->SN = sn;
				else
				{
					ok = 2;
					break;
				}
			}
			ok = 0;
			break;
		}
		tmp = tmp->next;
	}
	return ok;
}

int chkAddUpdHListLink(HLISTLINK **hlistLink1, HLISTLINK **hlistLink2, UINT_16 hostid, UINT_8 sn)
{
	int i;

	i = updateHListLink(hl_oper_rpl, hlistLink1, hlistLink2, hostid, sn);
	if (i == 0) return 1;
	else if (i == 2) return 2;

	i = addHListLink(hlistLink1, hlistLink2, hostid, sn);
	if (i == 0) return 0;
	else return 3;
}

void emptyHListLink(HLISTLINK **hlistLink1, HLISTLINK **hlistLink2)
{
	HLISTLINK *tmp, *tmp2;

	if(*hlistLink1 != NULL)
	{
		tmp = *hlistLink1;
		while(tmp != NULL)
		{
			tmp2 = tmp;
			tmp = tmp->next;
			delete tmp2;
		}
		*hlistLink1 = NULL;
	}
	*hlistLink2 = *hlistLink1;
}

void dumpHListLink(HLISTLINK *hlistLink1, UINT_16 hostid)
{
	HLISTLINK *tmp;
	int i;

	fprintf(stdout, "[%.2f] --- dump host list (by host %d) ---\n", Scheduler::instance().clock(), hostid);
	tmp = hlistLink1;
	i = 1;
	while(tmp != NULL)
	{
		fprintf(stdout, "\t%d:\tfrom host %d:\tSN = %d\n", i, tmp->hostID, tmp->SN);
		tmp = tmp->next;
		i++;
	}
}

// End of file: p802_15_4hlist.cc
