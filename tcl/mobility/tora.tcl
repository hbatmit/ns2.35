# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Copyright (c) 1996-1998 Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# $Header: /cvsroot/nsnam/ns-2/tcl/mobility/tora.tcl,v 1.3 2000/08/30 23:27:51 haoboy Exp $

# ======================================================================
# Default Script Options
# ======================================================================

set opt(imep)		"ON"			;# Enable/Disable IMEP
set opt(ragent)		Agent/rtProto/TORA
set opt(pos)		NONE			;# Box or NONE

if { $opt(pos) == "Box" } {
	puts "*** TORA using Box configuration..."
}

# ======================================================================
Agent instproc init args {
        $self next $args
}       
Agent/rtProto instproc init args {
        puts "DOWN HERE 2"
        $self next $args
}       
Agent/rtProto/TORA instproc init args {
       puts "DOWN HERE" 
       $self next $args
}       

Agent/rtProto/TORA set sport_	0
Agent/rtProto/TORA set dport_	0

# ======================================================================

proc create-routing-agent { node id } {
	global ns_ ragent_ tracefd opt

	#
	#  Create the Routing Agent and attach it to port 255.
	#
	set ragent_($id) [new $opt(ragent) $id]
	set ragent $ragent_($id)
	$node attach $ragent [Node set rtagent_port_]

	$ragent if-queue [$node set ifq_(0)]	;# ifq between LL and MAC

	# now that the Beacon/Hello messages have been
	# moved to the IMEP layer, this is not necessary.
#	$ns_ at 0.$id "$ragent_($id) start"	;# start BEACON/HELLO Messages

        #
        # XXX: The routing protocol and the IMEP agents needs handles
        # to each other.
        #
        $ragent imep-agent [$node set imep_(0)]
        [$node set imep_(0)] rtagent $ragent

	#
	# Drop Target (always on regardless of other tracing)
	#
	set drpT [cmu-trace Drop "RTR" $node]
	$ragent drop-target $drpT
	
	#
	# Log Target
	#
	set T [new Trace/Generic]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
	$T set src_ $id
	$ragent log-target $T

        #
        # XXX: let the IMEP agent use the same log target.
        #
        [$node set imep_(0)] log-target $T
}


proc create-mobile-node { id } {
	global ns_ chan prop topo tracefd opt node_
	global chan prop tracefd topo opt

	set node_($id) [new Node/MobileNode]

	set node $node_($id)
	$node random-motion 0		;# disable random motion
	$node topography $topo

	#
	# This Trace Target is used to log changes in direction
	# and velocity for the mobile node.
	#
	set T [new Trace/Generic]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
	$T set src_ $id
	$node log-target $T

	$node add-interface $chan $prop $opt(ll) $opt(mac)	\
		$opt(ifq) $opt(ifqlen) $opt(netif) $opt(ant)

	#
	# Create a Routing Agent for the Node
	#
	create-routing-agent $node $id

	# ============================================================

	if { $opt(pos) == "Box" } {
		#
		# Box Configuration
		#
		set spacing 200
		set maxrow 7
		set col [expr ($id - 1) % $maxrow]
		set row [expr ($id - 1) / $maxrow]
		$node set X_ [expr $col * $spacing]
		$node set Y_ [expr $row * $spacing]
		$node set Z_ 0.0
		$node set speed_ 0.0

		$ns_ at 0.0 "$node_($id) start"
	}
}






