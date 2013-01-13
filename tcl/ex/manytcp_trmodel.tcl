#manytcp_trmodel.tcl
# use of manytcp traffic model


set ns [new Simulator]

# for now we use manytcp topology (default option - n number of 
# nodes attached on both sides of the bottle-neck link)
# FUTUREWORK: Other option will include defining the bottle-neck link # and endpoints (set of nodes to be connected).
# EX: set topo "-bottle_link_l $n(0) -bottle_link_r $n(1) \
# -client-nodes-l $n(2),$n(4),$n(6) -client-nodes-r $n(3),$n(5),$n(7)"
# 



set seed 12345
set params "-print-drop-rate 1 -debug 0 -trace-filename out"
set p1 "-bottle-queue-length 50 -bottle-queue-method RED"
set p2 "-client-arrival-rate 120 -bottle-bw 10Mb -ns-random-seed \
	$seed" 
set p3 "-client-mouse-chance 90 -client-mouse-packets 10" 
set p4 "-client-bw 100Mb -node-number 100 -client-reverse-chance 10"
set p5 "initial-client-count 0"
set p6 "duration 2 graph-results 1 graph-join-queueing 0 -graph-scale 2"

set model [eval new TrafficGen/ManyTCP $params $p1 $p2 $p3 $p4 $p5 $p6]

puts "Starting.."
$ns at 0 "$model start" 

# add other traffic/flows

$ns run
