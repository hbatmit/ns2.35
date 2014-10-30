#
# Used for energy study
#
#

# ======================================================================
# Define options
# ======================================================================
set opt(chan)		Channel/WirelessChannel
set opt(prop)		Propagation/TwoRayGround
set opt(netif)		Phy/WirelessPhy
set opt(mac)		Mac/802_11
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(ant)            Antenna/OmniAntenna

set opt(x)		670	;# X dimension of the topography
set opt(y)		670		;# Y dimension of the topography
set opt(cp)		"../Scene/S1/cbr-50-50-1-512.tcl"
set opt(sc)		"../Scene/S1/mobility.tcl"

set opt(ifqlen)		50		;# max packet in ifq
set opt(nn)		50		;# number of nodes
set opt(seed)		0.0
set opt(stop)		1500.0		;# simulation time
set opt(tr)		estudy-aodv.tr	;# trace file
set opt(nam)		demo.nam
set opt(rp)             AODV            ;# routing protocol script
set opt(lm)             "off"           ;# log movement
set opt(energymodel)    EnergyModel     ;
set opt(initialenergy)  100               ;# Initial energy in Joules
#set opt(logenergy)      "on"           ;# log energy every 150 seconds

# ======================================================================
# needs to be fixed later
set AgentTrace			ON
set RouterTrace			ON
set MacTrace			OFF

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used
LL set off_prune_		0	;# not used
LL set off_CtrMcast_		0	;# not used

Agent/Null set sport_		0
Agent/Null set dport_		0

Agent/CBR set sport_		0
Agent/CBR set dport_		0

Agent/TCPSink set sport_	0
Agent/TCPSink set dport_	0

Agent/TCP set sport_		0
Agent/TCP set dport_		0
Agent/TCP set packetSize_	1460

Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

# unity gain, omni-directional antennas
# set up the antennas to be centered in the node and 1.5 meters above it
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0

# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/WirelessPhy set CPThresh_ 10.0
Phy/WirelessPhy set CSThresh_ 1.559e-11
Phy/WirelessPhy set RXThresh_ 3.652e-10
Phy/WirelessPhy set Rb_ 2*1e6
Phy/WirelessPhy set Pt_ 0.2818
Phy/WirelessPhy set freq_ 914e+6 
Phy/WirelessPhy set L_ 1.0

# ======================================================================

proc usage { argv0 }  {
	puts "Usage: $argv0"
	puts "\tmandatory arguments:"
	puts "\t\t\[-x MAXX\] \[-y MAXY\]"
	puts "\toptional arguments:"
	puts "\t\t\[-cp conn pattern\] \[-sc scenario\] \[-nn nodes\]"
	puts "\t\t\[-seed seed\] \[-stop sec\] \[-tr tracefile\]\n"
}


proc getopt {argc argv} {
	global opt
	lappend optlist cp nn seed sc stop tr x y

	for {set i 0} {$i < $argc} {incr i} {
		set arg [lindex $argv $i]
		if {[string range $arg 0 0] != "-"} continue

		set name [string range $arg 1 end]
		set opt($name) [lindex $argv [expr $i+1]]
	}
}

# ======================================================================
# Main Program
# ======================================================================
getopt $argc $argv

#source ../lib/ns-bsnode.tcl
#source ../mobility/com.tcl

# do the get opt again incase the routing protocol file added some more
# options to look for
getopt $argc $argv

if { $opt(x) == 0 || $opt(y) == 0 } {
	usage $argv0
	exit 1
}

if {$opt(seed) > 0} {
	puts "Seeding Random number generator with $opt(seed)\n"
	ns-random $opt(seed)
}

#
# Initialize Global Variables
#
set ns_		[new Simulator]
set topo	[new Topography]
set god_         [new God]

set tracefd	[open $opt(tr) w]
set namtrace    [open $opt(nam) w]

$topo load_flatgrid $opt(x) $opt(y)

$ns_ trace-all $tracefd
$ns_ namtrace-all-wireless $namtrace $opt(x) $opt(y)

#$ns_ use-newtrace

#
# Create God
#
create-god $opt(nn)


#
#  Create the specified number of nodes $opt(nn) and "attach" them
#  the channel.
#  Each routing protocol script is expected to have defined a proc
#  create-mobile-node that builds a mobile node and inserts it into the
#  array global $node_($i)
#

	#global node setting

        $ns_ node-config -adhocRouting $opt(rp) \
			 -llType $opt(ll) \
			 -macType $opt(mac) \
			 -ifqType $opt(ifq) \
			 -ifqLen $opt(ifqlen) \
			 -antType $opt(ant) \
			 -propType $opt(prop) \
			 -phyType $opt(netif) \
			 -channelType $opt(chan) \
			 -topoInstance $topo \
			 -energyModel $opt(energymodel) \
			 -macTrace ON \
			 -rxPower 0.3 \
			 -txPower 0.6 \
			 -initialEnergy $opt(initialenergy)
			 
	for {set i 0} {$i < $opt(nn) } {incr i} {
		set node_($i) [$ns_ node]	
		$node_($i) random-motion 0		;# disable random motion
	}

#
# Source the Connection and Movement scripts
#
if { $opt(cp) == "" } {
	puts "*** NOTE: no connection pattern specified."
        set opt(cp) "none"
} else {
	puts "Loading connection pattern..."
	source $opt(cp)
}


#
# Tell all the nodes when the simulation ends
#
for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).000000001 "$node_($i) reset";
}
$ns_ at $opt(stop).00000001 "puts \"NS EXITING...\" ; $ns_ halt"


if { $opt(sc) == "" } {
	puts "*** NOTE: no scenario file specified."
        set opt(sc) "none"
} else {
	puts "Loading scenario file..."
	source $opt(sc)
	puts "Load complete..."
}

# Define node initial position in nam

for {set i 0} {$i < $opt(nn)} {incr i} {

    # 20 defines the node size in nam, must adjust it according to your scenario
    # The function must be called after mobility model is defined

    $ns_ initial_node_pos $node_($i) 20
}



puts $tracefd "G -t * -N $opt(nn) -X $opt(x) -Y $opt(y) -p $opt(rp) -E $opt(initialenergy)"
puts $tracefd "G -t * -sc $opt(sc) -cp $opt(cp) -seed $opt(seed)"
puts $tracefd "G -t * -prop $opt(prop) -ant $opt(ant)"

puts "Starting Simulation..."
$ns_ run

