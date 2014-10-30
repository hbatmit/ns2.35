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
# $Header: /cvsroot/nsnam/ns-2/tcl/mobility/landmark.tcl,v 1.2 2000/08/30 23:27:51 haoboy Exp $

# ======================================================================
# Default Script Options
# ======================================================================
Agent/landmark set sport_        0
Agent/landmark set dport_        0
Agent/landmark set be_random_    1        ;# Flavor the performance numbers :)
Agent/landmark set verbose_      0        ;# 
Agent/landmark set trace_wst_    0        ;# 
Agent/landmark set debug_        1


#Class Agent/DSDV

set opt(ragent)		Agent/landmark
set opt(pos)		NONE			;# Box or NONE

if { $opt(pos) == "Box" } {
	puts "*** Landmark using Box configuration..."
}

# ======================================================================
Agent instproc init args {
        eval $self next $args
}       

Agent/landmark instproc init args {
        eval $self next $args
}       

# ===== Get rid of the warnings in bind ================================

# ======================================================================

proc create-landmark-agent { node id tag_dbase } {
    global ns_ ragent_ tracefd opt

    #
    #  Create the Routing Agent and attach it to port 255.
    #
    set ragent_($id) [new $opt(ragent)]
    set ragent $ragent_($id)

    # setup address (supports hier-addr) for Landmark agent and mobilenode
    set addr [$node node-addr]
    $ragent addr $addr
    $node addr $addr
    
    $node attach $ragent [Node set rtagent_port_]

    # Add a pointer to node so that agents can get location information
    $ragent node $node
        
    # XXX FIX ME XXX
    # Where's the DSR stuff?
    #$ragent ll-queue [$node get-queue 0]    ;# ugly filter-queue hack
    $ns_ at 0.0 "$ragent_($id) start"	;# start updates
    $ns_ at $opt(stop) "$ragent_($id) dumprtab"
    $ns_ at $opt(stop) "$ragent_($id) print-nbrs"

    if {$opt(update-period) > 0} {
	    $ragent set-update-period $opt(update-period)
    }

    #
    # Set-up link to global tag database
    #
    $ragent attach-tag-dbase $tag_dbase

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

proc create-query-agent { node id tag_dbase} {
    global ns_ qryagent_ tracefd opt

    set qryagent_($id) [new Agent/SensorQuery]
    set addr [$node node-addr]
    $qryagent_($id) addr $addr

    $node attach $qryagent_($id) 0
	
    #
    # Set-up link to global tag database
    #
    $qryagent_($id) attach-tag-dbase $tag_dbase

    #
    # Log Target
    #
    set T [new Trace/Generic]
    $T target [$ns_ set nullAgent_]
    $T attach $tracefd
    $T set src_ $id
    $qryagent_($id) tracetarget $T
}



proc landmark-create-mobile-node { id tag_dbase } {
	global ns ns_ chan prop topo tracefd opt node_
	global chan prop tracefd topo opt
	
	set ns_ [Simulator instance]
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
        # Add notion of energy to node
        #
        if [info exists opt(energymodel)] {
                $node addenergymodel [new $opt(energymodel) $opt(initialenergy)]
        }


	#
	# Create hierarchical landmark routing agent for the Node
	#
	create-landmark-agent $node $id $tag_dbase


	#
	# Create query agent for each sensor node
	#
	create-query-agent $node $id $tag_dbase

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









