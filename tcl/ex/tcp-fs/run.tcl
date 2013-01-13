#! /usr/local/bin/tclsh

#set pdrop_opt {-null -null "-recn -rdrop" "-pdrop" "-recn -rdrop -pdrop"}
#set tcptype_opt {-fack -fackfs -fackfs -fackfs -fackfs}

#set pdrop_opt {-null -null "-null -nofrt" "-null -nofrt -noflr" -pdrop "-pdrop -nofrt" "-pdrop -nofrt -noflr"}
#set tcptype_opt {-newreno -newrenofs -newrenofs -newrenofs -newrenofs -newrenofs -newrenofs}

#set pdrop_opt {-null -null -pdrop "-pdrop -nofrt" "-pdrop -nofrt -noflr"}
#set tcptype_opt {-newreno -newrenofs -newrenofs -newrenofs -newrenofs}

set pdrop_opt {-null -pdrop}
set tcptype_opt {-newreno -newrenofs}

#initial values
set tick 0.1
set maxnumconn 4
set numburstconn 1
set tcptype newreno
set burstonly 3
set bulkCrossTraffic 1
set burstsize 50
set qsize 20
set delay 50ms
set firstburstsize 0
set window 32
set burstwin 32
set redwt 0.002
set pause 30
set duration 59
set burstStartTime 0
set burstEndTime 1
set bulkStartTime 5
set bulkEndTime 10
set fbw 1.5Mb
set dir "."
set opt(tk) tick
set opt(nc) maxnumconn
set opt(nbc) numburstconn
set opt(tt) tcptype
set opt(bo) burstonly
set opt(bulk) bulkCrossTraffic
set opt(bs) burstsize
set opt(q) qsize
set opt(del) delay 
set opt(fbs) firstburstsize
set opt(win) window
set opt(bwin) burstwin
set opt(redwt) redwt
set opt(pause) pause
set opt(dur) duration
set opt(burststart) burstStartTime
set opt(burstend) burstEndTime
set opt(bulkstart) bulkStartTime
set opt(bulkend) bulkEndTime
set opt(dir) dir
set opt(fbw) fbw

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

# Compute results for a combination of bulk transfers and Web-like transfers
proc BulkWebComputeResults { } {
	global pause burstStartTime fid errfid cmd iter qsize delay j
	global tcptype_opt pdrop_opt
	global dir
	global numconn
	global burstsize

	set thruputsum 0
	set thruputs [exec gawk {BEGIN {s="";} {if (index($1,"0,") == 0) { sum += $7;	s = sprintf("%s %d", s, $7);}} END {printf "%s %d", s, sum;}} thruput]
	set thruputsum [exec gawk {{if (index($1,"0,") == 0) { sum += $7;}} END {printf "%d", sum;}} thruput]
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

	set dropcount [exec gawk -v start=[expr $burstStartTime+$pause] -v end=[expr $burstStartTime+$pause+5] {BEGIN {drops[0] = 0; drops[1] = 0;} {if ($1 == "d" && $2 >= start && $2 <= end) drops[int($9)]++;} END {print drops[0], drops[1]}} $dir/out.tr]

	set outstr [format "%2s %11s %26s %3d %6s %2d %8s %8s %10s" $iter [lindex $tcptype_opt $j] [lindex $pdrop_opt $j] $qsize $delay $numconn $thruputsum $responseTimeAvg(0) $dropcount]
	puts $fid "$outstr"
	flush $fid
#	puts "$outstr"
}


# Compute results for just one set of Web-like transfers
proc WebOnlyComputeResults { } {
	global pause burstStartTime fid errfid cmd iter qsize delay j
	global tcptype_opt pdrop_opt
	global dir

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

	set dropcount [exec gawk -v start=[expr $burstStartTime+$pause] {BEGIN {drops[0] = 0; drops[1] = 0;} {if ($1 == "d" && $2 >= start) drops[int($9)]++;} END {print drops[0], drops[1]}} $dir/out.tr]
	
	set outstr [format "%2s %11s %26s %3d %6s 0 %8s %8s %10s" $iter [lindex $tcptype_opt $j] [lindex $pdrop_opt $j] $qsize $delay $responseTimeAvg(0) $responseTimeAvg(1) $dropcount]
	puts $fid "$outstr"
	flush $fid
#	puts "$outstr"
}


getopt $argc $argv
set fid [open "$tcptype-$maxnumconn-$numburstconn.res" w]
set errfid [open "err-$tcptype-$maxnumconn-$numburstconn.res" w]
set cmdfid [open "cmd-$tcptype-$maxnumconn-$numburstconn.res" w]

#set numconn $maxnumconn
for {set iter 0} {$iter < 3} {incr iter} {
for {set numconn $maxnumconn} {$numconn <= $maxnumconn} {incr numconn} { 
	set seed [expr int([exec rand 0 1000])]
#	puts -nonewline $cmdfid "$iter "
	for {set k 0} {$k < $numburstconn} {incr k} {
#		set cmd1_rand($k) [exec rand 0 10]
		set cmd1_start($k) [exec rand $burstStartTime $burstEndTime]
		set cmd1_rand($k) [exec rand 0 1]
#		puts -nonewline $cmdfid " $cmd1_rand($k)"
	}
	set cmd2 ""
	if { $numconn > 0 } {
		append cmd2 " -src 01 -dst 31"
	}
	for {set k 0} {$k < $numconn} {incr k} {
		if {$bulkCrossTraffic} {
			append cmd2 " -$tcptype [exec rand $bulkStartTime $bulkEndTime]"
		} else {
			append cmd2 " -burst -$tcptype [exec rand $bulkStartTime $bulkEndTime] -rand [exec rand 0 1]"
		}
	}
#	puts $cmdfid "$cmd2"
#	flush $cmdfid
#	foreach qsize {200} {
#		foreach delay {200ms} {
#	for {set burstsize 10} {$burstsize < 200} {incr burstsize 10} {
			for {set j 0} {$j < [llength $pdrop_opt]} {incr j} {
				set cmd1 ""
				if {[lindex $tcptype_opt $j] == "-null"} {
					append cmd1 " -pause 100"
				}
				append cmd1 " -src 00 -dst 30"
				for {set k 0} {$k < $numburstconn} {incr k} {
					append cmd1 " -burst [lindex $tcptype_opt $j] $cmd1_start($k) -rand $cmd1_rand($k)"
				}
				if {[file exists "thruput"]} {
					exec rm "thruput"
				}
				foreach f [glob -nocomplain $dir/{burst*.out}] {
					exec rm $f
				}
				set cmd "../../../../ns.bsdi ../test1.tcl [lindex $pdrop_opt $j] -seed $seed -overhead 0.001 -tk $tick -fp -dur $duration -fbw $fbw -del $delay -nonfifo -sred -redwt $redwt -q $qsize -mb 4 -bs $burstsize -fbs $firstburstsize -pause $pause -ton [expr $burstStartTime+$pause] -toff [expr $burstStartTime+$pause+5] -win $window -bwin $burstwin -dir $dir"
#				set cmd "../../../../ns.solaris ../test1.tcl [lindex $pdrop_opt $j] -seed $seed -overhead 0.001 -tk $tick -fp -dur $duration -del $delay -nonfifo -sred -redwt $redwt -q $qsize -mb 4 -bs $burstsize -fbs $firstburstsize -pause $pause -ton [expr $burstStartTime+$pause] -toff [expr $burstStartTime+$pause+5] -win $window -bwin $burstwin -dir $dir"
				append cmd $cmd1
				append cmd $cmd2
				puts -nonewline $cmdfid "$iter "
				puts $cmdfid "$cmd"
				flush $cmdfid
#				puts $cmd
				eval "exec $cmd"
				if {$bulkCrossTraffic} {
					BulkWebComputeResults
				} else {
					WebOnlyComputeResults
				}
			}
#		}
		#	}
#	}
}
}
			
close $fid
close $errfid
close $cmdfid


proc oldComputeResults { } {
	global dir

	set thruputs [exec gawk {BEGIN {s="";} {if (index($1,"(0,") == 0) { sum += $7;	s = sprintf("%s %d", s, $7);}} END {printf "%s %d", s, sum;}} thruput]
	for {set src 0} {$src < 2} {incr src} {
		set responseTimeSum($src) 0
		set responseTimeCount($src) 0
		set responseTimeAvg($src) 0
	}
	set otherDropsSum 0
	set otherDropsCount 0
	set otherDropsAvg 0
	set fsDropsSum 0
	set fsDropsCount 0
	set fsDropsAvg 0
	foreach burstfile [glob $dir/{burst*.out}] {
		set src [exec gawk {{if (NR == 1) print $1;}} $burstfile]
		set responseTime($src) [exec gawk {{if ($5 >= 30 && $5 <= 31) {flag = 1; print $6 - $5; }} END {if (!flag) print -1;}} $burstfile]
		if {$responseTime($src) == -1} {
			puts $errfid "$cmd"
			flush $errfid
			set responseTimeAvg($src) -1
			set otherDropsAvg -1
			set fsDropsAvg -1
			break
		}
		set responseTimeSum($src) [expr $responseTimeSum($src)+$responseTime($src)]
		incr responseTimeCount($src)
		set startTime [exec gawk {{if (NR == 2) print $5;} END {if (NR < 2) print -1;}} $burstfile]
		set endTime [exec gawk {{if (NR == 2) print $6;} END {if (NR < 2) print -1;}} $burstfile]
		if {$startTime != -1 && $endTime != -1} {
			set drops [exec gawk -v startTime=$startTime -v endTime=$endTime -f ../drop.awk $dir/out.tr]
			set otherDrops [lindex $drops 0]
			set otherDropsSum [expr $otherDropsSum+$otherDrops]
			incr otherDropsCount
			set fsDrops [lindex $drops 1]
			set fsDropsSum [expr $fsDropsSum+$fsDrops]
			incr fsDropsCount
		} else {
			set otherDropsAvg -1
			set fsDropsAvg -1
			break
		}
	}
	
	for {set src 0} {$src < 2} {incr src} {
		if {$responseTimeAvg($src) != -1} {
			set responseTimeAvg($src) [expr $responseTimeSum($src)/$responseTimeCount($src)]
			set otherDropsAvg [expr $otherDropsSum/$otherDropsCount]
			set fsDropsAvg [expr $fsDropsSum/$fsDropsCount]
		}
	}
	#				set outstr [format "%2s %11s %20s %3d %6s %8s %25s %4s %4s" $i [lindex $tcptype_opt $j] [lindex $pdrop_opt $j] $qsize $delay $responseTimeAvg(0) $thruputs $otherDropsAvg $fsDropsAvg]
	set outstr [format "%2s %11s %20s %3d %6s %8s %8s" $i [lindex $tcptype_opt $j] [lindex $pdrop_opt $j] $qsize $delay $responseTimeAvg(0) $responseTimeAvg(1)]
	puts $fid "$outstr"
	flush $fid
	puts "$outstr"
}



