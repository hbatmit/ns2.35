
#
# Copyright (c) 1995 The Regents of the University of California.
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
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
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

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP RTP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:

Simulator instproc get-link { node1 node2 } {
    $self instvar link_
    set id1 [$node1 id]
    set id2 [$node2 id]
    return $link_($id1:$id2)
}

#Simple SRR Test-suite 
proc set_queue {} {
	global q 
	$q maxqueuenumber 256
	$q mtu 250
	$q granularity  10000
#	$q qlimit	30
#	$q maxqueuelen  20

	$q setqueue 1 0 
	$q setqueue 2 1 
	$q setqueue 3 2 
	$q setqueue 4 3 

	$q setrate 0 80000
	$q setrate 1 40000
	$q setrate 2 20000
	$q setrate 3 10000
}

# create 4 cbr sources 
proc create_traffic {} {

	global ns lm0 n0 n1
	for {set i 0} {$i < 4} { incr i} {
		global udp$i cbr$i
	}

	
	for {set i 0} {$i < 4} { incr i} {
		set agt [new Agent/UDP]
		set app [new Application/Traffic/CBR]
	
		$agt set fid_ [expr $i + 1 ] 
		$ns attach-agent $n0 $agt
		$app attach-agent $agt

		$app set packetSize_ 250
		$app set rate_ 100kb
		$app set random_ 0 

		set udp$i $agt
		set cbr$i $app
	}


	# receiver 0 :
	set lm0 [new Agent/Null]
	$ns attach-agent $n1 $lm0

	for {set i 0} {$i < 4} { incr i} {
		global udp$i
		set agent [set udp$i]
		$ns connect $agent $lm0
	}
}


proc start_sim {} {
	global ns cbr0 cbr1 cbr2 cbr3
	
	$ns at 0.0 "$cbr0 start"
	$ns at 0.0 "$cbr1 start"
	$ns at 0.0 "$cbr2 start"
	$ns at 0.0 "$cbr3 start"

}

proc stop_sim {} {
	global ns f cbr0 cbr1 cbr2 cbr3
	$ns at 5.0 "$cbr0 stop"
	$ns at 5.0 "$cbr1 stop"
	$ns at 5.0 "$cbr2 stop"
	$ns at 5.0 "$cbr3 stop"

	$ns at 5.0 "close $f;finish"
}


proc finish {} {
    puts "processing output ..."
    exec awk  { 
	{
	    n[$8]=$8; \
	    if ($1=="-") r[$8]++; \
	    if ($1=="+") s[$8]++; \
	    if ($1=="d") d[$8]++ \
	    }
	END \
	    { 
		printf ("Flow#\t#sent\t#recvd\t#drop\t%%recvd\n"); \
		    for (i in r) \
		    printf ("%d\t%d\t%d\t%d\t%f\n",n[i],s[i],r[i],d[i],r[i]*1.0/s[i])\
		}
    } temp.rands > srr_out.txt
#    exec cat srr_out.txt &
    exit 0
}
proc test_srr {} {
	global  ns n0 n1 f l q argv

	set ns [new Simulator]
	set xx0 [$ns node]
	set xy0 [$ns node]
	set n0 [$ns node]
	set n1 [$ns node]
	set f [open temp.rands w]


	$ns duplex-link $n0 $n1 150kb 1ms SRR

	# trace the bottleneck queue 
	$ns trace-queue $n0 $n1 $f


	#Alternate way for setting parameters for the SRR queue
	set l [$ns get-link $n0 $n1]
	set q [$l queue]

	create_traffic
	set_queue
	start_sim 
	stop_sim
	$ns run
}

global argv arg0

test_srr 

