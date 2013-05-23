Class Stats

Stats instproc init { id } {
    $self set srcid_ $id
    $self set numbytes_ 0
    $self set ontime_ 0.0;     # total time connection was in ON state
    $self set throughput_ 0.0
    $self set cumrtt_ 0.0
    $self set rtt_samples_ {}
    $self set nsamples_ 0
    $self set nconns_ 0
}

Stats instproc update_pkts { newpkts newtime } {
    global opt
    $self instvar numbytes_ ontime_ throughput_
    
    if { $opt(verbose) == "true" } {
        puts "updating $newpkts $newtime"
    }
    set numbytes_ [expr $numbytes_ + $opt(pktsize)*$newpkts]
    set ontime_ [expr $ontime_ + $newtime]
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

Stats instproc showstats { } {
    $self instvar srcid_ numbytes_ ontime_ throughput_ rtt_samples_ cumrtt_ nsamples_

    if { $nsamples_ > 0.0 } {
        set avgrtt [expr 1000.0*$cumrtt_/$nsamples_]; # in milliseconds
    } else {
        set avgrtt 0.0
    }
    if { $ontime_ > 0.0 } {
        set throughput [expr 8.0*$numbytes_/1000000 / $ontime_  ]
    } else {
        set throughput 0.0
    }
    if { $throughput > 0.0 && $avgrtt > 0.0 } {
        set util [expr log(1000000.0/8) + log($throughput) - log($avgrtt) ]
    } else {
        set util 0.0
    }
    
    puts [format "%d %d bytes %.3f Mbps %.0f ms %.2f" $srcid_ $numbytes_ $throughput $avgrtt $util]
}
