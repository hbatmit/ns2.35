## wireless-mobileIP-simulation

#                       o W1                 WIRED NODES
#                       |
#                       o W2
#                      / \
#                     /   \                    
#--*--*--*--*--*--*- o     o base-stn nodes  --*-*-*-*-*-*-*-
#                   HA      FA               
#                       o
#                  o    WL       o          WIRELESS NODE MOVING
#                WL               WL    FROM HA TO FA.
#
#

#options
set opt(chan)		Channel/WirelessChannel
set opt(prop)		Propagation/TwoRayGround
set opt(netif)		Phy/WirelessPhy
set opt(mac)		Mac/802_11
set opt(ifq)		Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(ant)            Antenna/OmniAntenna
set opt(x)		670	;# X & Y dimension of the topography
set opt(y)		670     ;# hard wired for now...
set opt(rp)             dsr    ;# rotuing protocls: dsdv/dsr
set opt(ifqlen)		50		;# max packet in ifq
set opt(seed)		0.0
set opt(stop)		250.0		;# simulation time
set opt(cc)             "off"
set opt(tr)		wireless-mip-out.tr	;# trace file
set opt(cp)             ""
set opt(sc)             ""
set opt(ftp1-start)     100.0

# =================================================================

set num_wired_nodes    2
set num_bs_nodes       2
set num_wireless_nodes 1

set opt(nn)            3         ;# total number of wireless nodes

#==================================================================

# Other class settings

set AgentTrace			ON
set RouterTrace			OFF
set MacTrace			OFF

LL set mindelay_		50us
LL set delay_			25us

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

# ==================================================================
#source ../lib/ns-bsnode.tcl
#source ../mobility/com.tcl
#source ../mobility/dsr.tcl
#source ../lib/ns-mip.tcl
source ../lib/ns-wireless-mip.tcl

# intial setup - set addressing to hierarchical
set ns [new Simulator]
$ns set-address-format hierarchical

# set mobileIP flag
Simulator set mobile_ip_ 1

set namtrace [open wireless-mip.nam w]
$ns namtrace-all $namtrace
set trace [open wireless-mip.tr w]
$ns trace-all $trace

AddrParams set domain_num_ 3
lappend cluster_num 2 1 1
AddrParams set cluster_num_ $cluster_num
lappend eilastlevel 1 1 4 1
AddrParams set nodes_num_ $eilastlevel

##debug 1
## setup the wired nodes
set temp {0.0.0 0.1.0}
for {set i 0} {$i < $num_wired_nodes} {incr i} {
    set W($i) [$ns node [lindex $temp $i]] 
}

## create common objects reqd for wireless sim.
if { $opt(x) == 0 || $opt(y) == 0 } {
	puts "No X-Y boundary values given for wireless topology\n"
}
set chan        [new $opt(chan)]
set prop        [new $opt(prop)]
set topo	[new Topography]
set tracefd	[open $opt(tr) w]

# setup topography and propagation model
$topo load_flatgrid $opt(x) $opt(y)
$prop topography $topo

# Create God
create-god $opt(nn)

## setup ForeignAgent and HomeAgent nodes
set HA [create-base-station-node 1.0.0]
set FA [create-base-station-node 2.0.0]

#provide some co-ord (fixed) to these base-station nodes.
$HA set X_ 1.000000000000
$HA set Y_ 2.000000000000
$HA set Z_ 0.000000000000

$FA set X_ 650.000000000000
$FA set Y_ 600.000000000000
$FA set Z_ 0.000000000000

# create a mobilenode that would be moving between HA and FA.
# note address of MH indicates its in the same domain as HA.

set MH [$opt(rp)-create-mobile-node 0 1.0.2]
set HAaddress [AddrParams addr2id [$HA node-addr]]
[$MH set regagent_] set home_agent_ $HAaddress

# movement of the MH

$MH set Z_ 0.000000000000
$MH set Y_ 2.000000000000
$MH set X_ 2.000000000000

# starts to move towards FA
$ns at 100.000000000000 "$MH setdest 640.000000000000 610.000000000000 20.000000000000"
# goes back to HA
$ns at 200.000000000000 "$MH setdest 2.000000000000 2.000000000000 20.000000000000"


if { $opt(x) == 0 || $opt(y) == 0 } {
	usage $argv0
	exit 1
}

if {$opt(seed) > 0} {
	puts "Seeding Random number generator with $opt(seed)\n"
	ns-random $opt(seed)
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

if { $opt(sc) == "" } {
	puts "*** NOTE: no scenario file specified."
        set opt(sc) "none"
} else {
	puts "Loading scenario file..."
	source $opt(sc)
	puts "Load complete..."
}

# create links between wired and BaseStation nodes
$ns duplex-link $W(0) $W(1) 5Mb 2ms DropTail
$ns duplex-link $W(1) $HA 5Mb 2ms DropTail
$ns duplex-link $W(1) $FA 5Mb 2ms DropTail

$ns duplex-link-op $W(0) $W(1) orient down
$ns duplex-link-op $W(1) $HA orient left-down
$ns duplex-link-op $W(1) $FA orient right-down

# setup TCP connections between a wired node and the MobileHost

set tcp1 [new Agent/TCP]
$tcp1 set class_ 2
set sink1 [new Agent/TCPSink]
$ns attach-agent $W(0) $tcp1
$ns attach-agent $MH $sink1
$ns connect $tcp1 $sink1
set ftp1 [new Application/FTP]
$ftp1 attach-agent $tcp1
$ns at $opt(ftp1-start) "$ftp1 start"

#
# Tell all the nodes when the simulation ends
#
for {set i 0} {$i < $num_wireless_nodes } {incr i} {
    $ns_ at $opt(stop).0000010 "$node_($i) reset";
}
$ns_ at $opt(stop).0000010 "$HA reset";
$ns_ at $opt(stop).0000010 "$FA reset";

$ns_ at $opt(stop).21 "finish"
$ns_ at $opt(stop).20 "puts \"NS EXITING...\" ; "
###$ns_ halt"

proc finish {} {
	global ns_ trace namtrace
	$ns_ flush-trace
	close $namtrace
	close $trace

	#puts "running nam..."
	#exec nam out.nam &
        puts "Finishing ns.."
	exit 0
}

puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y) rp $opt(rp)"
puts $tracefd "M 0.0 sc $opt(sc) cp $opt(cp) seed $opt(seed)"
puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"

puts "Starting Simulation..."
$ns_ run



