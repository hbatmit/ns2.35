REDPDSim instproc init { ns redpdq redpdflowmon redpdlink id enable } { 

	$self instvar ns_
	set ns_ $ns

	$self instvar last_reward_ last_detect_
#	$self instvar Reward_interval_ reward_pending_
	$self instvar detect_pending_

	$self instvar id_
	set id_ $id
			
	#for testing identification
	global testIdent_
	$self instvar testIdentOnly_ counter_
	set testIdentOnly_ $testIdent_
	set counter_ 0

	$self instvar verbose_
	global verbosity_
	set verbose_ $verbosity_	; #-1 means no messages
	
	$self instvar redpdq_ redpdflowmon_ redpdlink_
	set redpdq_ $redpdq
	set redpdflowmon_ $redpdflowmon
	set redpdlink_ $redpdlink
	
	# the amount of history to be kept for each monitored flow
	$self instvar Hist_Max_ 
	set Hist_Max_ 5

	set MinTimeToUnmonitor_ 15
	set MaxDropRateToUnmonitor_ 0.005

	#maximum reduction in probability in one step
	$self instvar maxPReduction_ maxPReductionUnresp_
	set maxPReduction_ 0.05
	set maxPReductionUnresp_ 0.05

	#whether unresponsive testing is ON
	$self instvar unresponsiveTestOn_
	global unresponsive_test_
	set unresponsiveTestOn_ $unresponsive_test_
	
	#set MaxDropList_ to infinity (greater than total number of flows) to get infinite memory list. 
	$self instvar MaxDropList_ 
	set MaxDropList_ 50

	#the lists - number of lists, "regular", history
	$self instvar MaxHighDRFlowHist_ XinRegular_ high_dr_flow_hist_
	global listMode_
	if {$testIdent_ == 1 && $listMode_ == "single"} {
		set MaxHighDRFlowHist_ 1
		set XinRegular_ 1
	} else {
		set MaxHighDRFlowHist_ 5
		set XinRegular_ 3
	}

	#the number of elements in the drop rate history at the router
	$self instvar dropRateHist_  drop_rate_list_ avg_drop_rate_
	set dropRateHist_ 8
	set avg_drop_rate_ -1

	set detect_pending_ false
	set reward_pending_ false
	set last_reward_ 0.0
	set last_detect_ 0.0

	#all 3 quantities below in seconds
	$self instvar Mintime_ Maxtime_ TargetRTT_
	global target_rtt_ 
	set TargetRTT_ $target_rtt_
	set Mintime_  [expr 2*$TargetRTT_] 
	set Maxtime_  [expr 10*$TargetRTT_]
	
	$self instvar BList_ Bindex_ minBtoConsider_ minTimetoConsider_
	set Bindex_ 0
	set minBtoConsider_ 4
	set minTimetoConsider_ 0.400
	for {set i 0} {$i < $MaxHighDRFlowHist_} {incr i} {
	    set BList_($i) 0
	}

	#turn to -1 to switch the test off.
	$self instvar P_testTFRp_ 
	set P_testTFRp_ [$redpdq_ set P_testFRp_]
	
	if { $enable == "true" || $enable == 1 } {
	    # start detect after 11s of simulation start
	    $self sched-detect-reward 10.1
	} else {
	    $self sched-print-stats 0.1
	    puts stderr "(red-pd disabled)"
	}
}      

REDPDSim instproc monitor-link {} {
	
	$self instvar redpdlink_

	set fmon [new QueueMonitor/ED/Flowmon]
	set cl [new Classifier/Hash/Fid 33]
	$fmon classifier $cl
	$cl proc unknown-flow { src dst fid } {
		set nflow [new QueueMonitor/ED/Flow]
		set slot [$self installNext $nflow]
		## puts "here1"
		$self set-hash auto $src $dst $fid $slot
	}
	$cl proc no-slot slotnum {
		puts stderr "classifier $self, no-slot for slotnum $slotnum"
	}

	$redpdlink_ attach-monitors [new SnoopQueue/In] [new SnoopQueue/Out] \
		[new SnoopQueue/Drop] $fmon

	return $fmon
}

REDPDSim instproc frac { num denom } {
	if { $denom == 0 } {
		return 0.0
	}
	return [expr double($num) / $denom]
}

REDPDSim instproc vprint args {
	$self instvar verbose_ id_
	set level [lindex $args 0]
	set a [lrange $args 1 end]
	if { $level <= $verbose_ } {
		$self instvar ns_
		puts "[$ns_ now] ($id_) $a"
		flush stdout
	}
}

REDPDSim instproc vprint-nonewline args {
	$self instvar verbose_ id_
	set level [lindex $args 0]
	set a [lrange $args 1 end]
	if { $level <= $verbose_ } {
		$self instvar ns_
		puts -nonewline "[$ns_ now] ($id_) $a"
		flush stdout
	}
}

#
#generic function to print a list of lists 
#
REDPDSim instproc printListOfLists {level listOfLists} {
    $self instvar verbose_
    if { $level <= $verbose_ } {
	$self instvar ns_
    	foreach i $listOfLists {
	    puts -nonewline "{ "
	    foreach j $i {
		puts -nonewline "$j "
	    }
	    puts -nonewline " } "
	}

	puts ""
	flush stdout
    }
}
