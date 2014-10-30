#Create a simulator object
set ns [new Simulator]

#Open the ns trace file
set nf [open out.ns w]
$ns trace-all $nf

proc finish {} {
        global ns nf
        $ns flush-trace
        close $nf
        exit 0
}

#Create two nodes
set n0 [$ns node]
set n1 [$ns node]

#Create a duplex link between the nodes
$ns duplex-link $n0 $n1 1Mb 10ms DropTail


#Create a TCP agent and attach it to node n0
set tcp [new Agent/TCP/Reno]
set snk [new Agent/TCPSink]

$tcp set syn_ true

$ns attach-agent $n0 $tcp
$ns attach-agent $n1 $snk

$ns connect $tcp $snk

$ns at 0.5 "$tcp advanceby 1"
$ns at 5.0 "finish"

#Run the simulation
$ns run
