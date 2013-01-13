# simple directed diffusion example
# This example tests directed diffusion in 2 simple scenarios
# (1) where all 3 nodes can hear each other and a source and sink application pair is setup on two of the nodes.
# and
# (2) where node0 and node2 donot hear one another and communicats thru the third node1. src and snk apps reside on node 0 and node2.

# ==================================================================
# Define options
# ===================================================================
set opt(chan)		Channel/WirelessChannel
set opt(prop)		Propagation/TwoRayGround
set opt(netif)		Phy/WirelessPhy
set opt(mac)		Mac/802_11
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(ant)            Antenna/OmniAntenna
set opt(filters)        GradientFilter    ;# options can be one or more of 
                                ;# TPP/OPP/Gear/Rmst/SourceRoute/Log/TagFilter
set opt(x)		670	;# X dimension of the topography
set opt(y)		670     ;# Y dimension of the topography
set opt(ifqlen)		50	;# max packet in ifq
set opt(nn)		3	;# number of nodes
set opt(seed)		0.0
set opt(stop)		50	;# simulation time
set opt(prestop)        19       ;# time to prepare to stop
set opt(tr)		"diffusion.tr"	;# trace file
set opt(nam)            "diffusion.nam"  ;# nam file
set opt(adhocRouting)   Directed_Diffusion
#set opt(traf)		"diffusion-traf.tcl"      ;# traffic file

# ==================================================================

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used

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

# ==================================================================
# Main Program
# =================================================================

#
# Initialize Global Variables
#

set ns_		[new Simulator] 
set topo	[new Topography]

set tracefd	[open $opt(tr) w]
$ns_ trace-all $tracefd

set nf [open $opt(nam) w]
$ns_ namtrace-all-wireless $nf $opt(x) $opt(y)

#$ns_ use-newtrace

$topo load_flatgrid $opt(x) $opt(y)

set god_ [create-god $opt(nn)]

# log the mobile nodes movements if desired

#if { $opt(lm) == "on" } {
#    log-movement
#}

#global node setting

$ns_ node-config -adhocRouting $opt(adhocRouting) \
		 -llType $opt(ll) \
		 -macType $opt(mac) \
		 -ifqType $opt(ifq) \
		 -ifqLen $opt(ifqlen) \
		 -antType $opt(ant) \
		 -propType $opt(prop) \
		 -phyType $opt(netif) \
		 -channelType $opt(chan) \
		 -topoInstance $topo \
                 -diffusionFilter $opt(filters) \
		 -agentTrace ON \
                 -routerTrace OFF \
                 -macTrace ON
                  

#  Create the specified number of nodes [$opt(nn)] and "attach" them
#  to the channel. 

for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node $i]
        $node_($i) color black
	$node_($i) random-motion 0		;# disable random motion
        $god_ new_node $node_($i)
}

puts "Loading connection pattern..."


$node_(0) set X_ 500.716707738489
$node_(0) set Y_ 620.707335765875
$node_(0) set Z_ 0.000000000000

$node_(1) set X_ 320.192740186325
$node_(1) set Y_ 450.384818286195
$node_(1) set Z_ 0.000000000000

# 3rd node in hearing range of other two
#$node_(2) set X_ 477.438972165239
#$node_(2) set Y_ 545.843469852367
#$node_(2) set Z_ 0.000000000000

#3rd node NOT in hearing range of other two
$node_(2) set X_ 177.438972165239
$node_(2) set Y_ 245.843469852367
$node_(2) set Z_ 0.000000000000



#Diffusion src application

set src_(0) [new Application/DiffApp/PingSender/TPP]
$ns_ attach-diffapp $node_(0) $src_(0)
$ns_ at 0.123 "$src_(0) publish"


# another diff application


#set src_(1) [new Application/DiffApp/PingSender/TPP]
#$ns_ attach-diffapp $node_(1) $src_(1)
#$ns_ at 1.3 "$src_(1) publish"


#Diffusion sink application
set snk_(0) [new Application/DiffApp/PingReceiver/TPP]
$ns_ attach-diffapp $node_(2) $snk_(0)
$ns_ at 1.456 "$snk_(0) subscribe"

# Define node initial position in nam

for {set i 0} {$i < $opt(nn)} {incr i} {

    # 20 defines the node size in nam, must adjust it according to your scenario
    # The function must be called after mobility model is defined
    
    $ns_ initial_node_pos $node_($i) 20
}

#
# Tell nodes when the simulation ends
#
for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).000000001 "$node_($i) reset";
}
# tell nam the simulation stop time
$ns_ at  $opt(stop)	"$ns_ nam-end-wireless $opt(stop)"

$ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"

puts $tracefd "Directed Diffusion:"
puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y)"
puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"

puts "Starting Simulation..."
$ns_ run

