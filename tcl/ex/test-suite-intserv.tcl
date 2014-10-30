#
# Copyright (c) Xerox Corporation 1997. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
# 
# Linking this file statically or dynamically with other modules is making
# a combined work based on this file.  Thus, the terms and conditions of
# the GNU General Public License cover the whole combination.
# 
# In addition, as a special exception, the copyright holders of this file
# give you permission to combine this file with free software programs or
# libraries that are released under the GNU LGPL and with code included in
# the standard release of ns-2 under the Apache 2.0 license or under
# otherwise-compatible licenses with advertising requirements (or modified
# versions of such code, with unchanged license).  You may copy and
# distribute such a system following the terms of the GNU GPL for this
# file and the licenses of the other code concerned, provided that you
# include the source code of that other code when and as the GNU GPL
# requires distribution of source code.
# 
# Note that people who make modified versions of this file are not
# obligated to grant this special exception for their modified versions;
# it is their choice whether to do so.  The GNU General Public License
# gives permission to release a modified version without this exception;
# this exception also makes it possible to release a modified version
# which carries forward this exception.
#



#This script demonstrates controlled load support in ns.
#There are currently four admission control algorithms that can be run
#using this script :
#1.Measured Sum (MS)
#2.Hoeffding Bounds (HB)
#3.Acceptance Region-Tangent at Origin (ACTO)
#4.Acceptance Region-Tangent at Peak. (ACTP)

#In addition there are 3 kinds of estimators supported:
#1.TimeWindow (to be used with MS)
#2.ExpAvg (to be used with HB)
#3.PointSample (to be used with ACTO, ACTP).

#Each of the above algorithms have their specific set of a parameters.
#1.MS-TimeWindow (utilization_,S_,T_)
#2.HB-ExpAvg (w_,S_,epsilon_)
#3.AC-PointSample(S_,s_).

#While running this script, one needs to specify the admission control
#algorithm, the estimator and the values for their parameters.
#for e.g:
#$ns test-suite-intserv.tcl ADC=HB EST=ExpAvg w_=1/8.0 epsilon_=0.7 S_=5e3
#or
#$ns test-suite-intserv.tcl ADC=ACTO EST=PointSample s_=2e-6 S_=2.5e4
#or
#$ns test-suite-intserv.tcl ADC=ACTP EST=PointSample s_=2e-6 S_=2.5e4
#or
#$ns test-suite-intserv.tcl ADC=MS EST=TimeWindow S_=5e3 T_=3 utilization_=0.95 trace_flow=1

# For a more complete set of parameters, pl. refer to the Sigmetrics '98
# publication "An Empirical Comparison of the Performance Frontier of
# Four Measurement-based Admission Control Algoritms" 
# by Sugih Jamin and Scott Shenker.

#In the absence of any command line parameters, the MS admission
#control with Time Window estimator with defaults (utilization_=0.95,
#S_=5e3*pkt-transmission-time, T_=3*S_) is used. 
  
#This script creates a simple 2 node topology connected by a
#duplex-intserv-link of link-bandwidth 10Mbps and propagation delay
#1ms. The simulation runs for 3000 sec. The performance for each of the
#admission control algorithms above is calculated by the measuring the
#actual link utilization and the drop rate.These numbers are measured
#starting after an intial warmup period of 1500 sec and printed
#at the end of the simulation. In addition, xgraph plots a snap shot
#of actual and estimated bandwidth utilized in the period [2000,2100]
#seconds at the end of the simulation.Also if you set the trace_flow flag
#to 1(as in e.g. 5 above), the output would indicate times at which
#flows come in and leave. 

#PLEASE NOTE THAT BECAUSE OF THE ABOVE SIMULATION RUN TIME PARAMETERS
#AND THE LARGE NUMBER OF FLOWS CONCURRENTLY ACTIVE, THIS SCRIPT TAKES
#ABOUT 40 MIN and ABOUT 8MB OF MEMORY TO RUN ON A SUN ULTRA.

#The source model used is an exponential on/off source with peak rate
#of 64k.
#The flow arrival distibution is exponential with an average of 400ms
#The flow lifetime also has an exponential distribution with an
#average of 300sec.

#defaults
Queue/SimpleIntServ set qlimit1_ 160
Queue/SimpleIntServ set qlimit0_ 0
set S_ [expr 5e3]
set T_ 3
set utilization_ 0.95
set s_ [expr 2e-3]
set w_ [expr pow(2,-4)]
set epsilon_ 0.7
set simtime 3000
set meastime 1500
set srcno 1
set ADC MS
set EST TimeWindow
set trace_flow 0

#Set pkt size and link bw
set psize 125
set bandwidth 10e6
set ptime [expr $psize*8.0/$bandwidth]


#set avg flowinterarrival time and holdtimes
set fint 0.4
set hold 300
set fvar [new RandomVariable/Exponential]
$fvar set avg_ $fint
set hvar [new RandomVariable/Exponential]
$hvar set avg_ $hold

#Use a diffent random variable for flow distribution
set myrng [new RNG]
#picked up an arbitrary predefined seed from rng.cc
$myrng seed 1973272912

$fvar use-rng $myrng
$hvar use-rng $myrng

set i 0
while { $i < $argc } {
	set L [split [lindex $argv $i] =]
	if { [llength $L] == 2 } {
		set var [lindex $L 0]
		set val [lindex $L 1]
		set $var $val
	}
	incr i
}



set adc $ADC
set est $EST
puts "Using ADC: $adc EST: $est simtime: $simtime s"

ADC/MS set utilization_ [expr $utilization_]
ADC/ACTO set s_ [expr $s_]
ADC/ACTP set s_ [expr $s_]
ADC/HB set epsilon_ [expr $epsilon_]
Est/TimeWindow set T_ $T_
Est/ExpAvg set w_ [expr $w_]
Est set period_ [expr $S_*$ptime]

#Helper function to schedule stop time for the new flow
Agent/SA instproc sched-stop { decision } {
	global hold simtime ns trace_flow
	$self instvar node_ lifetime_
	
	if { $decision == 1 } {
		set leavetime [expr [$ns now] + $lifetime_]
		$ns at [expr [$ns now] + $lifetime_] "$self stop;\
                if { $trace_flow } { \
		    puts \"Flow [$self set fid_] left @ $leavetime\" \
	    }; \
            $ns detach-agent $node_ $self; \
            delete [$self set trafgen_]; delete $self" 
	} else {
		set leavetime [$ns now]
		$ns at-now "if { $trace_flow } { \
		    puts \"Flow [$self set fid_] left @ $leavetime\"\
	    }; \
            $ns detach-agent $node_ $self; \
            delete [$self set trafgen_]; delete $self" 
	}
}

set ns [new Simulator]
$ns expand-port-field-bits 16

#Create a simple 2 node toplogy
set n0 [$ns node]
set n1 [$ns node]
$ns duplex-intserv-link $n0 $n1 $bandwidth 1ms SimpleIntServ SALink $adc $est CL

#Set up queue-monitor to measure link utilization and drop rates
set f [open out.tr w]
set qmon [$ns monitor-queue $n0 $n1 $f]
set l01 [$ns link $n0 $n1]
$l01 set qBytesEstimate_ 0
$l01 set qPktsEstimate_ 0
$l01 set sampleInterval_ 0
$ns at $meastime "$l01 trace-util [expr $ptime*$S_] $f"

#create 1 receiver agent
set r [new Agent/SAack]
$ns attach-agent $n1 $r


proc show-simtime {} {
	global ns
	puts [$ns now]
	$ns at [expr [$ns now]+50.0] "show-simtime"
}

proc create-source {node rcvr starttime  i} {
	global ns hold hvar
	set a [new Agent/SA]
	$ns attach-agent $node $a
	
	$a set fid_ $i
	
	$ns connect $a $rcvr
	
	
	set exp1 [new Application/Traffic/Exponential]
	$exp1 set packetSize_ 125
	$exp1 set burst_time_ [expr 20.0/64]
	$exp1 set idle_time_ 325ms
	$exp1 set rate_ 64k
	
	#set up (r,b)
	$a set rate_ 64k
	$a set bucket_ 1
	$a attach-traffic $exp1
	$a set lifetime_ [$hvar value]
	$ns at $starttime "$a start"
	$a instvar trafgen_
	set trafgen_ $exp1
	
}

proc finish { file } {
	global f r psize bandwidth simtime meastime qmon
	
	$qmon instvar pdrops_ pdepartures_ bdepartures_
	set utlzn [expr $bdepartures_*8.0/($bandwidth*($simtime-$meastime))]
	set d [expr $pdrops_*1.0/($pdrops_+ $pdepartures_)]
	puts "Drops : $d Utilization : $utlzn"
	if [ info exists f ] {
		close $f
	}
	set output [ open temp.rands w ]
	puts $output "TitleText: $file"
        puts $f "Device: Postscript"

	exec rm -f temp.p1 temp.p2
        exec awk {
		{
			print $1,$2
		}
	} out.tr > temp.p1
        exec awk {
		{
			print $1,$3
		}
	} out.tr > temp.p2
         

	puts $output [format "\n\"Estimate"]
	exec cat temp.p1 >@ $output
	puts $output [format "\n\"Actual Utilzn"]
        exec cat temp.p2 >@ $output
	flush $f
        close $f

	exec rm -f temp.p1 temp.p2
#        exec xgraph -bb -tk -m -x time -y bandwidth -ly [expr $bandwidth/2.0],$bandwidth -lx [expr 2/3.0*$simtime],[expr 2/3.0*$simtime+100.0] temp.rands &
        exec xgraph -bb -tk -m -x time -y bandwidth -lx [expr 2/3.0*$simtime],[expr 2/3.0*$simtime+100.0] temp.rands &
	
	exec rm -f out.tr
	exit 0
}



#proc to create a source and schedule another one after a time from
#expo dist  
proc sched-source { node receiver } {
	global srcno ns fint hold fvar trace_flow
	
	create-source $node $receiver [$ns now]  $srcno
	if { $trace_flow } {
		puts "Flow $srcno started @ [$ns now]"
	}
	incr srcno
	
	#generate another startime
	set starttime [expr [$ns now]+[$fvar value]]
	$ns at $starttime "sched-source $node $receiver"
}

#Sched the first flow
set starttime  [expr $srcno + double([ns-random] % 10000000) / 1e7]
$ns at $starttime "sched-source $n0 $r"    


$ns at [expr $simtime] "finish $f"

$ns at $meastime "$qmon set bdepartures_ 0;$qmon set bdrops_ 0"

$ns at 0.0 "show-simtime"
puts "Running simulation...."
$ns run

