Class FlowRanvar

FlowRanvar instproc init {} {
    global opt
    $self instvar u_
    set rng [new RNG]
    set run [expr $opt(seed) + 2]
    for { set j 1 } {$j < $run} {incr j} {
        $rng next-substream
    }
    $self set u_ [new RandomVariable/Uniform]
    $u_ set min_ 0.0
    $u_ set max_ 1.0
    $u_ use-rng $rng
}

FlowRanvar instproc value {} {
    $self instvar u_
    global flowcdf opt

    set r [$u_ value]
    set idx [expr int(100000 * $r)]
    if { $idx > [llength $flowcdf] } {
        set idx [expr [llength $flowcdf] - 1]
    }
    return  [expr $opt(flowoffset) + [lindex $flowcdf $idx]]
}
