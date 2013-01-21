############################################################
#
# Dawn Choe 		Caltech 12-18-2003
# Bartek Wydrowski 	Caltech 03-18-2004
#  
############################################################


# ---------- HELPER FUNCTIONS ------------

# Create a FAST Tcp connection
proc create_tcp_fast { idx src dst start stop } {
	global tcps
	global ftps
	global ns
	global sinks
	global tcps_start
	global tcps_stop

	puts "New TCP connection";
	# set source agent and application
	set tcps($idx) [new Agent/TCP/Fast]
	$tcps($idx) set class_ 2 
	$tcps($idx) set window_ 100000
	$tcps($idx) set alpha_ 800
	$tcps($idx) set beta_ 800
	$ns attach-agent $src $tcps($idx)
	set ftps($idx) [new Application/FTP]
	$ftps($idx) attach-agent $tcps($idx)

	# set sink 
	set sinks($idx) [new Agent/TCPSink/Sack1]
	$ns attach-agent $dst $sinks($idx)
	$ns connect $tcps($idx) $sinks($idx)
	set tcps_start($idx) [expr $start]
	set tcps_stop($idx)  [expr $stop]
	$ns at $start "$ftps($idx) start"
	$ns at $stop  "$ftps($idx) stop"
}

# Record Procedure
proc record {} {
	global tcps ntcps
	global f0    
	global queueMoniF queueMoniB
	global tcps_start tcps_stop

	#get an instance of the simulator
	set ns [Simulator instance]
	#set the time afterwhich the procedure should be called again
	set time 0.001

	set now [$ns now]
	set queueF [$queueMoniF set pkts_]
	set queueB [$queueMoniB set pkts_]
	
	set rate 0

	for {set i 0} {$i < $ntcps} {incr i} {
		if { ($now > $tcps_start($i)) && ($now < $tcps_stop($i)) } {
			set window      [expr [$tcps($i) set cwnd_]];
			set rtt 	[expr [$tcps($i) set avgRTT_]];
			set bRTT	[expr [$tcps($i) set baseRTT_]];
			set old_window 	[expr [$tcps($i) set avg_cwnd_last_RTT_]];

			if { $rtt > 0 }  {
		        	set rate	[expr $window/$rtt]
			}
			if { $bRTT < 9999999 } {
		  		puts $f0 "$i $now $queueF $queueB $rate $window $rtt $bRTT $old_window"  
			}
		}
	}
        
	# Re-schedule the procedure
	$ns at [expr $now+$time] "record"
}
# Define 'finish' procedure (include post-simulation processes)
proc finish {} {
	global ns
	global f0
#	global f_all

	$ns flush-trace
#	close $f_all	
	close $f0
	puts "Finished."
	exit 0
}

# ------ SIMULATION SPECIFIC SCRIPT -----------

# The preamble
set ns [new Simulator]
$ns use-scheduler Heap
# Predefine tracing
set Name "fast-test3-record.dat";
set Q1 "fast-test3-queue1.dat";
set Q2 "fast-test3-queue2.dat";
set f0 [open $Name w]
set f1 [open $Q1 w]
set f2 [open $Q2 w]
#set f_all [open nstrace.dat w]
#$ns trace-all $f_all

# Define the topology
set Cap 200
set Del 50
set src1 [$ns node]
set src2 [$ns node]
set src3 [$ns node]
set bottleneck [$ns node]
set dest [$ns node]

$ns duplex-link $src1 $bottleneck  [expr 2*$Cap]Mbps 0ms DropTail
$ns duplex-link $src2 $bottleneck  [expr 2*$Cap]Mbps 0ms DropTail
$ns duplex-link $src3 $bottleneck  [expr 2*$Cap]Mbps 0ms DropTail
$ns duplex-link $bottleneck $dest  [expr $Cap]Mbps [expr $Del]ms DropTail

set botqF [[$ns link $bottleneck $dest] queue]
set botqB [[$ns link $dest $bottleneck] queue]	
$ns queue-limit $bottleneck $dest 1300; 
$ns queue-limit $dest $bottleneck 1300; 

$ns queue-limit $src1 $bottleneck 1000000; 
$ns queue-limit $src2 $bottleneck 1000000; 
$ns queue-limit $src3 $bottleneck 1000000; 
$ns queue-limit $bottleneck $src1 1000000; 
$ns queue-limit $bottleneck $src2 1000000; 
$ns queue-limit $bottleneck $src3 1000000; 

# Create Sources and Start & Stop times
set ntcps    3;
set SimDuration 200;
create_tcp_fast 0 $src1 $dest 0  200
create_tcp_fast 1 $src2 $dest 20 180
create_tcp_fast 2 $src3 $dest 40 160

#$ns create-connection TCP $r1 TCPSink $r2 1
set starttime 0.0
set finishtime $SimDuration

# Monitor the queue between source switch and destination switch
set queueMoniF [$ns monitor-queue $bottleneck $dest $f1 0.1]
set queueMoniB [$ns monitor-queue $dest $bottleneck $f2 0.1]

# Run the Simulation
$ns at 0.0 "record"
$ns at $SimDuration "finish"
puts "Simulating $SimDuration Secs."
$ns run
