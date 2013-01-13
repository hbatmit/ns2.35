# Testing passing real world traffic through the 
#  ns  Simulator 
# Author : Alefiya Hussain and Ankur Sheth
# Date   : 05/14/2001


set ns [new Simulator]
$ns use-scheduler RealTime

set f [open out.tr w]
$ns trace-all $f 
set nf [open out.nam w]
$ns namtrace-all $nf

# Create the nodes needed to the transducer 
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]

# Setup connections between the nodes 
$ns simplex-link $n1 $n5 10Mb 5ms DropTail 
$ns simplex-link $n5 $n4 10Mb 5ms DropTail
$ns simplex-link $n3 $n5 10Mb 5ms DropTail
$ns simplex-link $n5 $n2 10Mb 5ms DropTail

# Configure the first entry node  
set tap1 [new Agent/IPTap];         # Create the TCPTap Agent
set bpf1 [new Network/Pcap/Live];   # Create the bpf
set dev [$bpf1 open readonly xl0]
$bpf1 filter "src 128.9.160.95 and dst 128.9.160.196"
$tap1 network $bpf1;                # Connect bpf to TCPTap Agent
$ns attach-agent $n1 $tap1;         # Attach TCPTap Agent to the node

# Configure the first exit node 
set tap2 [new Agent/IPTap];         # Create a TCPTap Agent
set ipnet2 [new Network/IP];        # Create a Network agent
$ipnet2 open writeonly        
$tap2 network $ipnet2;              # Connect network agent to tap agent
$ns attach-agent $n2 $tap2;         # Attach agent to the node.


# Configure the second entry node 
set tap3 [new Agent/IPTap];         # Create the TCPTap Agent
set bpf3 [new Network/Pcap/Live];   # Create the bpf
set dev [$bpf3 open readonly xl0]
$bpf3 filter "src 128.9.160.196 and dst 128.9.160.95"
$tap3 network $bpf3;                # Connect bpf to TCPTap Agent
$ns attach-agent $n3 $tap3;         # Attach TCPTap Agent to the node


# Configure the second exit node 
set tap4 [new Agent/IPTap];         # Create a TCPTap Agent
set ipnet4 [new Network/IP];        # Create a Network agent
$ipnet4 open writeonly        
$tap4 network $ipnet4;              # Connect network agent to tap agent
$ns attach-agent $n4 $tap4;         # Attach agent to the node.


# Connect the agents.
$ns simplex-connect $tap1 $tap4
$ns simplex-connect $tap3 $tap2


$ns at 50.0 "finish"

proc finish {} {
    global ns f nf
    $ns flush-trace
    close $f
    close $nf
    exit 0
}

$ns run




