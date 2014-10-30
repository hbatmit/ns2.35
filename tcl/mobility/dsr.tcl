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
# Ported from CMU-Monarch project's mobility extensions -Padma, 10/98.
# dsr.tcl
# $Id: dsr.tcl,v 1.16 2003/12/23 17:36:35 haldar Exp $

# ======================================================================
# Default Script Options
# ======================================================================

set opt(rt_port) 255
set opt(cc)      "off"            ;# have god check the caches for bad links?

# ======================================================================
# god cache monitoring

#source tcl/ex/timer.tcl
Class CacheTimer -superclass Timer
CacheTimer instproc timeout {} {
    global opt node_;
    $self instvar agent;
    $agent check-cache
    $self sched 1.0
}

proc checkcache {a} {
    global cachetimer ns

    set cachetimer [new CacheTimer]
    $cachetimer set agent $a
    $cachetimer sched 1.0
}

# ======================================================================
Class SRNode -superclass Node/MobileNode

SRNode instproc init {args} {
	global ns ns_ opt tracefd RouterTrace
	$self instvar dsr_agent_ dmux_ entry_point_ address_
        set ns_ [Simulator instance]

	eval $self next $args	;# parent class constructor
	if {$dmux_ == "" } {
		set dmux_ [new Classifier/Port]
		$dmux_ set mask_ [AddrParams PortMask]
		$dmux_ set shift_ [AddrParams PortShift]
		#
		# point the node's routing entry to itself
		# at the port demuxer (if there is one)
		#
	}
	# puts "making dsragent for node [$self id]"
	set dsr_agent_ [new Agent/DSRAgent]
	# setup address (supports hier-address) for dsragent

	$dsr_agent_ addr $address_
	$dsr_agent_ node $self
	if [Simulator set mobile_ip_] {
	    $dsr_agent_ port-dmux [$self set dmux_]
	}
	# set up IP address
	$self addr $address_
	
    if { $RouterTrace == "ON" } {
	# Recv Target
	set rcvT [cmu-trace Recv "RTR" $self]
	$rcvT target $dsr_agent_
	set entry_point_ $rcvT	
    } else {
	# Recv Target
	set entry_point_ $dsr_agent_
    }

    #
    # Drop Target (always on regardless of other tracing)
    #
    set drpT [cmu-trace Drop "RTR" $self]
    $dsr_agent_ drop-target $drpT

    #
    # Log Target
    #

    set T [new Trace/Generic]
    $T target [$ns_ set nullAgent_]
    $T attach $tracefd
    $T set src_ [$self id]
    $dsr_agent_ log-target $T

    $dsr_agent_ target $dmux_

    # packets to the DSR port should be dropped, since we've
    # already handled them in the DSRAgent at the entry.
    set nullAgent_ [$ns_ set nullAgent_]
    $dmux_ install $opt(rt_port) $nullAgent_

    # SRNodes don't use the IP addr classifier.  The DSRAgent should
    # be the entry point
    $self instvar classifier_
    set classifier_ "srnode made illegal use of classifier_"

}

SRNode instproc start-dsr {} {
    $self instvar dsr_agent_
    global opt;

    $dsr_agent_ startdsr
    if {$opt(cc) == "on"} {checkcache $dsr_agent_}
}

SRNode instproc entry {} {
        $self instvar entry_point_
        return $entry_point_
}



SRNode instproc add-interface {args} {
# args are expected to be of the form
# $chan $prop $tracefd $opt(ll) $opt(mac)
    global ns ns_ opt RouterTrace

    eval $self next $args

    $self instvar dsr_agent_ ll_ mac_ ifq_

    $dsr_agent_ mac-addr [$mac_(0) id]

    if { $RouterTrace == "ON" } {
	# Send Target
	set sndT [cmu-trace Send "RTR" $self]
	$sndT target $ll_(0)
	$dsr_agent_ add-ll $sndT $ifq_(0)
    } else {
	# Send Target
	$dsr_agent_ add-ll $ll_(0) $ifq_(0)
    }
    
    # setup promiscuous tap into mac layer
    $dsr_agent_ install-tap $mac_(0)

}

SRNode instproc reset args {
    $self instvar dsr_agent_
    eval $self next $args

    $dsr_agent_ reset
}

# ======================================================================

proc dsr-create-mobile-node { id args } {
	global ns_ chan prop topo tracefd opt node_
	set ns_ [Simulator instance] 
        if [Simulator hier-addr?] {
	    if [Simulator set mobile_ip_] {
		set node_($id) [new SRNode/MIPMH $args]
	    } else {
		set node_($id) [new SRNode $args]
	    }
	} else {
	    set node_($id) [new SRNode]
	}
	set node $node_($id)
	$node random-motion 0		;# disable random motion
	$node topography $topo

	# XXX Activate energy model so that we can use sleep, etc. But put on 
	# a very large initial energy so it'll never run out of it.
	if [info exists opt(energy)] {
		$node addenergymodel [new $opt(energy) $node 1000 0.5 0.2]
	}

	if ![info exist inerrProc_] {
	    set inerrProc_ ""
	}
	if ![info exist outerrProc_] {
	    set outerrProc_ ""
	}
	if ![info exist FECProc_] {
	    set FECProc_ ""
	}

        # connect up the channel
        $node add-interface $chan $prop $opt(ll) $opt(mac)     \
	    $opt(ifq) $opt(ifqlen) $opt(netif) $opt(ant) $topo \
	    $inerrProc_ $outerrProc_ $FECProc_ 

	#
	# This Trace Target is used to log changes in direction
	# and velocity for the mobile node and log actions of the DSR agent
	#
	set T [new Trace/Generic]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
	$T set src_ $id
	$node log-target $T

        $ns_ at 0.0 "$node start-dsr"
	return $node
}
