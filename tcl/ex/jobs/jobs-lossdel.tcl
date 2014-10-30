# jobs-lossdel.tcl
#
# Tests that the delay/loss differentiation is working properly
# Outputs a nam animation and tracefiles.
#
# Topology is as follows:
#
# source(1)  --\                                  /- sink(1)
# source(2)  ---\  core_node(1) -- core_node(2)  /-- sink(2)
# source(3)  ---/                                \-- sink(3)
# source(4)  __/                                  \_ sink(4)
#
# Links:
# - : 10 Mbps, 1 ms
#
# Classes: 
# 1: ADC=5 ms, ALC=1 %, no ARC, no RDC, no RLC
# 2: no ADC, no ALC, RDC = 4, RLC = 2
# 3: no ADC, no ALC, RDC = 4, RLC = 2
# 4: no ADC, no ALC, RDC = 4, RLC = 2
#
# Traffic is randomized CBR
#
# Auxiliary functions

Simulator instproc get-link { node1 node2 } {
    	$self instvar link_
    	set id1 [$node1 id]
    	set id2 [$node2 id]
    	return $link_($id1:$id2)
}

Simulator instproc get-queue { node1 node2 } {
    	global ns
    	set l [$ns get-link $node1 $node2]
    	set q [$l queue]
    	return $q
}

# JoBS duplex link: rate in kbps, delay in ms, buff_sz in packets

proc build-jobs-link {src dst rate delay buff_sz} {
    	global ns
    	$ns duplex-link $src $dst [expr $rate*1000.0] [expr $delay/1000.0] JoBS
    	$ns queue-limit $src $dst $buff_sz
    	$ns queue-limit $dst $src $buff_sz
}

# FCFS-marker duplex link: rate in kbps, delay in ms, buff_sz in packets

proc build-marker-link {src dst rate delay buff_sz} {
    	global ns
    	$ns simplex-link $src $dst [expr $rate*1000.0] [expr $delay/1000.0] Marker
    	$ns simplex-link $dst $src [expr $rate*1000.0] [expr $delay/1000.0] Demarker
    	$ns queue-limit $src $dst $buff_sz
    	$ns queue-limit $dst $src $buff_sz
}

# FCFS-demarker duplex link: rate in kbps, delay in ms, buff_sz in packets

proc build-demarker-link {src dst rate delay buff_sz} {
	global ns
	$ns simplex-link $src $dst [expr $rate*1000.0] [expr $delay/1000.0] Demarker
	$ns simplex-link $dst $src [expr $rate*1000.0] [expr $delay/1000.0] Marker
	$ns queue-limit $src $dst $buff_sz
	$ns queue-limit $dst $src $buff_sz
}

# Basic procedure for connecting agents, etc.

proc flow_setup {id src src_agent dst dst_agent app_flow pksize} {
	global ns
	$ns attach-agent $src $src_agent
	$ns attach-agent $dst $dst_agent
	$ns connect $src_agent $dst_agent
	$src_agent set packetSize_ 	$pksize
	$src_agent set fid_ $id

	$app_flow set flowid_ $id
	$app_flow attach-agent $src_agent

	return $src_agent    
}

# CBR flow: Packet size:bytes, Rate: kbps, Start time:seconds
proc build-cbr {id src src_agent dst dst_agent app_flow rate pksize start_tm} {
    	global ns

    	flow_setup $id $src $src_agent $dst $dst_agent $app_flow $pksize

    	$app_flow set rate_        [expr $rate * 1000.0]
    	$app_flow set random_ 1
    	$ns at $start_tm "$app_flow start"

    	puts [format "\tCBR-$id starts at %.4fsec" $start_tm]
}
# main 
puts " "
set ns [new Simulator]
set rnd [new RNG]
set seed 16
puts "Random seed: $seed"
$rnd seed $seed 
set nf [open jobs-lossdel/out.nam w]
$ns namtrace-all $nf
Queue/JoBS set drop_front_ false
Queue/JoBS set trace_hop_ true
Queue/JoBS set adc_resolution_type_ 0
Queue/JoBS set shared_buffer_ 1
Queue/JoBS set mean_pkt_size_ 4000
Queue/Demarker set demarker_arrvs1_ 0
Queue/Demarker set demarker_arrvs2_ 0
Queue/Demarker set demarker_arrvs3_ 0
Queue/Demarker set demarker_arrvs4_ 0
Queue/Marker set marker_arrvs1_ 0
Queue/Marker set marker_arrvs2_ 0
Queue/Marker set marker_arrvs3_ 0
Queue/Marker set marker_arrvs4_ 0
#
puts "\nTOPOLOGY SETUP"

# Number of hops (=k)
set hops 2
puts "Number of hops: $hops"

# Link  latency (ms)
set DELAY  	1.0
puts "Links latency: $DELAY (ms)"

# links: bandwidth (kbps) 
set BW		10000.0
puts "Links capacity: $BW (kbps)"

# Buffer size in gateways (in packets)
set GW_BUFF	50
puts "Gateway Buffer Size: $GW_BUFF (packs)"

# Packet Size (in bytes); assume common for all sources
set PKTSZ      500
set MAXWIN     50
puts "Packet Size: $PKTSZ (bytes)"

# Number of monitored flows (equal to # of classes)
set N_CL	4	
puts "Number of classes: $N_CL"

set N_USERS	4
puts "Number of flows: $N_USERS"

set START_TM 	0.0
puts "Monitored flows start at: $START_TM"

set max_time 	10.0
puts "Max time: $max_time (sec)"

# Statistics-related parameters
set STATS_INTR  2; # interval between reporting statistics 
set START_STATS 0; # start-time for reporting statistics 

# Marker Types
# Deterministic 
set DETERM	1	

# Demarker Types
# Create a trace file for each class (for user traffic)
set VERBOSE	1
# Do not create trace files 
set QUIET	2

proc print_time {interval} {
	global ns 
    	puts ""
    	puts -nonewline stdout [format "%.2f\t" [$ns now]]
    	$ns at [expr [$ns now]+$interval] "print_time $interval"
}

# 1. Core nodes ($hops nodes)
for {set i 1} {$i <= $hops} {incr i} {
	set core_node($i) [$ns node]
	$core_node($i) color "black"
	$core_node($i) shape "square"
	$core_node($i) label QoSbox
}
puts "Core nodes: OK"

# 2. User traffic nodes and sinks (N_USERS sources and sinks)
for {set i 1} {$i <= $N_USERS} {incr i} {
	set source($i) [$ns node]
	$source($i) color blue
	set sink($i) [$ns node]
	$sink($i) color blue
	set source_agent($i) [new Agent/UDP]
	set sink_agent($i) [new Agent/LossMonitor]
}
puts "Traffic nodes and sinks: OK"

# 3. Core link (JoBS)
build-jobs-link $core_node(1) $core_node(2) $BW $DELAY $GW_BUFF
set q [$ns get-queue $core_node(1) $core_node(2)]
set l [$ns get-link $core_node(1) $core_node(2)]
$q copyright-info
$q init-rdcs -1 4 4 4 
$q init-rlcs -1 2 2 2 
$q init-alcs 0.01 -1 -1 -1
$q init-adcs 0.001 -1 -1 -1
$q init-arcs -1 -1 -1 -1
$q trace-file jobs-lossdel/hoptrace
$q link [$l link]
$q sampling-period 1
$q id $i
$q initialize

# It's a duplex link
# Even though data won't travel on the reverse path (UDP sources), 
# it is always a good idea to configure the JoBS queue on the reverse
# path as well, in order to avoid unexpected results for TCP traffic. 

set q [$ns get-queue $core_node(2) $core_node(1)]
set l [$ns get-link $core_node(2) $core_node(1)]
$q init-rdcs -1 4 4 4 
$q init-rlcs -1 2 2 2
$q init-alcs 0.01 -1 -1 -1
$q init-adcs 0.001 -1 -1 -1
$q init-arcs -1 -1 -1 -1
$q trace-file null
$q link [$l link]
$q sampling-period 1
$q id $i
$q initialize

puts "Core link: OK"

# 6. Edge links (marker)
for {set i 1} {$i <= $N_USERS} {incr i} {
	build-marker-link    $source($i) $core_node(1) $BW $DELAY [expr $GW_BUFF*10]
    	set q [$ns get-queue $source($i) $core_node(1)]
   	$q marker_type $DETERM
	$q marker_class $i
    	set q [$ns get-queue $core_node(1) $source($i)]
	# assign a bogus id
    	$q id 99
	set l [$ns get-link $source($i) $core_node(1)]
    	
	build-demarker-link  $core_node($hops) $sink($i) $BW $DELAY [expr $GW_BUFF*10]
    	set q [$ns get-queue $core_node($hops) $sink($i)]
    	$q id 99

    	set q [$ns get-queue $sink($i) $core_node($hops)]
    	$q marker_type $DETERM
	$q marker_class $i
}
puts "Edge marker/demarker links: OK" 

#
# Create traffic
#
# User traffic (UDP)
for {set i 1} {$i <= $N_USERS} {incr i} {
	set flow($i) [new Application/Traffic/CBR]
	$flow($i) set random_ 1
	build-cbr $i $source($i) $source_agent($i) $sink($i) $sink_agent($i) $flow($i) 2600.0 $PKTSZ $START_TM 
}
puts "Flows: OK"

#
# Queue Monitors
#

$ns at $STATS_INTR "print_time $STATS_INTR"

$ns at [expr $max_time+.000000001] "puts \"Finishing Simulation...\""
$ns at [expr $max_time+1] "finish"
proc finish {} {
	global ns nf
        puts "\nSimulation End"
	$ns flush-trace
	close $nf
	exec nam jobs-lossdel/out.nam & 
	exit 0
}

# nam stuff
$ns duplex-link-op $source(1) $core_node(1) orient 300deg
$ns duplex-link-op $source(1) $core_node(1) color "blue"
$ns duplex-link-op $source(2) $core_node(1) orient 330deg
$ns duplex-link-op $source(2) $core_node(1) color "blue"
$ns duplex-link-op $source(3) $core_node(1) orient 30deg
$ns duplex-link-op $source(3) $core_node(1) color "blue"
$ns duplex-link-op $source(4) $core_node(1) orient 60deg
$ns duplex-link-op $source(4) $core_node(1) color "blue"
$ns duplex-link-op $core_node(1) $core_node(2) orient right
$ns duplex-link-op $core_node(1) $core_node(2) color black
$ns duplex-link-op $core_node(1) $core_node(2) queuePos 0.5
$ns duplex-link-op $core_node(2) $sink(1) orient 60deg
$ns duplex-link-op $core_node(2) $sink(1) color "blue"
$ns duplex-link-op $core_node(2) $sink(2) orient 30deg
$ns duplex-link-op $core_node(2) $sink(2) color "blue"
$ns duplex-link-op $core_node(2) $sink(3) orient 330deg
$ns duplex-link-op $core_node(2) $sink(3) color "blue"
$ns duplex-link-op $core_node(2) $sink(4) orient 300deg
$ns duplex-link-op $core_node(2) $sink(4) color "blue"
$ns color 1 red
$ns color 2 green
$ns color 3 blue
$ns color 4 purple
puts "\ngo!\n"
$ns run

