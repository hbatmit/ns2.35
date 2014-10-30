#
# A simple example for tcp/srm  
#
#
#
# Create a simple six node topology:
#
#        s1                 s3
#         \                 /
# 10Mb,2ms \  1.5Mb,20ms   / 1.5Mb,10ms
#           r1 --------- r2
# 10Mb,3ms /               \ 10Mb,5ms
#         /                 \
#        s2                 s4
#
# updated to use -multicast on and allocaddr by Lloyd Wood

proc build_topology { ns } {

    global node_

    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 10
    $ns queue-limit $node_(r2) $node_(r1) 10
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
 
    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0
    $ns duplex-link-op $node_(s3) $node_(r2) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r2) orient left-up

}
 
source ../mcast/srm-nam.tcl

set ns [new Simulator -multicast on]

set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

build_topology $ns

set group [Node allocaddr]
set mh [$ns mrtproto DM {}]

set srmStats [open srmStats.tr w]
set srmEvents [open srmEvents.tr w]

set n(0) $node_(r2)
set n(1) $node_(r1)
set n(2) $node_(s3)
set n(3) $node_(s4)

set fid 0
foreach i [array names n] {
        set srm($i) [new Agent/SRM]
        $srm($i) set dst_addr_ $group
        $srm($i) set dst_port_ 0
        $srm($i) set fid_ [incr fid]
        $srm($i) log $srmStats
        $srm($i) trace $srmEvents

        $ns attach-agent $n($i) $srm($i)
}

# Attach a data source to srm(0)
set packetSize 210
set exp0 [new Application/Traffic/Exponential]
$exp0 set packetSize_ $packetSize
$exp0 set burst_time_ 500ms
$exp0 set idle_time_ 500ms
$exp0 set rate_ 100k
$exp0 attach-agent $srm(0)
$srm(0) set tg_ $exp0
$srm(0) set app_fid_ 0
$srm(0) set packetSize_ $packetSize     ;# so repairs are correct

$ns at 0.0 "$srm(0) start; $srm(0) start-source"
$ns at 1.0 "$srm(1) start"
$ns at 1.1 "$srm(2) start"
$ns at 1.2 "$srm(3) start"

# tcp setup

set redq [[$ns link $node_(r1) $node_(r2)] queue]
$redq set setbit_ true
 
set tcp1 [$ns create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
$tcp1 set window_ 15
$tcp1 set ecn_ 1
# 
#set tcp2 [$ns create-connection TCP/Reno $node_(s2) TCPSink $node_(s1) 1]
#$tcp2 set window_ 15
#$tcp2 set ecn_ 1
 
set ftp1 [$tcp1 attach-app FTP]    
#set ftp2 [$tcp2 attach-app FTP]
 
$ns at 4.0 "$ftp1 start"
#$ns at 0.0 "$ftp2 start"

proc finish src {

        $src stop
 
        global ns srmStats srmEvents
        $ns flush-trace
        #close $f
        #close $nf
	close $srmStats
        close $srmEvents

        #XXX
        puts "Filtering ..."
	exec tclsh8.0 ../../../nam-1/bin/namfilter.tcl out.nam

        puts "running nam..."
        exec nam out.nam &
        exit 0
}

$ns at 5.0 "finish $exp0"
$ns run


