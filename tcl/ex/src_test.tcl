# 
# Contributed by Rishi Bhargava <rishi_bhargava@yahoo.com> May, 2001.
# Example script for source-routing; also see ~ns/src_rtg/README.txt
#

#Create a simulator object
set ns [new Simulator] 

# enable source routing

$ns src_rting 1

$ns color 1 Blue
$ns color 2 Red

#Open the nam trace file
set nf [open out.tr w]
$ns trace-all $nf
set f [open out.nam w]
$ns namtrace-all $f

#Create four nodes
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]

#Create links between the nodes
$ns duplex-link $n0 $n2 1Mb 10ms DropTail
$ns duplex-link $n1 $n2 1Mb 10ms DropTail
$ns duplex-link $n3 $n2 1Mb 10ms DropTail
$ns duplex-link $n2 $n4 1Mb 10ms DropTail
$ns duplex-link $n4 $n3 1Mb 10ms DropTail
$ns duplex-link $n1 $n3 1Mb 10ms DropTail

#Create a TCP agent and attach it to node n0
set cbr0 [new Agent/TCP]
$ns attach-agent $n0 $cbr0
$cbr0 set fid_ 0

#Create a TCP agent and attach it to node n1
set cbr1 [new Agent/TCP]
$ns attach-agent $n3 $cbr1

$cbr1 set fid_ 1

$cbr0 target [$n0 set src_agent_]
$cbr1 target [$n3 set src_agent_]

set temp [$n0 set src_agent_]
$temp install_connection [$cbr0 set fid_] 0 1 0 2 4 3 1
set temp [$n3 set src_agent_]
$temp install_connection [$cbr1 set fid_] 3 4 3 2 4 

set null0 [new Agent/TCPSink]
set null1 [new Agent/TCPSink]

$ns attach-agent $n1 $null0
$ns attach-agent $n4 $null1


$ns connect $cbr0 $null0  
$ns connect $cbr1 $null1

set ftp1 [$cbr0 attach-source FTP]
set ftp2 [$cbr1 attach-source FTP]


$ns at 0.5 "$ftp1 start"
$ns at 0.5 "$ftp2 start"
$ns at 5.0 "$ftp1 stop"
$ns at 10.5 "$ftp2 stop"


$ns at 11.0 "$ns halt"

$ns run
