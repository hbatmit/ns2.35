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
	$tcps($idx) set alpha_ 1000
	$tcps($idx) set beta_ 1000
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
	global queueMoni1F queueMoni1B queueMoni2F queueMoni2B queueMoni3F queueMoni3B

	global tcps_start tcps_stop

	#get an instance of the simulator
	set ns [Simulator instance]
	#set the time afterwhich the procedure should be called again
	set time 0.005

	set now [$ns now]
	set queueF1 [$queueMoni1F set pkts_]
	set queueB1 [$queueMoni1B set pkts_]
	set queueF2 [$queueMoni2F set pkts_]
	set queueB2 [$queueMoni2B set pkts_]
	set queueF3 [$queueMoni3F set pkts_]
	set queueB3 [$queueMoni3B set pkts_]
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
		  		puts $f0 "$i $now $queueF1 $queueB1 $rate $window $rtt $bRTT $old_window $queueF2 $queueB2 $queueF3 $queueB3"  
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
	global f_all

	$ns flush-trace
	close $f_all	
	close $f0
	puts "Finished."
	exit 0
}

# ------ SIMULATION SPECIFIC SCRIPT -----------

# The preamble
set ns [new Simulator]
$ns use-scheduler Heap
# Predefine tracing
set Name "fast-test2-record.dat";
set Q1 "fast-test2-queue1F.dat";
set Q2 "fast-test2-queue1B.dat";
set Q3 "fast-test2-queue2F.dat";
set Q4 "fast-test2-queue2B.dat";
set Q5 "fast-test2-queue3F.dat";
set Q6 "fast-test2-queue3B.dat";
set f0 [open $Name w]
set f1 [open $Q1 w]
set f2 [open $Q2 w]
set f3 [open $Q3 w]
set f4 [open $Q4 w]
set f5 [open $Q5 w]
set f6 [open $Q6 w]
set f_all [open nstrace.dat w]
#$ns trace-all $f_all

# Define the topology
set Cap1 1000
set Cap2 1000
set Cap3 1000
set Del1 20
set Del2 20
set Del3 20
set r1 [$ns node]
set r2 [$ns node]
set r3 [$ns node]
set r4 [$ns node]
$ns duplex-link $r1 $r2  [expr $Cap1]Mbps [expr $Del1]ms DropTail
$ns duplex-link $r2 $r3  [expr $Cap2]Mbps [expr $Del2]ms DropTail
$ns duplex-link $r3 $r4  [expr $Cap3]Mbps [expr $Del3]ms DropTail
set botqF1 [[$ns link $r1 $r2] queue]
set botqB1 [[$ns link $r2 $r1] queue]
set botqF2 [[$ns link $r1 $r2] queue]
set botqB2 [[$ns link $r2 $r1] queue]
set botqF3 [[$ns link $r1 $r2] queue]
set botqB3 [[$ns link $r2 $r1] queue]
$ns queue-limit $r1 $r2 2000000; 
$ns queue-limit $r2 $r1 2000000; 
$ns queue-limit $r2 $r3 2000000; 
$ns queue-limit $r3 $r2 2000000; 
$ns queue-limit $r3 $r4 2000000; 
$ns queue-limit $r4 $r3 2000000; 

# Create Sources and Start & Stop times
set ntcps    4;
set SimDuration 100;
create_tcp_fast 0 $r1 $r4 0  100
create_tcp_fast 1 $r1 $r2 20 100 
create_tcp_fast 2 $r2 $r4 40 100
create_tcp_fast 3 $r3 $r4 60 100

#$ns create-connection TCP $r1 TCPSink $r2 1
set starttime 0.0
set finishtime $SimDuration

# Monitor the queue between source switch and destination switch
set queueMoni1F [$ns monitor-queue $r1 $r2 $f1 0.1]
set queueMoni1B [$ns monitor-queue $r2 $r1 $f2 0.1]
set queueMoni2F [$ns monitor-queue $r2 $r3 $f3 0.1]
set queueMoni2B [$ns monitor-queue $r3 $r2 $f4 0.1]
set queueMoni3F [$ns monitor-queue $r3 $r4 $f5 0.1]
set queueMoni3B [$ns monitor-queue $r4 $r3 $f6 0.1]

# Run the Simulation
$ns at 0.0 "record"
$ns at $SimDuration "finish"
puts "Simulating $SimDuration Secs."
$ns run
