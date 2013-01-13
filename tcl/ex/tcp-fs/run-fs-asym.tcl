#! /usr/sww/bin/tclsh

set burstsize 30
set firstburstsize 0
set pause 50
set duration 99
set redwt 0.002
set window 32
set qsize 30
set rqsize 30
set acksz 6
set dir "."
set tcptype newreno

set opt(bs) burstsize
set opt(fbs) firstburstsize
set opt(pause) pause
set opt(dur) duration
set opt(redwt) redwt
set opt(win) window
set opt(q) qsize
set opt(rq) rqsize
set opt(acksz) acksz
set opt(dir) dir
set opt(tt) tcptype

proc getopt {argc argv} {
	global opt

	for {set i 0} {$i < $argc} {incr i} {
		set arg [lindex $argv $i]
		if {[string range $arg 0 0] != "-"} continue

		set name [string range $arg 1 end]
		if {[info exists opt($name)]} {
			eval "global $opt($name)"
			eval "set $opt($name) [lindex $argv [incr i]]"
		}
	}
}

proc Asym2WayComputeResults { } {
	global pause burstStartTime fid errfid cmd iter j rqsize
	global tcptype_opt rgw_opt dir 

	set thruputsum 0
	set thruputs [exec gawk {BEGIN {s="";} {if (index($1,"0,") == 0) { sum += $7;	s = sprintf("%s %d", s, $7);}} END {printf "%s %d", s, sum;}} thruput]
	set thruputsum [exec gawk {{if (match($1,"^0,") == 0) { sum += $7;}} END {printf "%g", sum;}} thruput]
	for {set src 0} {$src < 2} {incr src} {
		set responseTimeSum($src) 0
		set responseTimeCount($src) 0
		set responseTimeAvg($src) 0
	}
	foreach burstfile [glob $dir/{burst*.out}] {
		set src [exec gawk {{if (NR == 1) print $1;}} $burstfile]
		set responseTime($src) [exec gawk -v start=[expr $burstStartTime+$pause] -v end=[expr $burstStartTime+$pause+1] {{if (($5 >= start) && ($5 <= end)) {flag = 1; print $6 - $5; }} END {if (flag==0) print -1;}} $burstfile]
		if {$responseTime($src) == -1} {
			puts $errfid "$cmd"
			flush $errfid
			set responseTimeAvg($src) -1
			break
		}
		set responseTimeSum($src) [expr $responseTimeSum($src)+$responseTime($src)]
		incr responseTimeCount($src)
	}
	
	for {set src 0} {$src < 2} {incr src} {
		if {$responseTimeAvg($src) != -1 && $responseTimeCount($src) != 0} {
			set responseTimeAvg($src) [expr $responseTimeSum($src)/$responseTimeCount($src)]
		}
	}
	set outstr [format "%2s %11s %26s %3d %6s %2d %8s %8s" $iter [lindex $tcptype_opt $j] [lindex $rgw_opt $j] $rqsize - 1 $thruputsum $responseTimeAvg(0)]
	puts $fid "$outstr"
	flush $fid
}

set rgw_opt {-null -null -acksfirst -acksfirst}
set tcptype_opt {-newreno -newrenofs -newreno -newrenofs}

getopt $argc $argv
set fid [open "$tcptype.res" w]
set errfid [open "err-tcptype.res" w]
for {set iter 0} {$iter < 30} {incr iter} {
	set seed [expr int([exec rand 0 1000])]
	set burstStartTime [exec rand 0 5]
	foreach rqsize {20 30} {
		for {set j 0} {$j < [llength $tcptype_opt]} {incr j} {
			set cmd "../../../../ns.solaris ../test1.tcl -fp -nonfifo -sred -topo asym -mb 4 -bs $burstsize -fbs $firstburstsize -pause $pause -dur $duration -redwt $redwt -win $window -seed $seed -overhead 0.001 -q $qsize -rq $rqsize -acksz $acksz -ton [expr $burstStartTime+$pause] -toff [expr $burstStartTime+$pause+5] -dir $dir [lindex $rgw_opt $j] -dn -burst [lindex $tcptype_opt $j] $burstStartTime -up -$tcptype 40 -up -$tcptype 41 -up -$tcptype 42 -up -$tcptype 43"
			eval "exec $cmd"
			Asym2WayComputeResults
		}
	}
}
close $fid
close $errfid
