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
# $Header: /cvsroot/nsnam/ns-2/tcl/mobility/dsdv.tcl,v 1.14 2003/12/23 17:36:35 haldar Exp $
#
# Ported from CMU-Monarch project's mobility extensions -Padma, 10/98.

# ======================================================================
# Default Script Options
# ======================================================================
Agent/DSDV set sport_        0
Agent/DSDV set dport_        0
Agent/DSDV set wst0_         6        ;# As specified by Pravin
Agent/DSDV set perup_       15        ;# As given in the paper (update period)
Agent/DSDV set use_mac_      0        ;# Performance suffers with this on
Agent/DSDV set be_random_    1        ;# Flavor the performance numbers :)
Agent/DSDV set alpha_        0.875    ;# 7/8, as in RIP(?)
Agent/DSDV set min_update_periods_ 3  ;# Missing perups before linkbreak
Agent/DSDV set verbose_      0        ;# 
Agent/DSDV set trace_wst_    0        ;# 

set opt(ragent)		Agent/DSDV
set opt(pos)		NONE			;# Box or NONE

if { $opt(pos) == "Box" } {
	puts "*** DSDV using Box configuration..."
}

# ======================================================================
Agent instproc init args {
        eval $self next $args
}       

Agent/DSDV instproc init args {
        eval $self next $args
}       

# ===== Get rid of the warnings in bind ================================

# ======================================================================

proc create-dsdv-routing-agent { node id } {
    global ns_ ragent_ tracefd opt

    #
    #  Create the Routing Agent and attach it to port 255.
    #
    #set ragent_($id) [new $opt(ragent) $id]
    set ragent_($id) [new $opt(ragent)]
    set ragent $ragent_($id)

    ## setup address (supports hier-addr) for dsdv agent 
    ## and mobilenode
    set addr [$node node-addr]
    
    $ragent addr $addr
    $ragent node $node
    if [Simulator set mobile_ip_] {
	$ragent port-dmux [$node set dmux_]
    }
    $node addr $addr
    $node set ragent_ $ragent
    
    $node attach $ragent [Node set rtagent_port_]

    ##$ragent set target_ [$node set ifq_(0)]	;# ifq between LL and MAC
        
    # XXX FIX ME XXX
    # Where's the DSR stuff?
    #$ragent ll-queue [$node get-queue 0]    ;# ugly filter-queue hack
    $ns_ at 0.0 "$ragent_($id) start-dsdv"	;# start updates

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
    $ragent tracetarget $T
}


proc dsdv-create-mobile-node { id args } {
	global ns ns_ chan prop topo tracefd opt node_
	global chan prop tracefd topo opt
	
	set ns_ [Simulator instance]
	if [Simulator hier-addr?] {
		if [Simulator set mobile_ip_] {
			set node_($id) [new MobileNode/MIPMH $args]
		} else {
			set node_($id) [new Node/MobileNode/BaseStationNode $args]
		}
	} else {
		set node_($id) [new Node/MobileNode]
	}
	set node $node_($id)
	$node random-motion 0		;# disable random motion
	$node topography $topo
    
	# XXX Activate energy model so that we can use sleep, etc. But put on 
	# a very large initial energy so it'll never run out of it.
	if [info exists opt(energy)] {
		$node addenergymodel [new $opt(energy) $node 1000 0.5 0.2]
	}

	#
	# This Trace Target is used to log changes in direction
	# and velocity for the mobile node.
	#
	set T [new Trace/Generic]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
	$T set src_ $id
	$node log-target $T
    
	if ![info exist inerrProc_] {
		set inerrProc_ ""
	}
	if ![info exist outerrProc_] {
		set outerrProc_ ""
	}
	if ![info exist FECProc_] {
		set FECProc_ ""
	}

	$node add-interface $chan $prop $opt(ll) $opt(mac)	\
	    $opt(ifq) $opt(ifqlen) $opt(netif) $opt(ant) \
	    $topo $inerrProc_ $outerrProc_ $FECProc_ 
    
	#
	# Create a Routing Agent for the Node
	#
	create-$opt(rp)-routing-agent $node $id
    
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
	return $node
}








