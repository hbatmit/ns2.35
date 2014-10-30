# XXXXXXXX NOTE: This is an example of the older version of diffusion. For example scripts of the newer version, see tcl/ex/diffusion3. And see ~ns/diffusion3 for the implementation.

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
set opt(god)            off
set opt(x)		800	;# X dimension of the topography
set opt(y)		800     ;# Y dimension of the topography
set opt(traf)		"../test/sk-30-1-1-1-1-6-64.tcl"      ;# traffic file
set opt(topo)		"../test/scen-800x800-30-500-1.0-1"      ;# topology file
set opt(onoff)          ""      ;#node on-off
set opt(ifqlen)		50	;# max packet in ifq
set opt(nn)		30	;# number of nodes
set opt(seed)		0.0
set opt(stop)		20	;# simulation time
set opt(prestop)        19       ;# time to prepare to stop
set opt(tr)		"wireless.tr"	;# trace file
set opt(nam)            "wireless.nam"  ;# nam file
set opt(engmodel)       EnergyModel     ;
set opt(txPower)        0.660;
set opt(rxPower)        0.395;
set opt(idlePower)      0.035;
set opt(initeng)        10000.0         ;# Initial energy in Joules
set opt(logeng)         "off"           ;# log energy every 1 seconds
set opt(lm)             "off"           ;# log movement
set opt(adhocRouting)   DIFFUSION/PROB       
set opt(enablePos)      "true";
set opt(enableNeg)      "true";
set opt(duplicate)      "disable-duplicate"
set opt(suppression)    "false"

# ======================================================================

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

# ======================================================================

proc usage { argv0 }  {
    puts "Usage: $argv0"
    puts "\t\t\[-topo topology file\] \[-traf traffic file\]"
    puts "\t\t\[-x max x\] \[-y max y\] \[-seed seed\]"
    puts "\t\t\[-nam nam file\] \[-tr trace file\] \[-logeng on or off\]"
    puts "\t\t\[-stop time to stop\] \[-prestop time to prepare to stop\]"
    puts "\t\t\[-initeng initial energy\] \[-engmodel energy model\]" 
    puts "\t\t\[-chan channel model\] \[-prop propagation model\]"
    puts "\t\t\[-netif network interface\] \[-mac mac layer\]"
    puts "\t\t\[-ifq interface queue\] \[-ll link layer\] \[-ant antena\]"
    puts "\t\t\[-ifqlen interface queue length\] \[-nn number of nodes\]"
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


proc finish {} {
    global ns_ nf opt god_ node_

    $ns_ terminate-all-agents 

#    $god_ dump
    $god_ dump_num_send


    $ns_ flush-trace

    if [info exists tracefd] {
	close $tracefd
#	exec rm -f $opt(tr)
    }

    if [info exists nf] {
        close $nf
#	exec rm -f $opt(nam)
#        exec nam $opt(nam) &
#	exec gzip $opt(nam)
    }

    exit 0
}

proc cmu-trace { ttype atype node } {
	global ns_ tracefd

	if { $tracefd == "" } {
		return ""
	}
	set T [new CMUTrace/$ttype $atype]
	$T target [$ns_ set nullAgent_]
	$T attach $tracefd
        $T set src_ [$node id]

        $T node $node

	return $T
}

# ======================================================================
# Main Program
# ======================================================================
getopt $argc $argv

# do the get opt again incase the routing protocol file added some more
# options to look for

getopt $argc $argv

if {$opt(seed) > 0} {
	puts "Seeding Random number generator with $opt(seed)\n"
	ns-random $opt(seed)
}

#
# Initialize Global Variables
#

set ns_		[new Simulator] 

# define color index
  
$ns_ color 0 red
$ns_ color 1 blue
$ns_ color 2 chocolate
$ns_ color 3 yellow
$ns_ color 4 green
$ns_ color 5 tan
$ns_ color 6 gold
$ns_ color 7 black


#set chan	[new $opt(chan)]
#set prop	[new $opt(prop)]

set topo	[new Topography]


if { $opt(nam) != "" } {
    set nf [open $opt(nam) w]
    $ns_ namtrace-all-wireless $nf $opt(x) $opt(y)
}

if { $opt(tr) != ""} {
    set tracefd	[open $opt(tr) w]
    $ns_ trace-all $tracefd
}


$topo load_flatgrid $opt(x) $opt(y)

#$prop topography $topo


# Create God

set god_ [create-god $opt(nn)]
#$god_ on
$god_ $opt(god)
$god_ allow_to_stop
$god_ num_data_types 1

# log the mobile nodes movements if desired

if { $opt(lm) == "on" } {
    log-movement
}


#
# Create Diffusion Mobile Nodes
#
#
#for {set i 0} {$i < $opt(nn) } {incr i} {
#    diffusion-create-mobile-node $i
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
		 -agentTrace ON \
                 -routerTrace ON \
                 -macTrace ON \
		 -energyModel $opt(engmodel) \
		 -initialEnergy $opt(initeng) \
		 -txPower  $opt(txPower) \
		 -rxPower  $opt(rxPower) \
		 -idlePower  $opt(idlePower)
                  


#  Create the specified number of nodes [$opt(nn)] and "attach" them
#  to the channel. 

for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node $i]
        $node_($i) color black
	$node_($i) random-motion 0		;# disable random motion
        $god_ new_node $node_($i)
}


if { $opt(topo) == "" } {
	puts "*** NOTE: no topology file specified."
        set opt(topo) "none"
} else {
	puts "Loading topology file..."
	source $opt(topo)
	puts "Load complete..."
}

for {set i 0} {$i < $opt(nn)} {incr i} {
    $node_($i) namattach $nf
# 20 defines the node size in nam, must adjust it according to your scenario
   $ns_ initial_node_pos $node_($i) 20
}

if { $opt(onoff) == "" } {
	puts "*** NOTE: no node-on-off file specified."
        set opt(onoff) "none"
} else {
	puts "Loading node on-off file..."
	source $opt(onoff)
	puts "Load complete..."
}


#
# log energy if desired
#

if { $opt(logeng) == "on" } {
    log-energy
}


#
# Source the traffic scripts
#

if { $opt(traf) == "" } {
	puts "*** NOTE: no traffic file specified."
        set opt(traf) "none"
} else {
	puts "Loading traffic file..."
	source $opt(traf)
}


#
# Tell all the nodes when the simulation ends
#

$ns_ at $opt(prestop) "$ns_ prepare-to-stop"
$ns_ at $opt(stop).00000001    "finish"

for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).00000002 "$node_($i) reset";
}

# tell nam the simulation stop time
$ns_ at  $opt(stop).00000003	"$ns_ nam-end-wireless $opt(stop)"

$ns_ at $opt(stop).00000004 "puts \"NS EXITING...\" ; $ns_ halt"


puts $tracefd "M 0.0 nn $opt(nn) x $opt(x) y $opt(y)"
puts $tracefd "M 0.0 topo $opt(topo) traf $opt(traf) seed $opt(seed)"
puts $tracefd "M 0.0 prop $opt(prop) ant $opt(ant)"

puts "Starting Simulation..."
$ns_ run








