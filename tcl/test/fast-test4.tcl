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
	$tcps($idx) set window_ 1000000
	$tcps($idx) set alpha_ 10000 
	$tcps($idx) set beta_  10000
	$tcps($idx) set mi_threshold_ 0.0015
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
	set time 0.005

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
	global f_all

	$ns flush-trace
	#close $f_all	
	close $f0
	puts "Finished."
	exit 0
}

# ------ SIMULATION SPECIFIC SCRIPT -----------

# The preamble
set ns [new Simulator]
$ns use-scheduler Heap
# Predefine tracing
set Name "fast-test4-record.dat";
set Q1 "fast-test4-queue1.dat";
set Q2 "fast-test4-queue2.dat";
set f0 [open $Name w]
set f1 [open $Q1 w]
set f2 [open $Q2 w]
#set f_all [open nstrace.dat w]
#$ns trace-all $f_all

# Define the topology
set Cap 10000
set Del 20
set r1 [$ns node]
set r2 [$ns node]
set r3 [$ns node]

$ns duplex-link $r1 $r2  [expr $Cap]Mbps [expr $Del]ms DropTail

set botqF [[$ns link $r1 $r2] queue]
set botqB [[$ns link $r2 $r1] queue]	
$ns queue-limit $r1 $r2 2000000; 
$ns queue-limit $r2 $r1 2000000; 

# Create Sources and Start & Stop times
set ntcps    3;
set SimDuration 300;
create_tcp_fast 0 $r1 $r2 0  300
create_tcp_fast 1 $r1 $r2 60 240
create_tcp_fast 2 $r1 $r2 120 180

#$ns create-connection TCP $r1 TCPSink $r2 1
set starttime 0.0
set finishtime $SimDuration

# Monitor the queue between source switch and destination switch
set queueMoniF [$ns monitor-queue $r1 $r2 $f1 0.1]
set queueMoniB [$ns monitor-queue $r2 $r1 $f2 0.1]

# Run the Simulation
$ns at 0.0 "record"
$ns at $SimDuration "finish"
puts "Simulating $SimDuration Secs."
$ns run
