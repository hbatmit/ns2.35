#
# Copyright (c) 1998 University of Southern California.
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

#
# ****** Note: please keep this test-suite sync'ed with the example scripts 
#              provided in the tutorial. Thanks.
#
# This test suite is for validating all the examples in Marc Greis tutorial
# that's now on the ns web pages at http://www.isi.edu/nsnam/ns/tutorial/index.html
#
# To run all tests: test-all-greis
# to run individual test:
# ns test-suite-greis.tcl example1
# ns test-suite-greis.tcl example1a
# ....
#
# To view a list of available test to run with this script:
# ns test-suite-greis.tcl
#
#remove-all-packet-headers       ; # removes all except common
#add-packet-header Flags IP RTP TCP  ; # hdrs reqd for validation test

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

Class TestSuite

Class Test/example1 -superclass TestSuite
Class Test/example1a -superclass TestSuite
Class Test/example1b -superclass TestSuite
Class Test/example2 -superclass TestSuite
Class Test/example3 -superclass TestSuite
Class Test/example4 -superclass TestSuite
Class Test/ping -superclass TestSuite 
Class Test/pingOneWay -superclass TestSuite

proc usage {} {
    global argv0
    puts stderr "usage: ns $argv0 <tests> "
    puts "Valid Tests: example1 example1a example1b example2 \
                       example3 example4 ping pingOneWay"
    exit 1
}

#example1.tcl
Test/example1 instproc init {} {
    $self instvar ns testName nf
    set testName example1

    #create a simulator object
    set ns [new Simulator]

    #open a file for writing that is used for nam trace data
    set nf [open temp.rands w]
    $ns namtrace-all $nf
}

#finish procedure that closes the trace file and starts nam
Test/example1 instproc finish {} {
    global quiet
    $self instvar ns nf
    $ns flush-trace
    close $nf
    if {$quiet == "false"} { 
	puts "finishing.." 
    }
    exit 0
}

Test/example1 instproc run {} {
    $self instvar ns 

    #execute the "finish" procedure after 5.0 seconds of simulation time
    $ns at 5.0 "$self finish"

    #start the simulation
    $ns run
}

#example1a.tcl
Test/example1a instproc init {} {
    $self instvar ns testName nf
    set testName example1a

    #Create a simulator object
    set ns [new Simulator]

    #Open the nam trace file
    set nf [open temp.rands w]
    $ns namtrace-all $nf
}

#Define a 'finish' procedure
Test/example1a instproc finish {} {
        global quiet
        $self instvar ns nf
        $ns flush-trace
	#Close the trace file
        close $nf
	#Execute nam on the trace file
        if {$quiet == "false"} { 
	puts "finishing.."
	}
        exit 0
}


Test/example1a instproc run {} {
    $self instvar ns

    # Insert your own code for topology creation
    # and agent definitions, etc. here
    set n0 [$ns node]
    set n1 [$ns node]

    $ns duplex-link $n0 $n1 1Mb 10ms DropTail

    #Call the finish procedure after 5 seconds simulation time
    $ns at 5.0 "$self finish"

    #Run the simulation
    $ns run
}

#example1b.tcl
Test/example1b instproc init {} {
    $self instvar ns testName nf
    set testName example1b
    #Create a simulator object
    set ns [new Simulator]
    
    #Open the nam trace file
    set nf [open temp.rands w]
    $ns namtrace-all $nf
}

#Define a 'finish' procedure
Test/example1b instproc finish {} {
        global quiet
        $self instvar ns nf
        $ns flush-trace
	#Close the trace file
        close $nf
	#Execute nam on the trace file
        if {$quiet == "false"} { 
	    puts "finishing.." 
	}
        exit 0
}

Test/example1b instproc run {} {
    $self instvar ns
    #Create two nodes
    set n0 [$ns node]
    set n1 [$ns node]

    #Create a duplex link between the nodes
    $ns duplex-link $n0 $n1 1Mb 10ms DropTail

    #Create a CBR agent and attach it to node n0
    set udp0 [new Agent/UDP]
    $ns attach-agent $n0 $udp0
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 set packetSize_ 500
    $cbr0 set interval_ 0.005
    $cbr0 attach-agent $udp0

    #Create a Null agent (a traffic sink) and attach it to node n1
    set null0 [new Agent/Null]
    $ns attach-agent $n1 $null0
    
    #Connect the traffic source with the traffic sink
    $ns connect $udp0 $null0  

    #Schedule events for the CBR traffic generator
    $ns at 0.5 "$cbr0 start"
    $ns at 4.5 "$cbr0 stop"

    #Call the finish procedure after 5 seconds of simulation time
    $ns at 5.0 "$self finish"

    #Run the simulation
    $ns run
}

#example2.tcl
Test/example2 instproc init {} {
    $self instvar ns testName nf
    set testName example2

    #Create a simulator object
    set ns [new Simulator]

    #Define different colors for data flows
    $ns color 1 Blue
    $ns color 2 Red

    #Open the nam trace file
    set nf [open temp.rands w]
    $ns namtrace-all $nf
}

#Define a 'finish' procedure
Test/example2 instproc finish {} {
        global quiet
        $self instvar ns nf
        $ns flush-trace
	#Close the trace file
        close $nf
	#Execute nam on the trace file
        if {$quiet == "false"} { puts "finishing.." }
        exit 0
}

Test/example2 instproc run {} {
    $self instvar ns

    #Create four nodes
    set n0 [$ns node]
    set n1 [$ns node]
    set n2 [$ns node]
    set n3 [$ns node]

    #Create links between the nodes
    $ns duplex-link $n0 $n2 1Mb 10ms DropTail
    $ns duplex-link $n1 $n2 1Mb 10ms DropTail
    $ns duplex-link $n3 $n2 1Mb 10ms SFQ

    $ns duplex-link-op $n0 $n2 orient right-down
    $ns duplex-link-op $n1 $n2 orient right-up
    $ns duplex-link-op $n2 $n3 orient right

    #Monitor the queue for the link between node 2 and node 3
    $ns duplex-link-op $n2 $n3 queuePos 0.5

    #Create a UDP agent with CBR traffic and attach it to node n0
    set udp0 [new Agent/UDP]
    $ns attach-agent $n0 $udp0
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 set packetSize_ 500
    $cbr0 set interval_ 0.005
    $cbr0 set fid_ 0
    $cbr0 attach-agent $udp0

    #Create a UDP agent with CBR traffic and attach it to node n1
    set udp1 [new Agent/UDP]
    $ns attach-agent $n1 $udp1
    set cbr1 [new Application/Traffic/CBR]
    $cbr1 set packetSize_ 500
    $cbr1 set interval_ 0.005
    $cbr1 set fid_ 2
    $cbr1 attach-agent $udp1

    #Create a Null agent (a traffic sink) and attach it to node n3
    set null0 [new Agent/Null]
    $ns attach-agent $n3 $null0

    #Connect the traffic sources with the traffic sink
    $ns connect $udp0 $null0  
    $ns connect $udp1 $null0

    #Schedule events for the CBR traffic generators
    $ns at 0.5 "$cbr0 start"
    $ns at 1.0 "$cbr1 start"
    $ns at 4.0 "$cbr1 stop"
    $ns at 4.5 "$cbr0 stop"

    #Call the finish procedure after 5 seconds of simulation time
    $ns at 5.0 "$self finish"

    #Run the simulation
    $ns run
}

#example3.tcl
Test/example3 instproc init {} {
    $self instvar ns testName nf 
    set testName example3

    #Create a simulator object
    set ns [new Simulator]

    #Tell the simulator to use dynamic routing
    $ns rtproto DV

    #Open the nam trace file
    set nf [open temp.rands w]
    $ns namtrace-all $nf
}

#Define a 'finish' procedure
Test/example3 instproc finish {} {
        global quiet
        $self instvar ns nf
        $ns flush-trace
	#Close the trace file
        close $nf
	#Execute nam on the trace file
        if {$quiet == "false"} { 
	    puts "finishing.." 
	}
        exit 0
}

Test/example3 instproc run {} {
    $self instvar ns n

    #Create seven nodes
    for {set i 0} {$i < 7} {incr i} {
        set n($i) [$ns node]
    }

    #Create links between the nodes
    for {set i 0} {$i < 7} {incr i} {
        $ns duplex-link $n($i) $n([expr ($i+1)%7]) 1Mb 10ms DropTail
    }

    #Create a UDP agent with CBR traffic and attach it to node n(0)
    set udp0 [new Agent/UDP]
    $ns attach-agent $n(0) $udp0
    set cbr0 [new Application/Traffic/CBR]
    $cbr0 set packetSize_ 500
    $cbr0 set interval_ 0.005
    $cbr0 attach-agent $udp0

    #Create a Null agent (a traffic sink) and attach it to node n(3)
    set null0 [new Agent/Null]
    $ns attach-agent $n(3) $null0

    #Connect the traffic source with the traffic sink
    $ns connect $udp0 $null0  

    #Schedule events for the CBR agent and the network dynamics
    $ns at 0.5 "$cbr0 start"
    $ns rtmodel-at 1.0 down $n(1) $n(2)
    $ns rtmodel-at 2.0 up $n(1) $n(2)
    $ns at 4.5 "$cbr0 stop"

    #Call the finish procedure after 5 seconds of simulation time
    $ns at 5.0 "$self finish"

    #Run the simulation
    $ns run
}

#example4.tcl
Test/example4 instproc init {} {
    global f0 f1 f2
    $self instvar ns testName 
    set testName example4

    #Create a simulator object
    set ns [new Simulator]

    exec rm -f temp.rands

    #Open the output files
    set f0 [open out0.tr w]
    set f1 [open out1.tr w]
    set f2 [open out2.tr w]
}

#Define a 'finish' procedure
Test/example4 instproc finish {} {
    global f0 f1 f2 quiet

    #Close the output files
    puts $f0 " "
    puts $f1 " "
    puts $f2 " "
    close $f0
    close $f1
    close $f2

    exec cat out0.tr out1.tr out2.tr > temp.rands
    exec rm -f out0.tr out1.tr out2.tr

    #Call xgraph to display the results
     if {$quiet == "false"} {
        exec xgraph -bb -tk temp.rands &
    }

    exit 0
}


#Define a procedure that attaches a UDP agent to a previously created node
#'node' and attaches an Expoo traffic generator to the agent with the
#characteristic values 'size' for packet size 'burst' for burst time,
#'idle' for idle time and 'rate' for burst peak rate. The procedure connects
#the source with the previously defined traffic sink 'sink' and returns the
#source object.
proc attach-expoo-traffic { node sink size burst idle rate } {
        #Get an instance of the simulator
        set ns [Simulator instance]

	#Create a UDP agent and attach it to the node
	set source [new Agent/UDP]
	$ns attach-agent $node $source
	#Create an Expoo traffic agent and set its configuration parameters
	set traffic [new Application/Traffic/Exponential]
	$traffic set packetSize_ $size
	$traffic set burst_time_ $burst
	$traffic set idle_time_ $idle
	$traffic set rate_ $rate
	#Attach the traffic source  to the traffic generator
	$traffic attach-agent $source
	#Connect the source and the sink
	$ns connect $source $sink
	return $traffic
}

#Define a procedure which periodically records the bandwidth received by the
#three traffic sinks sink0/1/2 and writes it to the three files f0/1/2.
Test/example4 instproc record {} {
    global sink0 sink1 sink2 f0 f1 f2
    $self instvar ns

    #Set the time after which the procedure should be called again
     set time 0.5
	
    #How many bytes have been received by the traffic sinks?
     set bw0 [$sink0 set bytes_]
     set bw1 [$sink1 set bytes_]
     set bw2 [$sink2 set bytes_]
	
    #Get the current time
     set now [$ns now]
	
    #Calculate the bandwidth (in MBit/s) and write it to the files
     puts $f0 "$now [expr $bw0/$time*8/1000000]"
     puts $f1 "$now [expr $bw1/$time*8/1000000]"
     puts $f2 "$now [expr $bw2/$time*8/1000000]"
	
    #Reset the bytes_ values on the traffic sinks
     $sink0 set bytes_ 0
     $sink1 set bytes_ 0
     
     $sink2 set bytes_ 0
     #Re-schedule the procedure
     $ns at [expr $now+$time] "$self record"
}


Test/example4 instproc run {} {
    global sink0 sink1 sink2 f0 f1 f2 source0 source1 source2
    $self instvar ns 

    #Create 5 nodes
    set n0 [$ns node]
    set n1 [$ns node]
    set n2 [$ns node]
    set n3 [$ns node]
    set n4 [$ns node]

    #Connect the nodes
    $ns duplex-link $n0 $n3 1Mb 100ms DropTail
    $ns duplex-link $n1 $n3 1Mb 100ms DropTail
    $ns duplex-link $n2 $n3 1Mb 100ms DropTail
    $ns duplex-link $n3 $n4 1Mb 100ms DropTail

    #Create three traffic sinks and attach them to the node n4
    set sink0 [new Agent/LossMonitor]
    set sink1 [new Agent/LossMonitor]
    set sink2 [new Agent/LossMonitor]
    $ns attach-agent $n4 $sink0
    $ns attach-agent $n4 $sink1
    $ns attach-agent $n4 $sink2

    #Create three traffic sources
    set source0 [attach-expoo-traffic $n0 $sink0 200 2s 1s 100k]
    set source1 [attach-expoo-traffic $n1 $sink1 200 2s 1s 200k]
    set source2 [attach-expoo-traffic $n2 $sink2 200 2s 1s 300k]

    #Start logging the received bandwidth
    $ns at 0.0 "$self record"

    #Start the traffic sources
    $ns at 10.0 "$source0 start"
    $ns at 10.0 "$source1 start"
    $ns at 10.0 "$source2 start"

    #Stop the traffic sources
    $ns at 50.0 "$source0 stop"
    $ns at 50.0 "$source1 stop"
    $ns at 50.0 "$source2 stop"

    #Call the finish procedure after 60 seconds simulation time
    $ns at 60.0 "$self finish"

    #Run the simulation
    $ns run
}

Test/ping instproc init {} {
    $self instvar ns testName nf
    set testName ping

    #Create a simulator object
    set ns [new Simulator]

    #Open the nam trace file
    set nf [open temp.rands w]
    $ns namtrace-all $nf
}    

Test/ping instproc finish {} {
        global quiet
        $self instvar ns nf
        $ns flush-trace
        #Close the trace file
        close $nf
        #Execute nam on the trace file
        if {$quiet == "false"} {
            puts "finishing.."
        }
        exit 0
}       

Test/ping instproc run {} {
    $self instvar ns
    #Create three nodes
    set n0 [$ns node] 
    set n1 [$ns node]
    set n2 [$ns node]

    #Connect the nodes with two links
    $ns duplex-link $n0 $n1 1Mb 10ms DropTail
    $ns duplex-link $n1 $n2 1Mb 10ms DropTail

    #Define a 'recv' function for the class 'Agent/Ping'
    Agent/Ping instproc recv {from rtt} {
        $self instvar node_
        puts "node [$node_ id] received ping answer from \
              $from with round-trip-time $rtt ms."
    }

    #Create two ping agents and attach them to the nodes n0 and n2
    set p0 [new Agent/Ping]
    $ns attach-agent $n0 $p0
   
    set p1 [new Agent/Ping]
    $ns attach-agent $n2 $p1

    #Connect the two agents
    $ns connect $p0 $p1

    #Schedule events
    $ns at 0.2 "$p0 send"
    $ns at 0.4 "$p1 send"
    $ns at 0.6 "$p0 send"
    $ns at 0.6 "$p1 send"
    $ns at 1.0 "$self finish"

    #Run the simulation
    $ns run  
}

Test/pingOneWay instproc init {} [Test/ping info instbody init]
Test/pingOneWay instproc finish {} [Test/ping info instbody finish]

Test/pingOneWay instproc run {} {
    $self instvar ns
    #Create three nodes
    set n0 [$ns node] 
    set n1 [$ns node]
    set n2 [$ns node]

    #Connect the nodes with two links
    $ns simplex-link $n1 $n0 1Mb 10ms DropTail
    $ns simplex-link $n0 $n1 1Mb 100ms DropTail
    $ns duplex-link $n1 $n2 0.01Mb 10ms DropTail

    #Define a 'recv' function for the class 'Agent/Ping'
    Agent/Ping instproc recv {from seq ff bf} {
        $self instvar node_
        global rttf
        puts "ping reply from node $from to [$node_ id] with seqno\
              $seq: forward $ff ms, reverse $bf ms."
    }

    #Create two ping agents and attach them to the nodes n0 and n2
    set p0 [new Agent/Ping]
    $ns attach-agent $n0 $p0
    $p0 oneway
   
    set p1 [new Agent/Ping]
    $ns attach-agent $n2 $p1
    $p1 oneway

    #Connect the two agents
    $ns connect $p0 $p1

    #Schedule events
    $ns at 0.2 "$p0 send"
    $ns at 0.4 "$p1 send"
    $ns at 0.6 "$p0 send"
    $ns at 0.6 "$p1 send"
    $ns at 0.61 "$p0 send"
    $ns at 0.61 "$p1 send"
    $ns at 1.0 "$self finish"

    #Run the simulation
    $ns run  
}

proc runtest {arg} {
    global quiet
    set quiet false
    set b [llength $arg]
    if {$b == 1} {
	set test $arg
    } elseif {$b == 2} {
	set test [lindex $arg 0]
	set second [lindex $arg 1]
	if {$second == "QUIET" || $second == "quiet"} {
		set quiet true
	}
    } else {
	usage
    }
    if [catch {set t [new Test/$test]} result] {
	puts stderr $result
	puts "Error: Unknown test:$test"
	usage
	exit 1
    }
    $t run
}

global argv argv0
runtest $argv



