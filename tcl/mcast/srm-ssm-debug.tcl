set ssm_trace stdout

proc ftime t {
    return [format "%7.4f" $t]
}

proc ft {ns obj} {
    return "[ftime [$ns now]] [[$obj set node_] id] $obj"
}

Agent/SRM/SSM instproc dump-status {} {
	$self instvar ns_ numrep_ numloc_ scope_flag_ rep_id_ addr_ 
	$self instvar local_scope_ 
	global ssm_trace
	puts $ssm_trace "[ft $ns_ $self] $addr_ numreps: $numrep_ nl: $numloc_ \status : $scope_flag_ rep: $rep_id_ localscope: $local_scope_"
#	set dist [$self distances?]
#	if {$dist != ""} {
#	    puts "[format "%7.4f" [$ns_ now]] distances $dist"
#	}
}

