# Author : Hari Balakrishnan
# Class that simulates random on/off traffic, where
# on and off durations are drawn from a specific distribution.

Class LoggingApp -superclass Application

LoggingApp instproc init {id tcp_sender} {
    $self instvar srcid_ nbytes_ cumrtt_ numsamples_ tcp_sender_
    global opt
    $self set srcid_ $id
    $self set tcp_sender_ $tcp_sender
    $self set nbytes_ 0
    $self set cumrtt_ 0.0
    $self set numsamples_ 0
    $self set nrtt_ 0
    $self next
}

LoggingApp instproc recv { bytes } {
    # there's one of these objects for each src/dest pair
    $self instvar nbytes_ srcid_ cumrtt_ numsamples_ nrtt_ tcp_sender_
    global ns opt

    set nbytes_ [expr $nbytes_ + $bytes]
    incr numsamples_

    set rtt_ [expr [$tcp_sender_ set rtt_] * [$tcp_sender_ set tcpTick_]]
    if {$rtt_ > 0.0} {
        set cumrtt_ [expr $rtt_  + $cumrtt_]
        incr nrtt_
    }
#    if { $opt(verbose) } {
#        puts "[$ns now]: $srcid_ rcd $bytes total $nbytes_ npkts $numsamples_ rtt $rtt_"
#    }
}

LoggingApp instproc dumpstats { } {
    $self instvar srcid_ nbytes_ numsamples_ nrtt_ cumrtt_

    if {$nrtt_ > 0} {
        puts "\trcv $srcid_: $nbytes_ bytes $numsamples_ pkts [expr 1.0*$cumrtt_/$nrtt_] ms"
    } else {
        puts "\trcv $srcid_: $nbytes_ bytes $numsamples_ pkts 0.0 ms"
    }
}
