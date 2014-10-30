##
## simple example to demonstrate use of traffic generation modules
##
## topology consists of 6 nodes.  4 traffic sources are attached
## to each of 4 'access' nodes.  These nodes have links to a single
## 'concentrator' node, which is connected by the bottleneck link
## to the destination node.
## 
## there are 4 source agents that generate packets
## 1 generates traffic based on an Exponential On/Off distribution
## 1 generates traffic based on a Pareto On/Off distribution
## 2 generate traffic using the trace file 'example-trace'.
##
set stoptime 10.0
set plottime 11.0

set ns [new Simulator]

set n0 [$ns node]
set n1 [$ns node]

set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]

set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail

$ns duplex-link $n0 $n2 10Mb 5ms DropTail
$ns duplex-link $n0 $n3 10Mb 5ms DropTail
$ns duplex-link $n0 $n4 10Mb 5ms DropTail
$ns duplex-link $n0 $n5 10Mb 5ms DropTail

$ns duplex-link-op $n0 $n1 orient right
$ns duplex-link-op $n0 $n2 orient left-up
$ns duplex-link-op $n0 $n3 orient left
$ns duplex-link-op $n0 $n4 orient left-down
$ns duplex-link-op $n0 $n5 orient down

$ns duplex-link-op $n0 $n1 queuePos 0.5

#$ns trace-queue $n0 $n1 $f

## set up the exponential on/off source, parameterized by packet size,
## ave on time, ave off time and peak rate

set s1 [new Agent/UDP]
$ns attach-agent $n2 $s1

set null1 [new Agent/Null]
$ns attach-agent $n1 $null1

$ns connect $s1 $null1

set exp1 [new Application/Traffic/Exponential]
$exp1 set packetSize_ 210
$exp1 set burst_time_ 500ms
$exp1 set idle_time_ 500ms
$exp1 set rate_ 100k

$exp1 attach-agent $s1

## set up the pareto on/off source, parameterized by packet size,
## ave on time, ave  off time, peak rate and pareto shape parameter

set s2 [new Agent/UDP]
$ns attach-agent $n3 $s2

set null2 [new Agent/Null]
$ns attach-agent $n1 $null2

$ns connect $s2 $null2

set pareto2 [new Application/Traffic/Pareto]
$pareto2 set packetSize_ 210
$pareto2 set burst_time_ 500ms
$pareto2 set idle_time_ 500ms
$pareto2 set rate_ 200k
$pareto2 set shape_ 1.5

$pareto2 attach-agent $s2

## initialize a trace file
set tfile [new Tracefile]
$tfile filename example-trace

## set up a source that uses the trace file

set s3 [new Agent/UDP]
$ns attach-agent $n4 $s3

set null3 [new Agent/Null]
$ns attach-agent $n1 $null3

$ns connect $s3 $null3

set tfile [new Tracefile]
#$tfile filename /tmp/example-trace
$tfile filename example-trace

set trace3 [new Application/Traffic/Trace]
$trace3 attach-tracefile $tfile

$trace3 attach-agent $s3

## attach a 2nd source to the same trace file

set s4 [new Agent/UDP]
$ns attach-agent $n5 $s4

set null4 [new Agent/Null]
$ns attach-agent $n1 $null4

$ns connect $s4 $null4

set trace4 [new Application/Traffic/Trace]
$trace4 attach-tracefile $tfile

$trace4 attach-agent $s4

$ns at 1.0 "$exp1 start"
$ns at 1.0 "$pareto2 start"
$ns at 1.0 "$trace3 start"
$ns at 1.0 "$trace4 start"

$ns at $stoptime "$exp1 stop"
$ns at $stoptime "$pareto2 stop"
$ns at $stoptime "$trace3 stop"
$ns at $stoptime "$trace4 stop"

$ns at $plottime "close $f"
$ns at $plottime "finish tg"

proc finish file {
 
        set f [open temp.rands w]
        puts $f "TitleText: $file"
        puts $f "Device: Postscript"

        exec rm -f temp.p1 temp.p2 temp.p3 temp.p4
        exec touch temp.p1 temp.p2 temp.p3 temp.p4
        #
        #
	exec awk {
		{
			if (($1 == "+") && ($3 != 0) && ($9 == 2.0)) \
			     print $2, 2
		}
	} out.tr > temp.p1
	exec awk {
		{
			if (($1 == "+") && ($3 != 0) && ($9 == 3.0)) \
			     print $2, 3
		}
	} out.tr > temp.p2
	exec awk {
		{
			if (($1 == "+") && ($3 != 0) && ($9 == 4.0)) \
			     print $2, 4
		}
	} out.tr > temp.p3
	exec awk {
		{
			if (($1 == "+") && ($3 != 0) && ($9 == 5.0)) \
			     print $2, 5
		}
	} out.tr > temp.p4

	puts $f [format "\n\"ExpOnOff"]
	flush $f
	exec cat temp.p1 >@ $f
	flush $f
	puts $f [format "\n\"ParetoOnOff"]
	flush $f
	exec cat temp.p2 >@ $f
	flush $f
	puts $f [format "\n\"Trace1"]
	flush $f
	exec cat temp.p3 >@ $f
	flush $f
	puts $f [format "\n\"Trace2"]
	flush $f
	exec cat temp.p4 >@ $f
	flush $f

        close $f
	exec rm -f temp.p1 temp.p2 temp.p3 temp.p4
        exec xgraph -bb -tk -nl -m -x time -y "source node" temp.rands &

	puts "running nam..."
	exec nam out.nam &

	exec rm -f out.tr

}

$ns run


exit 0
