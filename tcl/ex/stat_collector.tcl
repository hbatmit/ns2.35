# Author : Hari Balakrishnan
# Class that encapsulates all statistics collection.

Class StatCollector 

StatCollector instproc init {id ctype} {
    $self set srcid_ $id
    $self set ctype_ $ctype;    # type of congestion control / tcp
    $self set numbytes_ 0
    $self set ontime_ 0.0;     # total time connection was in ON state
    $self set cumrtt_ 0.0
    $self set nsamples_ 0
    $self set nconns_ 0
}

StatCollector instproc update {newbytes newtime cumrtt nsamples} {
    global ns opt
    $self instvar srcid_ numbytes_ ontime_ cumrtt_ nsamples_ nconns_
    incr numbytes_ $newbytes
    set ontime_ [expr $ontime_ + $newtime]
    set cumrtt_ $cumrtt
    set nsamples_ $nsamples
    incr nconns_
#    puts "[$ns now]: updating stats for $srcid_: $newbytes $newtime $cumrtt $nsamples"
#    puts "[$ns now]: \tTO: $numbytes_ $ontime_ $cumrtt_ $nsamples_"
    if { $opt(partialresults) } {
        showstats False
    }
}

StatCollector instproc results { } {
    $self instvar numbytes_ ontime_ cumrtt_ nsamples_ nconns_
    return [list $numbytes_ $ontime_ $cumrtt_ $nsamples_ $nconns_]
}
