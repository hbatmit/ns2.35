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

// File:  p802_15_4nam.h
// Mode:  C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t

// $Header: /cvsroot/nsnam/ns-2/wpan/p802_15_4nam.h,v 1.2 2011/06/22 04:03:26 tom_henderson Exp $

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


#ifndef p802_15_4nam_h
#define p802_15_4nam_h

#define mac_oper_del	1
#define mac_oper_est	2

class Mac802_15_4;

class MACLINK 
{
public:
	Mac802_15_4 *mac;
	int addr;
	MACLINK *last;
	MACLINK *next;
	MACLINK(int ad,Mac802_15_4 *m)
	{
		addr = ad;
		mac = m;
		last = NULL;
		next = NULL;
	}
};

int addMacLink(int ad,Mac802_15_4 *m);
int updateMacLink(int oper,int ad);
Mac802_15_4 *getMacLink(int ad);
int chkAddMacLink(int ad,Mac802_15_4 *m);

//------------------------------------------------------------------

#define at_oper_del	1
#define at_oper_est	2

extern int ATTRIBUTE_SN;

class ATTRIBUTELINK 
{
public:
	packet_t ptype;
	char color[21];
	int src,dst;
	int attribute;
	ATTRIBUTELINK *last;
	ATTRIBUTELINK *next;
	ATTRIBUTELINK(packet_t pt,char *clr,int s,int d)
	{
		ptype = pt;
		strncpy(color,clr,20);
		color[20] = 0;
		src = s;
		dst = d;
		attribute = ATTRIBUTE_SN++;
		last = NULL;
		next = NULL;
	}
};

packet_t nam_pktName2Type(const char *name);
int addAttrLink(packet_t pt,char *clr,int s = -2,int d = -2);
int updateAttrLink(int oper,packet_t pt,int s = -2,int d = -2);
int chkAddAttrLink(packet_t pt,char *clr,int s = -2,int d = -2);
ATTRIBUTELINK *findAttrLink(packet_t pt,int s = -2,int d = -2);

//------------------------------------------------------------------

class Nam802_15_4
{
	friend class Mac802_15_4;
	friend class Phy802_15_4;
public:
	Nam802_15_4(const char *ncolor,const char *mcolor,Mac802_15_4 *m);
	//~Nam802_15_4();
	static void	flowAttribute(int clrID,const char *clrName);
	static void	changePlaybackRate(double atTime,const char *stepLen);

protected:
	void		changeNodeColor(double atTime,const char *newColor,bool save = true);
	void		changeBackNodeColor(double atTime);
	void		flashNodeColor(double atTime,const char *flashColor = "");
	void		changeMarkColor(double atTime,const char *newColor);
	void		flashNodeMark(double atTime);
	void		changeLabel(double atTime,const char *lb);
	void		flashLinkFail(double atTime,int dst,const char *flashColor = "");
	void		annotate(double atTime,const char *note);

public:
	static bool Nam_Status;
	static bool emStatus;			//energy model used?
	static bool emHandling;			//need special handling for energy model?
	static char def_PANCoor_clr[21];
	static char def_Coor_clr[21];
	static char def_Dev_clr[21];
	static char def_Node_clr[21];
	static char def_Mark_clr[21];
	static char def_ColFlash_clr[21];
	static char def_NodeFail_clr[21];
	static char def_LinkFailFlash_clr[21];

protected:
	char	nodePreColor[21];
	char	nodeColor[21];
	char	markColor[21];
	char	label[81];

private:
	Mac802_15_4 *mac;

};

#endif

// End of file: p802_15_4nam.h
