# Copyright (C) 1999 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
# Re-tooled version of Polly's web traffic models (tcl/ex/web-traffic.tcl, 
# tcl/ex/large-scale-traffic.tcl) in order to save memory.
#
# The main strategy is to move everything into C++, only leave an OTcl 
# configuration interface. Be very careful as what is configuration and 
# what is functionality.
#
# $Header: /cvsroot/nsnam/ns-2/tcl/webcache/webtraf.tcl,v 1.22 2011/10/01 22:00:14 tom_henderson Exp $

PagePool/WebTraf set debug_ false
PagePool/WebTraf set TCPTYPE_ Reno
PagePool/WebTraf set TCPSINKTYPE_ TCPSink   ;#required for SACK1 Sinks.

#0 for default, fid=id
#1 for increasing along with new agent allocation (as in red-pd). 
#   useful when used with flow monitors (as they tend to run out of space in hash tables)
#   (see red-pd scripts for usage of this)
#2 for assigning the same fid to all connections.
#    useful when want to differentiate between various web traffic generators using flow monitors.
#   (see pushback scripts for usage of this).
PagePool/WebTraf set FID_ASSIGNING_MODE_ 0 
PagePool/WebTraf set VERBOSE_ 0

# Support the reuse of page level attributes to save
#  memory for large simulations
PagePool/WebTraf set recycle_page_ 1

# If set, do not recycle TCP agents
PagePool/WebTraf set dont_recycle_ 0

# To use fullTCP (xuanc), we need to:
# 1. set the flag fulltcp_ to 1
# 2. set TCPTYPE_ FullTcp
PagePool/WebTraf set fulltcp_ 0

# Trace web request and response flows
PagePool/WebTraf set req_trace_ 0
PagePool/WebTraf set resp_trace_ 0

# Timer for application level control of connection
PagePool/WebTraf set enable_conn_timer_ 0
# Default value for end-users' average waiting time
PagePool/WebTraf set avg_waiting_time_ 30

# options to control traffic based on flow size, 
# used to evaluate SFD algorithm.
# The threshold to classify short and long flows (in TCP packets, ie 15KB)
PagePool/WebTraf set FLOW_SIZE_TH_ 15
# Option to modify traffic mix:
# 0: original settings without change
# 1: Allow only short flows
# 2: Chop long flows to short ones.
PagePool/WebTraf set FLOW_SIZE_OPS_ 0

PagePool/WebTraf instproc launch-req { id pid clnt svr ctcp csnk size} {
    $self instvar timer_

    set launch_req 1
    set flow_th [PagePool/WebTraf set FLOW_SIZE_TH_]

    if {[PagePool/WebTraf set FLOW_SIZE_OPS_] == 1 && $size > $flow_th} {
	set launch_req 0
    }

    if {$launch_req == 1} {
	set ns [Simulator instance]
	
	# create request connection from client to server
	$ns attach-agent $clnt $ctcp
	$ns attach-agent $svr $csnk
	$ns connect $ctcp $csnk
	
	# sink needs to listen for fulltcp
	if {[PagePool/WebTraf set fulltcp_] == 1} {
	    $csnk listen
	}

	if {[PagePool/WebTraf set FID_ASSIGNING_MODE_] == 0} {
	    $ctcp set fid_ $id

	    if {[PagePool/WebTraf set fulltcp_] == 1} {
		$csnk set fid_ $id
	    }
	}

	set timer_ [$self get-conn-timer $ctcp $csnk $clnt $svr]
	
	
	# define call-back function for request TCP
	$ctcp proc done {} "$self done-req $id $pid $clnt $svr $ctcp $csnk $size $timer_"
	
	# Trace web traffic flows (send request: client==>server).
	if {[PagePool/WebTraf set req_trace_]} {	
	    puts "req + $id $pid $size [$clnt id] [$svr id] [$ns now]"
	}	
	
	# Send a single packet as a request
	$self send-message $ctcp 1
    }

}

PagePool/WebTraf instproc done-req { id pid clnt svr ctcp csnk size timer} {
    # Cancel the connection timer if any
    if {[PagePool/WebTraf set enable_conn_timer_]} {
	$timer cancel
	delete $timer
    }

    set ns [Simulator instance]
    
    # Recycle client-side TCP agents
    $ns detach-agent $clnt $ctcp
    $ns detach-agent $svr $csnk
    $ctcp reset
    $csnk reset
    # do NOT recycle tcp & sink, reuse for response connection

    # request has completed successfully, now pass the request to web server
    # notify web server about the request
    set delay [$self job_arrival $id $clnt $svr $ctcp $csnk $size $pid]
    #$self launch-resp $id $pid $svr $clnt $ctcp $csnk $size

    # this job is actually dropped due to server's queue limit
    if {$delay == 0} {
	# Trace web traffic flows (recv request: client==>server).
	# Request has been rejected by server due to its capacity
	if {[PagePool/WebTraf set req_trace_]} {	
	    puts "req d $id $pid $size [$clnt id] [$svr id] [$ns now]"
	}
	# Recycle TCP agents
	$self recycle $ctcp $csnk	
	# is there any difference for rejected requests?
	$self doneObj $pid
    } else {
	# Trace web traffic flows (recv request: client==>server).
	# Request has been received by server successfully
	if {[PagePool/WebTraf set req_trace_]} {	
	    puts "req - $id $pid $size [$clnt id] [$svr id] [$ns now]"
	}
    }
    
}

PagePool/WebTraf instproc launch-resp { id pid svr clnt stcp ssnk size} {
    set flow_th [PagePool/WebTraf set FLOW_SIZE_TH_]

    set ns [Simulator instance]

    # create response connection from server to client
    $ns attach-agent $svr $stcp
    $ns attach-agent $clnt $ssnk
    $ns connect $stcp $ssnk
	
    # sink needs to listen for fulltcp
    if {[PagePool/WebTraf set fulltcp_] == 1} {
	$ssnk listen
    }

    if {[PagePool/WebTraf set FID_ASSIGNING_MODE_] == 0} {
	$stcp set fid_ $id
	
	if {[PagePool/WebTraf set fulltcp_] == 1} {
	    $ssnk set fid_ $id
	}
    }
    
    if {[PagePool/WebTraf set FLOW_SIZE_OPS_] == 2 && $size > $flow_th} {
	set sent $flow_th
    } else {
	set sent $size
    }

    # define callback function for response tcp
    $stcp proc done {} "$self done-resp $id $pid $clnt $svr $stcp $ssnk $size $sent $flow_th [$ns now] [$stcp set fid_]"
    
    # Trace web traffic flows (send responese: server->client).
    if {[PagePool/WebTraf set resp_trace_]} {
	puts "resp + $id $pid $sent $size [$svr id] [$clnt id] [$ns now]"
    }
    # Send a single packet as a request
    $self send-message $stcp $sent
}

PagePool/WebTraf instproc done-resp { id pid clnt svr stcp ssnk size sent sent_th {startTime 0} {fid 0} } {
    set ns [Simulator instance]
    
    # modified to trace web traffic flows (recv responese: server->client).
    if {[PagePool/WebTraf set resp_trace_]} {
	puts "resp - $id $pid $sent $size [$svr id] [$clnt id] [$ns now]"
    }
    
    if {[PagePool/WebTraf set VERBOSE_] == 1} {
	puts "done-resp - $id [$svr id] [$clnt id] $size $startTime [$ns now] $fid"
    }
    
    # Reset server-side TCP agents
    $stcp reset
    $ssnk reset
    $ns detach-agent $clnt $ssnk
    $ns detach-agent $svr $stcp

    # if there are some packets left, keeps on sending.
    if {$sent < $size} {
	$ns attach-agent $svr $stcp
	$ns attach-agent $clnt $ssnk
	$ns connect $stcp $ssnk

	if {[PagePool/WebTraf set fulltcp_] == 1} {
	    $ssnk listen
	}

	if {[PagePool/WebTraf set FID_ASSIGNING_MODE_] == 0} {
	    $stcp set fid_ $id

	    if {[PagePool/WebTraf set fulltcp_] == 1} {
		$ssnk set fid_ $id
	    }
	}

	set left [expr $size - $sent]
	if {$left <= $sent_th} {
	    # modified to trace web traffic flows (send responese: server->client).
	    if {[PagePool/WebTraf set resp_trace_]} {
		puts "resp + $id $pid $left $size [$svr id] [$clnt id] [$ns now]"
	    }
	    set sent [expr $sent + $left]
	    $stcp proc done {} "$self done-resp $id $pid $clnt $svr $stcp $ssnk $size $sent $sent_th [$ns now] [$stcp set fid_]"

	    $self send-message $stcp $left
	} else {
	    # modified to trace web traffic flows (send responese: server->client).
	    if {[PagePool/WebTraf set resp_trace_]} {
		puts "resp + $id $pid $sent_th $size [$svr id] [$clnt id] [$ns now]"
	    }
	    set sent [expr $sent + $sent_th]
	    $stcp proc done {} "$self done-resp $id $pid $clnt $svr $stcp $ssnk $size $sent $sent_th [$ns now] [$stcp set fid_]"

	    $self send-message $stcp $sent_th
	}
    } else {
	# Recycle TCP agents
	$self recycle $stcp $ssnk	
	$self doneObj $pid
    }
}

# XXX Should allow customizable TCP types. Can be easily done via a 
# class variable
PagePool/WebTraf instproc alloc-tcp {} {
    set tcp [new Agent/TCP/[PagePool/WebTraf set TCPTYPE_]]
    
    set fidMode [PagePool/WebTraf set FID_ASSIGNING_MODE_]
    if {$fidMode == 1} {
	$self instvar maxFid_
	$tcp set fid_ $maxFid_
	incr maxFid_
    } elseif  {$fidMode == 2} {
	$self instvar sameFid_
	$tcp set fid_ $sameFid_
    }
    
    return $tcp
}

PagePool/WebTraf instproc alloc-tcp-sink {} {
    return [new Agent/[PagePool/WebTraf set TCPSINKTYPE_]]
}

PagePool/WebTraf instproc send-message {tcp num_packet} {
    #puts "send message: [[Simulator instance] now], $num_packet"
    if {[PagePool/WebTraf set fulltcp_] == 1} {
	# for fulltcp: need to use flag
	$tcp sendmsg [expr $num_packet * 1000] "MSG_EOF"
    } else {
	#Advance $num_packet packets: for one-way tcp
	$tcp advanceby $num_packet
    }
}

# Debo

#PagePool/WebTraf instproc create-session { args } {
#
#    set ns [Simulator instance]
#    set asimflag [$ns set useasim_]
#    #puts "Here"
#    $self use-asim
#    $self cmd create-session $args
#
#} 

PagePool/WebTraf instproc  add2asim { srcid dstid lambda mu } {

    set sf_ [[Simulator instance] set sflows_]
    set nsf_ [[Simulator instance] set nsflows_]
    
    lappend sf_ $srcid:$dstid:$lambda:$mu
    incr nsf_

    [Simulator instance] set sflows_ $sf_
    [Simulator instance] set nsflows_ $nsf_

    #puts "setup short flow .. now sflows_ = $sf_"

}

# Set a timer for connection duration
PagePool/WebTraf instproc get-conn-timer { tcp snk clnt svr } {
    if {[PagePool/WebTraf set enable_conn_timer_]} {
	set timer_ [new ConnTimer $self [PagePool/WebTraf set avg_waiting_time_]]
	$timer_ sched $tcp $snk $clnt $svr
    } else {
	set timer_ 0
    }

    return $timer_
}

# Timer for Application level retransmission
Class ConnTimer -superclass Timer

ConnTimer instproc init {webtraf delay} {
     $self instvar webtraf_ avg_delay_

     $self set webtraf_ $webtraf
     $self set avg_delay_ $delay
 
     $self next [Simulator instance]
 }
 
 ConnTimer instproc sched {tcp snk n1 n2} {
     $self instvar tcp_ snk_ n1_ n2_ avg_delay_
 
     $self set tcp_ $tcp
     $self set snk_ $snk
     $self set n1_ $n1
     $self set n2_ $n2
 
     set waiting_time [new RandomVariable/Exponential]
     $waiting_time set avg_ $avg_delay_
     set delay [$waiting_time value]
     puts "delay: $delay"
     delete $waiting_time

     $self next $delay
 }
 
 ConnTimer instproc timeout {} {
     $self instvar webtraf_
     $self instvar n1_ n2_ tcp_ snk_

     #puts "timeout!!"

     set v [new RandomVariable/Uniform]
     set p [$v value]
     #puts "p: $p"
     delete $v

     if {$p > 0.5} {
	 # continue to wait
	 $self sched $tcp_ $snk_ $n1_ $n2_
     } else {
	 # terminate request
	 set ns [Simulator instance] 
	 
	 $ns detach-agent $n1_ $tcp_
	 $ns detach-agent $n2_ $snk_
	 $tcp_ reset
	 $snk_ reset
	 $webtraf_ recycle $tcp_ $snk_
	 
	 delete $self
     }
 }

