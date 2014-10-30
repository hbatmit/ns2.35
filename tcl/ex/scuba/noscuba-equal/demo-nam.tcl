proc makelinks { net bw delay pairs } {
	global ns node
	foreach p $pairs {
		set src [lindex $p 0]
		set dst [lindex $p 1]
		set angle [lindex $p 2]

	        mklink $net $src $dst $bw $delay $angle
	}
}

proc nam_config {net} {
	foreach k {0 1 2 4 5 6} {
	        $net node $k circle
	}
	foreach k {3 7} {
	        $net node $k square
	}

	makelinks $net 1.5Mb 10ms {
		{ 0 3 right-down }
		{ 1 3 right }
		{ 2 3 right-up }
		{ 7 4 right-up }
		{ 7 5 right }
		{ 7 6 right-down }
	}

	makelinks $net 400kb 50ms {
		{ 3 7 right }
	}

        $net queue 3 7 0.5
        $net queue 7 3 0.5

	# rtcp reports
        $net color 32 red
	# scuba reports
        $net color 33 white
	
	$net color 1 gold
	$net color 2 blue
	$net color 3 green
	$net color 4 magenta
}
