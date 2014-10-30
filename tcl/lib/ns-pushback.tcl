# -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
#
# Copyright (c) 2000  International Computer Science Institute
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
#	This product includes software developed by ACIRI, the AT&T 
#      Center for Internet Research at ICSI (the International Computer
#      Science Institute).
# 4. Neither the name of ACIRI nor of ICSI may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-pushback.tcl,v 1.5 2001/01/20 20:48:38 ratul Exp $



Node instproc add-pushback-agent {} {
	$self instvar pushback_
	set pushback_ [new Agent/Pushback]
	[Simulator instance] attach-agent $self $pushback_
	# puts "Pushback Agent = $pushback_"
	$pushback_ initialize $self [[Simulator instance] get-routelogic]
	return $pushback_
}

Node instproc get-pushback-agent {} {
	$self instvar pushback_
	if [info exists pushback_] {
		return $pushback_
	} else {
		return -1
	}
}
	
Simulator instproc pushback-duplex-link {n1 n2 bw delay} {

	$self pushback-simplex-link $n1 $n2 $bw $delay
	$self pushback-simplex-link $n2 $n1 $bw $delay
}

Simulator instproc pushback-simplex-link {n1 n2 bw delay} {
	
	set pba [$n1 get-pushback-agent]
	if {$pba == -1} {
		puts "Node does not have a pushback agent"
		exit
	}
	$self simplex-link $n1 $n2 $bw $delay RED/Pushback $pba
	
	set link [$self link $n1 $n2]
	set qmon [new QueueMonitor/ED]
	$link attach-monitors [new SnoopQueue/In] [new SnoopQueue/Out] [new SnoopQueue/Drop] $qmon
	
	set queue [$link queue]
	$queue set pushbackID_ [$pba add-queue $queue]
	$queue set-monitor $qmon
	$queue set-src-dst [$n1 set id_] [$n2 set id_]

}

Agent/Pushback instproc get-pba-port {nodeid} {

	set node [[Simulator instance] set Node_($nodeid)]
	set pba [$node get-pushback-agent]
	if {$pba == -1} {
		return -1
	} else {
		return [$pba set agent_port_]
	}
}

Agent/Pushback instproc check-queue { src dst qToCheck } {

	set link [[Simulator instance] set link_($src:$dst)]
	set queue [$link queue]
	if {$qToCheck == $queue} {
		return 1
	} else {
		return 0
	}
}
	
Queue/RED/Pushback set pushbackID_ -1
Queue/RED/Pushback set rate_limiting_ 1

Agent/Pushback set last_index_ 0
Agent/Pushback set intResult_ -1
Agent/Pushback set enable_pushback_ 1
Agent/Pushback set verbose_ false

#
# Added to be able to trace the drops in rate-limiter
#
Queue/RED/Pushback instproc attach-traces {src dst file {op ""}} {
	
	$self next $src $dst $file $op
		
		set ns [Simulator instance]
		
#set this up late if you want.
#	set type [$self mon-trace-type]
		
#nam does not understand anything else yet
#	if {$op == "nam"} {
#		set type "Drop"
#	}
		
		set type "Drop"
		set rldrop_trace [$ns create-trace $type $file $src $dst $op]
		
		set oldTrace [$self rldrop-trace]
		if {$oldTrace!=0} {
		puts "exists"
			$rldrop_trace target $oldTrace
			} else {
#	puts "Does not exist"
				$rldrop_trace target [$ns set nullAgent_]
					}
	
	$self rldrop-trace $rldrop_trace
	
}

