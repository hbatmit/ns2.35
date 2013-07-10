Class Application/FTP/BulkSender -superclass { Application/FTP }

Application/FTP/BulkSender instproc init {} {
    global opt

    $self set lastack_ 0
    $self set lastrtt_ 0.0
    $self next
}

Application/FTP/BulkSender instproc setup_and_start { id tcp } {
    $self instvar id_ tcp_ stats_
    global ns opt

    set id_ $id
    set tcp_ $tcp
    set stats_ [new Stats $id]
    $ns at 0.0 "$self send -1"
}

Application/FTP/BulkSender instproc send { bytes_or_time } {
    global ns opt
    $self instvar id_

    [$self agent] send -1
    if { $opt(verbose) == "true" } {
            puts "[$ns now] $id_ turning ON forever"
    }
    $self sched $opt(checkinterval);
}

Application/FTP/BulkSender instproc sched { delay } {
    global ns
    $self instvar eventid_

    $self cancel
    set eventid_ [$ns at [expr [$ns now] + $delay] "$self timeout"]
}

Application/FTP/BulkSender instproc cancel {} {
    global ns
    $self instvar eventid_
    if [info exists eventid_] {
        $ns cancel $eventid_
        unset eventid_
    }
}

Application/FTP/BulkSender instproc timeout {} {
    global ns opt
    $self instvar id_ tcp_ stats_ lastrtt_ lastack_

    set done false
    set rtt [expr [$tcp_ set rtt_] * [$tcp_ set tcpTick_] ]
    set ack [$tcp_ set ack_]
    if { $rtt != $lastrtt_ || $ack != $lastack_ } {
      if { $rtt > 0.0 } {
        $stats_ update_rtt $rtt
      }
    }
    set lastrtt_ $rtt
    set lastack_ $ack

    # still the same connection
    $self sched $opt(checkinterval); # check again in a little time
}

Application/FTP/BulkSender instproc dumpstats {} {
    global ns opt
    $self instvar id_ stats_ lastack_
    # the connection might not have completed; calculate #acked pkts

    set rem_pkts [expr $lastack_ - [$stats_ set npkts_]]
    if { $rem_pkts > 0 } {
      $stats_ update_flowstats "bt" $rem_pkts [$ns now]
    }
}
