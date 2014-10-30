# This test suite is for validating scheduler simultaneous event ordering in ns
#
# To run all tests:  test-all-scheduler
#
# To run individual tests:
# ns test-suite-scheduler.tcl List
# ns test-suite-scheduler.tcl Calendar
# ns test-suite scheduler.tcl Heap
#
# To view a list of available tests to run with this script:
# ns test-suite-scheduler.tcl
#
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:

# What does this simple test do?
#   - it schedules $TIMES batches of events.  Each batch contains $SIMUL events, 
#     all of which will occur at the same time.  All events are permuted and
#     scheduled in a random order.  The output should be a list of integers 
#     from 1 to ($TIMES*$SIMUL) in increasing order.
#   - if the output differs it exits with status 1, otherwise it exits with status 0.

proc comp {a b} {
	set a1 [lindex $a 0]
	set b1 [lindex $b 0]
	if {$a1 > $b1} {
		return 1
	}
	return 0
}

Class TestSuite

TestSuite instproc init { quiet } {
	$self instvar ns_ rng_ N_ quiet_
	set ns_ [new Simulator]
	set rng_ [new RNG]
	set N_ 0
	set quiet_ $quiet
}

TestSuite instproc run { scheduler } {
	$self instvar ns_ rng_ N_
	if { [catch "$ns_ use-scheduler $scheduler"] } {
		puts "*** WARNING: scheduler Scheduler/$scheduler is not supported, test was not run"
		exit 0
	}
	
	set TIMES 20  ;# $TIMES different times for events
	set SIMUL 50  ;# each occurs $SIMUL times
	set TIMEMIN 0 ;# random events are taken from [TIMEMIN, TIMEMAX]
	set TIMEMAX 5

	# generate random event timings and put them in increasing order by time to occur
	for {set i 0 } { $i < $TIMES } { incr i } {
		lappend timings [list [$rng_ uniform $TIMEMIN $TIMEMAX] $i $SIMUL]
	}
	set stimings [lsort -command "comp" $timings]
	for {set i 0 } { $i < $TIMES } { incr i } {
		set e [lindex $stimings $i]
		set idx [lsearch $timings $e]
		set timings [lreplace $timings $idx $idx [lappend e $i]]
	}

	while 1 {
		# pull out timings in random order
		set i [expr int([$rng_ uniform 0 [llength $timings]])]
		set e [lindex $timings $i]
		
		set order [lindex $e 3]
		set left  [lindex $e 2]
		set label [expr $SIMUL - $left + 1 + $order*$SIMUL]

		$ns_ at [lindex $e 0] "$self assert $label"

		incr left -1
		if {$left==0} {
			set timings [lreplace $timings $i $i]
			if {$timings == ""} break
		} else {
			set e [lreplace $e 2 2 $left]
			set timings [lreplace $timings $i $i $e]
		}
	}
	$ns_ run
	exit 0
}


TestSuite instproc assert { n } {
	$self instvar N_ quiet_
	if { $quiet_ != "QUIET" } {
		puts $n
	}
	if [expr $n != $N_ + 1 ] {
		exit 1
	}
	set N_ $n
}

TestSuite proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <scheduler> \[quiet\]"
	exit 1
}



global argc argv
set quiet ""
if { $argc == 2 } {
	set quiet [lindex $argv 1]
	if { $quiet != "QUIET" && $quiet != "quiet" } {
		TestSuite usage
	}
	set quiet "QUIET"
	
}

if { $argc > 0 && $argc < 3 } {
	set scheduler [lindex $argv 0]
} else {
	TestSuite usage
}

set test [new TestSuite $quiet]
$test run $scheduler

