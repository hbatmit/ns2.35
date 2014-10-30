# jobs-cn2002.tcl
#
# Topology is as follows:
#
#                  cross_source(1...10) cross_source(11...20) cross_source(21...30)
#                          \|/               \|/                  \|/
# user_source(1)  ___       |                 |                    |                             __ user_sink(1)
# user_source(2)  ---\  core_node(1) ==== core_node(2) ====    core_node(3)  ====  core_node(4) /-- user_sink(2)
# user_source(3)  ---/                        |                    |                   |        \-- user_sink(3)
# user_source(4)  __/                        /|\                  /|\                 /|\        \_ user_sink(4)
#                                       cross_sink(1...10)   cross_sink(11...20)  cross_sink(21...30)
#
# Links:
# = : 45 MBps, 3 ms
# - : 100 MBps, 1 ms
#
# Classes:
# 1: ADC=2 ms, ALC=0.5%, no ARC, no RDC, no RLC
# 2, 3, 4: no ADC, no ALC, no ARC, RDC = 4, RLC = 2
#
# Traffic mix:
# user_source(1) -> user_sink(1) : Class 1, TCP, greedy
# user_source(2) -> user_sink(2) : Class 2, TCP, greedy
# user_source(3) -> user_sink(3) : Class 3, TCP, greedy
# user_source(4) -> user_sink(4) : Class 4, UDP, greedy
# cross_sources: 6 TCP flows, 4 UDP flows, 10% class 1, 20% class 2, 30% class 3, 40% class 4
#
# This simulation is very similar to what was used in 
# "Rate Allocation and Buffer Management for Differentiated Services,"
# by J. Liebeherr and N. Christin, Computer Networks, May 2002.
#
# Auxiliary functions

# topology primitives

Simulator instproc get-link { node1 node2 } {
    	$self instvar link_
    	set id1 [$node1 id]
    	set id2 [$node2 id]
    	return $link_($id1:$id2)
}

Simulator instproc get-queue { node1 node2 } {
    	global ns
    	set l [$ns get-link $node1 $node2]
    	set q [$l queue]
    	return $q
}

# JoBS duplex link: rate in kbps, delay in ms, buff_sz in packets

proc build-jobs-link {src dst rate delay buff_sz} {
    	global ns
    	$ns duplex-link $src $dst [expr $rate*1000.0] [expr $delay/1000.0] JoBS
    	$ns queue-limit $src $dst $buff_sz
    	$ns queue-limit $dst $src $buff_sz
}

# FCFS-marker duplex link: rate in kbps, delay in ms, buff_sz in packets

proc build-marker-link {src dst rate delay buff_sz} {
    	global ns
    	$ns simplex-link $src $dst [expr $rate*1000.0] [expr $delay/1000.0] Marker
    	$ns simplex-link $dst $src [expr $rate*1000.0] [expr $delay/1000.0] Demarker
    	$ns queue-limit $src $dst $buff_sz
    	$ns queue-limit $dst $src $buff_sz
}

# FCFS-demarker duplex link: rate in kbps, delay in ms, buff_sz in packets

proc build-demarker-link {src dst rate delay buff_sz} {
	global ns
	$ns simplex-link $src $dst [expr $rate*1000.0] [expr $delay/1000.0] Demarker
	$ns simplex-link $dst $src [expr $rate*1000.0] [expr $delay/1000.0] Marker
	$ns queue-limit $src $dst $buff_sz
	$ns queue-limit $dst $src $buff_sz
}

# Traffic models primitives

# Basic procedure for connecting agents, etc.

proc flow_setup {id src src_agent dst dst_agent app_flow pksize} {
	global ns
	$ns attach-agent $src $src_agent
	$ns attach-agent $dst $dst_agent
	$ns connect $src_agent $dst_agent
	$src_agent set packetSize_ 	$pksize
	$src_agent set fid_ $id

	$app_flow set flowid_ $id
	$app_flow attach-agent $src_agent

	return $src_agent    
}

# TCP flows

# Create "infinite-duration" FTP connection

proc inf_ftp {id src src_agent dst dst_agent app_flow maxwin pksize starttm} {
	global ns 

	flow_setup $id $src $src_agent $dst $dst_agent $app_flow $pksize
	$src_agent set window_ $maxwin
	$src_agent set ecn_	1

	puts [format "\tFTP-$id starts at %.4fsec" $starttm]
	$ns at $starttm "$app_flow start"
	return $src_agent
}

# Web-like flows: Create successive short transfers within single TCP connection 

proc init_mouse {id src src_agent dst dst_agent app_flow maxwin pksize starttm sleeptm minpkts maxpkts} {
	global ns 

	flow_setup $id $src $src_agent $dst $dst_agent $app_flow $pksize
	$src_agent set window_ 	$maxwin
	$src_agent set ecn_	1

	puts [format "\tMouse-$id starts at %.4fs - avg-sleep-time %.3fs - packets %d to %d " $starttm $sleeptm $minpkts $maxpkts]
	$ns at $starttm "actv_mouse $src_agent $app_flow $id $sleeptm $minpkts $maxpkts"
	return $src_agent
}

proc actv_mouse {tcp ftp id sleeptm minpkts maxpkts} {
	global ns rnd 
	set numpkts [expr round([$rnd uniform $minpkts $maxpkts+1])]
	set prd_time [expr [$rnd exponential] * $sleeptm + [$ns now]]
	$ns at $prd_time "$ftp producemore $numpkts"
	$ns at $prd_time "actv_mouse $tcp $ftp $id $sleeptm $minpkts $maxpkts" 
}

# UDP flows

# Build a Pareto On-Off source 
#
# packet trains with both on and off durations being pareto distributed
# Packet size: bytes, Burst and Idle time: ms, Peak rate: kbps, Start time: seconds
proc build-pareto-on-off {id src src_agent dst dst_agent app_flow peak_rate pksize shape_par start_tm idle_tm burst_tm} {
	global ns

	flow_setup $id $src $src_agent $dst $dst_agent $app_flow $pksize

	$app_flow set burst-time_  [expr $burst_tm  / 1000.]
	$app_flow set idle-time_   [expr $idle_tm  / 1000.]
	$app_flow set rate_        [expr $peak_rate * 1000.0]
	$app_flow set shape_       $shape_par
	$ns at $start_tm "$app_flow start"

	puts [format "\tPareto-$id starts at %.4fsec" $start_tm]
}

# statistics primitives

# Dump the statistics of a (unidirectional) link periodically 

proc linkDump {link binteg pinteg qmon interval name} {
	global ns
	set now_time [$ns now]
	$ns at [expr $now_time + $interval] "linkDump $link $binteg $pinteg $qmon $interval $name"
	set bandw [[$link link] set bandwidth_]
	set queue_bd [$binteg set sum_]
	set abd_queue [expr $queue_bd/[expr 1.*$interval]]
	set queue_pd [$pinteg set sum_]
	set apd_queue [expr $queue_pd/[expr 1.*$interval]]
	set utilz [expr 8*[$qmon set bdepartures_]/[expr 1.*$interval*$bandw]]
	if {[$qmon set parrivals_] != 0} {
		set drprt [expr [$qmon set pdrops_]/[expr 1.*[$qmon set parrivals_]]]
	} else {
		set drprt 0
	}
	if {$utilz != 0} {
		set a_delay [expr ($abd_queue*8*1000)/($utilz*$bandw)]
	} else {
		set a_delay 0.
	}
	puts [format "Link %s: Util=%.3f\tDrRt=%.3f\tADel=%.1fms\tAQuP=%.0f\tAQuB=%.0f" $name $utilz $drprt $a_delay $apd_queue $abd_queue]
	puts -nonewline [format "%.3f\t" $utilz]
	$binteg reset
	$pinteg reset
	$qmon reset
}

proc printFlow {f outfile fm interval} {
	global ns tot_drop tot_arv
	puts $outfile [format "%d %.2f %d %d %d %d %.0f %.3f" [$f set flowid_] [$ns now] [expr 8*[$f set barrivals_]] [$f set parrivals_] [expr 8*[$f set bdrops_]] [$f set pdrops_] [expr [$f set barrivals_]*8/($interval*1000.)] [expr [$f set bdrops_]/double([$f set barrivals_])] ]
}

proc flowDump {link fm file_p interval} {
	global ns tot_drop 
	$ns at [expr [$ns now] + $interval]  "flowDump $link $fm $file_p $interval"
	set theflows [$fm flows]
	if {[llength $theflows] == 0} {
		return
	} else {
		set total_arr [expr double([$fm set barrivals_])]
		if {$total_arr > 0} {
			foreach f $theflows {
				set arr [expr [expr double([$f set barrivals_])] / $total_arr]
				if {$arr >= 0.0001} {
					printFlow $f $file_p $fm $interval
					set z [$f set flowid_] 
					if {$z <= 4} {
						set tot_drop($z) [expr $tot_drop([$f set flowid_])+[$f set pdrops_]]
					}
				}       
			    $f reset
			}       
		    $fm reset
		}
	}
}

proc totalDrops {id interval} {
	global ns total_drops_file tot_drop N_USERS user_source core_node

	set q [$ns get-queue $user_source(1) $core_node(1)]
	set tot_arv(1) [$q set marker_arrvs1_]

	set q [$ns get-queue $user_source(2) $core_node(1)]
	set tot_arv(2) [$q set marker_arrvs2_]

	set q [$ns get-queue $user_source(3) $core_node(1)]
	set tot_arv(3) [$q set marker_arrvs3_]
    
	set q [$ns get-queue $user_source(4) $core_node(1)]
	set tot_arv(4) [$q set marker_arrvs4_]
    
	if {$tot_arv($id) > 0} {
		puts $total_drops_file($id) [format "%.2f %.2f %d %d" [$ns now] [expr 100.*$tot_drop($id)/double($tot_arv($id))] $tot_drop($id) $tot_arv($id)]
	} else {
		puts $total_drops_file($id) [format "%.2f 0.00" [$ns now]]
	}
	set tot_arv($id) 0
	$ns at [expr [$ns now] + $interval] "totalDrops $id $interval"
}

proc record_cwnd {} {
	global ns N_CL N_USERS_TCP user_source_agent cwnd_file
	set time 0.5
	set now [$ns now]
	for {set i 1} {$i <= $N_USERS_TCP} {incr i} {
		set window($i) [$user_source_agent($i) set cwnd_]
		puts $cwnd_file($i) [format "%.2f %.2f" $now $window($i)]
	}
	$ns at [expr $now+$time] "record_cwnd"
}

proc record_thru {} {
	global ns N_CL user_sink_agent thru_file
	set time 0.5
	set tot_recv 0
	set now [$ns now]
	for {set i 1} {$i <= $N_CL} {incr i} {
		set recv($i) [$user_sink_agent($i) set bytes_]
		set tot_recv [expr $tot_recv + [$user_sink_agent($i) set bytes_]]
		puts $thru_file($i) [format "%.2f %.2f" $now [expr 8.*$recv($i)/$time]]
		$user_sink_agent($i) set bytes_ 0
	}
	puts $thru_file([expr $N_CL+1]) [format "%.2f %.2f" $now [expr 8.*$tot_recv/$time]]
	$ns at [expr $now+$time] "record_thru"
}

puts " "
puts "JoBS multi-hop simulation."
puts "(c) Multimedia Networks Group, University of Virginia, 2000-2"
set ns [new Simulator]
set rnd [new RNG]
set seed 16
puts "Random seed: $seed"
$rnd seed $seed 

Queue/JoBS set drop_front_ false
Queue/JoBS set trace_hop_ true
Queue/JoBS set adc_resolution_type_ 0
Queue/JoBS set shared_buffer_ 1
Queue/JoBS set mean_pkt_size_ 4000
Queue/Demarker set demarker_arrvs1_ 0
Queue/Demarker set demarker_arrvs2_ 0
Queue/Demarker set demarker_arrvs3_ 0
Queue/Demarker set demarker_arrvs4_ 0
Queue/Marker set marker_arrvs1_ 0
Queue/Marker set marker_arrvs2_ 0
Queue/Marker set marker_arrvs3_ 0
Queue/Marker set marker_arrvs4_ 0
# need to patch tcp-sink.h tcp-sink.cc for this
Agent/TCPSink/DelAck set bytes_ 0 
#
puts "\nTOPOLOGY SETUP"

# Number of hops (=k)
set hops 4
puts "Number of hops: $hops"

# Link  latency (ms)
# note: an east coast-west coast 
# connection w/ 4 hops has a per-hop prop. del. roughly equal to 4.4ms
set EDGE_DEL  	1.0
set CORE_DEL    3.0 
puts "Edge link latency: $EDGE_DEL (ms)"
puts "Core link latency: $CORE_DEL (ms)"

# links: bandwidth (kbps) 
set CORE_BW	45000.0
set EDGE_BW	100000.0
puts "Core bandwidth: $CORE_BW (kbps)"
puts "Edge bandwidth: $EDGE_BW (kbps)"

# Buffer size in gateways (in packets)
set GW_BUFF	800
puts "Gateway Buffer Size: $GW_BUFF (packs)"

# Utilization factor for all core links
set utilz 0.0005
puts "Per source utilization (cross-traffic): $utilz"

# Packet Size (in bytes); assume common for all sources
set PKTSZ      500
set MAXWIN     50
puts "Packet Size: $PKTSZ (bytes)"

# Number of monitored flows (equal to # of classes)
set N_CL	4	
puts "Number of classes: $N_CL"

set N_USERS_TCP	3	
set N_USERS	4
puts "Number of flows: $N_USERS"

# Number of cross-traffic sources 
set N_C_TCP	6
set N_CT	10	
puts "Number of cross-traffic sources (per hop) $N_CT"

# Peak Rate of a cross-traffic source (set to link bandwidth, 
# because a cross-traffic source models an incoming link's aggregate traffic)
set PEAK_RT_C	$EDGE_BW	
puts "Peak rate of cross-traffic sources: $PEAK_RT_C (kbps)"

# Average Rate of a cross-traffic source (kbps); there is some algebra here 
set AVG_RT_C	[expr $utilz * $EDGE_BW / ($N_CT*1.)] 
puts "Average cross-traffic rate (per source): $AVG_RT_C (kbps)"

# Average idle duration for cross-traffic sources (msec)
set IDLE_TM_C 	[expr ($PEAK_RT_C-$AVG_RT_C)/($PEAK_RT_C*$AVG_RT_C) * $PKTSZ*8] 
puts "Average cross-traffic idle time: $IDLE_TM_C (ms)"

# Alpha parameter of the Pareto distribution for cross-traffic sources
set PARETO_ALPHA_C	1.9
puts "Alpha pareto parameter of cross-traffic sources: $PARETO_ALPHA_C"

set START_TM 	10.0
puts "Monitored flows start at: $START_TM"

set max_time 	70.0
puts "Max time: $max_time (sec)"

# Statistics-related parameters
set STATS_INTR  2; # interval between reporting statistics 
set START_STATS 0; # start-time for reporting statistics 

# Marker Types
# Deterministic (for user traffic)
set DETERM	1	
# Statistical (for cross-traffic)
set STATIS	2	

# Demarker Types
# Create a trace file for each class (for user traffic)
set VERBOSE	1
# Do not create trace files 
set QUIET	2

# 1. Core nodes ($hops nodes)
for {set i 1} {$i <= $hops} {incr i} {
	set core_node($i) [$ns node]
}
puts "Core nodes: OK"

# 2. User traffic nodes and sinks (N_USERS sources and sinks)
for {set i 1} {$i <= $N_USERS} {incr i} {
	set user_source($i) [$ns node]
	set user_sink($i) [$ns node]
	if {$i <= $N_USERS_TCP} {
		set user_source_agent($i) [new Agent/TCP/Newreno]
    		set user_sink_agent($i) [new Agent/TCPSink/DelAck]
	} else {
		set user_source_agent($i) [new Agent/UDP]
		set user_sink_agent($i) [new Agent/LossMonitor]
	}	
}
puts "User traffic nodes and sinks: OK"

# 3. Cross traffic nodes and sinks ($hops-1 sources, $hops-1 sinks)
for {set i 1} {$i < $hops} {incr i} {
	for {set j 1} {$j <= $N_CT} {incr j} {
		set cross_source([expr ($i-1)*$N_CT+$j]) [$ns node]
		set cross_sink([expr ($i-1)*$N_CT+$j]) [$ns node]
		if {$j > $N_C_TCP} {
			set cross_source_agent([expr ($i-1)*$N_CT+$j]) [new Agent/UDP]
			set cross_sink_agent([expr ($i-1)*$N_CT+$j]) [new Agent/LossMonitor]
		} else { 
			set cross_source_agent([expr ($i-1)*$N_CT+$j]) [new Agent/TCP/Newreno]
			set cross_sink_agent([expr ($i-1)*$N_CT+$j]) [new Agent/TCPSink/DelAck]
		}
		puts "Creating cross_source/sink_agent [expr ($i-1)*$N_CT+$j] ($i,$j)"
	}
}
puts "Cross traffic nodes and sink: OK"

# 4. Core links (JoBS)
for {set i 1} {$i < $hops} {incr i} {
	build-jobs-link $core_node($i) $core_node([expr $i+1]) $CORE_BW $CORE_DEL $GW_BUFF
    	set q [$ns get-queue $core_node($i) $core_node([expr $i+1])]
	set l [$ns get-link $core_node($i) $core_node([expr $i+1])]
    	if {$i == 1} {
		$q copyright-info
    	}    
    	$q init-rdcs -1 4 4 4
    	$q init-rlcs 2 2 2 2 
    	$q init-alcs 0.005 -1 -1 -1
    	$q init-adcs 0.002 -1 -1 -1
    	$q trace-file jobs-cn2002/hoptrace.$i
	$q link [$l link]
	$q sampling-period 1
    	$q id $i
    	$q initialize

    	# It's a duplex link

    	set q [$ns get-queue $core_node([expr $i+1]) $core_node($i)]
    	set l [$ns get-link $core_node([expr $i+1]) $core_node($i)]
    	$q init-rdcs -1 4 4 4
    	$q init-rlcs 2 2 2 2 
    	$q init-alcs 0.005 -1 -1 -1 
    	$q init-adcs 0.002 -1 -1 -1
	$q trace-file null
    	$q link [$l link]
	$q sampling-period 1
    	$q id [expr $i+$hops]
	$q initialize
}
puts "Core links: OK"

# 5. Cross-traffic links (marker)
for {set i 1} {$i < $hops} {incr i} {
	for {set j 1} {$j <= $N_CT} {incr j} {
		build-marker-link $cross_source([expr ($i-1)*$N_CT+$j]) $core_node($i) [expr $EDGE_BW/(1.*$N_CT)] $EDGE_DEL [expr $GW_BUFF*100]
		set q [$ns get-queue $cross_source([expr ($i-1)*$N_CT+$j]) $core_node($i)]
		if {$j == 1} {
			$q marker_type $DETERM
			$q marker_class 1
		} elseif {($j >=2) && ($j <= 3)} {
			$q marker_type $DETERM
			$q marker_class 2
		} elseif {($j >= 4) && ($j <= 6)} {
			$q marker_type $DETERM
			$q marker_class 3
		} else {
			$q marker_type $DETERM
			$q marker_class 4
		}
		set q [$ns get-queue $core_node($i) $cross_source([expr ($i-1)*$N_CT+$j])]
		$q id 99
		$q trace-file null

		build-marker-link  $cross_sink([expr ($i-1)*$N_CT+$j]) $core_node([expr $i+1]) [expr $EDGE_BW/(1.*$N_CT)] $EDGE_DEL [expr $GW_BUFF*100]
		set q [$ns get-queue $cross_sink([expr ($i-1)*$N_CT+$j]) $core_node([expr $i+1])]
		if {$j == 1} {
			$q marker_type $DETERM
			$q marker_class 1
		} elseif {($j >=2) && ($j <= 3)} {
			$q marker_type $DETERM
			$q marker_class 2
	 	} elseif {($j >= 4) && ($j <= 6)} {
			$q marker_type $DETERM
			$q marker_class 3
		} else {
			$q marker_type $DETERM
			$q marker_class 4
		}
		set q [$ns get-queue $core_node([expr $i+1]) $cross_sink([expr ($i-1)*$N_CT+$j])]
		$q id 99
		$q trace-file null
	}
}   
puts "Cross traffic links: OK"

# 6. Edge links (marker)
for {set i 1} {$i <= $N_USERS} {incr i} {
	build-marker-link    $user_source($i) $core_node(1) $EDGE_BW $EDGE_DEL $GW_BUFF
    	set q [$ns get-queue $user_source($i) $core_node(1)]
   	$q marker_type $DETERM
	$q marker_class $i
    	set q [$ns get-queue $core_node(1) $user_source($i)]
	# assign a bogus id
    	$q id 99
    	build-demarker-link  $core_node($hops) $user_sink($i) $EDGE_BW $EDGE_DEL $GW_BUFF

    	set q [$ns get-queue $core_node($hops) $user_sink($i)]
    	$q id $i
	$q trace-file jobs-cn2002/e2edelay

    	set q [$ns get-queue $user_sink($i) $core_node($hops)]
    	$q marker_type $DETERM
	$q marker_class $i
}
puts "Edge marker/demarker links: OK" 

#
# Create traffic
#
# Cross-traffic (UDP, on/off Pareto)
for {set i 1} {$i < $hops} {incr i} {
	for {set j 1} {$j <= $N_CT} {incr j} {
		if {$j > $N_C_TCP} {
			set cross_flow([expr ($i-1)*$N_CT+$j]) [new Application/Traffic/Pareto]
			build-pareto-on-off [expr ($i-1)*$N_CT+$j+$N_USERS] $cross_source([expr ($i-1)*$N_CT+$j]) $cross_source_agent([expr ($i-1)*$N_CT+$j]) $cross_sink([expr ($i-1)*$N_CT+$j]) $cross_sink_agent([expr ($i-1)*$N_CT+$j]) $cross_flow([expr ($i-1)*$N_CT+$j]) 2200.0 $PKTSZ $PARETO_ALPHA_C 0.0 100.0 120.0
		} else {
			set cross_flow([expr ($i-1)*$N_CT+$j]) [new Application/FTP]
			init_mouse [expr ($i-1)*$N_CT+$j+$N_USERS] $cross_source([expr ($i-1)*$N_CT+$j]) $cross_source_agent([expr ($i-1)*$N_CT+$j]) $cross_sink([expr ($i-1)*$N_CT+$j]) $cross_sink_agent([expr ($i-1)*$N_CT+$j]) $cross_flow([expr ($i-1)*$N_CT+$j]) $MAXWIN $PKTSZ 0.0 0.2 300 1000
		}
		puts "Connecting cross_source/sink_agent [expr ($i-1)*$N_CT+$j] ($i,$j) - via flow [expr ($i-1)*$N_CT+$j+$N_USERS]"
	}
}   
puts "Cross-traffic sources: OK"
# User traffic (TCP/FTP and UDP)
for {set i 1} {$i <= $N_USERS} {incr i} {
	if {$i <= $N_USERS_TCP} {
		set user_flow($i) [new Application/FTP]
		inf_ftp $i $user_source($i) $user_source_agent($i) $user_sink($i) $user_sink_agent($i) $user_flow($i) $MAXWIN $PKTSZ $START_TM
    } else {
		set user_flow($i) [new Application/Traffic/Pareto]
		build-pareto-on-off $i $user_source($i) $user_source_agent($i) $user_sink($i) $user_sink_agent($i) $user_flow($i) 5000.0 $PKTSZ $PARETO_ALPHA_C $START_TM 10.0 10.0
	}
}
puts "TCP flows: OK"

for {set i 1} {$i <= $N_USERS} {incr i} {
	if {$i <= $N_USERS_TCP} {
    		set cwnd_file($i) [open jobs-cn2002/cwnd$i.tr w]
	}
	set thru_file($i) [open jobs-cn2002/thru$i.tr w]
}
set thru_file([expr $N_USERS + 1]) [open jobs-cn2002/thru_sum.tr w]

#
# Queue Monitors
#

for {set i 1} {$i < $hops} {incr i} {
	set qmon_xy($i) [$ns monitor-queue $core_node($i) $core_node([expr $i+1]) ""]
	$ns at $START_STATS  "$qmon_xy($i) reset"
    	set bing_xy($i) [$qmon_xy($i) get-bytes-integrator]
    	set ping_xy($i) [$qmon_xy($i) get-pkts-integrator]
    	$ns at $START_STATS  "$bing_xy($i) reset"
    	$ns at $START_STATS  "$ping_xy($i) reset"
    	$ns at [expr $START_STATS+$STATS_INTR] "linkDump [$ns link $core_node($i) $core_node([expr $i+1])] $bing_xy($i) $ping_xy($i) $qmon_xy($i) $STATS_INTR N($i)-N([expr $i+1])"   
}

# Stats dump

for {set i 1} {$i < $hops} {incr i} {
	set flow_file($i) [open jobs-cn2002/flows-hop$i.tr w]
    	set fm [$ns makeflowmon Fid]
    	$ns attach-fmon [$ns get-link $core_node($i) $core_node([expr $i+1])] $fm
    	$ns at $START_TM "flowDump [$ns get-link $core_node($i) $core_node([expr $i+1])] $fm $flow_file($i) 0.5"
}
for {set i 1} {$i <= $N_USERS} {incr i} {
    	set tot_drop($i) 0
    	set old_arv($i) 0
    	set total_drops_file($i) [open jobs-cn2002/totaldrops-flow$i.tr w]
    	$ns at $START_TM "totalDrops $i 0.5"
}
$ns at $START_TM "record_thru"
$ns at $START_TM "record_cwnd"

$ns at $max_time "finish"
proc finish {} {
        puts "\nSimulation End"
	global N_CT N_USERS user_source core_node cross_source cross_sink user_sink ns hops

	puts "\nMonitored Flows (send):"
	for {set i 1} {$i <= $N_USERS} {incr i} {
		set q [$ns get-queue $user_source($i) $core_node(1)]
		puts "$i:   arrivals class-1 = [$q set marker_arrvs1_]" 
		puts "$i:   arrivals class-2 = [$q set marker_arrvs2_]" 
		puts "$i:   arrivals class-3 = [$q set marker_arrvs3_]" 
		puts "$i:   arrivals class-4 = [$q set marker_arrvs4_]" 
	}
	puts "\nMonitored Flows (receive):"
	for {set i 1} {$i <= $N_USERS} {incr i} {
		set q [$ns get-queue $core_node($hops) $user_sink($i)]
		puts "$i:   arrivals class-1 = [$q set demarker_arrvs1_]" 
		puts "$i:   arrivals class-2 = [$q set demarker_arrvs2_]" 
		puts "$i:   arrivals class-3 = [$q set demarker_arrvs3_]" 
		puts "$i:   arrivals class-4 = [$q set demarker_arrvs4_]" 
	}

	for {set i 1} {$i < $hops} {incr i} {
		for {set j 1} {$j <= $N_CT} {incr j} {
			set q [$ns get-queue $cross_source([expr ($i-1)*$N_CT+$j]) $core_node($i)]
			puts "\nCore Input Link ($i,$j):"
			puts "    cross-arrivals class-1 = [$q set marker_arrvs1_]" 
			puts "    cross-arrivals class-2 = [$q set marker_arrvs2_]" 
			puts "    cross-arrivals class-3 = [$q set marker_arrvs3_]" 
			puts "    cross-arrivals class-4 = [$q set marker_arrvs4_]" 

			set q [$ns get-queue $core_node([expr $i+1]) $cross_sink([expr ($i-1)*$N_CT+$j])]
			puts "\nCore Output Link ($i,$j):"
			puts "    cross-arrivals class-1 = [$q set demarker_arrvs1_]" 
			puts "    cross-arrivals class-2 = [$q set demarker_arrvs2_]" 
			puts "    cross-arrivals class-3 = [$q set demarker_arrvs3_]" 
			puts "    cross-arrivals class-4 = [$q set demarker_arrvs4_]" 
		}
	}
	exit 0
}

puts "\ngo!\n"
$ns run
