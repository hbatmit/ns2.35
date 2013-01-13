#This code is a contribution of Arnaud Legout, Institut Eurecom, France.
#As the basis, for writing my scripts, I use the RLM scripts included in 
#ns. Therefore I gratefully thanks Steven McCanne who makes its scripts
#publicly available and the various ns team members who clean  and
#maintain the RLM scripts.
#
# Copyright (c)1996 Regents of the University of California.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/plm/plm-topo.tcl,v 1.1 2000/07/19 21:37:54 haoboy Exp $


Simulator instproc PLMcreate-agent { node type pktClass } {
	$self instvar Agents PortID 
	set agent [new $type]
	$agent set fid_ $pktClass
	$self attach-agent $node $agent
	$agent proc get var {
		return [$self set $var]
	}
	return $agent
}


Simulator instproc PLMcbr_flow_PP { node fid addr bw } {
	global packetSize PP_burst_length
	set agent [$self PLMcreate-agent $node Agent/UDP $fid]
	set cbr [new Application/Traffic/CBR_PP]
	$cbr attach-agent $agent
	
	#XXX abstraction violation
        $agent set dst_addr_ $addr
        $agent set dst_port_ 0
	
	$cbr set packet_size_ $packetSize
	$cbr set rate_ $bw
	$cbr set random_ 1
        $cbr set PBM_ $PP_burst_length
	return $cbr
}



#create a set fo layers for a given PLM source.
Simulator instproc PLMbuild_source_set { plmName rates addrs baseClass node when } {
    global src_plm src_rate
    set n [llength $rates]
    #mise en place de la layer de base avec PP
    set r [lindex $rates 0]
    set addr [expr [lindex $addrs 0]]
    set src_rate($addr) $r
    set k $plmName:0
    set src_plm($k) [$self PLMcbr_flow_PP $node $baseClass $addr $r]
    $self at 0 "$src_plm($k) set maxpkts_ 1; $src_plm($k) start"
    $self at $when "$src_plm($k) set maxpkts_ 268435456; $src_plm($k) start"
    #incr baseClass    
    
    for {set i 1} {$i<$n} {incr i} {
	set r [lindex $rates $i]
	set addr [expr [lindex $addrs $i]]
	
	set src_rate($addr) $r
	set k $plmName:$i
	set src_plm($k) [$self PLMcbr_flow_PP $node $baseClass $addr $r]
	$self at 0 "$src_plm($k) set maxpkts_ 1; $src_plm($k) start"
	$self at $when "$src_plm($k) set maxpkts_ 268435456; $src_plm($k) start"
	#$self at $when "$src_plm($k) start"
	#incr baseClass
    }

}

Class PLMTopology

PLMTopology instproc init { simulator } {
	$self instvar ns id
	set ns $simulator
    #id determine de fid de chaque session PLM, faire attention aux
    #interactions avec les fid d'autres flows pour FQ.
	set id 0
}

PLMTopology instproc mknode nn {
	$self instvar node ns
	if ![info exists node($nn)] {
		set node($nn) [$ns node]
	}
}


#
# build a link between nodes $a and $b
# if either node doesn't exist, create it as a side effect.
# (we don't build any sources)
#
PLMTopology instproc build_link { a b delay bw } {
	global buffers packetSize Queue_sched_
	if { $a == $b } {
		puts stderr "link from $a to $b?"
		exit 1
	}
	$self instvar node ns
	$self mknode $a
	$self mknode $b
	$ns duplex-link $node($a) $node($b) $bw $delay $Queue_sched_
}

PLMTopology instproc build_link-simple { a b delay bw f} {
	global buffers packetSize Queue_sched_ 
	if { $a == $b } {
		puts stderr "link from $a to $b?"
		exit 1
	}
	$self instvar node ns
	$self mknode $a
	$self mknode $b
	$ns duplex-link-trace $node($a) $node($b) $bw $delay $Queue_sched_ $f
}



#
# build a new source (by allocating a new address) and
# place it at node $nn.  start it up at $when.
#
PLMTopology instproc place_source { nn when } {
	#XXX
	global rates 
	$self instvar node ns id addrs

	incr id
	set addrs($id) {}
	foreach r $rates {
		lappend addrs($id) [Node allocaddr]
	}

	$ns PLMbuild_source_set s$id $rates $addrs($id) $id $node($nn) $when
	
	return $id
}

#by default nb=1 does not give any functionality, the instance of the last
#PLM receiver is in PLMrcvr(1)
#If one specifies nb when one create the receiver, PLMrcvr($nb) contains
#the PLM instance of each PLM receiver. One must specifies a different nb per receiver.
PLMTopology instproc place_receiver { nn id when check_estimate {nb 1}} {
	$self instvar ns  
	$ns at $when "$self build_receiver $nn $id $check_estimate $nb"
}

#
# build a new receiver for source $id and
# place it at node $nn.
#
PLMTopology instproc build_receiver { nn id check_estimate nb} {
    $self instvar node ns addrs
    global PLMrcvr
    set PLMrcvr($nb) [new PLM/ns $ns $node($nn) $addrs($id) $check_estimate $nn]
    
    global plm_debug_flag
    $PLMrcvr($nb) set debug_ $plm_debug_flag
}

