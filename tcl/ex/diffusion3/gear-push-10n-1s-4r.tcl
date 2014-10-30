#
# simple gear/push example
# This example tests directed diffusion in a 10 node scenario with a single sender and a 4 recvr using gear app with push rtg algorithm
#
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
set opt(filters)        GradientFilter/GeoRoutingFilter  ;# old name for twophasepull filter
set opt(x)		670	;# X dimension of the topography
set opt(y)		670     ;# Y dimension of the topography
set opt(ifqlen)		50	;# max packet in ifq
set opt(nn)		10	;# number of nodes
set opt(seed)		0.0
set opt(stop)		500	;# simulation time
set opt(prestop)        499      ;# time to prepare to stop
set opt(tr)		"gear-push-10n-1s-4r.tr"	;# trace file
set opt(nam)            "gear-push-10n-1s-4r.nam"  ;# nam file
set opt(adhocRouting)   Directed_Diffusion

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
		 -agentTrace OFF \
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

$node_(0) set X_ 180.587605569735
$node_(0) set Y_ 401.886827012678
$node_(0) set Z_ 0.000000000000
$node_(1) set X_ 11.901958617639
$node_(1) set Y_ 36.218497218617
$node_(1) set Z_ 0.000000000000
$node_(2) set X_ 224.282785411393
$node_(2) set Y_ 20.774608253697
$node_(2) set Z_ 0.000000000000
$node_(3) set X_ 158.840938304009
$node_(3) set Y_ 139.650216383776
$node_(3) set Z_ 0.000000000000
$node_(4) set X_ 101.186886005903
$node_(4) set Y_ 147.993190973633
$node_(4) set Z_ 0.000000000000
$node_(5) set X_ 321.560825121175
$node_(5) set Y_ 472.788096833600
$node_(5) set Z_ 0.000000000000
$node_(6) set X_ 149.543901734330
$node_(6) set Y_ 384.356581531832
$node_(6) set Z_ 0.000000000000
$node_(7) set X_ 381.066146464027
$node_(7) set Y_ 78.723958958779
$node_(7) set Z_ 0.000000000000
$node_(8) set X_ 113.578290026963
$node_(8) set Y_ 410.320583900348
$node_(8) set Z_ 0.000000000000
$node_(9) set X_ 258.053977148981
$node_(9) set Y_ 113.194171851670
$node_(9) set Z_ 0.000000000000

# starting gear filter separately- this will go away once gear has callbacks
# for updating node positions
for {set i 0} {$i < $opt(nn)} {incr i} {
    set gearf($i) [new Application/DiffApp/GeoRoutingFilter \
		       [$node_($i) get-dr]]
    $gearf($i) start
}


# 1 push sender
set src_(0) [new Application/DiffApp/GearSenderApp]
$ns_ attach-diffapp $node_(6) $src_(0)
$src_(0) push-pull-options push region 100 650 100 650
$ns_ at 0.123 "$src_(0) publish"


# 4 push receiver
set snk_(0) [new Application/DiffApp/GearReceiverApp]
$ns_ attach-diffapp $node_(8) $snk_(0)
$snk_(0) push-pull-options push point 101.186 147.993
$ns_ at 1.256 "$snk_(0) subscribe"

set snk_(1) [new Application/DiffApp/GearReceiverApp]
$ns_ attach-diffapp $node_(9) $snk_(1)
$snk_(1) push-pull-options push point 258.053 113.194
$ns_ at 1.456 "$snk_(1) subscribe"

set snk_(2) [new Application/DiffApp/GearReceiverApp]
$ns_ attach-diffapp $node_(7) $snk_(2)
$snk_(2) push-pull-options push point 321.560 472.788
$ns_ at 1.56 "$snk_(2) subscribe"

set snk_(3) [new Application/DiffApp/GearReceiverApp]
$ns_ attach-diffapp $node_(1) $snk_(3)
$snk_(3) push-pull-options push point 158.840 139.650
$ns_ at 1.73 "$snk_(3) subscribe"

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

puts $tracefd "Directed Diffusion:2PP/5nodes/1sender/1rcvr"
puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y)"
puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"

puts "Starting Simulation..."
$ns_ run

