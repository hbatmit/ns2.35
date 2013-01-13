#
# Copyright (c) 1997, 1998 The Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# this is still somewhat experimental,
# and should be considered of 'beta-test' quality  (kfall@ee.lbl.gov)
#
# This file tests "fulltcp", a version of tcp reno implemented in the 
# simulator based on the BSD Net/3 code. 
#
# This test suite is based on test-suite-fulltcp.tcl from ns-1.

set dir [pwd]
catch "cd tcl/test"
source misc.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for TCP

Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1

# To turn on debugging messages:
# Agent/TCP/FullTcp set debug_ true;

source topologies.tcl
catch "cd $dir"

Trace set show_tcphdr_ 1 ; # needed to plot ack numbers for tracing 

TestSuite instproc printtimers { tcp time} {
        global quiet
        if {$quiet == "false"} {
                puts "time: $time sRTT(in ticks): [$tcp set srtt_]/8 RTTvar(in ticks): [$tcp set rttvar_]/4 backoff: [$tcp set backoff_]"
        }
}

TestSuite instproc printtimersAll { tcp time interval } {
        $self instvar dump_inst_ ns_
        if ![info exists dump_inst_($tcp)] {
                set dump_inst_($tcp) 1
                $ns_ at $time "$self printtimersAll $tcp $time $interval"
                return
        }
        set newTime [expr [$ns_ now] + $interval]
        $ns_ at $time "$self printtimers $tcp $time"
        $ns_ at $newTime "$self printtimersAll $tcp $newTime $interval"
}

TestSuite instproc finish testname {
	global env quiet
	$self instvar ns_

	$ns_ halt

	set outtype text
	set tfile "out.tr"

	if { $quiet != "true" } {
                if { [info exists env(NSOUT)] } {
                        set outtype $env(NSOUT)
                } elseif { [info exists env(DISPLAY)] } {
                        set outtype xgraph
                }

		if { $outtype == "text" } {
			puts "output files are $fname.{p,packs,acks,d,ctrl,es,ecn,cact}"
			puts "  and $fname.r.{p,packs,acks,d,ctrl,es,ecn,cact}"
		} else {
			global TCLSH
			exec ../../bin/tcpf2xgr $TCLSH $tfile $outtype $testname &
		}
	}
	exec cp $tfile temp.rands; # verification scripts wants stuff in 'temp.rands'
}

TestSuite instproc bsdcompat tcp {
	$tcp set segsperack_ 2
	## 
	$tcp set dupseg_fix_ false
	$tcp set dupack_reset_ true
	$tcp set bugFix_ false
	$tcp set data_on_syn_ false
	$tcp set tcpTick_ 0.5
}

# Definition of test-suite tests
Class Test/full -superclass TestSuite
Test/full instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ full
	$self next
}
Test/full instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)

	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen ; # will figure out who its peer is
	$src set window_ 100
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]  
	$ns_ run
}

Class Test/close -superclass TestSuite
Test/close instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ close
	$self next
}
Test/close instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	$src set window_ 100
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.0 "$ftp1 produce 50"
	$ns_ at 5.5 "$src close"

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/twoway -superclass TestSuite
Test/twoway instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ twoway
	$self next
}
Test/twoway instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	
	set startt 3.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	$sink set iss_ 2144
	$src set window_ 100
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"
	set ftp2 [$sink attach-app FTP]
	$ns_ at $startt "$ftp2 start"

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/twoway1 -superclass TestSuite
Test/twoway1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ twoway1
	$self next
}
Test/twoway1 instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	
	set startt 1.9

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	$sink set iss_ 2144
	$src set window_ 100
	$sink set window_ 100
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"
	set ftp2 [$sink attach-app FTP]
	$ns_ at $startt "$ftp2 start"

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#Class Test/twoway_bsdcompat -superclass TestSuite
#Test/twoway_bsdcompat instproc init topo {
#	$self instvar net_ defNet_ test_
#	set net_ $topo
#	set defNet_ net0
#	set test_ twoway_bsdcompat
#	$self next
#}
#Test/twoway_bsdcompat instproc run {} {
#	$self instvar ns_ node_ testName_
#
#	set stopt 6.0	
#	set startt 1.9
#
#	# set up connection (do not use "create-connection" method because
#	# we need a handle on the sink object)
#	set src [new Agent/TCP/FullTcp]
#	set sink [new Agent/TCP/FullTcp]
#	$ns_ attach-agent $node_(s1) $src
#	$ns_ attach-agent $node_(k1) $sink
#	$src set fid_ 0
#	$sink set fid_ 0
#	$ns_ connect $src $sink
#
#	# set up TCP-level connections
#	$sink listen
#	$src set window_ 100
#	$sink set window_ 100
#	$self bsdcompat $src
#	$self bsdcompat $sink
#	set ftp1 [$src attach-app FTP]
#	$ns_ at 0.0 "$ftp1 start"
#	set ftp2 [$sink attach-app FTP]
#	$ns_ at $startt "$ftp2 start"
#
#	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
#	$ns_ run
#}
#
#Class Test/oneway_bsdcompat -superclass TestSuite
#Test/oneway_bsdcompat instproc init topo {
#	$self instvar net_ defNet_ test_
#	set net_ $topo
#	set defNet_ net0
#	set test_ oneway_bsdcompat
#	$self next
#}
#Test/oneway_bsdcompat instproc run {} {
#	$self instvar ns_ node_ testName_
#
#	set stopt 6.0	
# 	set startt 1.9
#
#	# set up connection (do not use "create-connection" method because
#	# we need a handle on the sink object)
#	set src [new Agent/TCP/FullTcp]
#	set sink [new Agent/TCP/FullTcp]
#	$ns_ attach-agent $node_(s1) $src
#	$ns_ attach-agent $node_(k1) $sink
#	$src set fid_ 0
#	$sink set fid_ 0
#	$ns_ connect $src $sink
#
#	# set up TCP-level connections
#	$sink listen ; # will figure out who its peer is
#	$src set window_ 100
#	$sink set window_ 100
#	$self bsdcompat $src
#	$self bsdcompat $sink
#	set ftp1 [$src attach-app FTP]
#	$ns_ at 0.0 "$ftp1 start"
#
#	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
#	$ns_ run
#}
#
Class Test/twowayrandom -superclass TestSuite
Test/twowayrandom instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ twowayrandom
	$self next
}
Test/twowayrandom instproc run {} {
	$self instvar ns_ node_ testName_
	global quiet

	set stopt 6.0	
	if { $quiet == "true" } {
		set startt 1
	} else {
		set startt [expr [ns-random 0] % 6]
	}
	puts "second TCP starting at time $startt"

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	$sink set iss_ 2144
	$src set window_ 100
	$sink set window_ 100
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"
	set ftp2 [$sink attach-app FTP]
	$ns_ at $startt "$ftp2 start"

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/delack -superclass TestSuite
Test/delack instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ delack
	$self next
}
Test/delack instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	$src set window_ 100
	$sink set segsperack_ 2
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"

	#forward
	$self instvar direction_
	set direction_ forward

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/iw=4 -superclass TestSuite
Test/iw=4 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ iw=4
	$self next
}
Test/iw=4 instproc run {} {
	$self instvar ns_ node_ testName_

	set stopt 6.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.0 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set windowInit_ 4
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedsyn -superclass TestSuite
Test/droppedsyn instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedsyn
	$self next
}
Test/droppedsyn instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 20.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedsyn: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 1.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.01
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedfirstseg -superclass TestSuite
Test/droppedfirstseg instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedfirstseg
	$self next
}
Test/droppedfirstseg instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 20.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedfirstseg: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 3.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"
#	$ns_ at 0.5 "$self printtimersAll $src 0.5 0.5"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.01
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedsecondseg -superclass TestSuite
Test/droppedsecondseg instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedsecondseg
	$self next
}
Test/droppedsecondseg instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 20.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedsecondseg: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 4.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"
#	$ns_ at 0.5 "$self printtimersAll $src 0.5 0.5"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.01
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/simul-open -superclass TestSuite
Test/simul-open instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ simul-open
	$self next
}
Test/simul-open instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 9.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "simul-open: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 20.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink; # this is bi-directional

	# set up TCP-level connections
	set ftp1 [$src attach-app FTP]
	set ftp2 [$sink attach-app FTP]
	$ns_ at 0.7 "$ftp1 start; $ftp2 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$sink set window_ 100
	$sink set delay_growth_ true
	$sink set tcpTick_ 0.500
	$sink set packetSize_ 1460

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/simul-close -superclass TestSuite
Test/simul-close instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ simul-close
	$self next
}
Test/simul-close instproc run {} {
        global quiet
	$self instvar ns_ node_ testName_ topo_
        if {$quiet == "false"} {
                Agent/TCP/FullTcp set debug_ true;
        }

	set stopt 9.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$sink set iss_ 10000
	$ns_ connect $src $sink; # this is bi-directional

	# set up TCP-level connections
	set ftp1 [$src attach-app FTP]
	set ftp2 [$sink attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"
	$ns_ at 0.9 "$ftp2 start"

	$ns_ at 1.6 "$src close; $sink close"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$sink set window_ 100
	$sink set delay_growth_ true
	$sink set tcpTick_ 0.500
	$sink set packetSize_ 1460

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppednearfin -superclass TestSuite
Test/droppednearfin instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppednearfin
	$self next
}
Test/droppednearfin instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppednearfin: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 7.0
	$errmodel set period_ 100.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"
	$ns_ at 1.5 "$src close"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedlastseg -superclass TestSuite
Test/droppedlastseg instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedlastseg
	$self next
}
Test/droppedlastseg instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedlastseg: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 9.0
	$errmodel set period_ 100.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"
	$ns_ at 1.5 "$src close"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedretransmit -superclass TestSuite
Test/droppedretransmit instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedretransmit
	$self next
}
Test/droppedretransmit instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedretransmit: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 62.0
	$errmodel set period_ 100.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppednearretransmit -superclass TestSuite
Test/droppednearretransmit instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppednearretransmit
	$self next
}
Test/droppednearretransmit instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppednearretransmit: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 63.0
	$errmodel set period_ 100.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedfastrexmit -superclass TestSuite
Test/droppedfastrexmit instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedfastrexmit
	$self next
}
Test/droppedfastrexmit instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedfastrexmit: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 61.0
	$errmodel set period_ 100.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/ecn1 -superclass TestSuite
Test/ecn1 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ ecn1
	$self next
}
Test/ecn1 instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "ecn1: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 10.0
	$errmodel set period_ 30.0
	$errmodel set markecn_ true; # mark ecn's, don't drop

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$src set ecn_ true
	$sink set ecn_ true

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/ecn2 -superclass TestSuite
Test/ecn2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ ecn2
	$self next
}
Test/ecn2 instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "ecn2: confused by >1 err models..abort"
		exit 1
	}

	#$errmodel set offset_ 30.0
	$errmodel set offset_ 130.0
	$errmodel set period_ 100.0
	$errmodel set markecn_ true; # mark ecn's, don't drop

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$src set ecn_ true
	$sink set ecn_ true

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/droppedfin -superclass TestSuite
Test/droppedfin instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ droppedfin
	$self next
}
Test/droppedfin instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "droppedfin: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 10.0
	$errmodel set period_ 100.0

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"
	$ns_ at 1.5 "$src close"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

Class Test/smallpkts -superclass TestSuite
Test/smallpkts instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ smallpkts
	$self next
}
Test/smallpkts instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$sink set interval_ 200ms
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	$ns_ at 0.5 "$src advance-bytes 30"
	$ns_ at 0.75 "$src advance-bytes 300"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# this test sets the receiver's notion of the mss larger than
# the sender and sets segsperack to 3.  So, only if there is
# > 3*4192 bytes accumulated would an ACK occur [i.e. never].
# So, because the
# delack timer is set for 200ms, the upshot here is that
# we see ACKs as pushed out by this timer only
#
Class Test/telnet -superclass TestSuite
Test/telnet instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ telnet(200ms-delack)
	$self next
}
Test/telnet instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$sink set interval_ 200ms
	$sink set segsize_ 4192; # or wait up to 3*4192 bytes to ACK
	$sink set segsperack_ 3
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set telnet1 [$src attach-app Telnet]
	$telnet1 set interval_ 0ms
	$ns_ at 0.5 "$telnet1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# this test is the same as the last one, but changes the
# receiver's notion of the mss, delack interval, and segs-per-ack.
# The output indicates some places where ACKs are generated due
# to the timer and other are due to meeting the segs-per-ack limit
# before the timer
#
Class Test/telnet2 -superclass TestSuite
Test/telnet2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ telnet2(3segperack-600ms-delack)
	$self next
}
Test/telnet2 instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$sink set interval_ 600ms
	$sink set segsize_ 536; # or wait up to 3*536 bytes to ACK
	$sink set segsperack_ 3
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set telnet1 [$src attach-app Telnet]
	$telnet1 set interval_ 0ms
	$ns_ at 0.5 "$telnet1 start"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set segsize_ 536
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# this test exercises the "slow_start_restart_" option
# with ssr set to false, illustrates the line-rate-bursts which occur
# after the window has grown large but the app has stopped writes
#
Class Test/SSR -superclass TestSuite
Test/SSR instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ slow-start-restart
	$self next
}
Test/SSR instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set slow_start_restart_ false
	$sink listen
	$ns_ at 0.5 "$src advance-bytes 10000"
	$ns_ at 5.0 "$src advance-bytes 10000"

	# set up special params for this test
	$src set window_ 100
	$src set slow_start_restart true
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set segsize_ 536
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# same test as SSR, but this time turn slow_start_restart on
#
Class Test/SSR2 -superclass TestSuite
Test/SSR2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ slow-start-restart-fix
	$self next
}
Test/SSR2 instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set slow_start_restart_ true
	$sink listen
	$ns_ at 0.5 "$src advance-bytes 10000"
	$ns_ at 5.0 "$src advance-bytes 10000"

	# set up special params for this test
	$src set window_ 100
	$src set slow_start_restart true
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set segsize_ 536
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# A test of the timestamp option
#
Class Test/tsopt -superclass TestSuite
Test/tsopt instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ tsopt
	$self next
}
Test/tsopt instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$sink set timestamps_ true
	$ns_ connect $src $sink

	# set up TCP-level connections
	$src set timestamps_ true
	$sink listen
	$ns_ at 0.5 "$src advance-bytes 100000"

	# set up special params for this test
	$src set window_ 100
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set segsize_ 536
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# A test where we are window limited
#
Class Test/winlimited -superclass TestSuite
Test/winlimited instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ winlimited
	$self next
}
Test/winlimited instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 10.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	$ns_ at 0.5 "$src advance-bytes 100000"

	# set up special params for this test
	$src set window_ 5
	$src set delay_growth_ true
	$src set tcpTick_ 0.500
	$src set segsize_ 536
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}


TestSuite runTest
