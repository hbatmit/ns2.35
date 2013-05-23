source timer.tcl

Class Application/FTP/OnOffSender -superclass { Application/FTP }

Application/FTP/OnOffSender instproc init {} {
    global opt flowcdf

    $self set npkts_ 0
    $self set laststart_ 0.0
    $self set lastack_ 0
    $self set lastrtt_ 0.0
    $self set sentinel_ 0
    $self set npkts_ 0
    $self next
}

Application/FTP/OnOffSender instproc setid { id tcp } {
    $self instvar id_ tcp_ stats_ on_ranvar_ off_ranvar_
    global ns opt

    set id_ $id
    set tcp_ $tcp
    set stats_ [new Stats $id]
    if { $opt(ontype) == "bytes" } {
        $self set on_ranvar_ [new RandomVariable/$opt(onrand)]
        $on_ranvar_ set avg_ $opt(avgbytes)
    } elseif { $opt(ontype) == "flowcdf" } {
        $self set on_ranvar_ [new FlowRanvar]
    }
    $self set off_ranvar_ [new RandomVariable/$opt(onrand)]
    $off_ranvar_ set avg_ $opt(offavg)

    $ns at [expr 0.5*[$off_ranvar_ value]] \
        "$self send [expr int([$on_ranvar_ value])]"
}

Application/FTP/OnOffSender instproc send { nbytes } {
    global ns opt
    $self instvar id_ npkts_ sentinel_ laststart_

    set laststart_ [$ns now]
    # The following two lines are because Tcl doesn't seem to do ceil() correctly!
    set npkts_ [expr int($nbytes / $opt(pktsize))]; # number of pkts for this on period
    if { $npkts_ * $opt(pktsize) != $nbytes } {
        incr npkts_
    }

#    if { $id_ == 0 } {
#        puts "sen $sentinel_ npkts $npkts_"
#    }
    set sentinel_ [expr $sentinel_ + $npkts_]; # stop when we send up to sentinel_
    [$self agent] advanceby $npkts_
    $self sched $opt(checkinterval);            # check in 5 milliseconds
    if { $opt(verbose) == "true" } {
        puts "[$ns now] $id_ turning ON for $nbytes bytes ( $npkts_ pkts )"
    }
}

Application/FTP/OnOffSender instproc sched { delay } {
    global ns
    $self instvar eventid_

    $self cancel
    set eventid_ [$ns at [expr [$ns now] + $delay] "$self timeout"]
}

Application/FTP/OnOffSender instproc cancel {} {
    global ns
    $self instvar eventid_
    if [info exists eventid_] {
        $ns cancel $eventid_
        unset eventid_
    }
}

Application/FTP/OnOffSender instproc timeout {} {
    global ns opt
    $self instvar id_ tcp_ stats_ sentinel_ npkts_ laststart_ lastrtt_ lastack_ on_ranvar_ off_ranvar_
    
    set rtt [expr [$tcp_ set rtt_] * [$tcp_ set tcpTick_] ]
    set ack [$tcp_ set ack_]
    if { $rtt != $lastrtt_ || $ack != $lastack_ } {
        $stats_ update_rtt $rtt
    }
    set lastrtt_ $rtt
    set lastack_ $ack
#    if {$id_ == 0} {
#        puts "[$ns now] ACK $ack sentinel $sentinel_"
#    }
    if { $ack >= $sentinel_ } { # have sent for this ON period
        if { $opt(verbose) == "true" } {
            puts "[$ns now] $id_ turning OFF"
        }
        $stats_ update_flowstats $npkts_ [expr [$ns now] - $laststart_]
        set npkts_ 0; # important to set to 0 here for correct stat calc
        $ns at [expr [$ns now]  +[$off_ranvar_ value]] \
            "$self send [expr int([$on_ranvar_ value])]"
    } else {
        # still the same connection
        $self sched $opt(checkinterval);       # check in 50 ms
    }
}

Application/FTP/OnOffSender instproc dumpstats {} {
    global ns
    $self instvar id_ stats_ laststart_ lastack_ sentinel_ npkts_
    # the connection might not have completed; calculate #acked pkts
    set acked [expr $npkts_ - ($sentinel_ - $lastack_)]
#    puts "dumpstats $id_: $npkts_ $sentinel_ $lastack_ $acked"
    if { $acked > 0 } {
        $stats_ update_flowstats $acked [expr [$ns now] - $laststart_]
    }
}

Class FlowRanvar 

FlowRanvar instproc init {} {
    $self instvar u_
    $self set u_ [new RandomVariable/Uniform]
    $u_ set min_ 0.0
    $u_ set max_ 1.0
}

FlowRanvar instproc value {} {
    $self instvar u_
    global flowcdf

    set r [$u_ value]
    set idx [expr int(100000 * $r)]
    if { $idx > [llength $flowcdf] } {
        set idx [expr [llength $flowcdf] - 1]
    }
    return  [expr 40 + [lindex $flowcdf $idx]]
}
