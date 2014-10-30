Class McastMonitor

McastMonitor instproc init {} {
    $self instvar period_ ns_

    set ns_ [Simulator instance]
    set period_ 0.03
}

McastMonitor instproc trace-topo {} {
    $self instvar ns_ period_

    $self trace-links [$ns_ all-links-list]
}

McastMonitor instproc trace-links links {
    $self instvar pktmon_

    foreach l $links {
	set pktmon_($l) [new PktInTranMonitor]
	$pktmon_($l) attach-link $l
	$l add-pktmon $pktmon_($l)
    }
}

McastMonitor instproc filter {header field value} {
    $self instvar pktmon_

    foreach index [array name pktmon_] {
	$pktmon_($index) filter $header $field $value
    }
}

McastMonitor instproc pktintran {} {
    $self instvar ns_ pktmon_

    set total 0
    foreach index [array name pktmon_] {
	if {[$index up?] == "up"} {
	    incr total [$pktmon_($index) pktintran]
	}
    }
    return $total
}

McastMonitor instproc print-trace {} {
    $self instvar ns_ period_ file_

    if [info exists file_] {
	puts $file_ "[$ns_ now] [$self pktintran]"
    } else {
	puts "[$ns_ now] [$self pktintran]"
    }
    $ns_ at [expr [$ns_ now] + $period_] "$self print-trace"
}

McastMonitor instproc attach file {
    $self instvar file_
    set file_ $file
}

###############
###############################Pkt In Transit Monitor###################
### constructed by
### front filter and front counter: keep track of #pkts passed into link
### rear filter and rear counter: keep track of #pkts passed out of link
### front count - rear count = #pkt in transit
###

Class PktInTranMonitor

PktInTranMonitor instproc init {} {
    $self instvar period_ ns_ front_counter_ rear_counter_ front_filter_ rear_filter_ 
    set ns_ [Simulator instance]
    set period_ 0.03
    set front_counter_ [new PktCounter]
    $front_counter_ set pktInTranMonitor_ $self
    set front_filter_ [new Filter/MultiField]
    $front_filter_ filter-target $front_counter_


    set rear_counter_ [new PktCounter]
    $rear_counter_ set pktInTranMonitor_ $self
    set rear_filter_ [new Filter/MultiField]
    $rear_filter_ filter-target $rear_counter_
}

PktInTranMonitor instproc reset {} {
    $self instvar front_counter_ rear_counter_  ns_ next_
    $front_counter_ reset
    $rear_counter_ reset
    if {[info exist next_] && $next_ != 0} {
	$next_ reset
    }
}

PktInTranMonitor instproc filter {header field value} {
    $self instvar front_filter_ rear_filter_
    $front_filter_ filter-field [PktHdr_offset PacketHeader/$header $field] $value
    $rear_filter_ filter-field [PktHdr_offset PacketHeader/$header $field] $value
}

PktInTranMonitor instproc attach-link link {
    $self instvar front_filter_ rear_filter_ front_counter_ rear_counter_
    
    set tmp [$link head]
    while {[$tmp target] != [$link link]} {
        set tmp [$tmp target]
    }

    $tmp target $front_filter_
    $front_filter_ target [$link link]
    $front_counter_ target [$link link]

    $rear_filter_ target [[$link link] target]
    $rear_counter_ target [[$link link] target]
    [$link link] target $rear_filter_
}

PktInTranMonitor instproc attach file {
    $self instvar file_
    set file_ $file
}

PktInTranMonitor instproc pktintran {} {
    $self instvar front_counter_ rear_counter_ 
    return [expr [$front_counter_ value] - [$rear_counter_ value]]
}

PktInTranMonitor instproc output {} {
    $self instvar front_counter_ rear_counter_ ns_ file_ 

    puts $file_ "[$ns_ now] [expr [$front_counter_ value] - [$rear_counter_ value]]"
}

    
PktInTranMonitor instproc periodical-output {} {
    $self instvar period_ ns_

    $self output
    $ns_ at [expr [$ns_ now] + $period_] "$self periodical-output"
}

################
Simulator instproc all-links-list {} {
    $self instvar link_
    set links ""
    foreach n [array names link_] {
	lappend links $link_($n)
    }
    set links
}

Link instproc add-pktmon pktmon {
    $self instvar pktmon_

    if [info exists pktmon_] {
	$pktmon set next_ $pktmon_
    } else {
	$pktmon set next_ 0
    }
    set pktmon_ $pktmon
}
	