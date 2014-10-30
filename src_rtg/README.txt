
This file gives a brief description of the commands existing in ns to
simulate source routing. There is a sample file "src_rting.tcl" which
contains a sample script to simulate two source routed
connections. The steps involved in simulation:

a) To enable source routing use node-config command in the following
   manner:
	> $ns src_rting 1

b) To establish a connection and to specify the source route to be
   used use the following command:
	> set temp [$n1 set src_agent_]
	> $temp install_connection 3 1 3 1 2 3

   The first argurment here is the flow id for the connection. This is
   same as the flow id of the agent generating the traffic ( TCP or
   UDP). The second arguement is the source node id of the
   connection. The third arguement is the destination node id of the
   connection. The arguements following this form the source route of
   the connection.
   
c) To insert the routes in the packets generated from your agent ( TCP
   or UDP) you need to set the target of the agent to the source routing
   agent:
	> $tcp target [$n1 set src_agent_]



This steps will get the source routing working for you. A sample code is shown below :

st login: Thu May 10 2001 07:25:54 -0700 from 63.108.172.129
No mail.
bash: /etc/profile.d/alias.sh: No such file or directory
[rishi@theory rishi]$ cd RA/ns-allinone-2.1b7.changedandwork/ns-2.1b7a
[rishi@theory ns-2.1b7a]$ vi src_test.tcl 
#Create a simulator object
set ns [new Simulator]

$ns src_rting 1

# $ns node-config -perhopAgent Src_rt

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

$cbr1 target [$n3 set src_agent_]

$cbr0 target [$n0 set src_agent_]
set temp [$n0 set src_agent_]
$temp install_connection [$cbr0 set fid_] 0 1 0 2 4 3 1
set temp [$n3 set src_agent_]
set null0 [new Agent/TCPSink]
set null1 [new Agent/TCPSink]

$ns attach-agent $n1 $null0
$ns attach-agent $n4 $null1


$ns connect $cbr0 $null0
$ns connect $cbr1 $null1

$temp install_connection [$cbr1 set fid_] 3 4 3 2 4
set ftp1 [$cbr0 attach-source FTP]
set ftp2 [$cbr1 attach-source FTP]


$ns at 0.5 "$ftp1 start"
$cbr1 set fid_ 1

$cbr1 target [$n3 set src_agent_]

$cbr0 target [$n0 set src_agent_]
set temp [$n0 set src_agent_]
$temp install_connection [$cbr0 set fid_] 0 1 0 2 4 3 1
set temp [$n3 set src_agent_]
set null0 [new Agent/TCPSink]
set null1 [new Agent/TCPSink]

$ns attach-agent $n1 $null0
$ns attach-agent $n4 $null1


$ns connect $cbr0 $null0
$ns connect $cbr1 $null1

$temp install_connection [$cbr1 set fid_] 3 4 3 2 4
set ftp1 [$cbr0 attach-source FTP]
set ftp2 [$cbr1 attach-source FTP]


$ns at 0.5 "$ftp1 start"
#Create a TCP agent and attach it to node n0
set cbr0 [new Agent/TCP]
$ns attach-agent $n0 $cbr0
$cbr0 set fid_ 0

#Create a TCP agent and attach it to node n1
set cbr1 [new Agent/TCP]
$ns attach-agent $n3 $cbr1

$cbr1 set fid_ 1

$cbr1 target [$n3 set src_agent_]

$cbr0 target [$n0 set src_agent_]
set temp [$n0 set src_agent_]
$temp install_connection [$cbr0 set fid_] 0 1 0 2 4 3 1
set temp [$n3 set src_agent_]
set null0 [new Agent/TCPSink]
set null1 [new Agent/TCPSink]

$ns attach-agent $n1 $null0
$ns attach-agent $n4 $null1


$ns connect $cbr0 $null0
$ns connect $cbr1 $null1

$temp install_connection [$cbr1 set fid_] 3 4 3 2 4
set ftp1 [$cbr0 attach-source FTP]
set ftp2 [$cbr1 attach-source FTP]


$ns at 0.5 "$ftp1 start"
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

$cbr1 target [$n3 set src_agent_]

$cbr0 target [$n0 set src_agent_]
set temp [$n0 set src_agent_]
$temp install_connection [$cbr0 set fid_] 0 1 0 2 4 3 1
set temp [$n3 set src_agent_]
set null0 [new Agent/TCPSink]
set null1 [new Agent/TCPSink]

$ns attach-agent $n1 $null0
$ns attach-agent $n4 $null1


$ns connect $cbr0 $null0
$ns connect $cbr1 $null1

$temp install_connection [$cbr1 set fid_] 3 4 3 2 4
set ftp1 [$cbr0 attach-source FTP]
set ftp2 [$cbr1 attach-source FTP]


$ns at 0.5 "$ftp1 start"

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

$cbr1 target [$n3 set src_agent_]

$cbr0 target [$n0 set src_agent_]
set temp [$n0 set src_agent_]
$temp install_connection [$cbr0 set fid_] 0 1 0 2 4 3 1
set temp [$n3 set src_agent_]
set null0 [new Agent/TCPSink]
set null1 [new Agent/TCPSink]

$ns attach-agent $n1 $null0
$ns attach-agent $n4 $null1


$ns connect $cbr0 $null0
$ns connect $cbr1 $null1

$temp install_connection [$cbr1 set fid_] 3 4 3 2 4
set ftp1 [$cbr0 attach-source FTP]
set ftp2 [$cbr1 attach-source FTP]


$ns at 0.5 "$ftp1 start"
$ns at 0.5 "$ftp2 start"
#Open the nam trace file
set nf [open out.tr w]
$ns trace-all $nf
set f [open out.nam w]
$ns namtrace-all $f

#Create four nodes
#Create a simulator object
set ns [new Simulator]

$ns src_rting 1

# $ns node-config -perhopAgent Src_rt

$ns color 1 Blue
$ns color 2 Red

#Open the nam trace file
set nf [open out.tr w]
$ns trace-all $nf
set f [open out.nam w]
$ns namtrace-all $f

#Create four nodes
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
"src_test.tcl" 73L, 1480C written
[rishi@theory ns-2.1b7a]$ vi src_test.tcl 
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

