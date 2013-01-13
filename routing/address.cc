/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1990-1997 Regents of the University of California.
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
 * $Header: /cvsroot/nsnam/ns-2/routing/address.cc,v 1.27 2005/07/27 01:13:44 tomh Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "address.h"
#include "route.h"

static class AddressClass : public TclClass {
public:
	AddressClass() : TclClass("Address") {} 
	TclObject* create(int, const char*const*) {
		return (new Address());
	}
} class_address;


Address* Address::instance_;

Address::Address() : 
	NodeShift_(NULL), NodeMask_(NULL), McastShift_(0), McastMask_(0), 
	levels_(0)
{
}


Address::~Address() 
{ 
	delete [] NodeShift_;
	delete [] NodeMask_;
}

int Address::command(int argc, const char*const* argv)
{
	int i, c, temp=0;

	Tcl& tcl = Tcl::instance();
	if ((instance_ == 0) || (instance_ != this))
		instance_ = this;
	if (argc == 3) {
		if (strcmp(argv[1], "str2addr") == 0) {
			tcl.resultf("%d", str2addr(argv[2]));
			return (TCL_OK);
		}
	}
	if (argc >= 3) {
		if (strcmp(argv[1], "bpl-are") == 0) {
			if (levels_ != (argc-2)) {
				tcl.resultf("#bpl don't match with #hier levels\n");
				return (TCL_ERROR);
			}
			bpl_ = new int[levels_ + 1];
			for (c=1; c<=levels_; c++)
				bpl_[c] = atoi(argv[c+1]);
			return TCL_OK;
		}
	}
 	if (argc == 4) {
		if (strcmp(argv[1], "mcastbits-are") == 0) {
			McastShift_ = atoi(argv[2]);
			McastMask_ = atoi(argv[3]);
			return (TCL_OK);
		}
	}
	if (argc >= 4) {
		if (strcmp(argv[1], "add-hier") == 0) {
			/*
			 * <address> add-hier <level> <mask> <shift>
			 */
			int level = atoi(argv[2]);
			int mask = atoi(argv[3]);
			int shift = atoi(argv[4]);
			if (levels_ < level)
				levels_ = level;
			NodeShift_[level] = shift;
			NodeMask_[level] = mask;
			return (TCL_OK);
		}

		if (strcmp(argv[1], "idsbits-are") == 0) {
			temp = (argc - 2)/2;
			if (levels_) { 
				if (temp != levels_) {
					tcl.resultf("#idshiftbits don't match with #hier levels\n");
					return (TCL_ERROR);
				}
			}
			else 
				levels_ = temp;
			NodeShift_ = new int[levels_ + 1];
			for (i = 3, c = 1; c <= levels_; c++, i+=2)
				NodeShift_[c] = atoi(argv[i]);
			return (TCL_OK); 
		}
	
		if (strcmp(argv[1], "idmbits-are") == 0) {
			temp = (argc - 2)/2;
			if (levels_) { 
				if (temp != levels_) {
					tcl.resultf("#idmaskbits don't match with #hier levels\n");
					return (TCL_ERROR);
				}
			}
			else 
				levels_ = temp;
			NodeMask_ = new int[levels_ + 1];
			for (i = 3, c = 1; c <= levels_; c++, i+=2) 
				NodeMask_[c] = atoi(argv[i]);
			return (TCL_OK);
		}
	}
	return TclObject::command(argc, argv);
}

char *Address::print_nodeaddr(int address)
{
	int a;
	char temp[SMALL_LEN];
	char str[SMALL_LEN];
	char *addrstr;

	str[0] = '\0';
	for (int i=1; i <= levels_; i++) {
		a = address >> NodeShift_[i];
		if (levels_ > 1)
			a = a & NodeMask_[i];
		//if (i < levels_)
		sprintf(temp, "%d.", a);
		//else
		//sprintf(temp, "%d", a);
		
		strcat(str, temp);
	}
	int len;
	len = strlen(str);
	addrstr = new char[len+1];
	str[len-1]= 0; //kill the last dot
	strcpy(addrstr, str);
	// printf("Nodeaddr - %s\n",addrstr);
	return(addrstr);
}

int Address::hier_addr(int address, int level) {
	if (level <= levels_) {
		return ( (address >> NodeShift_[level]) & NodeMask_[level]);
	}
	perror("Address::hier_addr: levels greater than total h_levels_\n");
	return -1;
}

char *Address::get_subnetaddr(int address)
{
	int a;
	char temp[SMALL_LEN];
	char str[SMALL_LEN];
	char *addrstr;

	if (levels_ > 1) {
		str[0] = '\0';
		for (int i=1; i < levels_; i++) {
			a = address >> NodeShift_[i];
			a = a & NodeMask_[i];
			if (i < (levels_-1))
				sprintf(temp, "%d.", a);
			else
				sprintf(temp, "%d", a);
			strcat(str, temp);
		}
		addrstr = new char[strlen(str)+1];
		strcpy(addrstr, str);
		//printf("Subnet_addr - %s\n",addrstr);
		return(addrstr);
	}
	return NULL;
}

// returns nodeaddr in integer form (relevant especially for hier-addr)
int Address::get_nodeaddr(int address)
{
	int a;
	char *temp;
	
	temp = print_nodeaddr(address);
	a = str2addr(temp);
	delete [] temp;
	return a;
}


//Sets address in pkthdr format (having port and node fields)
int Address::create_ipaddr(int nodeid, int)
{
	return nodeid;
	// The following code is obsolete
#if 0
	int address;
	if (levels_ < 2) 
		address = (nodeid & NodeMask_[1]) << NodeShift_[1];
	else 
		address = nodeid;
	address = ((portid & PortMask_) << PortShift_) | \
		((~(PortMask_) << PortShift_) & address);
	return address;
#endif
}

int Address::get_lastaddr(int address)
{
 	int a;
 	a = address >> NodeShift_[levels_];
 	a = a & NodeMask_[levels_];
 	return a;
 }


char *Address::print_portaddr(int address)
{
	char str[SMALL_LEN];
	char *addrstr;

	str[0] = '\0';
#if 0
	int a;
	a = address >> PortShift_;
	a = a & PortMask_;
#endif
	sprintf(str, "%d", address);
	addrstr = new char[strlen(str)+1];
	strcpy(addrstr, str);
	// printf("Portaddr - %s\n",addrstr);

	return(addrstr);
}

// Convert address in string format to binary format (int). 
int Address::str2addr(const char *str) const
{
	
	if (levels_ < 2) {
		int tmp = atoi(str);		
		if (tmp < 0)     
			return (tmp);
		u_int uitmp = (u_int) tmp;
		if (uitmp > ((unsigned long)(1 << bpl_[1])) ) {
			fprintf(stderr, "Error!!\nstr2addr:Address %u outside range of address field length %lu\n", 
				uitmp, ((unsigned long)(1<< bpl_[1])));
			exit(1);
		}
		return tmp;
	}
	/*
	  int istr[levels_], addr= 0;
	*/
	/*
	 * for VC++
	 */

	int *istr= new int[levels_];
	int addr= 0;
	
	RouteLogic::ns_strtok((char*)str, istr);
	for (int i = 0; i < levels_; i++) {
		assert (istr[i] - 1 >= 0);
		if (((unsigned long)--istr[i]) > ((unsigned long)(1 << bpl_[i+1])) ) {
			fprintf(stderr, "Error!!\nstr2addr:Address %d outside range of address field length %lu\n", 
				istr[i], ((unsigned long)(1<< bpl_[i+1])));
			exit(1);
		}
		addr = set_word_field(addr, istr[i],
				      NodeShift_[i+1], NodeMask_[i+1]);
	}
	
	delete [] istr;
	
	return addr;
}









