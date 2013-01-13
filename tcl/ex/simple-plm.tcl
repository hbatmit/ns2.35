# $Header: /cvsroot/nsnam/ns-2/tcl/ex/simple-plm.tcl,v 1.2 2000/07/27 00:53:14 haoboy Exp $


#choose your scenario
set scenario 0

set packetSize 500
set runtime 100
set plm_debug_flag 2
set rates "20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3 20e3"
set level [llength $rates]
set rates_cum [calc_cum $rates]
puts stderr $rates_cum
set run_nam 1

set Queue_sched_ FQ
set PP_burst_length 2
set PP_estimation_length 3

Queue/DropTail set limit_ 20

Class Scenario0 -superclass PLMTopology

#this scenario shows the convergence of a single plm session 
#on a heterogeneous topology
Scenario0 instproc init args {
	#Create the following topology
	#             _____ R1
	#            /256kb   ____R4
	#      1Mb  /        / 56Kb
	#   S------N1-------N2------R2
	#            128kb   \ 250kb
	#                     \
        #                 64kb \
	#                       \N3____R3
	#                         10Mb
	#
    eval $self next $args
    $self instvar ns node
    
    $self build_link 0 1 10ms  1e6
    $self build_link 1 2 50ms  256e3
    $self build_link 1 3 100ms  128e3
    $self build_link 3 4 50ms 250e3
    $self build_link 3 5 30ms 56e3
    $self build_link 3 6 30ms 64e3
    $self build_link 6 7 5ms 10e6

    $ns duplex-link-op $node(0) $node(1) orient right
    $ns duplex-link-op $node(1) $node(2) orient right
    $ns duplex-link-op $node(1) $node(3) orient right-down
    $ns duplex-link-op $node(3) $node(4) orient right
    $ns duplex-link-op $node(3) $node(5) orient right-up
    $ns duplex-link-op $node(3) $node(6) orient  right-down
    $ns duplex-link-op $node(6) $node(7) orient  right
    
    $ns duplex-link-op $node(0) $node(1) queuePos 0.5
    $ns duplex-link-op $node(1) $node(2) queuePos 0.5
    $ns duplex-link-op $node(1) $node(3) queuePos 0.5
    $ns duplex-link-op $node(3) $node(4) queuePos 0.5
    $ns duplex-link-op $node(3) $node(5) queuePos 0.5
    $ns duplex-link-op $node(3) $node(6) queuePos 0.5
    $ns duplex-link-op $node(6) $node(7) queuePos 0.5


    set addr [$self place_source 0 3]
    puts stderr "sender placed"

    set tm 10
    set time [expr  double([ns-random] % 10000000) / 1e7 * 5+$tm]
    $self place_receiver 2 $addr $time 1
    set time [expr  double([ns-random] % 10000000) / 1e7 * 5+$tm]
    $self place_receiver 4 $addr $time 1
    set time [expr  double([ns-random] % 10000000) / 1e7 * 5+$tm]
    $self place_receiver 6 $addr $time 1
    set time [expr  double([ns-random] % 10000000) / 1e7 * 5+$tm]
    $self place_receiver 7 $addr $time 1
    
    puts stderr "receivers placed"   

    #mcast set up
    DM set PruneTimeout 1000
    set mproto DM
    set mrthandle [$ns mrtproto $mproto {} ]
}

Class Scenario1 -superclass PLMTopology

#this simple scenario shows the influence of the number of
#receivers for a single plm session on a star topology.
Scenario1 instproc init args {
	
    eval $self next $args
    $self instvar ns node
    
    set nb_recv 20
    $self build_link 0 1 20ms 256e3
    for {set i 2} {$i<=[expr $nb_recv + 1]} {incr i} {
	set delay [uniform 5 150]ms
	set bp [uniform 500e3 1e6]
	puts stderr "$delay $bp"
	$self build_link 1 $i $delay  $bp
    }


    set addr [$self place_source 0 3]
    puts stderr "sender placed"

    set check_estimate 1
    for {set i 2} {$i<=[expr $nb_recv + 1]} {incr i} {
	set time 5
	$self place_receiver $i $addr $time $check_estimate
    }
    
    puts stderr "receivers placed"   

    #mcast set up
    DM set PruneTimeout 1000
    set mproto DM
    set mrthandle [$ns mrtproto $mproto {} ]
}

Class Scenario2 -superclass PLMTopology

#this scenario shows the effect of an increasing number of plm
#session, and the effect of variable bottleneck (simulated by a CBR flow)
Scenario2 instproc init args {
	
    eval $self next $args
    $self instvar ns node
    global f
    
    set nb_plm 3
    set nb_cbr 3
    set nb_src [expr $nb_plm + $nb_cbr]
    #puts $f "param $nb_plm $nb_cbr"
   
    $self build_link 0 1 20ms [expr 200e3 * $nb_plm]
    $ns duplex-link-op $node(0) $node(1) queuePos 0.5
  
    for {set i 2} {$i<=[expr $nb_src + 1]} {incr i} {
	set delay 5ms 
	set bp 10e6
	puts stderr "$delay $bp"
	$self build_link $i 0 $delay  $bp
    }

    for {set i [expr $nb_src + 2]} {$i<=[expr 2 * $nb_src + 1]} {incr i} {
	set delay 5ms 
	set bp 10e6	
	puts stderr "$delay $bp"
	$self build_link 1 $i $delay  $bp
    }

    for {set i 2} {$i<=[expr $nb_plm + 1]} {incr i} {
	set addr($i) [$self place_source $i 3]
    }
    puts stderr "sender placed"

    set check_estimate 1
    for {set i 2} {$i<=[expr $nb_plm + 1]} {incr i} {
	set time [expr 10 * ($i - 1)]
	$self place_receiver [expr $i + $nb_src] $addr($i) $time $check_estimate
    }

    for {set i 1} {$i<=$nb_cbr} {incr i} {
	set null($i) [new Agent/Null]
	set udp($i) [new Agent/UDP]
	$udp($i) set fid_ [expr $i + $nb_plm]
	$ns attach-agent $node([expr $i + $nb_plm + 1]) $udp($i)
	$ns attach-agent $node([expr $i + $nb_plm + $nb_src + 1]) $null($i)
	$ns connect $udp($i) $null($i)
	set cbr($i) [new Application/Traffic/CBR]
	$cbr($i) attach-agent $udp($i)
	$cbr($i) set random_ 0
	$cbr($i) set rate_ 500kb
	$ns at [expr 10 * $nb_plm + 10] "$cbr($i) start"
	$ns at [expr 10 * $nb_plm + 30] "$cbr($i) stop"
    }
    puts stderr "receivers placed"  
    
    
    #mcast set up
    DM set PruneTimeout 1000
    set mproto DM
    set mrthandle [$ns mrtproto $mproto {} ]
}


Class Scenario3 -superclass PLMTopology

#evaluate the behavior of a mix of plm and tcp flows
Scenario3 instproc init args {
    eval $self next $args
    $self instvar ns node
    global f
    
    Agent/TCP set window_ 4000
    set nb_tcp 2
    set nb_plm 1
    set nb_src [expr $nb_tcp + $nb_plm]
    #puts $f "param $nb_plm $nb_tcp"

    $self build_link 0 1 20ms [expr $nb_src * 100e3]
    $ns duplex-link-op $node(0) $node(1) queuePos 0.5

    for {set i 2} {$i<=[expr $nb_src + 1]} {incr i} {
	set delay 5ms
	set bp 10e6
	puts stderr "$delay $bp"
	$self build_link $i 0 $delay  $bp
    }

    for {set i [expr $nb_src + 2]} {$i<=[expr 2 * $nb_src + 1]} {incr i} {
	set delay 5ms
	set bp 10e6
	puts stderr "$delay $bp"
	$self build_link 1 $i $delay  $bp
    }

    for {set i 2} {$i<=[expr $nb_plm + 1]} {incr i} {
	set addr($i) [$self place_source $i 3]
    }
    puts stderr "sender placed"

    set check_estimate 1
    for {set i 2} {$i<=[expr $nb_plm + 1]} {incr i} {
	set time 20
	$self place_receiver [expr $i + $nb_src] $addr($i) $time $check_estimate
    }

    Agent/TCP set packetSize_ 500
    for {set i 1} {$i<=$nb_tcp} {incr i} {
	set sink($i) [new Agent/TCPSink]
	set tcp($i) [new Agent/TCP/Reno]
	$tcp($i) set fid_ [expr $i + $nb_plm]
	$ns attach-agent $node([expr $i + $nb_plm + 1]) $tcp($i)
	$ns attach-agent $node([expr $i + $nb_plm + $nb_src + 1]) $sink($i)
	$ns connect $tcp($i) $sink($i)
	set ftp($i) [new Application/FTP]
	$ftp($i) attach-agent $tcp($i)
        switch $i {
            1 {$ns at 0 "$ftp($i) start"}
            2 {$ns at 60 "$ftp($i) start"}
        }
    }
    puts stderr "receivers placed"  

#mcast set up
	DM set PruneTimeout 1000
	set mproto DM
	set mrthandle [$ns mrtproto $mproto {} ]
}








Simulator instproc finish {} {
    global   run_nam
    
    puts finish
    if {$run_nam} {
	puts "running nam..."
	exec nam -g 600x700 -f dynamic-nam.conf out.nam &
    }
    
    exit 0
}

Simulator instproc tick {} {
	puts stderr [$self now]
	$self at [expr [$self now] + 10.] "$self tick"
}


set ns [new Simulator -multicast on]
$ns multicast

$ns color 1 blue
$ns color 2 green
$ns color 3 red
$ns color 4 white
# prunes, grafts
$ns color 30 orange
$ns color 31 yellow

#set f [open out.tr w]
#$ns trace-all $f
$ns namtrace-all [open out.nam w]

$ns tick
set scn [new Scenario$scenario $ns]
$ns at [expr $runtime +1] "$ns finish"
$ns run

