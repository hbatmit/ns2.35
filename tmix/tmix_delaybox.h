/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */

/*
 * Copyright 2007, Old Dominion University 
 * Copyright 2007, University of North Carolina at Chapel Hill
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution.
 *    3. The name of the author may not be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * M.C. Weigle, P. Adurthi, F. Hernandez-Campos, K. Jeffay, and F.D. Smith, 
 * Tmix: A Tool for Generating Realistic Application Workloads in ns-2, 
 * ACM Computer Communication Review, July 2006, Vol 36, No 3, pp. 67-76.
 *
 * Contact: Michele Weigle (mweigle@cs.odu.edu)
 * http://www.cs.odu.edu/inets/Tmix
 */

#include "delaybox/delaybox.h"  // uses some DelayBox structures and functions

class Tmix_DelayBoxClassifier;

void packet_string (char* str, hdr_tcp* tcph, hdr_ip* iph, int size);

/*::::::::::::::::: TMIX_DELAYBOX CLASSIFIER ::::::::::::::::::::::::::::::::*/

class Tmix_DelayBoxClassifier : public DelayBoxClassifier {
public:
	Tmix_DelayBoxClassifier() : DelayBoxClassifier(), lossless_(false) {}
  
	~Tmix_DelayBoxClassifier();
	inline void set_lossless () {lossless_ = true;}
        void create_flow_table(const char* src, const char* dst, FILE* fp);

protected:
	virtual void recv(Packet *p, Handler *h);
	bool lossless_;
};

/*::::::::::::::::: TMIX_DELAYBOX NODE :::::::::::::::::::::::::::::::::::::*/

class Tmix_DelayBoxNode : public Node {
public:
	Tmix_DelayBoxNode();
	~Tmix_DelayBoxNode();
	int command(int argc, const char*const* argv);

protected:
	Tmix_DelayBoxClassifier* classifier_;
	FILE* rttfp_;
        FILE* cvecfp_;

        TclObject* lookup_obj(const char* name) {
		TclObject* obj = Tcl::instance().lookup(name);
		if (obj == NULL) 
			fprintf(stderr, "Bad object name %s\n", name);
		return obj;
	}

};

