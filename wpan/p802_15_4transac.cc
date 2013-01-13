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

// File:  p802_15_4transac.cc
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4transac.cc,v 1.3 2011/06/22 04:03:26 tom_henderson Exp $

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
#include "p802_15_4transac.h"
#include "p802_15_4pkt.h"


int addDeviceLink(DEVICELINK **deviceLink1, DEVICELINK **deviceLink2, IE3ADDR addr, UINT_8 cap)
{
	DEVICELINK *tmp;
	if(*deviceLink2 == NULL)		//not exist yet
	{
		*deviceLink2 = new DEVICELINK(addr,cap);
		if (*deviceLink2 == NULL) return 1;
		*deviceLink1 = *deviceLink2;
	}
	else
	{
		tmp=new DEVICELINK(addr,cap);
		if (tmp == NULL) return 1;
		tmp->last = *deviceLink2;
		(*deviceLink2)->next = tmp;
		*deviceLink2 = tmp;
	}
	return 0;
}

int updateDeviceLink(int oper, DEVICELINK **deviceLink1, DEVICELINK **deviceLink2, IE3ADDR addr)
{
	DEVICELINK *tmp;
	int rt;

	rt = 1;

	tmp = *deviceLink1;
	while(tmp != NULL)
	{
		if(tmp->addr64 == addr)
		{
			if (oper == tr_oper_del)	//delete an element
			{
				if(tmp->last != NULL)
				{
					tmp->last->next = tmp->next;
					if(tmp->next != NULL)
						tmp->next->last = tmp->last;
					else
						*deviceLink2 = tmp->last;
				}
				else if (tmp->next != NULL)
				{
					*deviceLink1 = tmp->next;
					tmp->next->last = NULL;
				}
				else
				{
					*deviceLink1 = NULL;
					*deviceLink2 = NULL;
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

int numberDeviceLink(DEVICELINK **deviceLink1)
{
	DEVICELINK *tmp;
	int num;

	num = 0;
	tmp = *deviceLink1;
	while(tmp != NULL)
	{
		num++;
		tmp = tmp->next;
	}
	return num;
}

int chkAddDeviceLink(DEVICELINK **deviceLink1, DEVICELINK **deviceLink2, IE3ADDR addr, UINT_8 cap)
{
        int i;

        i = updateDeviceLink(tr_oper_est, deviceLink1, deviceLink2, addr);
        if (i == 0) return 1;
        i = addDeviceLink(deviceLink1, deviceLink2, addr, cap);
        if (i == 0) return 0;
        else return 2;
}

void emptyDeviceLink(DEVICELINK **deviceLink1, DEVICELINK **deviceLink2)
{
	DEVICELINK *tmp, *tmp2;

	if(*deviceLink1 != NULL)
	{
		tmp = *deviceLink1;
		while(tmp != NULL)
		{
			tmp2 = tmp;
			tmp = tmp->next;
			delete tmp2;
		}
		*deviceLink1 = NULL;
	}
	*deviceLink2 = *deviceLink1;
}

void dumpDeviceLink(DEVICELINK *deviceLink1, IE3ADDR coorAddr)
{
	DEVICELINK *tmp;
	int i;

	fprintf(stdout, "[%.2f] --- dump associated device list (by coordinator %d) ---\n", Scheduler::instance().clock(), coorAddr);
	tmp = deviceLink1;
	i = 1;
	while(tmp != NULL)
	{
		fprintf(stdout, "\t%d:\taddress = 0x%x\n", i,tmp->addr64);
		tmp = tmp->next;
		i++;
	}
}

//--------------------------------------------------------------------------------------

void purgeTransacLink(TRANSACLINK **transacLink1, TRANSACLINK **transacLink2)
{
	//purge expired transactions
	TRANSACLINK *tmp,*tmp2;

	tmp = *transacLink1;
	while(tmp != NULL)
	{
		if (CURRENT_TIME > tmp->expTime)
		{
			tmp2 = tmp;
			if (tmp->next != NULL)
				tmp = tmp->next->next;
			else
				tmp = NULL;
			//--- delete the transaction ---
			//don't try to call updateTransacLink() -- to avoid re-entries of functions
			if(tmp2->last != NULL)
			{
				tmp2->last->next = tmp2->next;
				if(tmp2->next != NULL)
					tmp2->next->last = tmp2->last;
				else
					*transacLink2 = tmp2->last;
			}
			else if (tmp2->next != NULL)
			{
				*transacLink1 = tmp2->next;
				tmp2->next->last = NULL;
			}
			else
			{
				*transacLink1 = NULL;
				*transacLink2 = NULL;
			}
			//free the packet first
			Packet::free(tmp2->pkt);
			delete tmp2;
			//--------------------------------
		}
		else
			tmp = tmp->next;
	}
}

int addTransacLink(TRANSACLINK **transacLink1, TRANSACLINK **transacLink2, UINT_8 pendAM, IE3ADDR pendAddr, Packet *p, UINT_8 msduH, double kpTime)
{
	TRANSACLINK *tmp;
	if(*transacLink2 == NULL)		//not exist yet
	{
		*transacLink2 = new TRANSACLINK(pendAM,pendAddr,p,msduH,kpTime);
		if (*transacLink2 == NULL) return 1;
		*transacLink1 = *transacLink2;
	}
	else
	{
		tmp=new TRANSACLINK(pendAM,pendAddr,p,msduH,kpTime);
		if (tmp == NULL) return 1;
		tmp->last = *transacLink2;
		(*transacLink2)->next = tmp;
		*transacLink2 = tmp;
	}
	return 0;
}

Packet *getPktFrTransacLink(TRANSACLINK **transacLink1, UINT_8 pendAM, IE3ADDR pendAddr)
{
	TRANSACLINK *tmp;

	tmp = *transacLink1;

	while(tmp != NULL)
	{
		if((tmp->pendAddrMode == pendAM)
		&& (((pendAM == defFrmCtrl_AddrMode16)&&((UINT_16)pendAddr == tmp->pendAddr16))
		 ||((pendAM == defFrmCtrl_AddrMode64)&&(pendAddr == tmp->pendAddr64))))
		{
			return tmp->pkt;
		}
		tmp = tmp->next;
	}
	return NULL;
}

int updateTransacLink(int oper, TRANSACLINK **transacLink1, TRANSACLINK **transacLink2, UINT_8 pendAM, IE3ADDR pendAddr)
{
	TRANSACLINK *tmp;
	int rt;

	//purge first if (oper == tr_oper_est)
	if (oper == tr_oper_est)
		purgeTransacLink(transacLink1,transacLink2);

	rt = 1;

	tmp = *transacLink1;

	if (oper == tr_oper_EST)
	{
		while(tmp != NULL)
		{
			if((tmp->pendAddrMode == pendAM)
			&& (((pendAM == defFrmCtrl_AddrMode16)&&((UINT_16)pendAddr == tmp->pendAddr16))
		 	||((pendAM == defFrmCtrl_AddrMode64)&&(pendAddr == tmp->pendAddr64))))
			{
				rt = 0;
				tmp = tmp->next;
				break;
			}
			tmp = tmp->next;
		}
		if (rt) return 1;
	}

	rt = 1;

	while(tmp != NULL)
	{
		if((tmp->pendAddrMode == pendAM)
		&& (((pendAM == defFrmCtrl_AddrMode16)&&((UINT_16)pendAddr == tmp->pendAddr16))
		 ||((pendAM == defFrmCtrl_AddrMode64)&&(pendAddr == tmp->pendAddr64))))
		{
			if (oper == tr_oper_del)	//delete an element
			{
				if(tmp->last != NULL)
				{
					tmp->last->next = tmp->next;
					if(tmp->next != NULL)
						tmp->next->last = tmp->last;
					else
						*transacLink2 = tmp->last;
				}
				else if (tmp->next != NULL)
				{
					*transacLink1 = tmp->next;
					tmp->next->last = NULL;
				}
				else
				{
					*transacLink1 = NULL;
					*transacLink2 = NULL;
				}
				//free the packet first
				Packet::free(tmp->pkt);
				delete tmp;
			}
			rt = 0;
			break;
		}
		tmp = tmp->next;
	}
	return rt;
}

int updateTransacLinkByPktOrHandle(int oper, TRANSACLINK **transacLink1, TRANSACLINK **transacLink2, Packet *pkt, UINT_8 msduH)
{
	TRANSACLINK *tmp;
	int rt;
	
	//purge first if (oper == tr_oper_est)
	if (oper == tr_oper_est)
		purgeTransacLink(transacLink1,transacLink2);

	rt = 1;

	tmp = *transacLink1;

	while(tmp != NULL)
	{
		if(((pkt != NULL)&&(tmp->pkt == pkt))
		 ||((pkt == NULL)&&(tmp->msduHandle == msduH)))
		{
			if (oper == tr_oper_del)	//delete an element
			{
				if(tmp->last != NULL)
				{
					tmp->last->next = tmp->next;
					if(tmp->next != NULL)
						tmp->next->last = tmp->last;
					else
						*transacLink2 = tmp->last;
				}
				else if (tmp->next != NULL)
				{
					*transacLink1 = tmp->next;
					tmp->next->last = NULL;
				}
				else
				{
					*transacLink1 = NULL;
					*transacLink2 = NULL;
				}
				//free the packet first
				Packet::free(tmp->pkt);
				delete tmp;
			}
			rt = 0;
			break;
		}
		tmp = tmp->next;
	}
	return rt;
}

int numberTransacLink(TRANSACLINK **transacLink1, TRANSACLINK **transacLink2)
{
	//return the number of transactions in the link
	TRANSACLINK *tmp;
	int num;

	//purge first
	purgeTransacLink(transacLink1,transacLink2);

	num = 0;
	tmp = *transacLink1;
	while(tmp != NULL)
	{
		num++;
		tmp = tmp->next;
	}
	return num;
}

int chkAddTransacLink(TRANSACLINK **transacLink1, TRANSACLINK **transacLink2, UINT_8 pendAM, IE3ADDR pendAddr, Packet *p, UINT_8 msduH, double kpTime)
{
	int i;

	//purge first
	purgeTransacLink(transacLink1,transacLink2);

	i = numberTransacLink(transacLink1,transacLink2);
	if (i >= maxNumTransactions) return 1;

	i = addTransacLink(transacLink1,transacLink2,pendAM,pendAddr,p,msduH,kpTime);
	if (i == 0) return 0;
	else return 2;
}

void emptyTransacLink(TRANSACLINK **transacLink1, TRANSACLINK **transacLink2)
{
	TRANSACLINK *tmp, *tmp2;

	if(*transacLink1 != NULL)
	{
		tmp = *transacLink1;
		while(tmp != NULL)
		{
			tmp2 = tmp;
			tmp = tmp->next;
			//free the packet first
			Packet::free(tmp2->pkt);
			delete tmp2;
		}
		*transacLink1 = NULL;
	}
	*transacLink2 = *transacLink1;
}

void dumpTransacLink(TRANSACLINK *transacLink1, IE3ADDR coorAddr)
{
	TRANSACLINK *tmp;
	int i;
	char tmpstr[121],tmpstr2[61];
	FrameCtrl frmCtrl;

	fprintf(stdout, "[%.2f] --- dump transaction list (by coordinator %d) ---\n", Scheduler::instance().clock(), coorAddr);
	tmp = transacLink1;
	i = 1;
	while(tmp != NULL)
	{
		sprintf(tmpstr, "\t%d:\t",i);
		if (tmp->pendAddrMode == defFrmCtrl_AddrMode16)
			sprintf(tmpstr2,"pendAddrMode = 16-bit\tpendAddr = %d\t",tmp->pendAddr16);
		else
			sprintf(tmpstr2,"pendAddrMode = 64-bit\tpendAddr = %d\t",tmp->pendAddr64);
		strcat(tmpstr,tmpstr2);
		frmCtrl.FrmCtrl = HDR_LRWPAN(tmp->pkt)->MHR_FrmCtrl;
		frmCtrl.parse();
		if (frmCtrl.frmType == defFrmCtrl_Type_Data)
			strcat(tmpstr,"pktType = data\t");
		else
			strcat(tmpstr,"pktType = command\t");
		sprintf(tmpstr2,"expTime = %f\n",tmp->expTime);
		strcat(tmpstr,tmpstr2);
		fprintf(stdout, "%s\n", tmpstr);
		tmp = tmp->next;
		i++;
	}
}

// End of file: p802_15_4transac.cc
