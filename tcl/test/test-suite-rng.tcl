

# This test-suite uses the random number generator test (class rngtest)
# See rng.cc for details.

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation

# FOR UPDATING GLOBAL DEFAULTS:

Class TestSuite

Class Test/rngtest -superclass TestSuite

Test/rngtest instproc init {} {
    set rng [new RNG]
    $rng test
}

proc usage {} {
    global argv
    puts stderr "usage: ns $argv0 <tests> "
    puts "Valid tests: rngtest"
    exit 1
}
    

proc runtest {arg} {
    global quiet
    set quiet 0
    
    set b [llength $arg]
    if {$b == 1} {
	set test $arg
    } elseif {$b == 2} {
	set test [lindex $arg 0]
	if {[lindex $arg 1] == "QUIET"} {
	    set quiet 1
	}
    } else {
	usage
    }
    set t [new Test/$test]
}

global argv arg0
runtest $argv
