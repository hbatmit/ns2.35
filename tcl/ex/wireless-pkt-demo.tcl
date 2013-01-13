# Copyright (c) 1999 Regents of the University of Southern California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by the Computer Systems
#      Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# A simple example for visualization of packet flow in wireless simulation
# Kun-chan Lan kclan@isi.edu, 1999

# ======================================================================
# Define options
# ======================================================================

set opt(chan)	Channel/WirelessChannel
set opt(prop)	Propagation/TwoRayGround
set opt(netif)	Phy/WirelessPhy
set opt(mac)	Mac/802_11
set opt(ifq)	Queue/DropTail/PriQueue
set opt(ll)		LL
set opt(ant)        Antenna/OmniAntenna
set opt(x)		670   ;# X dimension of the topography
set opt(y)		670   ;# Y dimension of the topography
set opt(ifqlen)	50	      ;# max packet in ifq
set opt(seed)	0.0
set opt(tr)		pktdemo.tr    ;# trace file
set opt(nam)            pktdemo.nam   ;# nam trace file
set opt(adhocRouting)   DSDV
set opt(nn)             5             ;# how many nodes are simulated
set opt(stop)		90.0		;# simulation time

# =====================================================================
# Other default settings

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used

Agent/Null set sport_		0
Agent/Null set dport_		0

Agent/CBR set sport_		0
Agent/CBR set dport_		0

Agent/TCPSink set sport_	0
Agent/TCPSink set dport_	0

Agent/TCP set sport_		0
Agent/TCP set dport_		0
Agent/TCP set packetSize_	512

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
# Main Program
# ======================================================================


#
# Initialize Global Variables
#

# create simulator instance

set ns_		[new Simulator]

# set wireless channel, radio-model and topography objects

#set wchan	[new $opt(chan)]
#set wprop	[new $opt(prop)]
set wtopo	[new Topography]

# create trace object for ns and nam

set tracefd	[open $opt(tr) w]
set namtrace    [open $opt(nam) w]

$ns_ trace-all $tracefd
$ns_ namtrace-all-wireless $namtrace $opt(x) $opt(y)

# define topology
$wtopo load_flatgrid $opt(x) $opt(y)

#$wprop topography $wtopo

#
# Create God
#
set god_ [create-god $opt(nn)]

#
# define how node should be created
#

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
		 -topoInstance $wtopo \
		 -agentTrace ON \
                 -routerTrace OFF \
                 -macTrace OFF 

#
#  Create the specified number of nodes [$opt(nn)] and "attach" them
#  to the channel. 

for {set i 0} {$i < $opt(nn) } {incr i} {
	set node_($i) [$ns_ node]	
	$node_($i) random-motion 0		;# disable random motion
}


# 
# Define node movement model
#
puts "Loading connection pattern..."
#
# nodes: 5, pause: 2.00, max speed: 10.00  max x = 670.00, max y: 670.00
#
$node_(0) set X_ 422.716707738489
$node_(0) set Y_ 589.707335765875
$node_(0) set Z_ 0.000000000000
$node_(1) set X_ 571.192740186325
$node_(1) set Y_ 276.384818286195
$node_(1) set Z_ 0.000000000000
$node_(2) set X_ 89.641181272212
$node_(2) set Y_ 439.333721576041
$node_(2) set Z_ 0.000000000000
$node_(3) set X_ 481.858918255772
$node_(3) set Y_ 312.839552218736
$node_(3) set Z_ 0.000000000000
$node_(4) set X_ 404.354417812321
$node_(4) set Y_ 174.700530392536
$node_(4) set Z_ 0.000000000000
$god_ set-dist 0 1 16777215
$god_ set-dist 0 2 16777215
$god_ set-dist 0 3 16777215
$god_ set-dist 0 4 16777215
$god_ set-dist 1 2 16777215
$god_ set-dist 1 3 1
$god_ set-dist 1 4 1
$god_ set-dist 2 3 16777215
$god_ set-dist 2 4 16777215
$god_ set-dist 3 4 1
$ns_ at 2.000000000000 "$node_(0) setdest 251.814462320539 525.668444673569 4.321643209007"
$ns_ at 2.000000000000 "$node_(1) setdest 258.446979634068 108.386939063715 8.944551343066"
$ns_ at 2.000000000000 "$node_(2) setdest 71.986866627739 533.267476248005 0.544384676257"
$ns_ at 2.000000000000 "$node_(3) setdest 634.708039841038 468.026171380176 4.714370176280"
$ns_ at 2.000000000000 "$node_(4) setdest 296.110313540578 636.039939161363 1.093403720748"
$ns_ at 23.463858803761 "$god_ set-dist 1 3 2"
$ns_ at 25.330489080195 "$god_ set-dist 1 3 16777215"
$ns_ at 25.330489080195 "$god_ set-dist 3 4 16777215"
$ns_ at 28.014970660896 "$god_ set-dist 0 2 1"
$ns_ at 41.690257555628 "$node_(1) setdest 258.446979634068 108.386939063715 0.000000000000"
$ns_ at 43.690257555628 "$node_(1) setdest 458.034484352038 555.578911026289 7.384449428202"
$ns_ at 44.230770051977 "$node_(0) setdest 251.814462320539 525.668444673569 0.000000000000"
$ns_ at 46.230770051977 "$node_(0) setdest 29.583605260695 71.653642749196 4.369757451990"
$ns_ at 48.203506338157 "$node_(3) setdest 634.708039841038 468.026171380176 0.000000000000"
$ns_ at 50.203506338157 "$node_(3) setdest 168.404464525783 293.835434220046 8.837958163505"
$ns_ at 61.146282049441 "$god_ set-dist 1 3 2"
$ns_ at 61.146282049441 "$god_ set-dist 3 4 1"
$ns_ at 61.940183593125 "$god_ set-dist 0 1 1"
$ns_ at 61.940183593125 "$god_ set-dist 0 3 3"
$ns_ at 61.940183593125 "$god_ set-dist 0 4 2"
$ns_ at 61.940183593125 "$god_ set-dist 1 2 2"
$ns_ at 61.940183593125 "$god_ set-dist 2 3 4"
$ns_ at 61.940183593125 "$god_ set-dist 2 4 3"
$ns_ at 65.320001199434 "$god_ set-dist 0 3 2"
$ns_ at 65.320001199434 "$god_ set-dist 1 3 1"
$ns_ at 65.320001199434 "$god_ set-dist 2 3 3"
$ns_ at 72.479833272291 "$god_ set-dist 0 3 1"
$ns_ at 72.479833272291 "$god_ set-dist 2 3 2"
$ns_ at 74.141501617339 "$god_ set-dist 0 4 1"
$ns_ at 74.141501617339 "$god_ set-dist 2 4 2"
$ns_ at 91.315105963178 "$god_ set-dist 0 1 2"
$ns_ at 91.315105963178 "$god_ set-dist 1 2 3"
$ns_ at 93.313427973009 "$god_ set-dist 1 2 2"
$ns_ at 93.313427973009 "$god_ set-dist 2 3 1"
$ns_ at 98.710174388350 "$god_ set-dist 1 2 3"
$ns_ at 98.710174388350 "$god_ set-dist 1 3 2"
$ns_ at 105.625684740133 "$god_ set-dist 0 1 16777215"
$ns_ at 105.625684740133 "$god_ set-dist 1 2 16777215"
$ns_ at 105.625684740133 "$god_ set-dist 1 3 16777215"
$ns_ at 105.625684740133 "$god_ set-dist 1 4 16777215"
$ns_ at 106.526073618179 "$node_(3) setdest 168.404464525783 293.835434220046 0.000000000000"
$ns_ at 108.526073618179 "$node_(3) setdest 640.711745744611 202.311298064598 9.940100763372"
$ns_ at 110.006636507140 "$node_(1) setdest 458.034484352038 555.578911026289 0.000000000000"
$ns_ at 110.761159202811 "$god_ set-dist 0 4 2"
$ns_ at 112.006636507140 "$node_(1) setdest 219.327100186826 560.573034522603 0.164055503041"
$ns_ at 112.763789530447 "$god_ set-dist 2 3 2"
$ns_ at 112.763789530447 "$god_ set-dist 2 4 3"
$ns_ at 115.605147677116 "$god_ set-dist 0 2 16777215"
$ns_ at 115.605147677116 "$god_ set-dist 2 3 16777215"
$ns_ at 115.605147677116 "$god_ set-dist 2 4 16777215"
$ns_ at 126.409917266767 "$god_ set-dist 0 3 16777215"
$ns_ at 126.409917266767 "$god_ set-dist 0 4 16777215"
$ns_ at 138.496739882231 "$god_ set-dist 1 3 2"
$ns_ at 138.496739882231 "$god_ set-dist 1 4 1"
$ns_ at 151.316448327659 "$god_ set-dist 1 3 16777215"
$ns_ at 151.316448327659 "$god_ set-dist 3 4 16777215"
$ns_ at 156.925318894661 "$node_(3) setdest 640.711745744611 202.311298064598 0.000000000000"
$ns_ at 158.925318894661 "$node_(3) setdest 487.816263862197 607.947164671046 3.999957695392"
$ns_ at 161.909020998556 "$node_(0) setdest 29.583605260695 71.653642749196 0.000000000000"
$ns_ at 163.909020998556 "$node_(0) setdest 488.362330344042 405.686525544012 6.767689225264"
$ns_ at 177.571384787193 "$node_(2) setdest 71.986866627739 533.267476248005 0.000000000000"
$ns_ at 179.571384787193 "$node_(2) setdest 305.038606222002 613.855043777391 5.995840465332"
$ns_ at 187.725197579176 "$god_ set-dist 1 3 2"
$ns_ at 187.725197579176 "$god_ set-dist 3 4 1"
$ns_ at 193.657167555999 "$god_ set-dist 0 1 2"
$ns_ at 193.657167555999 "$god_ set-dist 0 3 2"
$ns_ at 193.657167555999 "$god_ set-dist 0 4 1"
$ns_ at 198.286255168888 "$god_ set-dist 1 3 1"

# 
# Define traffic model
#
puts "Loading scenario file..."
#
# nodes: 5, max conn: 8, send rate: 0.25, seed: 1.0
#
#
# 2 connecting to 3 at time 40.557023746220864
#
set udp_(0) [new Agent/UDP]
$ns_ attach-agent $node_(2) $udp_(0)
set null_(0) [new Agent/Null]
$ns_ attach-agent $node_(3) $null_(0)
set cbr_(0) [new Application/Traffic/CBR]
$cbr_(0) set packetSize_ 512
$cbr_(0) set interval_ 0.05
$cbr_(0) set random_ 1
$cbr_(0) set maxpkts_ 10000
$cbr_(0) attach-agent $udp_(0)
$ns_ connect $udp_(0) $null_(0)
$ns_ at 40.557023746220864 "$cbr_(0) start"
#
# 2 connecting to 4 at time 42.898102734190459
#
set udp_(1) [new Agent/UDP]
$ns_ attach-agent $node_(2) $udp_(1)
set null_(1) [new Agent/Null]
$ns_ attach-agent $node_(4) $null_(1)
set cbr_(1) [new Application/Traffic/CBR]
$cbr_(1) set packetSize_ 512
$cbr_(1) set interval_ 0.05
$cbr_(1) set random_ 1
$cbr_(1) set maxpkts_ 10000
$cbr_(1) attach-agent $udp_(1)
$ns_ connect $udp_(1) $null_(1)
$ns_ at 42.898102734190459 "$cbr_(1) start"
#
# 0 connecting to 4 at time 45.898102734190459
#
set udp_(2) [new Agent/UDP]
$ns_ attach-agent $node_(0) $udp_(2)
set null_(2) [new Agent/Null]
$ns_ attach-agent $node_(4) $null_(2)
set cbr_(2) [new Application/Traffic/CBR]
$cbr_(2) set packetSize_ 512
$cbr_(2) set interval_ 0.05
$cbr_(2) set random_ 1
$cbr_(2) set maxpkts_ 10000
$cbr_(2) attach-agent $udp_(2)
$ns_ connect $udp_(2) $null_(2)
$ns_ at 45.898102734190459 "$cbr_(2) start"
#
# nodes: 5, max conn: 8, send rate: 0.0, seed: 1.0
#
# 1 connecting to 3 at time 80.557023746220864
#
set tcp_(0) [$ns_ create-connection  TCP $node_(1) TCPSink $node_(3) 0]
$tcp_(0) set window_ 32
$tcp_(0) set packetSize_ 512
set ftp_(0) [$tcp_(0) attach-source FTP]
$ns_ at 80.557023746220864 "$ftp_(0) start"
#
# 2 connecting to 0 at time 45.557023746220864
#
set tcp_(1) [$ns_ create-connection  TCP $node_(2) TCPSink $node_(0) 0]
$tcp_(1) set window_ 32
$tcp_(1) set packetSize_ 512
set ftp_(1) [$tcp_(1) attach-source FTP]
$ns_ at 45.557023746220864 "$ftp_(1) start"
#
# 1 connecting to 4 at time 50.898102734190459
#
set tcp_(2) [$ns_ create-connection  TCP $node_(1) TCPSink $node_(4) 0]
$tcp_(2) set window_ 32
$tcp_(2) set packetSize_ 512
set ftp_(2) [$tcp_(2) attach-source FTP]
$ns_ at 50.898102734190459 "$ftp_(2) start"
#

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


puts "Starting Simulation..."
$ns_ run




