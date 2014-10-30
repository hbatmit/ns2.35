#
# A test of the emulation facility.  Note that this script
# must be run through "nse", the version of the simulator with
# the emulation extensions.  It listens on the SAP multicast
# address for session announcements, forwards them over a
# link with a large (1000ms) delay, while taking a trace.
# The effect of the delay should be visible in the trace
# file.
#
# If you run this on a non-multicast-capable LAN it will be
# highly uninteresting.
#

set sap_addr 224.2.127.254
set sap_port 9875
set stoptime 20

Class TestEmul
TestEmul instproc init {} {
	$self instvar ns_
	set ns_ [new Simulator]
	$ns_ use-scheduler RealTime
}

TestEmul instproc makenet ntype {
	$self instvar net_ ta_
	set net_ [new Network/$ntype]
	$net_ open readonly
	set ta_ [new Agent/Tap]
	$ta_ network $net_
}

TestEmul instproc maketopo { tfname } {
	$self instvar ns_ ta_ tfchan_

	set n0 [$ns_ node]
	set n1 [$ns_ node]

	#$ns_ duplex-link $n0 $n1 8Mb 1000ms DropTail
	$self dlink $n0 $n1 8Mb 1000ms DropTail

	$ns_ attach-agent $n0 $ta_
	set na [$ns_ nullagent]

	set tfchan_ [open $tfname w]
	[$ns_ link $n0 $n1] trace $ns_ $tfchan_
	$ns_ attach-agent $n1 $na
	$ns_ connect $ta_ $na
}

TestEmul instproc run {} {
	global sap_addr sap_port stoptime
	$self instvar net_ ns_
	$self makenet IP/UDP
	$self maketopo emout.tr
	$net_ bind $sap_addr $sap_port
	puts "running..."
	$ns_ at $stoptime "$self finish"
	$ns_ run
}

TestEmul instproc finish {} {
	$self instvar tfchan_ ns_
	puts "emulation complete.., writing output file"
	$ns_ halt
	$ns_ dumpq
	close $tfchan_
	exit 0
}

TestEmul instproc dlink { n1 n2 bw delay type } {
	$self instvar ns_
	$ns_ simplex-link $n1 $n2 $bw $delay $type
	$ns_ simplex-link $n2 $n1 $bw $delay $type
}

TestEmul instance
instance run
