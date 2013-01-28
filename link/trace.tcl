# override built-in link allocator, to create a BrownianLink
# cloned from tcl/ex/fq.tcl

$ns proc simplex-link { n1 n2 bw delay qtype args } {
	$self instvar link_ queueMap_ nullAgent_ useasim_
	set sid [$n1 id]
	set did [$n2 id]

	# Debo
	if { $useasim_ == 1 } {
		set slink_($sid:$did) $self
	}

	if [info exists queueMap_($qtype)] {
		set qtype $queueMap_($qtype)
	}
	# construct the queue
	set qtypeOrig $qtype
	if { [llength $args] == 0} {
	  set q [new Queue/$qtype]
	} else {
	  set q [new Queue/$qtype $args]
        }
	# Now create the link
        set link_($sid:$did) [ new SimpleLink $n1 $n2 $bw $delay $q DelayLink/TraceLink ]
	if {$qtype == "RED/Pushback"} {
		set pushback 1
	} else {
		set pushback 0
	}
	$n1 add-neighbor $n2 $pushback
	
	#XXX yuck
	if {[string first "RED" $qtype] != -1 || 
	    [string first "PI" $qtype] != -1 || 
	    [string first "Vq" $qtype] != -1 ||
	    [string first "REM" $qtype] != -1 ||  
	    [string first "GK" $qtype] != -1 ||  
	    [string first "RIO" $qtype] != -1 ||
	    [string first "XCP" $qtype] != -1} {
		$q link [$link_($sid:$did) set link_]
	}

	set trace [$self get-ns-traceall]
	if {$trace != ""} {
		$self trace-queue $n1 $n2 $trace
	}
	set trace [$self get-nam-traceall]
	if {$trace != ""} {
		$self namtrace-queue $n1 $n2 $trace
	}
	
	# Register this simplex link in nam link list. Treat it as 
	# a duplex link in nam
	$self register-nam-linkconfig $link_($sid:$did)
}
