#This code is a contribution of Arnaud Legout, Institut Eurecom, France.
#As the basis, for writing my scripts, I use the RLM scripts included in 
#ns. Therefore I gratefully thanks Steven McCanne who makes its scripts
#publicly available and the various ns team members who clean and
#maintain the RLM scripts.
#The following copyright is the original copyright included in the RLM scripts.
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
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/plm/plm-ns.tcl,v 1.1 2000/07/19 21:37:54 haoboy Exp $


Application/Traffic/CBR_PP instproc set args {
	$self instvar packet_size_ rate_ 
	if { [lindex $args 0] == "interval_" } {
		puts "Cannot use CBR_PP with interval_, specify rate_ instead"
	}
	eval $self next $args
}

Agent/LossMonitor/PLM instproc log-PP {} {
}

Class PLMLossTrace -superclass Agent/LossMonitor/PLM
PLMLossTrace set expected_ -1

PLMLossTrace instproc init {} {
    $self next
    $self instvar lastTime measure debug_
    set lastTime 0
    #set PP_estimate {}
    set measure -1
    global plm_debug_flag
    if [info exists plm_debug_flag] {
	set debug_ $plm_debug_flag
    }
}

PLMLossTrace instproc log-loss {} {
    $self instvar plm_
#    puts stderr [$self set nlost_]
    $plm_ log-loss
}

#variables
#PP_burst_length : number of packets in a PP burst
#PP_value : a single PP estimate.
#PP_estimate : array of the single PPs estimate (array size: PP_estimate_length)
#PP_estimate_value: global estimate: mean over all the single PP estimate contained in PP_estimate.
#PP_estimation_length : minimum number of PP used to calculate the global estimate

#Each time a packet is received log-PP check if this packet if the first packet 
#of a PP (flag_PP_=128). If log_PP see burst_length packets in sequence after
#the first packet of the PP, it calculates an estimate PP_value of the available 
#bandwidth based on this PP (of length PP_burst_length).
PLMLossTrace instproc log-PP {} {
    $self instvar plm_ PP_first measure next_pkt debug_
    global PP_burst_length packetSize
   
    #check if this is the first packet of a PP
    if {[$self set flag_PP_] == 128} {
	set measure 1
	set next_pkt [expr [$self set seqno_] + 1]
	set PP_first [$self set packet_time_PP_]
	if {$debug_>=2} {
	    trace_annotate "[$plm_ node]:  first PP [$self set seqno_], next: $next_pkt"
	} 	
    } elseif {$measure>-1} {
	#Only accept packets in sequence
	if {[$self set seqno_]==$next_pkt} {
	    set measure [expr $measure + 1]
	    set next_pkt [expr [$self set seqno_] + 1]	
	    if {$debug_>=2} {
		trace_annotate "[$plm_ node]:   pending measurement : $measure, next $next_pkt"
	    }
	    #if we receive PP_burst_length packets in sequence (and the first packet
	    #of the sequence has flag_PP_=128), i.e. a full PP, we calculate the 
	    #estimate
	    if {$measure==$PP_burst_length} {
		set PP_value [expr $packetSize*8.*($PP_burst_length - 1)/([$self set packet_time_PP_] - $PP_first)]
		set measure -1
		if {$debug_>=2} {
		    trace_annotate "[$plm_ node]:  measure : $PP_value"
		}
		#call the PLM machinery
		$plm_ make_estimate $PP_value
	    } 
	#if a packet if received out of sequence, we restart the estimation process 
	} else {
	    if {$debug_>=2} {
		trace_annotate "[$plm_ node]:  out of sequence : [$self set seqno_], next: $next_pkt"
	    }
	    set measure -1
	}
    }
}




Class PLMLayer/ns -superclass PLMLayer

PLMLayer/ns instproc init {ns plm addr layerNo} {
    $self next $plm
    
    $self instvar ns_ addr_ mon_
    set ns_ $ns
    set addr_ $addr
    set mon_ [$ns_ PLMcreate-agent [$plm node] PLMLossTrace 0]
    #layerNo : numero de la layer pour chaque PLM
    $mon_ set layerNo $layerNo
    $mon_ set plm_ $plm
    $mon_ set dst_addr_ $addr
    $mon_ set dst_port_ 0
}

#add a layer
PLMLayer/ns instproc join-group {} {
	$self instvar mon_ plm_ addr_
	$mon_ clear
	[$plm_ node] join-group $mon_ $addr_
	$self next
}

#drop a layer
PLMLayer/ns instproc leave-group {} {
	$self instvar mon_ plm_ addr_
	[$plm_ node] leave-group $mon_ $addr_
	$self next
}

#number of packets received for that layer
PLMLayer/ns instproc npkts {} {
	$self instvar mon_
	return [$mon_ set npkts_]
}

#number of packets lost for that layer
PLMLayer/ns instproc nlost {} {
	$self instvar mon_
	return [$mon_ set nlost_]
}

#allow to get statistics (number of packets received and lost) for a layer.
PLMLayer/ns instproc mon {} {
	$self instvar mon_
	return $mon_
}

#
# This class serves as an interface between the PLM class which
# implements the PLM protocol machinery, and the objects in ns
# that are involved in the PLM protocol (i.e., Node objects
# join/leave multicast groups, LossMonitor objects report packet
# loss, etc...).<p>
#
# See tcl/ex/test-plm.tcl for an example of how to create a
# simulation script that uses PLM
#
Class PLM/ns -superclass PLM

PLM/ns instproc init {ns localNode addrs check_estimate nn} {
	$self instvar ns_ node_ addrs_
	set ns_ $ns
	set node_ $localNode
	set addrs_ $addrs

	$self next [llength $addrs] $check_estimate $nn
}

PLM/ns instproc create-layer {layerNo} {
	$self instvar ns_ addrs_
	return [new PLMLayer/ns $ns_ $self [lindex $addrs_ $layerNo] $layerNo]
}

PLM/ns instproc now {} {
	$self instvar ns_
	return [$ns_ now]
}



#######


PLM/ns instproc node {} {
	$self instvar node_
	return $node_
}

PLM/ns instproc debug { msg } {
	$self instvar debug_ ns_
	if {$debug_ <1} { return }

	$self instvar subscription_ node_
	set time [format %.05f [$ns_ now]]
#	puts stderr "PLM/ns: $time node [$node_ id] layer $subscription_ $msg"
}

PLM/ns instproc trace { trace } {
        $self instvar layers_
        foreach s $layers_ {
		[$s mon] trace $trace
        }
}


PLM/ns instproc total_bytes_delivered {} {
	$self instvar layers_
        set v 0
        foreach s $layers_ {
                incr v [[$s mon] set bytes]
        }
        return $v
}

