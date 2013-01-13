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

// File:  p802_15_4nam.cc
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4nam.cc,v 1.2 2011/06/22 04:03:26 tom_henderson Exp $

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


#include "p802_15_4mac.h"
#include "p802_15_4nam.h"


MACLINK *macLink1 = NULL;
MACLINK *macLink2 = NULL;

int addMacLink(int ad,Mac802_15_4 *m)
{
	MACLINK *tmp;

	if (macLink2 == NULL)		//not exist yet
	{
		macLink2 = new MACLINK(ad,m);
		if (macLink2 == NULL) return 1;
		macLink1 = macLink2;
	}
	else
	{
		tmp=new MACLINK(ad,m);
		if (tmp == NULL) return 1;
		tmp->last = macLink2;
		macLink2->next = tmp;
		macLink2 = tmp;
	}
	return 0;
}

int updateMacLink(int oper,int ad)
{
	MACLINK *tmp;
	int rt;

	rt = 1;

	tmp = macLink1;
	while(tmp != NULL)
	{
		if (tmp->addr == ad)
		{
			if (oper == mac_oper_del)	//delete an element
			{
				if(tmp->last != NULL)
				{
					tmp->last->next = tmp->next;
					if(tmp->next != NULL)
						tmp->next->last = tmp->last;
					else
						macLink2 = tmp->last;
				}
				else if (tmp->next != NULL)
				{
					macLink1 = tmp->next;
					tmp->next->last = NULL;
				}
				else
				{
					macLink1 = NULL;
					macLink2 = NULL;
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

Mac802_15_4 *getMacLink(int ad)
{
	MACLINK *tmp;

	tmp = macLink1;
	while(tmp != NULL)
	{
		if(tmp->addr == ad)
			return tmp->mac;
		tmp = tmp->next;
	}

	return NULL;
}

int chkAddMacLink(int ad,Mac802_15_4 *m)
{
        int i;

        i = updateMacLink(at_oper_est,ad);
        if (i == 0) return 1;
        i = addMacLink(ad,m);
        if (i == 0) return 0;
        else return 2;
}

//-------------------------------------------------------------------

ATTRIBUTELINK *attrLink1 = NULL;
ATTRIBUTELINK *attrLink2 = NULL;
int ATTRIBUTE_SN = 32;

packet_t nam_pktName2Type(const char *name)
{
	//not all types included

	return (strcmp(packet_info.name(PT_TCP),name) == 0)?PT_TCP:
               (strcmp(packet_info.name(PT_UDP),name) == 0)?PT_UDP: 
               (strcmp(packet_info.name(PT_CBR),name) == 0)?PT_CBR: 
               (strcmp(packet_info.name(PT_ACK),name) == 0)?PT_ACK: 
               (strcmp(packet_info.name(PT_PRUNE),name) == 0)?PT_PRUNE: 
               (strcmp(packet_info.name(PT_GRAFT),name) == 0)?PT_GRAFT: 
               (strcmp(packet_info.name(PT_GRAFTACK),name) == 0)?PT_GRAFTACK: 
               (strcmp(packet_info.name(PT_RTCP),name) == 0)?PT_RTCP: 
               (strcmp(packet_info.name(PT_RTP),name) == 0)?PT_RTP: 
               (strcmp(packet_info.name(PT_SRM),name) == 0)?PT_SRM: 
               (strcmp(packet_info.name(PT_TELNET),name) == 0)?PT_TELNET: 
               (strcmp(packet_info.name(PT_FTP),name) == 0)?PT_FTP: 
               (strcmp(packet_info.name(PT_HTTP),name) == 0)?PT_HTTP: 
               (strcmp(packet_info.name(PT_MFTP),name) == 0)?PT_MFTP: 
               (strcmp(packet_info.name(PT_ARP),name) == 0)?PT_ARP: 
               (strcmp(packet_info.name(PT_MAC),name) == 0)?PT_MAC: 
               (strcmp(packet_info.name(PT_TORA),name) == 0)?PT_TORA: 
               (strcmp(packet_info.name(PT_DSR),name) == 0)?PT_DSR: 
               (strcmp(packet_info.name(PT_AODV),name) == 0)?PT_AODV: 
               (strcmp(packet_info.name(PT_IMEP),name) == 0)?PT_IMEP: 
               (strcmp(packet_info.name(PT_PING),name) == 0)?PT_PING: 
               (strcmp(packet_info.name(PT_LDP),name) == 0)?PT_LDP: 
               (strcmp(packet_info.name(PT_GAF),name) == 0)?PT_GAF: 
               (strcmp(packet_info.name(PT_PGM),name) == 0)?PT_PGM:PT_NTYPE;
}

int addAttrLink(packet_t pt,char *clr,int s,int d)
{
	ATTRIBUTELINK *tmp;

	if (attrLink2 == NULL)		//not exist yet
	{
		attrLink2 = new ATTRIBUTELINK(pt,clr,s,d);
		if (attrLink2 == NULL) return 1;
		attrLink1 = attrLink2;
	}
	else
	{
		tmp=new ATTRIBUTELINK(pt,clr,s,d);
		if (tmp == NULL) return 1;
		tmp->last = attrLink2;
		attrLink2->next = tmp;
		attrLink2 = tmp;
	}
	return 0;
}

int updateAttrLink(int oper,packet_t pt,int s,int d)
{
	ATTRIBUTELINK *tmp;
	int rt;

	rt = 1;

	tmp = attrLink1;
	while(tmp != NULL)
	{
		if ((tmp->ptype == pt)&&(tmp->src == s)&&(tmp->dst == d))
		{
			if (oper == at_oper_del)	//delete an element
			{
				if(tmp->last != NULL)
				{
					tmp->last->next = tmp->next;
					if(tmp->next != NULL)
						tmp->next->last = tmp->last;
					else
						attrLink2 = tmp->last;
				}
				else if (tmp->next != NULL)
				{
					attrLink1 = tmp->next;
					tmp->next->last = NULL;
				}
				else
				{
					attrLink1 = NULL;
					attrLink2 = NULL;
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

int chkAddAttrLink(packet_t pt,char *clr,int s,int d)
{
        int i;

        i = updateAttrLink(at_oper_est,pt,s,d);
        if (i == 0) return 1;
        i = addAttrLink(pt,clr,s,d);
        if (i == 0) return 0;
        else return 2;
}

ATTRIBUTELINK *findAttrLink(packet_t pt,int s,int d)
{
	ATTRIBUTELINK *tmp;

	tmp = attrLink1;
	while(tmp != NULL)
	{
		if((tmp->ptype == pt)&&(tmp->src == s)&&(tmp->dst == d))
			return tmp;
		tmp = tmp->next;
	}

	return NULL;
}

//---------------------------------------------------------------------------

bool Nam802_15_4::Nam_Status = false;		
bool Nam802_15_4::emStatus = false;		
bool Nam802_15_4::emHandling = true;
char Nam802_15_4::def_PANCoor_clr[] = "tomato";
char Nam802_15_4::def_Coor_clr[] = "blue";
char Nam802_15_4::def_Dev_clr[] = "green3";
char Nam802_15_4::def_Node_clr[] = "black";
char Nam802_15_4::def_Mark_clr[] = "black";
char Nam802_15_4::def_ColFlash_clr[] = "gold";
char Nam802_15_4::def_NodeFail_clr[] = "grey";
char Nam802_15_4::def_LinkFailFlash_clr[] = "red";

Nam802_15_4::Nam802_15_4(const char *ncolor,const char *mcolor,Mac802_15_4 *m)
{
	strncpy(nodeColor,ncolor,20);
	nodeColor[20] = 0;
	strcpy(nodePreColor,nodeColor);
	strncpy(markColor,mcolor,20);
	markColor[20] = 0;
	strcpy(label,"\"\"");
	mac = m;
}

void Nam802_15_4::flowAttribute(int clrID,const char *clrName)
{
	Tcl& tcl = Tcl::instance();

	if (!Nam_Status) return;

	tcl.evalf("[Simulator instance] puts-nam-traceall {c -t * -i %d -n %s}",
	clrID,clrName);
}

void Nam802_15_4::changePlaybackRate(double atTime,const char *stepLen)
{
	Tcl& tcl = Tcl::instance();

	if (!Nam_Status) return;

	if (atTime > 0.0)
		atTime += 0.000000001;		
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {v -t %.9f -e set_rate_ext %s 1}}",
	atTime,atTime,stepLen);
}

void Nam802_15_4::changeNodeColor(double atTime,const char *newColor,bool save)
{
	char t_newColor[21];
	Tcl& tcl = Tcl::instance();

	if (!Nam_Status) return;

	if (emStatus&&emHandling) return;

	if (atTime > 0.0)
		atTime += 0.000000001;		
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {n -t %.9f -s %d -S COLOR -c %s -o %s}}",
		atTime,atTime,mac->index_,newColor,nodeColor);
	if (save)
	{
		strncpy(t_newColor,newColor,20);
		t_newColor[20] = 0;
		strcpy(nodePreColor,nodeColor);	
		strncpy(nodeColor,t_newColor,20);
		nodeColor[20] = 0;
	}
}

void Nam802_15_4::changeBackNodeColor(double atTime)
{
	changeNodeColor(atTime,nodePreColor);
}

void Nam802_15_4::flashNodeColor(double atTime,const char *flashColor)
{
	char t_flashColor[21];
	Tcl& tcl = Tcl::instance();

	if (!Nam_Status) return;

	if (emStatus&&emHandling) return;

	if (flashColor[0] == 0)
		strcpy(t_flashColor,Nam802_15_4::def_ColFlash_clr);
	else
	{
		strncpy(t_flashColor,flashColor,20);
		t_flashColor[20] = 0;
	}

	atTime += 0.000000001;		
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {n -t %.9f -s %d -S COLOR -c %s -o %s}}",
		atTime,atTime,mac->index_,t_flashColor,nodeColor);
	atTime += 0.010000000;
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {n -t %.9f -s %d -S COLOR -c %s -o %s}}",
		atTime,atTime,mac->index_,"grey",t_flashColor);
	atTime += 0.010000000;
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {n -t %.9f -s %d -S COLOR -c %s -o %s}}",
		atTime,atTime,mac->index_,nodeColor,"grey");
}

void Nam802_15_4::changeMarkColor(double atTime,const char *newColor)
{
	Tcl& tcl = Tcl::instance();

	if (!Nam_Status) return;

	if (atTime > 0.0)
		atTime += 0.000000001;		
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {n -t %.9f -s %d -S COLOR -i %s -I %s -c %s -o %s}}",
		atTime,atTime,mac->index_,newColor,markColor,nodeColor,nodeColor);
	strncpy(markColor,newColor,20);
	markColor[20] = 0;
}

void Nam802_15_4::flashNodeMark(double atTime)
{
	Tcl& tcl = Tcl::instance();

	if (!Nam_Status) return;

	if (emStatus&&emHandling) return;

	atTime += 0.010000000;
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {n -t %.9f -s %d -S COLOR -i %s -I %s -c %s -o %s}}",
		atTime,atTime,mac->index_,"LightGrey",markColor,nodeColor,nodeColor);
	atTime += 0.010000000;
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {n -t %.9f -s %d -S COLOR -i %s -I %s -c %s -o %s}}",
		atTime,atTime,mac->index_,markColor,"LightGrey",nodeColor,nodeColor);
}

void Nam802_15_4::changeLabel(double atTime,const char *lb)
{
	Tcl& tcl = Tcl::instance();

	if (!Nam_Status) return;

	if (atTime > 0.0)
		atTime += 0.000000001;		
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {n -t %.9f -s %d -S DLABEL -l %s -L %s}}",
		atTime,atTime,mac->index_,lb,label);
	strncpy(label,lb,80);
	label[80] = 0;
}

void Nam802_15_4::flashLinkFail(double atTime,int dst,const char *flashColor)
{
	char t_flashColor[21];
	Tcl& tcl = Tcl::instance();

	if (!Nam_Status) return;

	if (flashColor[0] == 0)
		strcpy(t_flashColor,Nam802_15_4::def_LinkFailFlash_clr);
	else
	{
		strncpy(t_flashColor,flashColor,20);
		t_flashColor[20] = 0;
	}
	
	atTime += 0.000000001;		
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {n -t %.9f -s %d -S DLABEL -l \" ~ %d\" -L %s}}",
		atTime,atTime,mac->index_,dst,label);
	atTime += 0.020000000;
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {n -t %.9f -s %d -S DLABEL -l \" \" -L \" ~ %d\"}}",
		atTime,atTime,mac->index_,dst);
	atTime += 0.010000000;
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] puts-nam-traceall {n -t %.9f -s %d -S DLABEL -l %s -L \" ~ %d\"}}",
		atTime,atTime,mac->index_,label,dst);
}

void Nam802_15_4::annotate(double atTime,const char *note)
{
	Tcl& tcl = Tcl::instance();

	atTime += 0.000000001;		
	tcl.evalf("[Simulator instance] at %.9f {[Simulator instance] trace-annotate %s}",atTime,note);
}

// End of file: p802_15_4nam.cc
