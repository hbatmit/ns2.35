Class Stats

Stats instproc init { id } {
    $self set srcid_ $id
    $self set npkts_ 0
    $self set numbytes_ 0
    $self set ontime_ 0.0;     # total time connection was in ON state
    $self set throughput_ 0.0
    $self set cumrtt_ 0.0
    $self set rtt_samples_ {}
    $self set nsamples_ 0
    $self set nflows_ 0
}

# Called when a flow ends
Stats instproc update_flowstats { newpkts newtime } {
    global opt
    $self instvar srcid_ npkts_ numbytes_ ontime_ throughput_ nflows_
    
    if { $opt(verbose) == "true" } {
        puts "conn $srcid_ updating pkts $newpkts time $newtime"
    }
    set npkts_ [expr $npkts_ + $newpkts]
    set numbytes_ [expr $numbytes_ + ($opt(hdrsize) + $opt(pktsize))*$newpkts]
    set ontime_ [expr $ontime_ + $newtime]
    incr nflows_
    
    if { $opt(verbose) == "true" } {
        puts "updating ontime to $ontime_"
    }
}

Stats instproc update_rtt { rtt } {
    $self instvar rtt_samples_ cumrtt_ nsamples_

    set cumrtt_ [expr $cumrtt_ + $rtt]
    incr nsamples_
    lappend rtt_samples $rtt
}

Stats instproc showstats { rcd_bytes rcd_avgrtt } {
    $self instvar srcid_ numbytes_ ontime_ throughput_ rtt_samples_ cumrtt_ nsamples_ nflows_
    global opt

    if { $nsamples_ > 0.0 } {
        set avgrtt [expr 1000.0*$cumrtt_/$nsamples_]; # in milliseconds
    } else {
        set avgrtt 0.0
    }
    if { $ontime_ > 0.0 } {
        set throughput [expr 8.0*$numbytes_/1000000 / $ontime_  ]
        set rcdtput [expr 8.0*$rcd_bytes / 1000000 / $ontime_  ]
    } else {
        set throughput 0.0
        set rcdtput 0.0
    }
    if { $throughput > 0.0 && $avgrtt > 0.0 } {
        set util_s [expr log(1000000) + log($throughput) - log($avgrtt) ]
    } else {
        set util_s 0.0
    }
    if { $rcdtput > 0.0 && $avgrtt > 0.0 } {
        set util_r [expr log(1000000) + log($rcdtput) - log($avgrtt) ]
    } else {
        set util_r 0.0
    }
    if { $nflows_ > 0 } {
	set fct [expr 1000.0*$ontime_/$nflows_]
    } else {
	set fct 0.0
    }
    set on_perc [expr 100.0*$ontime_ / $opt(simtime)]
    
    puts [format "conn: %d rbytes: %d rMbps: %.2f fctMs: %.0f abytes: %d aMbps: %.3f sndrttMs %.1f rcdrttMs %.1f s_util: %.2f r_util: %.2f onperc: %.1f" $srcid_ $rcd_bytes $rcdtput $fct $numbytes_ $throughput $avgrtt $rcd_avgrtt $util_s $util_r $on_perc]
}
