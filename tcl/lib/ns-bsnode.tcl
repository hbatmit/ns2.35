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
# $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-bsnode.tcl,v 1.8 2000/09/14 18:19:27 haoboy Exp $
#

#THE CODE INCLUDED IN THIS FILE IS USED FOR BACKWARD COMPATIBILITY ONLY

#
#
# Special Base station nodes for communicating between wired and 
# wireless topologies in ns. Base stations are a hybrid form between 
# mobilenodes and hierarchical nodes. 
#
# XXX does otcl support multiple inheritence? then maybe could 
# inheriting from hiernode and mobilenode.
#

#
# The Node/MobileNode/BaseStationNode class
#
# ======================================================================
Class Node/MobileNode/BaseStationNode -superclass Node/MobileNode

Node/MobileNode/BaseStationNode instproc init args {
	$self next $args
}

Node/MobileNode/BaseStationNode instproc mk-default-classifier {} {
	$self instvar classifiers_ 
	set levels [AddrParams hlevel]
	for {set n 1} {$n <= $levels} {incr n} {
		set classifiers_($n) [new Classifier/Hash/Dest/Bcast 32]
		$classifiers_($n) set mask_ [AddrParams NodeMask $n]
		$classifiers_($n) set shift_ [AddrParams NodeShift $n]
	}
}


Node/MobileNode/BaseStationNode instproc entry {} {
	#XXX although mcast is not supported with wireless networking currently
	$self instvar ns_
	if ![info exist ns_] {
		set ns_ [Simulator instance]
	}
	if [$ns_ multicast?] { 
		$self instvar switch_
		return $switch_
	}
	$self instvar classifiers_
	return $classifiers_(1)
}

Node/MobileNode/BaseStationNode instproc add-target {agent port } {
	$self instvar dmux_ classifiers_
	$agent set sport_ $port
	set level [AddrParams hlevel]

	if { $port == [Node set rtagent_port_] } {	
		if { [Simulator set RouterTrace_] == "ON" } {
			#
			# Send Target
			#
			set sndT [cmu-trace Send "RTR" $self]
			$sndT target [$self set ll_(0)]
			$agent target $sndT
			#
			# Recv Target
			#
			set rcvT [cmu-trace Recv "RTR" $self]
			$rcvT target $agent
			for {set i 1} {$i <= $level} {incr i} {
				$classifiers_($i) defaulttarget $rcvT
				$classifiers_($i) bcast-receiver $rcvT
			}
			$dmux_ install $port $rcvT
		} else {
			$agent target [$self set ll_(0)]
			for {set i 1} {$i <= $level} {incr i} {
				$classifiers_($i) bcast-receiver $agent
				$classifiers_($i) defaulttarget $agent
			}
			$dmux_ install $port $agent
		}
	} else {
		if { [Simulator set AgentTrace_] == "ON" } {
			#
			# Send Target
			#
			set sndT [cmu-trace Send AGT $self]
			$sndT target [$self entry]
			$agent target $sndT
			#
			# Recv Target
			#
			set rcvT [cmu-trace Recv AGT $self]
			$rcvT target $agent
			$dmux_ install $port $rcvT
		} else {
			$agent target [$self entry]
			$dmux_ install $port $agent
		}
	}
}
