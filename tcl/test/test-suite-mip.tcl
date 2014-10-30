#
# Copyright (c) 1998 University of Southern California.
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
# This test suite is for validating the Mobile IP simulation support
# in ns.
#
# To run all tests:  test-all-mip
#
# To run individual tests:
# ns test-suite-mip.tcl mip-adv-one
# ns test-suite-mip.tcl mip-adv-zero
# ns test-suite-mip.tcl mip-adv-multi
# ...
#
# To view a list of available tests to run with this script:
# ns test-suite-mip.tcl
#

#remove-all-packet-headers       ; # removes all except common
#add-packet-header Flags IP TCP MIP IPinIP ARP LL Mac ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.
Agent/TCP set SetCWRonRetransmit_ true
# Changing the default value.

Class TestSuite

# Mobile IP test - simple case
Class Test/mip-adv-one -superclass TestSuite

# Mobile IP test - MH hears no advertisements from either HA or FA and 
# starts sending solicitations for a period of time
Class Test/mip-adv-zero -superclass TestSuite

# Mobile IP test - MH hears multiple advertisements for a period of time
Class Test/mip-adv-multi -superclass TestSuite

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests>"
	puts stderr "Valid tests are: mip-adv-one mip-adv-zero mip-adv-multi"
	exit 1
}

TestSuite instproc finish {} {
    $self instvar ns_
    global quiet
    
    $ns_ flush-trace
    if { !$quiet } {
	puts "running nam..."
	exec ../../../nam-1/nam temp.rands.nam &
    }
    exit 0
}

TestSuite instproc init {} {
    $self instvar ns_ testName_ lan_ node_ source_ tcp_
    set ns_ [new Simulator]
    $ns_ instvar link_
    
    $ns_ trace-all [open temp.rands w]
    $ns_ namtrace-all [open temp.rands.nam w]
    
    set node_(s) [$ns_ node]
    set node_(g) [$ns_ node]
    Simulator set node_factory_ Node/MIPBS
    set node_(ha) [$ns_ node]
    $node_(ha) shape "hexagon"
    set node_(fa) [$ns_ node]
    $node_(fa) shape "hexagon"
    Simulator set node_factory_ Node/MIPMH
    set node_(mh) [$ns_ node]
    $node_(mh) shape "circle"
    
    $ns_ duplex-link $node_(s) $node_(g) 2Mb 5ms DropTail 
    $ns_ duplex-link-op $node_(s) $node_(g) orient right
    
    $ns_ duplex-link $node_(g) $node_(ha) 2Mb 5ms DropTail
    $ns_ duplex-link-op $node_(g) $node_(ha) orient up
    $ns_ duplex-link $node_(g) $node_(fa) 2Mb 5ms DropTail
    $ns_ duplex-link-op $node_(g) $node_(fa) orient right-up
    $ns_ duplex-link $node_(ha) $node_(mh) 2Mb 5ms DropTail 
    $ns_ duplex-link-op $node_(ha) $node_(mh) orient right
    $ns_ duplex-link $node_(fa) $node_(mh) 4Mb 5ms DropTail 
    $ns_ duplex-link-op $node_(fa) $node_(mh) orient left

    set mhid [$node_(mh) id]
    set haid [$node_(ha) id]
    set faid [$node_(fa) id]

    [$node_(ha) set regagent_] add-ads-bcast-link $link_($haid:$mhid)
    [$node_(fa) set regagent_] add-ads-bcast-link $link_($faid:$mhid)

    [$node_(mh) set regagent_] set home_agent_ $haid
    [$node_(mh) set regagent_] add-sol-bcast-link $link_($mhid:$haid)
    [$node_(mh) set regagent_] add-sol-bcast-link $link_($mhid:$faid)
    
    $link_($mhid:$faid) dynamic
    $link_($faid:$mhid) dynamic
    $link_($mhid:$faid) down
    $link_($faid:$mhid) down
    $link_($mhid:$haid) dynamic
    $link_($haid:$mhid) dynamic
        
    $ns_ at 0.0 "$node_(ha) label HomeAgent"
    $ns_ at 0.0 "$node_(fa) label ForeignAgent"
    $ns_ at 0.0 "$node_(mh) label MobileHost"

    set tcp_ [$ns_ create-connection TCP/Reno \
	    $node_(s) TCPSink $node_(mh) 0]
    set source_ [$tcp_ attach-source FTP]

}

TestSuite instproc run {} {
    $self instvar ns_ source_
    $ns_ at 0.0 "$source_ start"
    $ns_ at 20.0 "$self finish"
    $ns_ run
}

Test/mip-adv-one instproc init {} {
    $self instvar ns_ testName_ lan_ node_ source_ tcp_
    set testName_ mip-adv-one
    $self next
    $ns_ instvar link_

    $ns_ at 0.0 "$node_(ha) color gold"
    $ns_ at 0.0 "$node_(mh) color gold"	

    $ns_ rtmodel-at 0.1 down $node_(mh) $node_(fa)
    
    set swtm [expr 3.0+(20.0-3.0)/4.0]
	$ns_ rtmodel-at $swtm down $node_(mh) $node_(ha)
	$ns_ rtmodel-at $swtm up $node_(mh) $node_(fa)
	$ns_ at $swtm "$node_(ha) color black"
	$ns_ at $swtm "$node_(fa) color gold"

	set swtm [expr 3.0+(20.0-3.0)/2.0]
	$ns_ rtmodel-at $swtm down $node_(mh) $node_(fa)
	$ns_ rtmodel-at $swtm up $node_(mh) $node_(ha)
	$ns_ at $swtm "$node_(ha) color gold"
	$ns_ at $swtm "$node_(fa) color black"


	set swtm [expr 3.0+(20.0-3.0)*3.0/4.0]
	$ns_ rtmodel-at $swtm down $node_(mh) $node_(ha)
	$ns_ rtmodel-at $swtm up $node_(mh) $node_(fa)
	$ns_ at $swtm "$node_(ha) color black"
	$ns_ at $swtm "$node_(fa) color gold"
}

Test/mip-adv-zero instproc init {} {
        $self instvar ns_ testName_ lan_ node_ source_ tcp_
        set testName_ mip-adv-zero
        $self next

	$ns_ instvar link_

	$ns_ at 0.0 "$node_(ha) color gold"
	$ns_ at 0.0 "$node_(mh) color gold"

	$ns_ rtmodel-at 0.1 down $node_(mh) $node_(fa)

        #MH cannot hear advertisements from either HA or FA
	set swtm [expr 3.0+(20.0-3.0)/4.0] 
        $ns_ rtmodel-at $swtm down $node_(mh) $node_(ha)
	$ns_ at $swtm "$node_(ha) color black"
	$ns_ at $swtm "$node_(fa) color black"

        set swtm [expr 3.0+(20.0-3.0)/4.0+2.0]   
	$ns_ rtmodel-at $swtm up $node_(mh) $node_(fa)
	$ns_ at $swtm "$node_(ha) color black"
	$ns_ at $swtm "$node_(fa) color gold"

	set swtm [expr 3.0+(20.0-3.0)/2.0]
	$ns_ rtmodel-at $swtm down $node_(mh) $node_(fa)
	$ns_ rtmodel-at $swtm up $node_(mh) $node_(ha)
	$ns_ at $swtm "$node_(ha) color gold"
	$ns_ at $swtm "$node_(fa) color black"

	set swtm [expr 3.0+(20.0-3.0)*3.0/4.0]
	$ns_ rtmodel-at $swtm down $node_(mh) $node_(ha)
	$ns_ rtmodel-at $swtm up $node_(mh) $node_(fa)
	$ns_ at $swtm "$node_(ha) color black"
	$ns_ at $swtm "$node_(fa) color gold"

}

Test/mip-adv-multi instproc init {} {
        $self instvar ns_ testName_ lan_ node_ source_ tcp_
        set testName_ mip-adv-multi

        $self next
        $ns_ instvar link_

	$ns_ at 0.0 "$node_(ha) color gold"
	$ns_ at 0.0 "$node_(mh) color gold"

	$ns_ rtmodel-at 0.1 down $node_(mh) $node_(fa)

        #MH can hear advertisements from both HA and FA
        set swtm [expr 3.0+(20.0-3.0)/4.0] 
	$ns_ rtmodel-at $swtm up $node_(mh) $node_(fa)
	$ns_ at $swtm "$node_(fa) color gold"

        #MH can hear advertisements from only FA now
        set swtm [expr 3.0+(20.0-3.0)/4.0+3.0]
	$ns_ rtmodel-at $swtm down $node_(mh) $node_(ha)
	$ns_ at $swtm "$node_(ha) color black"
	$ns_ at $swtm "$node_(fa) color gold"

        #MH can hear advertisements from HA only now
	set swtm [expr 3.0+(20.0-3.0)/2.0+1.0]
	$ns_ rtmodel-at $swtm down $node_(mh) $node_(fa)
	$ns_ rtmodel-at $swtm up $node_(mh) $node_(ha)
	$ns_ at $swtm "$node_(ha) color gold"
	$ns_ at $swtm "$node_(fa) color black"

        #MH can hear advertisements from only FA now
	set swtm [expr 3.0+(20.0-3.0)*3.0/4.0]
	$ns_ rtmodel-at $swtm down $node_(mh) $node_(ha)
	$ns_ rtmodel-at $swtm up $node_(mh) $node_(fa)
	$ns_ at $swtm "$node_(ha) color black"
	$ns_ at $swtm "$node_(fa) color gold"
}


proc runtest {arg} {
    global quiet
    set quiet 0
    set b [llength $arg]
    if {$b == 1} {
	set test $arg
    } elseif {$b == 2} {
	set test [lindex $arg 0]
	set q [lindex $arg 1]
	if { $q == "QUIET" } {
	    set quiet 1
	} else usage
    } else usage
    
    switch $test {
	mip-adv-one -
	mip-adv-zero -
	mip-adv-multi {
	    set t [new Test/$test]
	}
	default {
	    puts stderr "Unknown test $test"
	    exit 1
	}
    }
    $t run
}

global argv arg0
runtest $argv
