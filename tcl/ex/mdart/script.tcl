#------------------------------------------------------------------------------
# Defining options
# ------------------------------------------------------------------------------
set val(chan) Channel/WirelessChannel                              ;# channel type
set val(ant) Antenna/OmniAntenna                                   ;# antenna type
set val(propagation) Shado                                         ;# propagation model
set val(netif) Phy/WirelessPhy                                     ;# network interface type
set val(ll) LL                                                     ;# link layer type
set val(ifq) Queue/DropTail/PriQueue                               ;# interface queue type
set val(ifqlen) 50                                                 ;# max packet in ifq
set val(mac) Mac/802_11                                            ;# MAC type
set val(rp) MDART                                                  ;# routing protocol
set val(n) 16.0                                                    ;# node number
set val(density) 4096                                              ;# node density [node/km^2]
set val(end) 206.0                                                 ;# simulation time [s]
set val(dataStart) 100.0                                           ;# data start time [s]
set val(dataStop) [expr $val(end) - 6.0]                           ;# data stop time [s]
set val(seed) 1                                                    ;# general pseudo-random sequence generator
set val(macFailed) true                                            ;# ATR protocol: ns2 MAC failed callback
set val(etxMetric) true                                            ;# ATR protocol: ETX route metric
set val(throughput) 5.40                                           ;# CBR rate (<= 5.4Mb/s)



# ------------------------------------------------------------------------------
# Channel model
# ------------------------------------------------------------------------------
# set up the antennas to be centered in the node and 1.5 meters above it
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Gt_ 1                                      ;# transmitter antenna gain
Antenna/OmniAntenna set Gr_ 1                                      ;# receiver antenna gain
Phy/WirelessPhy set L_ 1.0                                         ;# system loss factor (mostly 1.0)
if {$val(propagation) == "TwoRay"} {                               ;# range tx = 250m 
	set val(prop) Propagation/TwoRayGround
	set prop [new $val(prop)]
	Phy/WirelessPhy set CPThresh_ 10.0                         ;# capture threshold in Watt
	Phy/WirelessPhy set CSThresh_ 1.559e-11                    ;# Carrier Sensing threshold
	Phy/WirelessPhy set RXThresh_ 3.652e-10                    ;# receiver signal threshold
	Phy/WirelessPhy set freq_ 2.4e9                            ;# channel frequency (Hz)
	Phy/WirelessPhy set Pt_ 0.28                               ;# transmitter signal power (Watt)
}
if {$val(propagation) == "Shado"} {
	set val(prop) Propagation/Shadowing
	set prop [new $val(prop)]
	$prop set pathlossExp_ 3.8                                 ;# path loss exponent
	$prop set std_db_ 2.0                                      ;# shadowing deviation (dB)
	$prop set seed_ 1                                          ;# seed for RNG
	$prop set dist0_ 1.0                                       ;# reference distance (m)
	$prop set CPThresh_ 10.0                                   ;# capture threshold in Watt
	$prop set RXThresh_ 2.37e-13                               ;# receiver signal threshold
	$prop set CSThresh_ [expr 2.37e-13 * 0.0427]               ;# Carrier Sensing threshold
	$prop set freq_ 2.4e9                                      ;# channel frequency (Hz)
	Phy/WirelessPhy set Pt_ 0.28
}



# ------------------------------------------------------------------------------
# Topology definition
# ------------------------------------------------------------------------------
#Creazione dello scenario in funzione della densita' di nodo scelta
set val(dim) [expr $val(n) / $val(density)] 
set val(x) [expr [expr sqrt($val(dim))] * 1000]
set val(y) [expr [expr sqrt($val(dim))] * 1000]



# ------------------------------------------------------------------------------
# Pseudo-random sequence generator
# ------------------------------------------------------------------------------

# General pseudo-random sequence generator
set genSeed [new RNG]
$genSeed seed $val(seed)
set randomSeed [new RandomVariable/Uniform]
$randomSeed use-rng $genSeed
$randomSeed set min_ 1.0
$randomSeed set max_ 100.0

# Mobility model: x node position [m]
set genNodeX [new RNG]
$genNodeX seed [expr [$randomSeed value]]
set randomNodeX [new RandomVariable/Uniform]
$randomNodeX use-rng $genNodeX
$randomNodeX set min_ 1.0
$randomNodeX set max_ [expr $val(x) - 1.0]

# Mobility model: y node position [m]
set posNodeY [new RNG]
$posNodeY seed [expr [$randomSeed value]]
set randomNodeY [new RandomVariable/Uniform]
$randomNodeY use-rng $posNodeY
$randomNodeY set min_ 1.0
$randomNodeY set max_ [expr $val(y) - 1.0]

# Data pattern: node
set genNode [new RNG]
$genNode seed [expr [$randomSeed value]]
set randomNode [new RandomVariable/Uniform]
$randomNode use-rng $genNode
$randomNode set min_ 0
$randomNode set max_ [expr $val(n) - 1]



# ------------------------------------------------------------------------------
# General definition
# ------------------------------------------------------------------------------
;#Instantiate the simulator
set ns [new Simulator]
;#Define topology
set topo [new Topography]
$topo load_flatgrid $val(x) $val(y)
;#Create channel
set chan [new $val(chan)]
$prop topography $topo
;#Create God
create-god $val(n)
;#Global node setting
$ns node-config -adhocRouting $val(rp) \
	-llType $val(ll) \
	-macType $val(mac) \
	-ifqType $val(ifq) \
	-ifqLen $val(ifqlen) \
	-antType $val(ant) \
	-propInstance $prop \
	-phyType $val(netif) \
	-channel $chan \
	-topoInstance $topo \
	-agentTrace ON \
	-routerTrace ON \
	-macTrace OFF \
	-movementTrace OFF



# ------------------------------------------------------------------------------
# Routing model (for M-DART)
# ------------------------------------------------------------------------------
if {$val(rp) == "M-DART"} {
	Agent/ATR set macFailed_ $val(macFailed)
	Agent/ATR set etxMetric_ $val(etxMetric)
}



# ------------------------------------------------------------------------------
# Trace file definition
# ------------------------------------------------------------------------------

#New format for wireless traces
$ns use-newtrace

#Create trace object for ns, nam, monitor and Inspect
set nsTrc [open ns.trc w]
$ns trace-all $nsTrc
#set namTrc [open nam.trc w] 
#$ns namtrace-all-wireless $namTrc $val(x) $val(y)
set scenarioTrc [open scenario.trc w]

proc fileTrace {} {
#	global ns nsTrc namTrc
	global ns nsTrc scenarioTrc
	$ns flush-trace 
	close $nsTrc
	close $scenarioTrc
#	close $namTrc
#	exec nam nam.trc &
}



# ------------------------------------------------------------------------------
# Nodes definition
# ------------------------------------------------------------------------------
;#  Create the specified number of nodes [$val(n)] and "attach" them to the channel.
for {set i 0} {$i < $val(n) } {incr i} {
	set node($i) [$ns node] 
	$node($i) random-motion 0        ;# disable random motion
}	



# ------------------------------------------------------------------------------
# Nodes placement
# ------------------------------------------------------------------------------
#parameters for trace Inspect
puts $scenarioTrc "# nodes: $val(n), max time: $val(end)"
puts $scenarioTrc "# nominal range: 250"
for {set i 0} {$i < $val(n)} {incr i} { 
	set X [expr [$randomNodeX value] ]
	$node($i) set X_ $X
	set Y [expr [$randomNodeY value] ]
	$node($i) set Y_ $Y
	$node($i) set Z_ 0.0
	$ns initial_node_pos $node($i) 20
	puts $scenarioTrc "\$node_($i) set X_ $X"
	puts $scenarioTrc "\$node_($i) set Y_ $Y"
	puts $scenarioTrc "\$node_($i) set Z_ 0.0"
}



# ------------------------------------------------------------------------------
# Data load
# ------------------------------------------------------------------------------
for {set i 0} {$i < $val(n)} {incr i} {
	set udp($i) [new Agent/UDP]
	$ns attach-agent $node($i) $udp($i)
	set dest [expr round([$randomNode value])]
	while {$dest == $i} {
		set dest [expr round([$randomNode value])]
	}
	set monitor($dest) [new Agent/LossMonitor]
	$ns attach-agent $node($dest) $monitor($dest)
	$ns connect $udp($i) $monitor($dest)
	set cbr($i) [new Application/Traffic/CBR]
	$cbr($i) attach-agent $udp($i)
	$cbr($i) set packetSize_ 1000
	$cbr($i) set random_ false
	$cbr($i) set rate_ [expr $val(throughput) / [expr $val(n) * sqrt($val(n))]]Mb
	$ns at [expr $val(dataStart) + [$randomSeed value]] "$cbr($i) start"
	$ns at $val(dataStop) "$cbr($i) stop"
}



# ------------------------------------------------------------------------------
# Tracing
# ------------------------------------------------------------------------------

# printing simulation time
proc timeTrace { tracePause} {
	global ns
   	set now [$ns now]
   	$ns at [expr $now + $tracePause] "timeTrace $tracePause"
   	puts "$now simulation seconds"
}

$ns at 10.0 "timeTrace 10.0"

if {$val(rp) == "MDART"} {
	for {set i 0} {$i < $val(n)} {incr i} {
		# printing the routing table
		$ns at [expr $val(end) - 1.0]  "[$node($i) agent 255] routingTablePrint"
		# printing the dht table
		$ns at [expr $val(end) - 0.5]  "[$node($i) agent 255] dhtTablePrint"
	}
}

# ------------------------------------------------------------------------------
# Starting & ending
# ------------------------------------------------------------------------------
for {set i 0} {$i < $val(n) } {incr i} {
    	$ns at $val(end) "$node($i) reset";
}

$ns at $val(end) "fileTrace"
$ns at $val(end) "$ns halt"

$ns run
