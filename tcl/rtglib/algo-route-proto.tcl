Class Agent/rtProto/Algorithmic -superclass Agent/rtProto
#Agent/rtProto/Algorithmic proc pre-init-all args {
#    [Simulator instance] set routingTable_ [new RouteLogic/Algorithmic]
#}

Agent/rtProto/Algorithmic proc init-all args {
    [Simulator instance] compute-algo-routes
}

Agent/rtProto/Algorithmic proc compute-all {} {
    [Simulator instance] compute-algo-routes
}

RouteLogic/Algorithmic instproc BFS {} {
    $self instvar ns_ children_ root_ rank_

    set ns_ [Simulator instance]
    if {[$ns_ info class] == "Simulator"} {
	$ns_ instvar link_
	foreach ln [array names link_] {
	    set L [split $ln :]
	    set srcID [lindex $L 0]
	    set dstID [lindex $L 1]
	    if ![info exist adj($srcID)] {
		set adj($srcID) ""
	    }
	    if ![info exist adj($dstID)] {
		set adj($dstID) ""
	    }
	    if {[lsearch $adj($srcID) $dstID] < 0} {
		lappend adj($srcID) $dstID
	    }
	    if {[lsearch $adj($dstID) $srcID] < 0} {
		lappend adj($dstID) $srcID
	    }
	}
    } elseif {[$ns_ info class] == "SessionSim"} {
	$ns_ instvar delay_
	foreach ln [array names delay_] {
	    set L [split $ln :]
	    set srcID [lindex $L 0]
	    set dstID [lindex $L 1]
	    if ![info exist adj($srcID)] {
		set adj($srcID) ""
	    }
	    if ![info exist adj($dstID)] {
		set adj($dstID) ""
	    }
	    if {[lsearch $adj($srcID) $dstID] < 0} {
		lappend adj($srcID) $dstID
	    }
	    if {[lsearch $adj($dstID) $srcID] < 0} {
		lappend adj($dstID) $srcID
	    }
	}
    }

#    foreach index [array names adj] {
#	puts "$index: $adj($index)"
#    }

    set rank_ 0
    set root_ 0
    set traversed($root_) 1
    set queue "$root_"

    while {[llength $queue] > 0} {
	# puts "queue: $queue, queue-length: [llength $queue]"
	set parent [lindex $queue 0]
	set queue [lreplace $queue 0 0]
	# puts "parent: $parent, queue: $queue, queue-length: [llength $queue]"
	if ![info exist children_($parent)] {
	    set children_($parent) ""
	}

	# puts "adj: $adj($parent)"
	foreach nd $adj($parent) {
	    if ![info exist traversed($nd)] {
		set traversed($nd) 0
	    }
	    if !$traversed($nd) {
		set traversed($nd) 1
		lappend children_($parent) $nd
		lappend queue $nd
	    }
	}
	set num_children [llength $children_($parent)]
	if {$rank_ < $num_children} {
	    set rank_ $num_children
	}
	# puts "rank: $rank_, queue: $queue, queue-length: [llength $queue]"
    }
}

RouteLogic/Algorithmic instproc compute {} {
    $self instvar root_ children_ rank_ id_ algoAdd_

    # set queue "$root_"
    # while {[llength $queue] > 0} {
	# set parent [lindex $queue 0]
	# set queue [lreplace $queue 0 0]
	# puts "$parent: $children_($parent)"
	# foreach child $children_($parent) {
	    # lappend queue $child
	# }
    # }

    set queue [list [list $root_ 0]]

    while {[llength $queue] > 0} {
#	puts $queue
	set parent [lindex $queue 0]
	set queue [lreplace $queue 0 0]
	set id [lindex $parent 0]
	set algoAdd [lindex $parent 1]
	set id_($algoAdd) $id
	set algoAdd_($id) $algoAdd

	set i 0
	foreach child $children_($id) {
	    incr i
	    lappend queue [list $child [expr [expr $algoAdd * $rank_] + $i]]
	}
    }
}

RouteLogic/Algorithmic instproc lookup {src dst} {
    $self instvar id_ algoAdd_
    set algosrc $algoAdd_($src)
    set algodst $algoAdd_($dst)
    set algonxt [$self algo-lookup $algosrc $algodst]
#    puts "lookup: $algosrc $algodst $algonxt $id_($algonxt)"
    return $id_($algonxt)
}


RouteLogic/Algorithmic instproc algo-lookup {src dst} {
    $self instvar rank_

    if {$src == $dst} {
	return $src
    }
    set a $src
    set b $dst
    set offset 0
    
    while {$b > $a} {
	set offset [expr $b % $rank_]
	set b [expr $b / $rank_]
	if {$offset == 0} {
	    set offset $rank_
	    set b [expr $b - 1]
	}
    }

    if {$b == $a} {
	return [expr [expr $a * $rank_] + $offset]
    } else {
	return [expr [expr $a - 1] / $rank_]
    }
}

