#
# Main test file for red-pd simulations
#
Class TestSuite
source monitoring.tcl
source helper.tcl
source sources.tcl
source plot.tcl
source traffic.tcl
source topology.tcl

#global options
set unresponsive_test_ 1
set target_rtt_ 0.040
set seed_ 0
set simtime_ 500
set verbosity_ 0
set plotgraphs_ 0
set plotq_ 0
set dump_interval_ 10
set testIdent_ 0
set listMode_ "multi"

#topology or traffic specific option
set topo_para1_ -1
set traf_para1_ -1
set traf_para2_ -1

TestSuite instproc init {} {
    $self instvar ns_  test_ net_ traffic_ enable_ 
    $self instvar topo_ node_
    $self instvar scheduler_ random_

    global testIdent_
    if {$traffic_ == "TestIdent"} {
	puts stderr "Running in Ident Only Mode"
	set testIdent_ 1
    }

    set ns_ [new Simulator]
    
    global seed_
	set random_ [new RNG]
    $random_ seed $seed_

    #$ns_ use-scheduler Heap
    set scheduler_ [$ns_ set scheduler_]
    
#    puts stderr "Instantiating Topology ...$net_"
    set topo_ [new Topology/$net_ $ns_]
#    puts stderr "Done ..."
    foreach i [$topo_ array names node_] {
	set node_($i) [$topo_ node? $i]
    }
    
}

TestSuite instproc  rnd {n} {
	$self instvar random_
	return [$random_ uniform 0 $n]
}


TestSuite instproc config { name } {
    $self instvar linkflowfile_ linkgraphfile_
    $self instvar flowfile_ graphfile_
    $self instvar redqfile_
    $self instvar label_ post_
    
    set label_ $name
    
    set linkflowfile_ $name.flows
    set linkgraphfile_ $name.xgr
    
    set flowfile_ $name.f.tr
    set graphfile_ $name.f.xgr
    
    set redqfile_ $name.q
    
    set post_ [new PostProcess $label_ $linkflowfile_ $linkgraphfile_ \
		   $flowfile_ $graphfile_ $redqfile_]
    
    $post_ set format_ "xgraph"
}

#
# prints "time: $time class: $class bytes: $bytes" for the link.
#
TestSuite instproc linkDumpFlows { linkmon interval stoptime } {
    $self instvar ns_ linkflowfile_
    set f [open $linkflowfile_ w]
#    puts "linkDumpFlows: opening file $linkflowfile_, fdesc: $f"
    TestSuite instproc dump1 { file linkmon interval } {
	$self instvar ns_ linkmon_
	$ns_ at [expr [$ns_ now] + $interval] \
	    "$self dump1 $file $linkmon $interval"
	foreach flow [$linkmon flows] {
	    set bytes [$flow set bdepartures_]
	    if {$bytes > 0} {
		puts $file \
		    "time: [$ns_ now] class: [$flow set flowid_] bytes: $bytes $interval"     
	    }
	}
	# now put in the total
	set bytes [$linkmon set bdepartures_]
	puts $file  "time: [$ns_ now] class: 0 bytes: $bytes $interval"   
    }
    
    $ns_ at $interval "$self dump1 $f $linkmon $interval"
    $ns_ at $stoptime "flush $f"
}

#
# called on exit
#
TestSuite instproc finish {} {
    $self instvar post_ scheduler_
    $self instvar linkflowfile_
    $self instvar tcpRateFileDesc_ tcpRateFile_ topo_
    $topo_ instvar bandwidth_
	
    $scheduler_ halt
    close $tcpRateFileDesc_       
	
    global plotgraphs_ plotq_
    if {$plotgraphs_ != 0} { 
	$post_ plot_bytes $bandwidth_  ;#plots the bandwidth consumed by various flows
	$post_ plot_tcpRate $tcpRateFile_  ;#plots TCP goodput
	if {$plotq_ == 1} { 
	    $post_ plot_queue             ;#plots red's average and instantaneous queue size
	}
    }
}


#
# for tests with a single congested link
#
Class Test/one -superclass TestSuite
Test/one instproc init { name topo traffic enable } {
    $self instvar net_ test_ traffic_ enable_
    set test_ $name
    set net_ $topo   
    set traffic_ $traffic
    set enable_ $enable
    $self next
    $self config $name.$topo.$traffic.$enable
}

Test/one instproc run {} {
    $self instvar ns_ net_ topo_ enable_ stoptime_ test_ traffic_ label_
    $topo_ instvar node_ rtt_ redpdq_ redpdflowmon_ redpdlink_ 
    $self instvar tcpRateFileDesc_ tcpRateFile_
    
    #set stoptime_ 100.0
    global simtime_
    set stoptime_ $simtime_

    set redpdsim [new REDPDSim $ns_ $redpdq_ $redpdflowmon_ $redpdlink_ 0 $enable_]
    set fmon [$redpdsim monitor-link]
    
#    $self instvar flowfile_
#    set flowf [open $flowfile_ w]
#    $fmon attach $flowf
    
    global dump_interval_
    $self linkDumpFlows $fmon $dump_interval_ $stoptime_

    global plotq_ 
    if {$plotq_ == 1} {
	$self instvar redqfile_
	set redqf [open $redqfile_ w]
	$redpdq_ trace curq_
	$redpdq_ trace ave_
	$redpdq_ attach $redqf
    }

    set tcpRateFile_ $label_.tcp
    set tcpRateFileDesc_ [open $tcpRateFile_ w]
    puts $tcpRateFileDesc_ "TitleText: $test_"
    puts $tcpRateFileDesc_ "Device: Postscript"
    
    
    $self traffic$traffic_

    $ns_ at $stoptime_ "$self finish"
    
    ns-random 0
    $ns_ run
}

#
#multiple congested link tests
#
Class Test/multi -superclass TestSuite
Test/multi instproc init { name topo traffic enable } {
    $self instvar net_ test_ traffic_ enable_
    set test_ $name
    set net_ $topo   
    set traffic_ $traffic
    set enable_ $enable
    $self next
    $self config $name.$topo.$traffic.$enable
}

Test/multi instproc run {} {
    $self instvar ns_ net_ topo_ enable_ stoptime_ test_ label_
    $topo_ instvar node_ rtt_ redpdq_ redpdflowmon_ redpdlink_ noLinks_
    $self instvar tcpRateFileDesc_ tcpRateFile_
    
    global simtime_
	set stoptime_ $simtime_
    
    for {set i 0} {$i < $noLinks_} {incr i} {
	set redpdsim($i) [new REDPDSim $ns_ $redpdq_($i) $redpdflowmon_($i) $redpdlink_($i) $i $enable_]
    }
    
    #monitor the last congested link
    set link [expr {$noLinks_ - 1}]
    
	global dump_interval_
    set fmon [$redpdsim($link) monitor-link]
	$self linkDumpFlows $fmon $dump_interval_ $stoptime_

#    $self instvar flowfile_
#    set flowf [open $flowfile_ w]
#    $fmon attach $flowf

    
    global plotq_ 
	if {$plotq_ == 1} {
		$self instvar redqfile_
		set redqf [open $redqfile_ w]
		$redpdq_($link) trace curq_
		$redpdq_($link) trace ave_
		$redpdq_($link) attach $redqf
	}

    set tcpRateFile_ $label_.tcp
    set tcpRateFileDesc_ [open $tcpRateFile_ w]
    puts $tcpRateFileDesc_ "TitleText: $test_"
    puts $tcpRateFileDesc_ "Device: Postscript"
    
    $self trafficMulti $noLinks_
    
    $ns_ at $stoptime_ "$self finish"
    ns-random 0
    $ns_ run
}


#-----------------------------------------

TestSuite proc usage {} {
        global argv0
    puts stderr "\nUsage: ns $argv0 <test> <topology> <traffic> \[options]"
    puts stderr "\nValid tests: [$self get-subclasses TestSuite Test/]"
    puts stderr "Valid Topologies: [$self get-subclasses Topology Topology/]"
    puts stderr "Valid Traffic: [$self get-subclasses TestSuite Traffic/]"
	puts stderr "\nOptions:"
	puts stderr "enable <0|1> - whether RED-PD is enabled (default 1)"
	puts stderr "testUnresp <0|1> - whether unresponsive testing is ON (default 1)"
	puts stderr "rtt <value> - target rtt in seconds (default 0.040)"
	puts stderr "seed <value> - seed for the random number generator (default 0)"
	puts stderr "time <value> - time in seconds for which to run the simulation (default 500)"
    puts stderr "verbose <value> - verbosity level \[-1,5] (default 0)"
	puts stderr "plotgraphs <0|1> - whether to plot instantaneous throughput graph"
	puts stderr "plotq <0|1> - whether to trace RED instantaneous and avg queue"
	puts stderr "period <value> - period in seconds for dumping flow information"
#	puts stderr "listmode <single|multi> - what list mode to run in (currently supported only for testIdent)"

	puts stderr "\nTopology or Traffic specific options:"
	puts stderr "p <value> - drop rate (fraction) for fixed drop rate simulations (netTestFRp & netPktsVsBytes)"
	puts stderr "gamma <value> - sending rate multiplier for the flow (TestFRp)"
	puts stderr "flows <value> - number of flows (TFRC & Response)"
	puts stderr "links <value> - number of links (netMulti)"
	puts stderr "ftype <cbr|tcp> - type of flow (Multi & TestFRp)"
	puts stderr "testRTT <value> - rtt of flows for testing identification (TestIdent)"
	exit  1
}

TestSuite proc isProc? {cls prc} {
        if [catch "Object info subclass $cls/$prc" r] {
                global argv0
                puts stderr "$argv0: no such $cls: $prc"
                $self usage
        }
}

TestSuite proc get-subclasses {cls pfx} {
        set ret ""
        set l [string length $pfx]

        set c $cls
        while {[llength $c] > 0} {
                set t [lindex $c 0]
                set c [lrange $c 1 end]
                if [string match ${pfx}* $t] {
                        lappend ret [string range $t $l end]
                }
                eval lappend c [$t info subclass]
        }
        set ret
}

TestSuite proc runTest {} {
    global argc argv
    set enable 1
	if {$argc < 3} {
		$self usage
	}

	set test [lindex $argv 0]
	$self isProc? Test $test
	
	set topo [lindex $argv 1]
	$self isProc? Topology $topo
	
	set traffic [lindex $argv 2]
	$self isProc? Traffic $traffic
	
	for {set i 3} {$i < $argc} {incr i} {
		set option [lindex $argv $i]
		incr i
		set value [lindex $argv $i]
		switch -exact $option {
			enable {
				set enable $value
			}
			testUnresp {
				global unresponsive_test_
				set unresponsive_test_ $value
			}
			rtt {
				global target_rtt_
				set target_rtt_ $value
			}
			seed {
				global seed_
				set seed_ $value
			}
			time {
				global simtime_
				set simtime_ $value
			}
			verbose {
				global verbosity_
				set verbosity_ $value
			}
			plotgraphs {
				global plotgraphs_
				set plotgraphs_ $value
			}
			plotq {
				global plotq_
				set plotq_ $value
			}
			period {
				global dump_interval_
				set dump_interval_ $value
			}
			listmode {
				global listMode_ 
				set listMode_ $value
			}
			p {
				global topo_para1_
				set topo_para1_ $value
			}
			gamma {
				global traf_para2_
				set traf_para2_ $value
			}
			flows {
				global traf_para1_
				set traf_para1_ $value
			}
			links {
				global topo_para1_
				set topo_para1_ $value
			}
			ftype {
				global traf_para1_
				set traf_para1_ $value
			}
			testRTT {
				global topo_para1_
				set topo_para1_ $value
			}
			default {
				puts "Unknown Option $option"
				$self usage
			}
		}
	}
		
    set t [new Test/$test $test $topo $traffic $enable]
    $t run
}

TestSuite runTest
