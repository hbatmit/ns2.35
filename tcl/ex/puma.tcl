set opt(stop)	15
set nodes       25
set mobility   	1
set scenario   	1
set pausetime 	0
set traffic    	cbr
set senders    	5
set receivers   20
set X		1000
set Y		1000

set ns_ [new Simulator]

set topo [new Topography]
$topo load_flatgrid $X $Y

set tracenam [open puma.nam w]
$ns_ namtrace-all-wireless $tracenam $X $Y

set tracefd [open puma.tr w]
#$ns_ use-newtrace
$ns_ trace-all $tracefd

set god_ [create-god $nodes]

$ns_ node-config -adhocRouting PUMA \
           -llType LL \
           -macType Mac/802_11 \
           -ifqLen 50 \
           -ifqType Queue/DropTail/PriQueue \
           -antType Antenna/OmniAntenna \
           -propType Propagation/TwoRayGround \
           -phyType Phy/WirelessPhy \
           -channel [new Channel/WirelessChannel] \
           -topoInstance $topo \
           -agentTrace ON \
           -routerTrace ON \
           -macTrace OFF

for {set i 0} {$i < $nodes} {incr i} {
   set node_($i) [$ns_ node]
   $node_($i) random-motion 0;
}

puts "Loading connection pattern ..."
source "puma-cbr-ex"

puts "Loading scenarios file..."
source "puma-scenario-ex"

for {set i 0} {$i < $nodes} {incr i} {
   $ns_ at $opt(stop) "$node_($i) reset";
}

Node instproc join { group } {
    $self instvar ragent_
    set group [expr $group]

    $ragent_ join $group
}

Node instproc leave { group } {
    $self instvar ragent_
    set group [expr $group] ;

    $ragent_ leave $group
}

$ns_ at $opt(stop) "$ns_ halt"
puts "Starting Simulation ..."
$ns_ run
puts "NS EXITING..."
