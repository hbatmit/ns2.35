source timer.tcl

Class Application/FTP/OnOffSender -superclass { Application/FTP }

Application/FTP/OnOffSender instproc init {} {
    global opt flowcdf

    $self set npkts_ 0
    $self set laststart_ 0.0
    $self set lastack_ 0
    $self set lastrtt_ 0.0
    $self set sentinel_ -1; # a sentinel of 0 means you need to send one packet
    $self set npkts_ 0
    $self set on_duration_ 0.0
    $self next
}

Application/FTP/OnOffSender instproc setup_and_start { id tcp } {
    $self instvar id_ tcp_ stats_ on_ranvar_ off_ranvar_
    global ns opt

    set id_ $id
    set tcp_ $tcp
    set stats_ [new Stats $id]
    set run [expr $opt(seed) + 2]
    if { $opt(ontype) == "bytes" || $opt(ontype) == "time" } {
        set on_rng [new RNG]
        for { set j 1 } {$j < $run} {incr j} {
            $on_rng next-substream
        }
        set on_ranvar_ [new RandomVariable/$opt(onrand)]
        if { $opt(ontype) == "bytes" } {        
            $on_ranvar_ set avg_ $opt(avgbytes)
        } else {
            $on_ranvar_ set avg_ $opt(onavg)
        }
        $on_ranvar_ use-rng $on_rng
    } elseif { $opt(ontype) == "flowcdf" } {
        $self set on_ranvar_ [new FlowRanvar]
    }
    set off_rng [new RNG]
    for { set j 1 } {$j < $run} {incr j} {
        $off_rng next-substream
    }
    set off_ranvar_ [new RandomVariable/$opt(onrand)]
    $off_ranvar_ set avg_ $opt(offavg)
    $off_ranvar_ use-rng $off_rng

    if { $opt(spike) == "true" } { # for spike, ontype must be "time"
        if { $id_ == 0 } {
            $ns at [$ns now] "$self send [expr $opt(simtime) - 1]"
        } else {
            $ns at [expr [$ns now] + $opt(spikestart)] "$self send $opt(spikeduration)"
        }
    } else {
        $ns at [expr 0.5*[$off_ranvar_ value]] \
            "$self send [$on_ranvar_ value]"
    }
}

Application/FTP/OnOffSender instproc send { bytes_or_time } {
    global ns opt
    $self instvar id_ npkts_ sentinel_ laststart_ on_duration_ tcp_
    
    set laststart_ [$ns now]
    if { $opt(ontype) == "bytes" || $opt(ontype) == "flowcdf" } {
        set nbytes [expr int($bytes_or_time)]
        # The next 3 lines are because Tcl doesn't seem to do ceil() correctly!
        set npkts_ [expr int($nbytes / $opt(pktsize))]; # pkts for this on period
	# this is a really ugly hack because for some reason TCP/Vegas does not
	# use packet headers! Weird...
	if { $opt(tcp) != "TCP/Vegas" } {
	    if { $npkts_ * $opt(pktsize) != $nbytes } {
		incr npkts_
	    }
	}
        set sentinel_ [expr $sentinel_ + $npkts_]; # stop when we send up to sentinel_
        [$self agent] advanceby $npkts_
#        [$self agent] send $nbytes
        if { $opt(verbose) == "true" } {
            puts "[$ns now] $id_ turning ON for $nbytes bytes ( $npkts_ pkts )"
        }
    } elseif { $opt(ontype) == "time" } {
        set on_duration_ $bytes_or_time
        [$self agent] send -1;  # "infinite" source, but we'll stop it later
        if { $opt(verbose) == "true" } {
            puts "[$ns now] $id_ turning ON for $on_duration_ seconds"
        }
    }
    $self sched $opt(checkinterval);
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
    $self instvar id_ tcp_ stats_ on_duration_ sentinel_ npkts_ laststart_ lastrtt_ lastack_ on_ranvar_ off_ranvar_

    set done false
    set rtt [expr [$tcp_ set rtt_] * [$tcp_ set tcpTick_] ]
    set ack [$tcp_ set ack_]
    if { $rtt != $lastrtt_ || $ack != $lastack_ } {
	if { $rtt > 0 } {
	    $stats_ update_rtt $rtt
	}
    }
    if { $opt(ontype) == "bytes" || $opt(ontype) == "flowcdf" } {
        if { $ack >= $sentinel_ } { # have sent for this ON period
            set done true
        }
    } elseif { $opt(ontype) == "time" } {
        set npkts_ [expr $npkts_ + $ack - $lastack_]
        if { [$ns now] - $laststart_ > $on_duration_ } {
            set done true
        }
    }
    set lastrtt_ $rtt
    set lastack_ $ack

    if { $done == true } {
        if { $opt(verbose) == "true" } {
                puts "[$ns now] $id_ turning OFF"
        }
        if { $npkts_ < 0 } {
            set npkts_ 0
        }
        $stats_ update_flowstats $npkts_ [expr [$ns now] - $laststart_]
        set npkts_ 0; # important to set to 0 here for correct stat calc
        if { $opt(ontype) == "time" } {
            [$self agent] advance 0; # causes TCP to pause
        }

        # Reset all connections whether bytes or time.
        $tcp_ reset
        if { $opt(spike) != "true" } {
            $ns at [expr [$ns now]  +[$off_ranvar_ value]] \
                "$self send [$on_ranvar_ value]"
        }
    } else {
        # still the same connection
        $self sched $opt(checkinterval); # check again in a little time
    }
}

Application/FTP/OnOffSender instproc dumpstats {} {
    global ns opt
    $self instvar id_ stats_ laststart_ lastack_ sentinel_ npkts_ tcp_
    # the connection might not have completed; calculate #acked pkts

    # Update last ack
    set lastack_ [$tcp_ set ack_]

    if {$opt(ontype) == "bytes" || $opt(ontype) == "flowcdf" } {
        set acked [expr $npkts_ - ($sentinel_ - $lastack_)]
        #    puts "dumpstats $id_: $npkts_ $sentinel_ $lastack_ $acked"
        if { $acked > 0 } {
            $stats_ update_flowstats $acked [expr [$ns now] - $laststart_]
        }
    }  else {
        set rem_pkts [expr $lastack_ - [$stats_ set npkts_] + 1]
        if { $rem_pkts > 0 } {
            $stats_ update_flowstats $rem_pkts [expr [$ns now] - $laststart_]
        }
    }
}

source flowranvar.tcl
