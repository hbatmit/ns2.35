#------------------------------------------------------------------------------
# Marco Fiore's Patch
# ------------------------------------------------------------------------------
#remove-all-packet-headers                                                     ;# removes all except common
#add-packet-header IP LL Mac AODV AOMDV ATR DSDV DSR OLSR UDP TCP CBR FTP      ;# needed headers
Mac/802_11 set CWMin_                 31
Mac/802_11 set CWMax_                 1023
Mac/802_11 set SlotTime_              0.000020                     ;# 20us
Mac/802_11 set SIFS_                  0.000010                     ;# 10us
Mac/802_11 set PreambleLength_        144                         ;# 144 bit
Mac/802_11 set ShortPreambleLength_   72                           ;# 72 bit
Mac/802_11 set PreambleDataRate_      1.0e6                        ;# 1Mbps
Mac/802_11 set PLCPHeaderLength_      48                           ;# 48 bits
Mac/802_11 set PLCPDataRate_          1.0e6                        ;# 1Mbps
Mac/802_11 set ShortPLCPDataRate_     2.0e6                        ;# 2Mbps
Mac/802_11 set RTSThreshold_          3000                         ;# bytes
Mac/802_11 set ShortRetryLimit_       7                            ;# retransmissions
Mac/802_11 set LongRetryLimit_        4                            ;# retransmissions
Mac/802_11 set newchipset_            false                        ;# use new chipset, allowing a more recent  packet to be correctly received in place of the first sensed packet
Mac/802_11 set dataRate_ 11Mb                                      ;# 802.11 data transmission rate
Mac/802_11 set basicRate_ 1Mb                                      ;# 802.11 basic transmission rate 
Mac/802_11 set aarf_ false                                         ;# 802.11 Auto Rate Fallback



#------------------------------------------------------------------------------
# Defining options
# ------------------------------------------------------------------------------
set val(chan) Channel/WirelessChannel                              ;# channel type
set val(ant) Antenna/OmniAntenna                                   ;# antenna type
set val(propagation) Shado ;# propagation model
set val(netif) Phy/WirelessPhy                                     ;# network interface type
set val(ll) LL                                                     ;# link layer type
set val(ifq) Queue/DropTail/PriQueue                               ;# interface queue type
set val(ifqlen) 50                                                 ;# max packet in ifq
set val(mac) Mac/802_11                                            ;# MAC type
set val(rp) AOMDV ;# routing protocol
set val(n) 16.0 ;# node number
set val(density) 4096 ;# node density [node/km^2]
set val(end) 2060.0                                                ;# simulation time [s]
set val(mobility) Static ;# mobility model
set val(minSpeed) 0.5                                              ;# movement minimum speed [m/s]
set val(maxSpeed) 1.5                                              ;# movement maximum speed [m/s]
set val(minPause) 0.0                                              ;# movement minimum pause time [s]
set val(maxPause) 0.0                                              ;# movement maximum pause time [s]
set val(movementStart) 950.0                                       ;# movement start time [s]
set val(traffic) Node-UDP ;# data pattern
set val(dataStart) 1000.0                                          ;# data start time [s]
set val(dataStop) [expr $val(end) - 60.0]                          ;# data stop time [s]
set val(seed) 1 ;# general pseudo-random sequence generator
set val(macFailed) true                                            ;# ATR protocol: ns2 MAC failed callback
set val(etxMetric) true                                            ;# ATR protocol: ETX route metric
set val(throughput) 5.4 ;# CBR rate (<= 5.4Mb/s)
set val(flow) 100                                                  ;# number of concurrent data flow for FLOW-**P data pattern
set val(nodes) $val(n)                                             ;# number of concurrent transmitting nodes for NODE-**P data pattern



# ------------------------------------------------------------------------------
# Fixing DSR bug
# ------------------------------------------------------------------------------
if {$val(rp) == "DSR"} {
	set val(ifq) CMUPriQueue
}



# ------------------------------------------------------------------------------
# Channel model
# ------------------------------------------------------------------------------
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

# Mobility model: node speed [m/s]
set genNodeSpeed [new RNG]
$genNodeSpeed seed [expr [$randomSeed value]]
set randomNodeSpeed [new RandomVariable/Uniform]
$randomNodeSpeed use-rng $genNodeSpeed
$randomNodeSpeed set min_ $val(minSpeed)
$randomNodeSpeed set max_ $val(maxSpeed)

# Mobility model: node movement pause [s] 
set genNodePause [new RNG]
$genNodePause seed [expr [$randomSeed value]]
set randomNodePause [new RandomVariable/Uniform]
$randomNodePause use-rng $genNodePause
$randomNodePause set min_ $val(minPause)
$randomNodePause set max_ $val(maxPause)

# Data pattern: node
set genNode [new RNG]
$genNode seed [expr [$randomSeed value]]
set randomNode [new RandomVariable/Uniform]
$randomNode use-rng $genNode
$randomNode set min_ 0
$randomNode set max_ [expr $val(n) - 1]

# Data pattern "Random": flow start time [s]
set genStartData [new RNG]
$genStartData seed [expr [$randomSeed value]]
set randomStartData [new RandomVariable/Uniform]
$randomStartData use-rng $genStartData
$randomStartData set min_ $val(dataStart)
$randomStartData set max_ $val(dataStop)

# Data pattern "Full: flow start time [s]
set genStartDataFull [new RNG]
$genStartDataFull seed [expr [$randomSeed value]]
set randomStartDataFull [new RandomVariable/Uniform]
$randomStartDataFull use-rng $genStartDataFull
$randomStartDataFull set min_ 0
$randomStartDataFull set max_ [expr ($val(n) * ($val(n) - 1)) - 1]

# Data pattern: flow end time [s]
set genEndData [new RNG]
$genEndData seed [expr [$randomSeed value]]
set randomEndData [new RandomVariable/Uniform]
$randomEndData use-rng $genEndData
$randomEndData set min_ 0.0
$randomEndData set max_ [expr $val(end) - $val(dataStart) - 20]



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
# Nodes mobility
# ------------------------------------------------------------------------------
#parameters for trace Inspect
puts $scenarioTrc "# nodes: $val(n), max time: $val(end)"
puts $scenarioTrc "# nominal range: 250"

if {$val(mobility) == "Static"} {
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
}



# ------------------------------------------------------------------------------
# Data load
# ------------------------------------------------------------------------------
if {$val(traffic) == "Node-UDP"} {
	for {set i 0} {$i < $val(nodes)} {incr i} {
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
#		$cbr($i) set rate_ [expr $val(throughput)]Mb
		$ns at [expr $val(dataStart) + [$randomSeed value]] "$cbr($i) start"
		$ns at $val(dataStop) "$cbr($i) stop"
#		$ns at [expr [$randomStartData value]] "$cbr($i) start"
#		set endData [expr [$randomStartData value] + [$randomEndData value]]
#		if {$endData > $val(dataStop)} {
#			set endData $val(dataStop)
#		}
#		$ns at $endData "$cbr($i) stop"
	}
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

#$ns at 500 "[$node(0) agent 255] storeInfo id1 tex#t1"
#$ns at 500 "[$node(0) agent 255] storeInfo id2 text2"

#$ns at 520 "[$node(0) agent 255] requestInfo id1"

# ------------------------------------------------------------------------------
# Starting & ending
# ------------------------------------------------------------------------------
for {set i 0} {$i < $val(n) } {incr i} {
    	$ns at $val(end) "$node($i) reset";
}

$ns at $val(end) "fileTrace"
$ns at $val(end) "$ns halt"

$ns run
