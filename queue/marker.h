/*
 * Copyright (c) 2000-2002, by the Rector and Board of Visitors of the 
 * University of Virginia.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, 
 * with or without modification, are permitted provided 
 * that the following conditions are met:
 *
 * Redistributions of source code must retain the above 
 * copyright notice, this list of conditions and the following 
 * disclaimer. 
 *
 * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following 
 * disclaimer in the documentation and/or other materials provided 
 * with the distribution. 
 *
 * Neither the name of the University of Virginia nor the names 
 * of its contributors may be used to endorse or promote products 
 * derived from this software without specific prior written 
 * permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *                                                                     
 * Marker module for JoBS (and WTP).
 *                                                                     
 * Authors: Constantinos Dovrolis <dovrolis@mail.eecis.udel.edu>, 
 *          Nicolas Christin <nicolas@cs.virginia.edu>, 2000-2002       
 *								      
 * $Id: marker.h,v 1.1 2003/02/02 22:18:22 xuanc Exp $
 */
#ifndef MARKER_H
#define MARKER_H

#define NO_CLASSES 4	// This number cannot be changed 
			// w/o modifying the code. 
			// This is, again, a prototype 
			// implementation...
#define	DETERM 1	// Deterministic marker: all traffic is marked with 
			// given class (marker_class_)
#define STATIS 2	// Probabilistic marker: class-marking follow given
			// (cumulative) distribution

class Marker: public Queue {
public:
	Marker();
	virtual	int command(int argc, const char*const* argv); 
	void	enque(Packet*);
	Packet* deque();
	double 	marker_arrvs_[NO_CLASSES+1];	// For monitoring purposes
protected:
	int	marker_type_; 			// Marker type 
	double	marker_frc_[NO_CLASSES+1];	// Class-marking fractions 
					  	// (STATIS)
						// marker_frc_ represent the 
						// *cumulative* marking distribution
	int 	marker_class_;			// Fixed class marking (DETERM)
	PacketQueue	*q_;			// Underlying FIFO queue 
	int	rn_seed_;			// Random seed
};

#endif /* MARKER_H */
