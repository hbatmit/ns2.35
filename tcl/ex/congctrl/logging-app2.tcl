# Author : Hari Balakrishnan
# Class that simulates random on/off traffic, where
# on and off durations are drawn from a specific distribution.
source timer.tcl

Class LoggingApp -superclass {Application Timer}

LoggingApp instproc init {id} {
    $self instvar srcid_ nbytes_ cumrtt_ numsamples_ u_ offtotal_ on_ranvar_ off_ranvar_ rtt_samples_ flowfile
    global opt
    $self set srcid_ $id
    $self reset
    $self set u_ [new RandomVariable/Uniform]
    $self set offtotal_ 0.0
    
    # Internet flow CDF distribution
    $self set flowfile flowcdf-allman-icsi.tcl

    # Create exponential on/off RandomVariables
    $self set on_ranvar_  [new RandomVariable/$opt(onrand)]
    $self set off_ranvar_ [new RandomVariable/$opt(offrand)]
    if { $opt(ontype) == "time" } {
        $on_ranvar_ set avg_ $opt(onavg)
    } elseif { $opt(ontype) == "bytes" } {
            $on_ranvar_ set avg_ $opt(avgbytes)
    } elseif { $opt(ontype) == "flowcdf" } {
        source $flowfile
    }
    set off_ranvar_ [new RandomVariable/$opt(offrand)]
    $off_ranvar_ set avg_ $opt(offavg)

    $self settype
    $self next
}

LoggingApp instproc reset { } {
    $self instvar nbytes_ cumrtt_ numsamples_ state_

    $self set state_ OFF
    $self set nbytes_ 0
    $self set cumrtt_ 0.0
    $self set numsamples_ 0
    $self set rtt_samples_ {}
}

LoggingApp instproc settype { } {
    $self instvar endtime_ maxbytes_ 
    global opt
    if { $opt(ontype) == "time" } {
        $self set maxbytes_ "infinity"; # not byte-limited
        $self set endtime_ 0
    } else {
        $self set endtime_ $opt(simtime)
        $self set maxbytes_ 0
    }
}

LoggingApp instproc recv { bytes } {
    # there's one of these objects for each src/dest pair 
    $self instvar nbytes_ srcid_ cumrtt_ numsamples_ maxbytes_ endtime_ laststart_ state_ u_ offtotal_ off_ranvar_ on_ranvar_ rtt_samples_
    global ns opt src tp stats flowcdf

    set nbytes_ [expr $nbytes_ + $bytes]
#    puts "[$ns now]: $srcid_ rcd $bytes total $nbytes_"
    
    set tcp_sender [lindex $tp($srcid_) 0]
    set rtt_ [expr [$tcp_sender set rtt_] * [$tcp_sender set tcpTick_]]
    if {$rtt_ > 0.0} {
        set cumrtt_ [expr $rtt_  + $cumrtt_]
        lappend rtt_samples_ $rtt_
        set numsamples_ [expr $numsamples_ + 1]
    }
}

LoggingApp instproc sample_off_duration {} {
    $self instvar off_ranvar_
    return [$off_ranvar_ value]
}
