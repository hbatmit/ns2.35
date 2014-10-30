Class REDPDSim

#
# routine which inserts high dr flows into history
#
REDPDSim instproc insert-high-dr-flows {} {

    $self instvar high_dr_flow_hist_ 
    $self instvar redpdflowmon_
    $self instvar MaxDropList_ MaxHighDRFlowHist_
    
    lappend drop_list
    set total_flows 0
    
    foreach flow [$redpdflowmon_ flows] {
	
	set flow_monbdrops [$flow set mon_ebdrops_]
	set flow_bdrops [expr [$flow set bdrops_] - $flow_monbdrops]
	set flow_monpdrops [$flow set mon_epdrops_]
	set flow_pdrops [expr [$flow set pdrops_] - $flow_monpdrops]
	
	# if one or more packets dropped - then record the number of bytes dropped.
	if { $flow_pdrops > 0 } {
	    lappend drop_list [list $flow $flow_bdrops]
	}
	
	if { $flow_pdrops != 0 } {
	    incr total_flows
	}
	
	#memory
	unset flow_monbdrops flow_bdrops flow_monpdrops flow_pdrops
    }

    if { [llength $drop_list] > $MaxDropList_ } {
	set sorted_drop_list [lsort -decreasing -integer -index 1 $drop_list]
	set drop_list [lrange $sorted_drop_list 0 [expr $MaxDropList_- 1]]
	#memory problems
	unset sorted_drop_list
    }
    
    #	$self printListOfLists $drop_list
    
    lappend high_dr_flow_hist_ $drop_list
    
    #memory 
    unset drop_list
    
    if { [llength $high_dr_flow_hist_] > $MaxHighDRFlowHist_ } {
	
	#set temp_list [lrange $high_dr_flow_hist_ 1 end]
	#set high_dr_flow_hist_ $temp_list
	#unset temp_list
	
	set high_dr_flow_hist_ [lrange $high_dr_flow_hist_ 1 end]
    }
    
    $self vprint-nonewline 2 "High DR History "
    $self printListOfLists 2 $high_dr_flow_hist_
    
    #unset total_flows
    #return $total_flows
}

#
# routine to check if some flow is a regular visitor to the high-dr-flow-hist
# regular for time being is 3 occurences (we maintain a history of max 5)
#
REDPDSim instproc check-regular {} {
    
    $self instvar high_dr_flow_hist_
    $self instvar redpdflowmon_
    $self instvar ret_list
    $self instvar XinRegular_
    
    if {[info exists ret_list]} {
	unset ret_list
    }
    
    lappend ret_list 
    
    foreach flow [$redpdflowmon_ flows] {
	
	#$self vprint 2 "Checking flow $flow"
	set flow_count 0
	set drop_count 0
	foreach drop_list $high_dr_flow_hist_ {
	    #		$self printListOfLists $drop_list
	    set exist [lsearch -regexp $drop_list "$flow \[0-9\]*"]
	    if { $exist == -1 } {
		#			puts "Not Found"
	    } else {
		#			puts "Found at $exist"
		incr flow_count
		incr drop_count [lindex [lindex $drop_list $exist] 1]
	    }
	    #memory
	    unset exist
	}
	
	if { $flow_count >= $XinRegular_ } {
	    $self vprint 2 "Putting flow $flow in retlist"
	    lappend ret_list [list $flow $drop_count] 
	}
	
	#memory 
	unset flow_count drop_count
    }
    $self vprint-nonewline 2 "Regular List "
    $self printListOfLists 2 $ret_list
    return $ret_list
}

#		
# routing used to check if a flow should be loosened	
# if a flow has no occurences in the high drop list for then let the flow go.
#
REDPDSim instproc check-irregular {} {
    
    $self instvar high_dr_flow_hist_
    $self instvar redpdflowmon_
    $self instvar ret_list
    
    if {[info exists ret_list]} {
	unset ret_list
    }
    
    lappend ret_list 
    
    foreach flow [$redpdflowmon_ flows] {
	
	if { [$flow set monitored_] == 1 } {
	    
	    set flow_count 0	
	    foreach drop_list $high_dr_flow_hist_ {
		set exist [lsearch -regexp $drop_list "$flow \[0-9\]*"]
		if { $exist != -1 } {
		    incr flow_count
		}
		
		#memory
		unset exist
	    }
	    
	    if { $flow_count == 0 } {
		#$self vprint 2 "Putting flow $flow in negative retlist"
		lappend ret_list $flow
	    }
	    
	    unset flow_count
	}
    }

    return $ret_list
}

#
# calculates average of enities at index "idx" of each list of the given "listOfLists"
#
REDPDSim instproc calculateAverage {listOfLists idx} {

    set count 0
    set sum 0
    foreach sublist $listOfLists {
	incr count
	set sum [expr $sum + [lindex $sublist $idx]]
    }
    
    if { $count == 0 } {
	return -1
    }
    
    set avg [expr $sum / $count.0]
    
    #memory
    unset count sum
    
    return $avg
}

#
# 1. inserts the current drop rate into the drop rate history
# 2. calculate the average drop rate
#
REDPDSim instproc insertNCalDropRate {currDropRate} {

	$self instvar dropRateHist_ drop_rate_list_ avg_drop_rate_

# 	#the tfrc method
# 	lappend drop_rate_list_ $currDropRate
 
# 	if { [llength $drop_rate_list_] > $dropRateHist_ } {
# 		set drop_rate_list_ [lrange $drop_rate_list_ 1 end]
# 	} else {
# 		#dirty short cut for initial periods.
# #		puts "length of drop_rate_list_ = [llength $drop_rate_list_]"
# 		return $currDropRate
# 	}

# 	set denom [expr {($dropRateHist_/2) + 1}]
# 	set sum 0
# 	set sigmaW 0

# 	for {set i 0} {$i < $dropRateHist_} {incr i} {
# 		set dr [lindex $drop_rate_list_ $i]
# 		if {$i >= [expr $dropRateHist_/2] } {
# 			set w 1
# 		} else {
# 			set w [expr {($i+1)/$denom}]
# 		}
# 		set sum [expr {$sum + $w*$dr}]
# 		set sigmaW [expr {$sigmaW + $w}]
# 	}

# 	set avgDropRate [expr {$sum/$sigmaW}]

# 	return $avgDropRate

	#ewma
	if {$avg_drop_rate_ == -1} {
	    set avg_drop_rate_ $currDropRate
	    return $avg_drop_rate_
	} else {
#	    set alpha 0.1
	    set alpha 0.5
	    set avg_drop_rate_ [expr {$alpha*$currDropRate + (1-$alpha)*$avg_drop_rate_}]
	    return $avg_drop_rate_
	}

}

REDPDSim instproc sched-print-stats {time} {

    $self instvar redpdflowmon_ ns_

    # calculate the current drop rate
    set bdrops [expr [$redpdflowmon_ set bdrops_] - [$redpdflowmon_ set mon_ebdrops_]]
    set barrivals [expr [$redpdflowmon_ set barrivals_] - [$redpdflowmon_ set mon_ebdrops_]]
    
    if { $barrivals == 0 } {
	set currDropRate 0
    } else {
	set currDropRate [expr $bdrops.0/$barrivals.0]
    }
    
    set avgDropRate [$self insertNCalDropRate $currDropRate]
    
    $self vprint 1 "currDropRate: $currDropRate avgDropRate: $avgDropRate avg_drop_count: disabled disabled bdrops: $bdrops barrivals: $barrivals"
    
    $redpdflowmon_ reset
    $ns_ at [expr [$ns_ now]+0.200] "$self sched-print-stats 0" 
}
    
REDPDSim instproc sched-detect-reward { timeAfter } {
    $self instvar detect_pending_
    $self instvar ns_ Mintime_ Maxtime_
    
    if { $detect_pending_ == "true" } {
	$self vprint 2 "SCHEDULING DETECT (NO, ALREADY PENDING)"
	return
    }
    
    set now [$ns_ now]
    set then_detect [expr $now + $timeAfter]
    set then_reward [expr $then_detect + 0.001]

    set detect_pending_ true
    $ns_ at $then_detect "$self do_detect"
    $ns_ at $then_reward "$self do_reward"
    
    $self vprint 3 "SCHEDULING DETECT for $then_detect"
    
    #memory
    unset now then_detect then_reward
}
	
#
# main routine to determine if there are bad flows to penalize
#
REDPDSim instproc do_detect {} {
    $self instvar ns_
    $self instvar last_detect_
    $self instvar redpdflowmon_ redpdq_
    $self instvar target_drop_rate_ 
    $self instvar detect_pending_
    $self instvar mon_flow_hist_
    $self instvar Mintime_
    $self instvar Hist_Max_
    $self instvar Bandwidth_
  
    set Bandwidth_ 1500000

    set detect_pending_ false
    set now [$ns_ now]
    $self vprint 3 "DO_DETECT started at time $now, last: $last_detect_"
    set elapsed [expr $now - $last_detect_]

    set last_detect_ $now

    # calculate the current drop rate
    set bdrops [expr [$redpdflowmon_ set bdrops_] - [$redpdflowmon_ set mon_ebdrops_]]
    set barrivals [expr [$redpdflowmon_ set barrivals_] - [$redpdflowmon_ set mon_ebdrops_]]
    
    if { $barrivals == 0 } {
	set currDropRate 0
    } else {
	set currDropRate [expr $bdrops.0/$barrivals.0]
    }
    
    set avgDropRate [$self insertNCalDropRate $currDropRate]
    
    if {$currDropRate <= $avgDropRate} {
	set usableDropRate $currDropRate
    } else {
	set usableDropRate $avgDropRate
    }

#    set usableDropRate $avgDropRate
    
    $self insert-high-dr-flows
    set regular_list [$self check-regular]
    set avg_drop_count [$self calculateAverage $regular_list 1]
    
    $self vprint 1 "currDropRate: $currDropRate avgDropRate: $avgDropRate avg_drop_count: $avg_drop_count [llength $regular_list] bdrops: $bdrops barrivals: $barrivals mon_drops: [$redpdflowmon_ set mon_ebdrops_]"

    # only for proving identification works 
    $self instvar testIdentOnly_
    if {$testIdentOnly_ == 1} {
	$self testIdent $barrivals $bdrops $avgDropRate $regular_list
	return
    }

    if { $currDropRate > 0.001 } {
	# monitor all flows which are regular
	foreach flow_drop_list $regular_list {
	    
	    set flow [lindex $flow_drop_list 0]
	    set drop_count [lindex $flow_drop_list 1]
	    set fid [$flow set flowid_]
	    
	    if { [info exists mon_flow_hist_($fid,lastChange)] } {
		set lastChange $mon_flow_hist_($fid,lastChange)
	    } else {
		set lastChange 0
	    }
	    
	    #		puts " $flow $lastChange" 
	    if { [$ns_ now] - $lastChange >= [$self minLastChange 4] } {
		
		#don't punish a flow if he had zero drops in the last interval
		# minor engineering. no major effect.
		if { [$flow set bdrops_] != 0 } {
		    
		    set pre_drop_rate [expr ($usableDropRate*$drop_count)/$avg_drop_count]
		    if { [$flow set monitored_] == 1 } {
			
			set oldTarget [$flow set targetBW_]
			set oldDropP [expr {1 - $oldTarget}]
			if { [$flow set unresponsive_] != 1} { 
			    set unresponsive [$self check-unresponsive $flow]
			    if { $unresponsive == 1 } {
				$self vprint 2 "Monitor: Declaring unresponsive flow $flow $fid"
				#$flow set auto_ true
				$redpdq_ unresponsive-flow $flow
			    }
			}
			if {[$flow set unresponsive_] != 1 } {
			    $self vprint 2 "$flow $pre_drop_rate $oldDropP $drop_count"  
			    if { $pre_drop_rate > $oldDropP } {
				set newDropP [expr {2*$oldDropP}]
			    } else {
				set newDropP [expr {$oldDropP + $pre_drop_rate}]
			    }
			    set newTarget [expr {1 - $newDropP}]
			    
			    if {$newTarget < 0} {
				set newTarget [expr {$oldTarget/2.0}]
			    }
			    
			    $flow set targetBW_ $newTarget
			    $self vprint 2 "Monitor: Strangling $flow $fid $oldTarget -> $newTarget"
			} else {
			    $self vprint 2 "$flow $pre_drop_rate $oldDropP"  
			    if { $pre_drop_rate > $oldDropP } {
				set newDropP [expr {3*$oldDropP}]
			    } else {
				set newDropP [expr {$oldDropP + 2*$pre_drop_rate}]
			    }
			    
			    set newTarget [expr {1 - $newDropP}]
			    
			    if {$newTarget < 0} {
				set newTarget [expr {$oldTarget/3.0}]
			    }
			    
			    $flow set targetBW_ $newTarget
			    $self vprint 2 "Monitor: Strangling unresponsive $flow $fid $oldTarget -> $newTarget"
			}
			#memory
			unset oldTarget newTarget newDropP oldDropP
		    } else {
			if { $pre_drop_rate > 2*$usableDropRate } {
			    set newDropP [expr {2*$usableDropRate}]
			} else {
			    set newDropP $pre_drop_rate
			}
			set newTarget [expr {1 - $newDropP}]
			
			if { $newTarget < 0.5 } {
			    set newTarget 0.5
			}
			
			$self vprint 2 "Monitor: Monitoring $flow $fid $newTarget"
			$redpdq_ monitor-flow $flow $newTarget 1 
			
			#memory
			unset newTarget newDropP
		    }
		    
		    set mon_flow_hist_($fid,lastChange) [$ns_ now]
		    set new_hist [list [expr [$flow set barrivals_].0/$elapsed] \
				      [expr [$flow set bdrops_].0/[$flow set barrivals_].0]] 
		    lappend mon_flow_hist_($fid,hist) $new_hist
		    
		    if { [llength $mon_flow_hist_($fid,hist)] > $Hist_Max_ } {
			$self vprint 2 "Curtailing the history for $flow $fid"
			set mon_flow_hist_($fid,hist) [lrange $mon_flow_hist_($fid,hist) 1 end]
		    }
		    
		    $self vprint-nonewline 2 "History of $flow $fid "
		    $self printListOfLists 2 $mon_flow_hist_($fid,hist)
		    
		    #memory
		    unset pre_drop_rate new_hist
		}
	    }
	    
	    #memory
	    unset flow drop_count fid lastChange
	}
    }
    
    #memory
    unset regular_list
        
    $self sched-detect-reward [$self calculateB $avgDropRate]
    
    foreach f [$redpdflowmon_ flows] {
	$f reset
    }
    $redpdflowmon_ reset
    $self vprint 3 "do_detect complete..."
}

#
# to calculate B, the list compilation interval
#
REDPDSim instproc calculateB { p } {
    $self instvar TargetRTT_
    $self instvar Mintime_ Maxtime_
    $self instvar MaxHighDRFlowHist_ XinRegular_
    $self instvar BList_ Bindex_

    $self instvar P_testTFRp_
    if {$P_testTFRp_ != -1} {
	set p $P_testTFRp_
    }

    if {$p == 0 } {
	return $Maxtime_
    }
    
    set sqrt1_5p [expr {sqrt(1.5*$p)}]  
    set x_by_yR [expr {[$self frac $XinRegular_ $MaxHighDRFlowHist_]*$TargetRTT_}]
    
    #deterministic model computation
    set time1 [expr {$x_by_yR / $sqrt1_5p}]
    
    set plusTerm [expr {1 + 9*$p*(1+32*$p*$p)}]
    
    #the newer model
    #set time2 [expr {$time1 * $plusTerm}]
    #set ns [Simulator instance]
    #puts "[$ns now] timeAfter: $p $time1 $time2"
    
    set timeAfter $time1
    #    set timeAfter $time2

    if { $timeAfter > $Maxtime_ } {
	set timeAfter $Maxtime_
    }
    if { $timeAfter < $Mintime_ } {
	set timeAfter $Mintime_
    }
    
    set BList_($Bindex_) $timeAfter
    incr Bindex_
    set Bindex_ [expr {$Bindex_ % $MaxHighDRFlowHist_}]

    return $timeAfter
}

#
# Gives the total time for last noB intervals.
#
REDPDSim instproc minLastChange { noB } {
    $self instvar BList_ Bindex_
    $self instvar minBtoConsider_ minTimetoConsider_

    set index [expr {$Bindex_ - 1}] 

    set sum 0
    for {set i 0} {$i < $noB} {incr i} {
	if {$index == -1} {
	    set index 4
	}
	set sum [expr {$sum + $BList_($index)}]
	incr index -1
    }

    if {$sum < $minTimetoConsider_} {
	return $minTimetoConsider_
    } else {
	return $sum
    }
}
	
REDPDSim instproc do_reward {} {

    $self instvar ns_
    $self instvar last_reward_ reward_pending_
    $self instvar Mintime_
    $self instvar redpdflowmon_ redpdq_
    $self instvar mon_flow_hist_
    $self instvar maxPReduction_ maxPReductionUnresp_
    
    #	$redpdflowmon_ dump

    set reward_pending_ false
    set now [$ns_ now]
    $self vprint 3 "DO_REWARD starting at $now, last: $last_reward_"
    set elapsed [expr $now - $last_reward_]
    set last_reward_ $now

#    if { $elapsed < $Mintime_ } {
#	puts "ERROR: do_reward: elapsed: $elapsed, min: $Mintime_"
#	exit 1
#    }
    
    
    foreach flow [$self check-irregular] {
	set fid [$flow set flowid_]
	set lastChange $mon_flow_hist_($fid,lastChange)	
	set oldTarget [$flow set targetBW_]
	

	if { [$flow set unresponsive_] != 1 && \
		 ([$ns_ now] - $lastChange) >= [$self minLastChange 3] } {
	    if { $oldTarget >= 0.995 } {
		$self vprint 2 "Monitor: Unmonitoring $flow $fid "
		$redpdq_ unmonitor-flow $flow
		unset mon_flow_hist_($fid,hist)
	    } else {
		set dropP [expr  {1 - $oldTarget}]
		if { $dropP >  2*$maxPReduction_ } {
		    set newTarget [expr {1 - ($dropP - $maxPReduction_)}]
		} else {
		    set newTarget [expr {1 - ($dropP/2.0)}]
		} 
		$flow set targetBW_ $newTarget
		$self vprint 2 "Monitor: Loosening $flow $fid $oldTarget -> $newTarget "
	    }
	    set mon_flow_hist_($fid,lastChange) [$ns_ now]	
 
	} elseif  { [$flow set unresponsive_] == 1 && \
			([$ns_ now] - $lastChange) >= [$self minLastChange 5] } {
	    if { $oldTarget >= 0.9975 } {
		$self vprint 2 "Monitor: Unmonitoring unresponsive flow $flow $fid "
		$redpdq_ unmonitor-flow $flow
		unset mon_flow_hist_($fid,hist)
	    } else {
		set dropP [expr  {1 - $oldTarget}]	
# 		set target1 [expr {1 - ($dropP/1.2)}]
# 		set target2 [expr {1.2*$oldTarget}]
# 		if { $target1 <= $target2 } {
# 		    set newTarget $target1
# 		} else {
# 		    set newTarget $target2
# 		}
		if { $dropP > 1.5*$maxPReductionUnresp_ } {
		    set newTarget [expr {1 - ($dropP - $maxPReductionUnresp_)}]
		} else {
		    set newTarget [expr {1 - ($dropP/1.5)}]
		}
		$flow set targetBW_ $newTarget
		$self vprint 2 "Monitor: Loosening unresponsive $flow $fid $oldTarget -> $newTarget"
	    }
	    set mon_flow_hist_($fid,lastChange) [$ns_ now]
	}
    }
	
    $self vprint 3 "do_reward complete..."
}

REDPDSim instproc check-unresponsive {flow} {

	$self instvar mon_flow_hist_ unresponsiveTestOn_

	#see if unresponsive test is ON
	if { $unresponsiveTestOn_ == 0} {
	    return 0
	}
	
	set target [$flow set targetBW_] 
	
	# goal is to catch just high-bw unresponsive flows. 
# 	if { $target > 0.79 } {
# 		return 0
# 	}
	
	set fid [$flow set flowid_]
	set flow_hist [lsort -real -index 1 $mon_flow_hist_($fid,hist)]
	$self vprint-nonewline 2 "checking unresponsiveness $flow $fid "
	$self printListOfLists 2 $flow_hist

	set good 0
	set bad 0
	foreach histItem1 $flow_hist {
		foreach histItem2 $flow_hist {
			set bw1 [lindex $histItem1 0] 
			set p1  [lindex $histItem1 1]
			set bw2 [lindex $histItem2 0]
			set p2  [lindex $histItem2 1]
			if { $p1 < $p2 && $p1/$p2 > 0.1 } { 
				set x [expr {$p1/$p2}]
				set y [expr {$bw2/$bw1}]
			} else {
				# we don't care about too small differences
				# and the reverse case will be taken care of anyway
				continue
			}
			set rootx [expr {sqrt($x)}]
			# y = rootx ideally.
			$self vprint 2 "unresp $histItem1 $histItem2 $x $y $rootx"
			if { $y > $rootx } {
				incr bad
			} elseif  { $y <= $rootx } {
				incr good
			}
		}
	}

	#memory
	unset flow_hist
	
	$self vprint 2 "unresponsive check $flow [$flow set flowid_] $bad $good"

	if { ($good == 0 && $bad >= 3) || ($good == 1 && $bad>=4) || ($good >= 2 && $bad >= 4*$good) } {
		return 1
	}
	
	return 0
}


REDPDSim instproc testIdent {barrivals bdrops avgDropRate regular_list} {

    $self instvar barrT bdrT
    $self instvar MaxHighDRFlowHist_
    $self instvar counter_
    $self instvar redpdflowmon_
    
    set j [expr $counter_ % $MaxHighDRFlowHist_]
    foreach f [$redpdflowmon_ flows] {
	set fid [$f set flowid_]
	set i [expr $fid*$MaxHighDRFlowHist_ + $j]
	set barrT($i) [$f set barrivals_];
	set bdrT($i) [$f set bdrops_];
    }
    set barrT($j) $barrivals
    set bdrT($j) $bdrops

    incr counter_
    
    lappend temp_flow_list
    unset temp_flow_list
    lappend temp_flow_list

    if { $counter_ %  $MaxHighDRFlowHist_ == 0 } {
	set count [llength $regular_list]
	foreach flow_drop_list $regular_list {
	    set flow [lindex $flow_drop_list 0]
	    set fid [$flow set flowid_]
	    
	    set flow_arr 0
	    set flow_dr 0
	    set cum_arr 0
	    set cum_dr 0
	    for {set j 0} {$j < $MaxHighDRFlowHist_} {incr j} {
		set i [expr $fid*$MaxHighDRFlowHist_ + $j]
		incr flow_arr $barrT($i)
		incr flow_dr  $bdrT($i)
		incr cum_arr $barrT($j)
		incr cum_dr $bdrT($j)
	    }
	    if { $MaxHighDRFlowHist_ != 1 || $flow_dr>=3000 } {
		puts "3:5-identified $counter_ $fid $flow_arr $cum_arr $flow_dr $cum_dr"
		lappend temp_flow_list $flow
		puts "1:1-3-identified $counter_ $fid $flow_arr $cum_arr $flow_dr $cum_dr"
		
	    }
	    if { $flow_dr >= 4000 } {
		puts "1:1-4-identified $counter_ $fid $flow_arr $cum_arr $flow_dr $cum_dr"
	    } else {
		puts "1:1-4-nodentified $counter_ $fid $flow_arr $cum_arr $flow_dr $cum_dr"
	    }
	}

	foreach flow [$redpdflowmon_ flows] {
	    if { [lsearch $temp_flow_list $flow] == -1} {
		set fid [$flow set flowid_] 
		set flow_arr 0
		set flow_dr 0
		set cum_arr 0
		set cum_dr 0
		for {set j 0} {$j < $MaxHighDRFlowHist_} {incr j} {
		    set i [expr $fid*$MaxHighDRFlowHist_ + $j]
		    incr flow_arr $barrT($i)
		    incr flow_dr  $bdrT($i)
		    incr cum_arr $barrT($j)
		    incr cum_dr $bdrT($j)
		}
		puts "3:5-nodentified $counter_ $fid $flow_arr $cum_arr $flow_dr $cum_dr"
		if { $flow_dr >= 3000 } {
		    puts "1:1-3-identified $counter_ $fid $flow_arr $cum_arr $flow_dr $cum_dr"
		} else {
		    puts "1:1-3-nodentified $counter_ $fid $flow_arr $cum_arr $flow_dr $cum_dr"
		}
		if { $flow_dr >= 4000 } {
		    puts "1:1-4-identified $counter_ $fid $flow_arr $cum_arr $flow_dr $cum_dr"
		} else {
		    puts "1:1-4-nodentified $counter_ $fid $flow_arr $cum_arr $flow_dr $cum_dr"
		}
	    }
	}
    }

    if { $MaxHighDRFlowHist_ == 1 } {
		$self sched-detect-reward [expr 5*[$self calculateB $avgDropRate]]
    } else {
    	$self sched-detect-reward [$self calculateB $avgDropRate]
    }

#    $self sched-detect 0.200
    foreach f [$redpdflowmon_ flows] {
	$f reset
    }
    $redpdflowmon_ reset
    
    return
}
