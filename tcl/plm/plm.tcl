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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/plm/plm.tcl,v 1.1 2000/07/19 21:37:54 haoboy Exp $
#


#The PLM structure: 
#When we create a new receiver (place_receiver) we instantiate the 
#class PLM/ns which inherit from the class PLM. The class PLM creates 
#as many PLMLayer/ns instance (create-layer) as there are layers. The 
#class PLMLayer/ns creates an instance of PLMLossTrace (which is 
#reponsible for monitoring received packets and monitoring losses).

#The PLM class is intended to implement all the PLM protocol without any
#specific interface with ns. The specific ns interface is implemented in PLM/ns.
#There is a similar relation between PLMLayer and PLMLayer/ns. 
#However, we do not guarantee the strict validity of this ns interfacing.



#The PLM class implement the PLM protocol (see 
#http://www.eurecom.fr/~legout/Research/research.html 
#for details about the protocol evaluation)
Class PLM

PLM instproc init {levels chk_estimate n_id} {
    $self next
    $self instvar PP_estimate wait_loss time_loss 
    $self instvar start_loss time_estimate check_estimate node_id
    global rates
    set PP_estimate {} 
    set start_loss -1
    set wait_loss 0
    set time_loss 0
    set time_estimate 0
    set check_estimate $chk_estimate
    set node_id $n_id
    
	$self instvar debug_ env_ maxlevel_

	set debug_ 0
	set env_ [lindex [split [$self info class] /] 1]
	set maxlevel_ $levels

	#XXX
	global plm_debug_flag
	if [info exists plm_debug_flag] {
		set debug_ $plm_debug_flag
	}

	$self instvar subscription_

	#
	# we number the subscription level starting at 1.
	# level 0 means no groups are subscribed to.
	# 
	$self instvar layer_ layers_
	set i 1
	while { $i <= $maxlevel_ } {
		set layer_($i) [$self create-layer [expr $i - 1]]
		lappend layers_ $layer_($i)
		incr i
	}
	
	#
	# set the subscription level to 0 and call add_layer
	# to start out with at least one group
	#
	set subscription_ 0
	$self add-layer
}



#make_estimate makes an estimate PP_estimate_value by taking the minimum PP_value 
#received during a check_estimate period (and at least PP_estimation_length  
#PP_value received). This PP_estimate_value is used to choose a layer.
#For each PP_value, make_estimate makes a stability_drop i.e. PLM drops layer(s) if a 
#PP_value is lower than the current subscription level.
PLM instproc make_estimate {PP_value} {
    $self instvar PP_estimate PP_estimate_value ns_ time_estimate check_estimate debug_
    global PP_estimation_length
    
    #Add PP_value to the list of single PP estimates PP_estimate
    lappend PP_estimate $PP_value
    
    #Drop layer(s) if the PP_value is lower than the current subscription level
    $self stability-drop $PP_value

    #time_estimate is the minimum period of time during which we collect PP_value
    #to make the global estimate PP_estimate_value
    set ns_time [$ns_ now]
    if {$time_estimate==0} {
	set time_estimate [expr $ns_time + $check_estimate]
    }
    if {$debug_>=3} {
	trace_annotate "[$self node]: check: $check_estimate $PP_estimate , nb: [llength $PP_estimate]"
    }

    #if we have collected PP_value for at least time_estimate and we have at least 
    #PP_estimation_length, we calculate the PP_estimate_value
    if {($time_estimate<=$ns_time) && ([llength $PP_estimate] >= $PP_estimation_length)} {
	#we take the minimum
	set PP_estimate_value [lindex [lsort -real $PP_estimate] 0]
	if {$debug_>=3} {
	    trace_annotate "[$self node]: check: $check_estimate PP estim: $PP_estimate, value: $PP_estimate_value"
	}
	#puts stderr [set PP_estimate_value]
	#puts stderr [set PP_estimate]
	if {$debug_>=2} {
	    trace_annotate [expr round($PP_estimate_value)]
	}
	set PP_estimate {}
	#puts stderr "noeud: [$self node] check_estimate: $check_estimate"
	set time_estimate [expr $ns_time + $check_estimate]
	#choode the layer according to the PP_estimate_value
	$self choose_layer $PP_estimate_value
	
    }
}


#stability_drop drops layer(s) if a PP_value is lower than the current subscription level.
PLM instproc stability-drop {PP_value} {
    $self instvar subscription_ start_loss time_estimate PP_estimate
    $self instvar check_estimate ns_
    global rates_cum

    set ns_time [$ns_ now]
    #puts stderr $PP_value
    for {set i 0} {[lindex $rates_cum $i] < [expr round($PP_value)]} {incr i} {
	if {$i > [llength $rates_cum]} {break}
    }
    #puts stderr [lindex $rates_cum $i]
    #puts stderr $PP_estimate_value
    #puts stderr $i
    
    if {$subscription_ > $i} {
	for {set j $subscription_} {$i < $j} {incr j -1} {
	    set start_loss -1
	    $self drop-layer	    
	}
	set PP_estimate {}
	set time_estimate [expr $ns_time + $check_estimate]
    }
}

#calculate the cumulated rates. (usefull for choose_layer)
proc calc_cum {rates} {
    set temp 0
    set rates_cum {}
    for {set i 0} {$i<[llength $rates]} {incr i} {
	set temp [expr $temp + [lindex $rates $i]]
	lappend rates_cum $temp
    }
    return $rates_cum
}

#choose_layer chooses a layer according to the PP_estimate_value 
#and the current subscription level.
PLM instproc choose_layer {PP_estimate_value} {
    $self instvar subscription_ start_loss
    global rates_cum

    #A assume an estimate will better ajust the rate than dropping
    #a layer due to the losses
    set start_loss -1

    #puts stderr $PP_estimate_value
    for {set i 0} {[lindex $rates_cum $i] < [expr round($PP_estimate_value)]} {incr i} {
	if {$i > [llength $rates_cum]} {break}
    }
    #puts stderr [lindex $rates_cum $i]
    #puts stderr $PP_estimate_value
    #puts stderr $i
    
    if {$subscription_ < $i} {
	for {set j $subscription_} {$j < $i} {incr j} {
	    $self add-layer	    
	}	    
    } elseif {$subscription_ > $i} {
	for {set j $subscription_} {$i < $j} {incr j -1} {
	    $self drop-layer	    
	}
    } elseif {$subscription_ == $i} {
	return
    }
}


#In case of loss, log-loss is called. As only one PP_value allows to drop 
#the right number of layers (with stability_drop), log-loss is very conservative 
#i.e. only drop layer in case of high and sustained loss rate (PLM always gives 
#a chance to receive a PP_value before dropping a layer due to loss).
PLM instproc log-loss {} {
    $self instvar subscription_ h_npkts h_nlost start_loss debug_
    $self instvar time_loss ns_ wait_loss
   
    $self debug "LOSS [$self plm_loss]" 

    #puts "pkt_lost" in the output file for each packet (or burst) lost 
    if {$debug_>=2} {
	trace_annotate "$self pkt_lost"
    }
    set ns_time [$ns_ now]
    

    #start a new loss cycle. when start_loss is set to -1 we reinitialize the 
    #counter of the number of packets received h_npkts and the number of packets
    #lost h_nlost (that avoid old packets lost to contribute to the actual loss rate)
    if {$time_loss <= $ns_time} {
	if {$debug_>=2} {
	    trace_annotate "not enough losses during 1s: reinitialize"
	}
	set start_loss -1
    }

    #we reinitialize h_npkts and h_nlost each time start_loss=-1 and
    #each time there is a loss whereas we drop a layer less than 500ms apart.
    if {($start_loss == -1) || ($wait_loss >= $ns_time)} {
	if {$debug_>=2} {
	    trace_annotate "$start_loss [expr $wait_loss >= $ns_time] reinitialize"
	}
    	set h_npkts [$self plm_pkts]
	set h_nlost [$self plm_loss]
	set start_loss 1
	#we calculate the loss rate at most on a 5 second interval.
	set time_loss [expr [$ns_ now] + 5]
	if {$debug_>=2} {
	    trace_annotate "time_loss : $time_loss"
	}
    }

    #drop a layer if the loss exceed a threshold and if there was no layer drop 
    #the 500ms preceding.
    if {([$self exceed_loss_thresh]) && ($wait_loss <= $ns_time)} {
	$self drop-layer
	set start_loss -1
	#we cannot drop another layer before 500ms. 500ms is largely enough to avoid 
	#cascade drop due to spurious inference as PLM does not need the bottleneck queue
	#to drain, but just a PP to pass the bottleneck queue.
	set wait_loss [expr $ns_time + 0.5]
	if {$debug_>=2} {
	    trace_annotate "drop layer wait_loss: $wait_loss"
	}
    }
}

#The loss rate is only calculated for more than 10 packets received. The loss
#threshlod is 10%
PLM instproc exceed_loss_thresh {} {
	$self instvar h_npkts h_nlost debug_
	set npkts [expr [$self plm_pkts] - $h_npkts]
	if { $npkts >= 10 } {
		set nloss [expr [$self plm_loss] - $h_nlost]
		#XXX 0.4
		set loss [expr double($nloss) / ($nloss + $npkts)]
		$self debug "H-THRESH $nloss $npkts $loss"
		if { $loss > 0.10 } {
			return 1
		}
	}
	return 0
}


PLM instproc drop-layer {} {
    $self instvar subscription_ layer_ node_id debug_
    set n $subscription_

    #
    # if we have an active layer, drop it
    #
    if { $n > 0 } {
	$self debug "DRP-LAYER $n"
	$layer_($n) leave-group 
	incr n -1
	set subscription_ $n
	if {$debug_>=2} {
	    trace_annotate " [$self set node_id] : change layer $subscription_ "
	}
    }
    
    #rejoin the session after 30 seconds if drop all the layers
    if { $subscription_ == 0 } {
	set ns [Simulator instance]
	set rejoin_timer 30
	$ns at [expr [$ns now] + $rejoin_timer] "$self add-layer"
	if {$debug_>=2} {
	    trace_annotate " Try to re-join the session after dropping all the layers "
	}
    }
}

PLM instproc add-layer {} {
    $self instvar maxlevel_ subscription_ layer_ node_id debug_
    set n $subscription_
    if { $n < $maxlevel_ } {
	$self debug "ADD-LAYER"
	incr n
	set subscription_ $n
	$layer_($n) join-group
	if {$debug_>=2} {
	    trace_annotate " [$self set node_id] : change layer $subscription_ "
	}
    }
}

#
# return the amount of loss across all the groups of the given plm
#
PLM instproc plm_loss {} {
	$self instvar layers_
	set loss 0
	foreach l $layers_ {
		incr loss [$l nlost]
	}
	return $loss
}

#
# return the number of packets received across all the groups of the given plm
#
PLM instproc plm_pkts {} {
	$self instvar layers_
	set npkts 0
	foreach l $layers_ {
		incr npkts [$l npkts]
	}
	return $npkts
}

PLM instproc debug { msg } {
	$self instvar debug_ subscription_ ns_

	if {$debug_ <1} { return }
	set time [format %.05f [$ns_ now]]
	puts stderr "PLM: $time  layer $subscription_ $msg"
}

Class PLMLayer

PLMLayer instproc init { plm } {
	$self next

	$self instvar plm_ npkts_
	set plm_ $plm
	set npkts_ 0
	# loss trace created in constructor of derived class
}

PLMLayer instproc join-group {} {
	$self instvar npkts_ add_time_ plm_
	set npkts_ [$self npkts]
	set add_time_ [$plm_ now]
	# derived class actually joins group
}

PLMLayer instproc leave-group {} {
	# derived class actually leaves group
}

PLMLayer instproc getting-pkts {} {
	$self instvar npkts_
	return [expr [$self npkts] != $npkts_]
}
