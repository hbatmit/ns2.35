Agent/SRM/SSM set group_scope_ 32
Agent/SRM/SSM set local_scope_ 2
Agent/SRM/SSM set scope_flag_  2
Agent/SRM/SSM set rep_id_ 0
Agent/SRM/SSM set numrep_ 0
Agent/SRM/SSM set repthresh_up_ 100
Agent/SRM/SSM set repthresh_low_ 7
Agent/SRM/SSM set Z1_ 1.5
Agent/SRM/SSM set S1_ 0.0
Agent/SRM/SSM set S2_ 3.0

Agent/SRM/SSM instproc init {} {
	$self next
	$self instvar numrep_ numloc_ repthresh_up_ repthresh_low_ Z1_ \
		S1_ S2_
	set numrep_ 0
	set numloc_ 0
	set repthresh_up_ [$class set repthresh_up_]
	set repthresh_low_ [$class set repthresh_low_]
	set Z1_ [$class set Z1_]
	set S1_ [$class set S1_]
	set S2_ [$class set S2_]
}

Agent/SRM/SSM instproc start {} {
	$self next 
	$self instvar deactivateID_ sessionDelay_ ns_
	set now [expr [$ns_ now]]
	set deactivateID_ [$ns_ at [expr $now + 3 * $sessionDelay_] \
		"$self deactivate-reps $now"]
}


#Currently not used
Agent/SRM/SSM instproc repid { rep} {

	$self instvar rep_id_
	$self set rep_id_ [$rep set addr_]
	$self ch-rep 

}

Agent/SRM/SSM instproc member-scope {scope } {
	$self instvar scope_flag_
	$self set scope_flag_ $scope
#	$self ch-scope scope

}

Agent/SRM/SSM instproc local-member? {} {
	$self instvar scope_flag_
	if {$scope_flag_ == 1 } {
		return 1
	} else {
		return 0
	}
}

Agent/SRM/SSM instproc global-member? {} {
	$self instvar scope_flag_
	if {$scope_flag_ == 2 } {
		return 1
	} else {
		return 0
	}
}

Agent/SRM/SSM instproc local-member {} {
	$self member-scope 1	
}

Agent/SRM/SSM instproc global-rep {} {
	$self member-scope 2
	set rep_id_ [$self set addr_]
	$self ch-rep
}

Agent/SRM/SSM instproc set-local-scope {scope} {
	$self instvar local_scope_
	$self set local_scope_ $scope
}

Agent/SRM/SSM instproc set-global-scope {scope} {
	$self instvar global-scope
	$self set global-scope $scope
}

Agent/SRM/SSM instproc set-repid {rep} {
	$self instvar rep_id_
	$self set rep_id_ [$rep set addr_]
	$self ch-rep 
}

Agent/SRM/SSM instproc dump-reps {} {
	$self instvar ns_ activerep_ numrep_
	puts "[ft $ns_ $self] numreps: $numrep_"
	if [info exists activerep_] {
		foreach i [array names activerep_] {
			set rtime [$activerep_($i) set recvTime_]
			set ttl [$activerep_($i) set ttl_]
			puts "rep: $i recvtime: [ftime $rtime] ttl: $ttl"
		}

	}
}

Agent/SRM/SSM instproc dump-locs {} {
	$self instvar ns_ activeloc_ numloc_
	puts "[ft $ns_ $self] numlocs: $numloc_"
	if [info exists activeloc_] {
		foreach i [array names activeloc_] {
			set rtime [$activeloc_($i) set recvTime_]
			set ttl [$activeloc_($i) set ttl_]
			set repid [$activeloc_($i) set repid_]
			puts "loc: $i recvtime: [ftime $rtime] ttl: \
				$ttl repid: $repid"
		}

	}
}


Agent/SRM/SSM instproc send-session {} {
	$self instvar session_
	$session_ send-session
}



# Called when rep change is detected as following
# a) rep sends a local session message
# b) do not hear from the rep for a longtime.

Agent/SRM/SSM instproc repchange-action {} {
	$self instvar rep_id_ tentativerep_ tentativettl_
	$self instvar ns_
	$self cur-num-reps
	set rep_id_ $tentativerep_
	puts "[ft $ns_ $self] chrep rep : $tentativerep_\
		ttl : $tentativettl_"
	$self set-local-scope $tentativettl_
	$self local-member
	$self ch-rep
	$self send-session
}


Agent/SRM/SSM instproc recv-lsess {sender repid ttl} {
	# If it is mychild deactivate-locs will adjust the ttl

	$self instvar activeloc_ ns_ numloc_ sessionDelay_ deactivatelocID_
	$self instvar activerep_ numrep_
	$self instvar ch_localID_ tentativerep_ addr_ rep_id_ tentativettl_

#	if { $rep_id_ == $sender} {
#		puts "[ft $ns_ $self] lsess from myparent: $sender== $repid"
#	}
	if [info exists activeloc_($sender)] {
		$activeloc_($sender) recv-lsess $repid $ttl
	} else {
		set activeloc_($sender) [new SRMinfo/loc $sender]
		incr numloc_
		$activeloc_($sender) set-params $ns_ $self
		$activeloc_($sender) recv-lsess	$repid $ttl
	}

#	if { $repid == [$self set addr_]} {
#		puts "[ft $ns_ $self] lsess,mychild: $sender== $repid"
#	}

# recvd a local session message from someone who was global
	if [info exists activerep_($sender)] {
		delete $activerep_($sender)
		unset activerep_($sender)
		incr numrep_ -1
		if [info exists ch_localID_] {
			if {[info exists tentativerep_] && $tentativerep_ == $sender } {
#				find new rep
				$self cur-num-reps
			}
			if { $repid == $addr_} {
				$ns_ cancel $ch_localID_
				$self unset ch_localID_
				$self check-status
			}
		}
		# If this is my rep then
		if { [$self local-member?]} {
			if { $sender == $rep_id_} {
				$self repchange-action
			}
		} else {
			if { $sender == $rep_id_} {
				puts "[ft $ns_ $self] error"

			}
		}			
	}

	# Currently we do this everytime session message is received
	set time [expr [$ns_ now] - 3 * $sessionDelay_]
	if [info exists deactivatelocID_] {
		$ns_ cancel $deactivatelocID_
		unset deactivatelocID_
	}
	$self deactivate-locs $time
}


Agent/SRM/SSM instproc recv-gsess {sender ttl} {
	$self instvar activerep_ ns_ numrep_ sessionDelay_
	$self instvar deactivateID_ local_scope_
	
#	puts "[ft $ns_ $self] gsess: $sender"
	$self instvar activeloc_ numloc_
	if [info exists activerep_($sender)] {
		$activerep_($sender) recv-gsess $ttl
	} else {
		set activerep_($sender) [new SRMinfo/rep $sender]
		incr numrep_
		$activerep_($sender) set-params $ns_ $self
		$activerep_($sender) recv-gsess	$ttl
	}
	# Currently we do this everytime session message is received
	set time [expr [$ns_ now] - 3 * $sessionDelay_]
	if [info exists deactivateID_] {
		$ns_ cancel $deactivateID_
		unset deactivateID_
	}
# recvd a global session message from someone who was local
	if [info exists activeloc_($sender)] {
		delete $activeloc_($sender)
		unset activeloc_($sender)
		incr numloc_ -1
	}
	if { [$self local-member?]} {
		if {$ttl < $local_scope_} {
			set rep_id_ $sender
			puts "[ft $ns_ $self] closerrep rep : $sender \
				ttl : $ttl"
			$self set-local-scope $ttl
			$self local-member
			$self ch-rep
			$self send-session
		}			
	}
	$self deactivate-reps $time
	$self check-status
}

Agent/SRM/SSM instproc bias {} {
	$self instvar activerep_  ns_ sessionDelay_
	set now [expr [$ns_ now]]
	set biasfactor 0
	set time [expr $now - 1.5 * $sessionDelay_]
	if [info exists activerep_] {
		foreach i [array names activerep_] {
			set rtime [$activerep_($i) set recvTime_]
			if { $rtime >= $time} {
				incr biasfactor 
			}
		}
	}
	return $biasfactor
}

Agent/SRM/SSM instproc my-loc {} {
	$self instvar activeloc_
	set num 0
	if [info exists activeloc_] {
		foreach i [array names activeloc_] {
			set repid [$activeloc_($i) set repid_]
			if { $repid == [$self set addr_]} {
				incr num
			}
		}
	}
	return $num
}

Agent/SRM/SSM instproc cur-num-reps {} {
	$self instvar activerep_  ns_ sessionDelay_ tentativerep_ tentativettl_ 
	$self instvar Z1_
	set now [expr [$ns_ now]]
	set num 0
	set min_ttl 32
	set time [expr $now - $Z1_ * $sessionDelay_]
	if [info exists activerep_] {
		foreach i [array names activerep_] {
			set rtime [$activerep_($i) set recvTime_]
			set ttl [$activerep_($i) set ttl_]
			if { $rtime >= $time} {
				if {$min_ttl > $ttl} {
					set tentativerep_ $i
					set min_ttl $ttl
				}
				incr num
			}
		}
	}
	set tentativettl_ $min_ttl
	return $num
}

Agent/SRM/SSM instproc compute-localdelay {} {
    $self instvar S1_ S2_ sessionDelay_
    set num [$self my-loc]
    if {$num > 0} {
	set rancomp [expr $S1_+ 1 + $S2_ * [uniform 0 1]]
    } else {
	set rancomp [expr $S1_+ $S2_ * [uniform 0 1]]
    }
#    set delay [expr $rancomp * [$self bias] * $sessionDelay_ / \
#		    $repthresh_up_ ]
    set delay [expr $rancomp * $sessionDelay_]
    return $delay
}

Agent/SRM/SSM instproc compute-globaldelay {} {
    $self instvar S1_ S2_ sessionDelay_
    set rancomp [expr $S1_ + $S2_ * [uniform 0 1]]
    set delay [expr $rancomp * $sessionDelay_]
    return $delay
}



Agent/SRM/SSM instproc schedule-ch-local {} {
    $self instvar ns_ ch_localID_
    set now [$ns_ now]
    set delay [$self compute-localdelay]
    set fireTime [expr $now + $delay]
    if [info exists ch_localID_] {
	puts "[new_ft $ns_ $self] scheduled called without cancel"
	$ns_ cancel $ch_localID_
	unset ch_localID_
    }

    set ch_localID_ [$ns_ at $fireTime "$self ch-local"]
    puts "[ft $ns_ $self] schlocal [ftime $fireTime] evid : $ch_localID_"

}

Agent/SRM/SSM instproc schedule-ch-global {} {
    $self instvar ns_ ch_globalID_
    set now [$ns_ now]
    set delay [$self compute-globaldelay]
    set fireTime [expr $now + $delay]
    if [info exists ch_globalID_] {
	puts "[ft $ns_ $self] glbscheduled called without cancel"
	$ns_ cancel $ch_globalID_
	unset ch_globalID_
    }

    set ch_globalID_ [$ns_ at $fireTime "$self ch-global"]
    puts "[ft $ns_ $self] schglobal [ftime $fireTime] evid : $ch_globalID_"

}




Agent/SRM/SSM instproc check-status {} {
	$self instvar ns_ numrep_ repthresh_up_ ch_localID_
	$self instvar ch_globalID_ repthresh_low_
	if { $numrep_ > $repthresh_up_ }  {
		if [info exists ch_localID_] {
			# Already scheduled..
			return;
		}
		if { [$self local-member?]} {
			# Already a local member, cancel changing to
			# global if scheduled
			if [info exists ch_globalID_] {
				$ns_ cancel $ch_globalID_
				unset ch_globalID_
			}
			return;
		}
		$self schedule-ch-local
		return;
	}
	if {$numrep_ < $repthresh_low_} {
		if [info exists ch_globalID_] {
			# Already scheduled..
			return;
		}
		if { [$self global-member?]} {
			# Already a global member, cancel changing to
			# local if scheduled
			if [info exists ch_localID_] {
				$ns_ cancel $ch_localID_
				unset ch_localID_
			}
			return;
		}
		$self schedule-ch-global
		return;
	}
	if [info exists ch_localID_] {
		$ns_ cancel $ch_localID_
		unset ch_localID_
	}
	if [info exists ch_globalID_] {
		$ns_ cancel $ch_globalID_
		unset ch_globalID_
	}
	
}



Agent/SRM/SSM instproc ch-local {} {
	$self instvar repthresh_up_ tentativerep_ tentativettl_ ns_ rep_id_
	if {[$self cur-num-reps] > $repthresh_up_} {
#change scope, scope status and rep
		set rep_id_ $tentativerep_
		puts "[ft $ns_ $self] chlocal rep : $tentativerep_\
			ttl : $tentativettl_"
		$self local-member
		$self ch-rep
		$self send-session
		# Moved this so that the first local message reaches
		# all the children for faster detection
		$self set-local-scope $tentativettl_
	}
	if [info exists ch_localID_] {	
		$ns_ cancel ch_localID_
		unset ch_localID_
	}
}

Agent/SRM/SSM instproc ch-global {} {
	$self instvar repthresh_low_ tentativerep_ tentativettl_ ns_ rep_id_
	if {[$self cur-num-reps] < $repthresh_low_} {
#change scope, scope status and rep
		set rep_id_ [$self set addr_]
		puts "[ft $ns_ $self] chglobal rep : $rep_id_\
			ttl : $tentativettl_"
		# Here currently setting the localscope to reach
		# nearest rep, change later..
		# $self set-local-scope $tentativettl_
		# Set to zero and modify if get lsess
		$self set-local-scope 0
		$self global-rep
		$self ch-rep
		$self send-session
	}
	if [info exists ch_globalID_] {	
		$ns_ cancel ch_globalID_
		unset ch_globalID_
	}
}


Agent/SRM/SSM instproc deactivate-reps {time} {
	$self instvar numrep_ activerep_ deactivateID_ ns_
	$self instvar sessionDelay_ rep_id_
	if [info exists activerep_] {
		foreach i [array names activerep_] {
			set rtime [$activerep_($i) set recvTime_]
			if { $rtime < $time} {
				delete $activerep_($i)
				unset activerep_($i)
				incr numrep_ -1
		# have not heard from the rep for a longtime
		# Currently just find a new rep
				if { $i == $rep_id_ } {
					puts "[ft $ns_ $self] $i == $rep_id_" 
					$self repchange-action
				}
			}
		}
		if {$numrep_ <= 0} {
			unset activerep_
		}
	}
	set now [expr [$ns_ now]]
	set deactivateID_ [$ns_ at [expr $now + 3 * $sessionDelay_] \
		"$self deactivate-reps $now"]
#	$self dump-reps	
}

Agent/SRM/SSM instproc deactivate-locs {time} {
	$self instvar numloc_ activeloc_ deactivatelocID_ ns_
	$self instvar sessionDelay_ local_scope_
	set maxttl 0
	if [info exists activeloc_] {
		foreach i [array names activeloc_] {
			set rtime [$activeloc_($i) set recvTime_]
			if { $rtime < $time} {
				delete $activeloc_($i)
				unset activeloc_($i)
				incr numloc_ -1
			} else {
				# Might want to set it for own
				# children only
				if { [$self global-member?] } {
					set ttl [$activeloc_($i) set ttl_]
					if {$maxttl < $ttl} {
						set maxttl $ttl
					}
					set local_scope_ $maxttl
				}
			}
		}
		if {$numloc_ <= 0} {
			unset activeloc_
		}
	}
	set now [expr [$ns_ now]]
	set deactivatelocID_ [$ns_ at [expr $now + 3 * $sessionDelay_] \
		"$self deactivate-locs $now"]
#	$self dump-locs
}




Class SRMinfo

SRMinfo set recvTime_ 0

SRMinfo instproc init {sender} {
	$self next
	$self instvar sender_ 
	set sender_ $sender
}

SRMinfo instproc set-params {ns agent} {
	$self instvar ns_ agent_
	set ns_ $ns
	set agent_ $agent
}

Class SRMinfo/rep -superclass SRMinfo

SRMinfo/rep instproc recv-gsess {ttl} {
	$self instvar recvTime_ ns_ ttl_
	set now [$ns_ now]
	set recvTime_ [expr $now]
	set ttl_ [expr $ttl]
}

Class SRMinfo/loc -superclass SRMinfo


SRMinfo/loc instproc recv-lsess {repid ttl} {
	$self instvar recvTime_ ns_ ttl_ repid_
	set now [$ns_ now]
	set recvTime_ [expr $now]
	set ttl_ [expr $ttl]
	set repid_ [expr $repid]
}
