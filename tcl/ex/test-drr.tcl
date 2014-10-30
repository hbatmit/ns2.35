#
# Copyright (c) Xerox Corporation 1997. All rights reserved.
#  
# License is granted to copy, to use, and to make and to use derivative
# works for research and evaluation purposes, provided that Xerox is
# acknowledged in all documentation pertaining to any such copy or derivative
# work. Xerox grants no other licenses expressed or implied. The Xerox trade
# name should not be used in any advertising without its written permission.
#  
# XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
# MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
# FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
# express or implied warranty of any kind.
#  
# These notices must be retained in any copies of any part of this software.
#
# This file contributed by Sandeep Bajaj <bajaj@parc.xerox.com>, Mar 1997.
#

# This script is a simple test for testing DRR functionality
# This script creates a simple topology and places 6 CBR sources of different
# rates. Finally it prints some stats to indicate the share of the bottleneck 
# link that each source gets.
# The flows can be discriminated by each individual source or by each 
# node (using the setmask command line option) 

# Run this script as
# ../../ns test-drr.tcl 
# or
# ../../ns test-drr.tcl setmask 
# After running this script cat "out" to view the flow stats

#default Mask parameter
set MASK 0

if { [lindex $argv 0] == "setmask" } {
    set MASK 1
    puts stderr "Ignoring port nos of sources"
}

set ns [new Simulator]


# Create a simple four node topology:
#
#	   n0
#	     \ 
# 10Mb,0.01ms \   1Mb,0.01ms
#	        n2 --------- n3
# 10Mb,0.01ms /
#	     /
#	   n1

set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

set f [open out.tr w]

#$ns trace-all $f

$ns duplex-link $n0 $n1 1.0Mb 0.01ms DRR 
$ns duplex-link $n2 $n0 10.0Mb 0.01ms DRR 
$ns duplex-link $n3 $n0 10.0Mb 0.01ms DRR 

# trace the bottleneck queue 
$ns trace-queue $n0 $n1 $f

Simulator instproc get-link { node1 node2 } {
    $self instvar link_
    set id1 [$node1 id]
    set id2 [$node2 id]
    return $link_($id1:$id2)
}

#Alternate way for setting parameters for the DRR queue
set l [$ns get-link $n0 $n1]
set q [$l queue]

$q mask $MASK
$q blimit 25000
$q quantum 500
$q buckets 7

# create 6 cbr sources :
#source 0 :
set udp0 [new Agent/UDP]
$ns attach-agent $n2 $udp0
set cbr0 [new Application/Traffic/CBR]
$cbr0 attach-agent $udp0
$cbr0 set packetSize_ 250
$cbr0 set interval_ 20ms
$cbr0 set random_ 1

#source 1 :
set udp1 [new Agent/UDP]
$ns attach-agent $n2 $udp1
set cbr1 [new Application/Traffic/CBR]
$cbr1 attach-agent $udp1
$cbr1 set packetSize_ 250
$cbr1 set interval_ 4ms
$cbr1 set random_ 1

#source 2:
set udp2 [new Agent/UDP]
$ns attach-agent $n2 $udp2
set cbr2 [new Application/Traffic/CBR]
$cbr2 attach-agent $udp2
$cbr2 set packetSize_ 250
$cbr2 set interval_ 2.5ms
$cbr2 set random_ 1

#source 3 :
set udp3 [new Agent/UDP]
$ns attach-agent $n3 $udp3
set cbr3 [new Application/Traffic/CBR]
$cbr3 attach-agent $udp3
$cbr3 set packetSize_ 250
$cbr3 set interval_ 2.5ms
$cbr3 set random_ 1

#source 4
set udp4 [new Agent/UDP]
$ns attach-agent $n3 $udp4
set cbr4 [new Application/Traffic/CBR]
$cbr4 attach-agent $udp4
$cbr4 set packetSize_ 250
$cbr4 set interval_ 4ms
$cbr4 set random_ 1

#source 5
set udp5 [new Agent/UDP]
$ns attach-agent $n3 $udp5
set cbr5 [new Application/Traffic/CBR]
$cbr5 attach-agent $udp5
$cbr5 set packetSize_ 250
$cbr5 set interval_ 20ms
$cbr5 set random_ 1

# receiver 0 :
set lm0 [new Agent/Null]
$ns attach-agent $n1 $lm0

$ns connect $udp0 $lm0
$ns connect $udp1 $lm0
$ns connect $udp2 $lm0
$ns connect $udp3 $lm0
$ns connect $udp4 $lm0
$ns connect $udp5 $lm0

$ns at 0.0 "$cbr0 start"
$ns at 0.0  "$cbr1 start"
$ns at 0.0  "$cbr2 start"
$ns at 0.0  "$cbr3 start"
$ns at 0.0  "$cbr4 start"
$ns at 0.0  "$cbr5 start"


$ns at 2.0 "$cbr0 stop"
$ns at 2.0 "$cbr1 stop"
$ns at 2.0  "$cbr2 stop"
$ns at 2.0  "$cbr3 stop"
$ns at 2.0  "$cbr4 stop"
$ns at 2.0  "$cbr5 stop"


$ns at 3.0 "close $f;finish"


proc finish {} {
    puts "processing output ..."
    exec awk  { 
	{
	    n[$9]=$9; \
	    if ($1=="-") r[$9]++; \
	    if ($1=="+") s[$9]++; \
	    if ($1=="d") d[$9]++ \
	    }
	END \
	    { 
		printf ("Flow#\t#sent\t#recvd\t#drop\t%%recvd\n"); \
		    for (i in r) \
		    printf ("%1.1f\t%d\t%d\t%d\t%f\n",n[i],s[i],r[i],d[i],r[i]*1.0/s[i])\
		}
    } out.tr > out
    exec cat out &
    exit 0
}

$ns run

