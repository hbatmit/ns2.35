###################################################
#         Peer-to-Peer Tree over 802.15.4         #
#                (beacon enabled)                 #
#        Copyright (c) 2003 Samsung/CUNY          #
# - - - - - - - - - - - - - - - - - - - - - - - - #
#           Prepared by Jianliang Zheng           #
#            (zheng@ee.ccny.cuny.edu)             #
###################################################

# ======================================================================
# Define options
# ======================================================================
set val(chan)           Channel/WirelessChannel    ;# Channel Type
set val(prop)           Propagation/TwoRayGround   ;# radio-propagation model
set val(netif)          Phy/WirelessPhy/802_15_4
set val(mac)            Mac/802_15_4
set val(ifq)            Queue/DropTail/PriQueue    ;# interface queue type
set val(ll)             LL                         ;# link layer type
set val(ant)            Antenna/OmniAntenna        ;# antenna model
set val(ifqlen)         50                         ;# max packet in ifq
set val(nn)             21                         ;# number of mobilenodes
set val(rp)             AODV                       ;# AODV Routing
set val(x)		50
set val(y)		50

set val(nam)		wpan_demo4.nam
set val(traffic)	mix                        ;# mix/cbr/poisson/ftp

#read command line arguments
proc getCmdArgu {argc argv} {
        global val
        for {set i 0} {$i < $argc} {incr i} {
                set arg [lindex $argv $i]
                if {[string range $arg 0 0] != "-"} continue
                set name [string range $arg 1 end]
                set val($name) [lindex $argv [expr $i+1]]
        }
}
getCmdArgu $argc $argv

set appTime1         	10.3	;# in seconds 
set appTime2         	10.6	;# in seconds 
set stopTime            100	;# in seconds 

# Initialize Global Variables
set ns_		[new Simulator]
set tracefd     [open ./wpan_demo4.tr w]
$ns_ trace-all $tracefd
if { "$val(nam)" == "wpan_demo4.nam" } {
        set namtrace     [open ./$val(nam) w]
        $ns_ namtrace-all-wireless $namtrace $val(x) $val(y)
}

$ns_ puts-nam-traceall {# nam4wpan #}		;# inform nam that this is a trace file for wpan (special handling needed)

Mac/802_15_4 wpanCmd verbose on
Mac/802_15_4 wpanNam namStatus on		;# default = off (should be turned on before other 'wpanNam' commands can work)
#Mac/802_15_4 wpanNam ColFlashClr gold		;# default = gold
#Mac/802_15_4 wpanNam NodeFailClr grey		;# default = grey

# For model 'TwoRayGround'
set dist(5m)  7.69113e-06
set dist(9m)  2.37381e-06
set dist(10m) 1.92278e-06
set dist(11m) 1.58908e-06
set dist(12m) 1.33527e-06
set dist(13m) 1.13774e-06
set dist(14m) 9.81011e-07
set dist(15m) 8.54570e-07
set dist(16m) 7.51087e-07
set dist(20m) 4.80696e-07
set dist(25m) 3.07645e-07
set dist(30m) 2.13643e-07
set dist(35m) 1.56962e-07
set dist(40m) 1.20174e-07
Phy/WirelessPhy set CSThresh_ $dist(12m)
Phy/WirelessPhy set RXThresh_ $dist(12m)

# set up topography object
set topo       [new Topography]
$topo load_flatgrid $val(x) $val(y)

# Create God
set god_ [create-god $val(nn)]

set chan_1_ [new $val(chan)]

# configure node

$ns_ node-config -adhocRouting $val(rp) \
		-llType $val(ll) \
		-macType $val(mac) \
		-ifqType $val(ifq) \
		-ifqLen $val(ifqlen) \
		-antType $val(ant) \
		-propType $val(prop) \
		-phyType $val(netif) \
		-topoInstance $topo \
		-agentTrace OFF \
		-routerTrace OFF \
		-macTrace ON \
		-movementTrace OFF \
                #-energyModel "EnergyModel" \
                #-initialEnergy 1 \
                #-rxPower 0.3 \
                #-txPower 0.3 \
		-channel $chan_1_

for {set i 0} {$i < $val(nn) } {incr i} {
	set node_($i) [$ns_ node]
	$node_($i) random-motion 0		;# disable random motion
}


#set Cm 4
#set Lm 3
#Agent/ZBR Cm $Cm
#Agent/ZBR Lm $Lm
#Agent/ZBR BSize calc
#set BlockSize [Agent/ZBR BSize]
#set cskip0 [Agent/ZBR CSkip 0]
#set cskip1 [Agent/ZBR CSkip 1]
#set cskip2 [Agent/ZBR CSkip 2]
#puts [format "Cm:        %s" $Cm]
#puts [format "Lm:        %s" $Lm]
#puts [format "BlockSize: %s" $BlockSize]
#$ns_ at 0.0 "$ns_ trace-annotate \"Cm = $Cm, Lm = $Lm, BlockSize = $BlockSize, CSkip0 = $cskip0, CSkip1 = $cskip1, CSkip2 = $cskip2\""

# default is RN+, so only set RN-
#$node_(3) RNType 0
#$node_(4) RNType 0
#$node_(7) RNType 0
#$node_(8) RNType 0
#$node_(9) RNType 0
#$node_(10) RNType 0
#$node_(13) RNType 0
#$node_(15) RNType 0
#$node_(18) RNType 0
#$node_(20) RNType 0

source ./wpan_demo4.scn

$ns_ at 0.0	"$node_(0) NodeLabel \"PAN Coor\""
#for {set i 1} {$i < $val(nn)} {incr i} {
#	set RNType [eval \$node_($i) RNType]
#	if {$RNType == "0"} {	
#		$ns_ at 0.0	"$node_($i) NodeLabel -"
#	} else {
#		$ns_ at 0.0	"$node_($i) NodeLabel +"
#	}
#}

$ns_ at 0.0	"$node_(0) sscs startCTPANCoord 1"	;# startCTPANCoord <txBeacon=1> <BO=3> <SO=3>
$ns_ at 0.3	"$node_(1) sscs startCTDevice 1 1" 	;# startCTDevice <isFFD=1> <assoPermit=1> <txBeacon=0> <BO=3> <SO=3>
$ns_ at 1.3	"$node_(9) sscs startCTDevice 1 1"
$ns_ at 1.7	"$node_(13) sscs startCTDevice 1 1"
$ns_ at 2.3	"$node_(19) sscs startCTDevice 1 1"

$ns_ at 3.3	"$node_(2) sscs startCTDevice 1 1 1"
$ns_ at 3.5	"$node_(7) sscs startCTDevice 1 1 1"
$ns_ at 3.6	"$node_(11) sscs startCTDevice 1 1 1"
$ns_ at 3.8	"$node_(16) sscs startCTDevice 1 1 1"

$ns_ at 4.3	"$node_(3) sscs startCTDevice 0 0"
$ns_ at 4.5	"$node_(6) sscs startCTDevice 1 1"
$ns_ at 4.8	"$node_(12) sscs startCTDevice 1 0"
$ns_ at 5.1	"$node_(17) sscs startCTDevice 1 0"

$ns_ at 5.6	"$node_(20) sscs startCTDevice 1 1"
$ns_ at 5.8	"$node_(5) sscs startCTDevice 1 1"
$ns_ at 6.0	"$node_(10) sscs startCTDevice 1 1"
$ns_ at 6.3	"$node_(14) sscs startCTDevice 1 1"

$ns_ at 6.8	"$node_(18) sscs startCTDevice 1 0"
$ns_ at 7.0	"$node_(4) sscs startCTDevice 0 0"
$ns_ at 7.3	"$node_(8) sscs startCTDevice 0 0"
$ns_ at 7.7	"$node_(15) sscs startCTDevice 0 0"

$ns_ at $appTime1 "$node_(0) sscs stopBeacon"
$ns_ at $appTime1 "$node_(2) sscs startBeacon 4 4"
$ns_ at $appTime1 "$node_(7) sscs startBeacon 4 4"
#$ns_ at $appTime1 "$node_(11) sscs stopBeacon"
#$ns_ at $appTime1 "$node_(16) sscs stopBeacon"

Mac/802_15_4 wpanNam PlaybackRate 4ms
$ns_ at $appTime1 "Mac/802_15_4 wpanNam PlaybackRate 1.0ms"
$ns_ at [expr $appTime1 + 0.5] "Mac/802_15_4 wpanNam PlaybackRate 2.0ms"

$ns_ at $appTime1 "puts \"\nTransmitting data ...\n\""

# Setup traffic flow between nodes

proc cbrtraffic { src dst interval starttime } {
   global ns_ node_
   set udp_($src) [new Agent/UDP]
   eval $ns_ attach-agent \$node_($src) \$udp_($src)
   set null_($dst) [new Agent/Null]
   eval $ns_ attach-agent \$node_($dst) \$null_($dst)
   set cbr_($src) [new Application/Traffic/CBR]
   eval \$cbr_($src) set packetSize_ 80
   eval \$cbr_($src) set interval_ $interval
   eval \$cbr_($src) set random_ 0
   #eval \$cbr_($src) set maxpkts_ 10000
   eval \$cbr_($src) attach-agent \$udp_($src)
   eval $ns_ connect \$udp_($src) \$null_($dst)
   $ns_ at $starttime "$cbr_($src) start"
}

proc poissontraffic { src dst interval starttime } {
   global ns_ node_
   set udp($src) [new Agent/UDP]
   eval $ns_ attach-agent \$node_($src) \$udp($src)
   set null($dst) [new Agent/Null]
   eval $ns_ attach-agent \$node_($dst) \$null($dst)
   set expl($src) [new Application/Traffic/Exponential]
   eval \$expl($src) set packetSize_ 70
   eval \$expl($src) set burst_time_ 0
   eval \$expl($src) set idle_time_ [expr $interval*1000.0-70.0/100]ms	;# idle_time + pkt_tx_time = interval
   eval \$expl($src) set rate_ 100k
   eval \$expl($src) attach-agent \$udp($src)
   eval $ns_ connect \$udp($src) \$null($dst)
   $ns_ at $starttime "$expl($src) start"
}

if {$val(rp) == "ZBR"} {
	Mac/802_15_4 wpanCmd callBack 2	;# 0=none; 1=failure only (default); 2=both failure and success
}

if { ("$val(traffic)" == "mix") || ("$val(traffic)" == "cbr") || ("$val(traffic)" == "poisson") } {
   if { "$val(traffic)" == "mix" } {
   	set trafficName "cbr + poisson"
   	set traffic1 cbr
   	set traffic2 poisson
   } else {
   	set trafficName $val(traffic)
   	set traffic1 $val(traffic)
   	set traffic2 $val(traffic)
   }
   puts "\nTraffic: $trafficName"
   #Mac/802_15_4 wpanCmd ack4data on
   puts [format "Acknowledgement for data: %s" [Mac/802_15_4 wpanCmd ack4data]]
   ${traffic1}traffic 3 18 0.2 $appTime1
   ${traffic2}traffic 9 17 0.2 $appTime2
   Mac/802_15_4 wpanNam FlowClr -p AODV -c tomato
   Mac/802_15_4 wpanNam FlowClr -p ARP -c green
   Mac/802_15_4 wpanNam FlowClr -p MAC -c navy
   $ns_ at $appTime1 "$node_(3) add-mark m1 blue circle"
   $ns_ at $appTime1 "$node_(18) add-mark m2 blue circle"
   $ns_ at $appTime1 "$ns_ trace-annotate \"(at $appTime1) $traffic1 traffic from node 3 to node 18\""
   $ns_ at $appTime2 "$node_(9) add-mark m3 green4 circle"
   $ns_ at $appTime2 "$node_(17) add-mark m4 green4 circle"
   $ns_ at $appTime2 "$ns_ trace-annotate \"(at $appTime2) $traffic2 traffic from node 9 to node 17\""
   $ns_ at [expr $appTime1 + 2.0] "Mac/802_15_4 wpanCmd link-down 0 19"
   set tmpTime [format "%.1f" [expr $appTime1 + 2.0]]
   $ns_ at [expr $appTime1 + 2.0] "$ns_ trace-annotate \"(at $tmpTime) link down: 0 -> 19\""
   $ns_ at [expr $appTime2 + 3.0] "$node_(2) node-down"
   set tmpTime [format "%.1f" [expr $appTime2 + 3.0]]
   $ns_ at [expr $appTime2 + 3.0] "$ns_ trace-annotate \"(at $tmpTime) node down: 2\""
   $ns_ at [expr $appTime1 + 8.0] "Mac/802_15_4 wpanCmd link-up 0 19"
   set tmpTime [format "%.1f" [expr $appTime1 + 8.0]]
   $ns_ at [expr $appTime1 + 8.0] "$ns_ trace-annotate \"(at $tmpTime) link up: 0 -> 19\""
   $ns_ at [expr $appTime2 + 10.0] "$node_(2) node-up"
   set tmpTime [format "%.1f" [expr $appTime2 + 10.0]]
   $ns_ at [expr $appTime2 + 10.0] "$ns_ trace-annotate \"(at $tmpTime) node up: 2\""
   if { "$val(traffic)" == "cbr" } {
   	set pktType cbr
   	set pktType2 cbr
   } elseif { "$val(traffic)" == "poission" } {
   	set pktType exp
   	set pktType2 exp
   } else {
   	set pktType cbr
   	set pktType2 exp
   }
   Mac/802_15_4 wpanNam FlowClr -p $pktType -s 3 -d 18 -c blue
   Mac/802_15_4 wpanNam FlowClr -p $pktType2 -s 9 -d 17 -c green4
}

proc ftptraffic { src dst starttime } {
   global ns_ node_
   set tcp($src) [new Agent/TCP]
   eval \$tcp($src) set packetSize_ 60
   set sink($dst) [new Agent/TCPSink]
   eval $ns_ attach-agent \$node_($src) \$tcp($src)
   eval $ns_ attach-agent \$node_($dst) \$sink($dst)
   eval $ns_ connect \$tcp($src) \$sink($dst)
   set ftp($src) [new Application/FTP]
   eval \$ftp($src) attach-agent \$tcp($src)
   $ns_ at $starttime "$ftp($src) start"
}

if { "$val(traffic)" == "ftp" } {
   puts "\nTraffic: ftp"
   #Mac/802_15_4 wpanCmd ack4data off
   puts [format "Acknowledgement for data: %s" [Mac/802_15_4 wpanCmd ack4data]]
   ftptraffic 16 3 $appTime1
   ftptraffic 8 11 $appTime2
   Mac/802_15_4 wpanNam FlowClr -p AODV -c tomato
   Mac/802_15_4 wpanNam FlowClr -p ARP -c green
   Mac/802_15_4 wpanNam FlowClr -p MAC -c navy
   $ns_ at $appTime1 "$node_(16) add-mark m1 blue circle"
   $ns_ at $appTime1 "$node_(3) add-mark m2 blue circle"
   $ns_ at $appTime1 "$ns_ trace-annotate \"(at $appTime1) ftp traffic from node 16 to node 3\""
   Mac/802_15_4 wpanNam FlowClr -p tcp -s 16 -d 3 -c blue
   Mac/802_15_4 wpanNam FlowClr -p ack -s 3 -d 16 -c blue
   $ns_ at $appTime2 "$node_(8) add-mark m3 green4 circle"
   $ns_ at $appTime2 "$node_(11) add-mark m4 green4 circle"
   $ns_ at $appTime2 "$ns_ trace-annotate \"(at $appTime2) ftp traffic from node 8 to node 11\""
   Mac/802_15_4 wpanNam FlowClr -p tcp -s 8 -d 11 -c green4
   Mac/802_15_4 wpanNam FlowClr -p ack -s 11 -d 8 -c green4
}

# defines the node size in nam
for {set i 0} {$i < $val(nn)} {incr i} {
	$ns_ initial_node_pos $node_($i) 2
}

# Tell nodes when the simulation ends
for {set i 0} {$i < $val(nn) } {incr i} {
    $ns_ at $stopTime "$node_($i) reset";
}

$ns_ at $stopTime "stop"
$ns_ at $stopTime "puts \"\nNS EXITING...\n\""
$ns_ at $stopTime "$ns_ halt"

proc stop {} {
    global ns_ tracefd appTime val env
    $ns_ flush-trace
    close $tracefd
    set hasDISPLAY 0
    foreach index [array names env] {
        #puts "$index: $env($index)"
        if { ("$index" == "DISPLAY") && ("$env($index)" != "") } {
                set hasDISPLAY 1
        }
    }
    if { ("$val(nam)" == "wpan_demo4.nam") && ("$hasDISPLAY" == "1") } {
    	exec nam wpan_demo4.nam &
    }
}

puts "\nStarting Simulation..."
$ns_ run
