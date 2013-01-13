# A simple tcp example that uses event tracing

#source ../lib/ns-default.tcl
Simulator set eventTraceAll_ 0
set ns [new Simulator]

#
#
# Create a simple six node topology:
#
#        s1                 s3
#         \                 /
# 10Mb,2ms \  1.5Mb,20ms   / 10Mb,4ms
#           r1 --------- r2
# 10Mb,3ms /               \ 10Mb,5ms
#         /                 \
#        s2                 s4
#

proc build_topology { ns } {

    global node_

    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 10
    $ns queue-limit $node_(r2) $node_(r1) 10
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
 
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0.5
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0.5

 
}

set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

#should add eventrace line after trace-all since trace-all file is used as defualt by eventracing.

#set file [open et.tr w]
$ns eventtrace-all
#$ns eventtrace-all $file
#$ns eventtrace-some tcp srm etc -future work

build_topology $ns
set redq [[$ns link $node_(r1) $node_(r2)] queue]
#$redq set setbit_ true

# Use nam trace format for TCP variable trace 
#Agent/TCP set nam_tracevar_ true

set tcp1 [$ns create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
$tcp1 set window_ 30
set tcp2 [$ns create-connection TCP/Reno $node_(s4) TCPSink $node_(s2) 1]
#$tcp1 set ecn_ 1
set ftp1 [$tcp1 attach-app FTP]   
set ftp2 [$tcp2 attach-app FTP]

#$ns add-agent-trace $tcp1 tcp1
#$ns monitor-agent-trace $tcp1
#$tcp1 tracevar cwnd_

$ns at 0.0 "$ftp1 start"
$ns at 1.0 "$ftp2 start"
$ns at 7.0 "finish"

proc finish {} {
        global ns f nf
        $ns flush-trace
        close $f
        close $nf
 
        #XXX
        puts "Filtering ..."
	exec tclsh8.0 ../nam/bin/namfilter.tcl out.nam

        puts "running nam..."
        exec nam out.nam &
        exit 0
}
 
$ns run


