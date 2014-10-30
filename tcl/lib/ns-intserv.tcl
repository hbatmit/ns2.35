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
# $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-intserv.tcl,v 1.12 2005/08/26 05:05:29 tomh Exp $

#defaults
Queue/SimpleIntServ set qlimit1_ 50
Queue/SimpleIntServ set qlimit0_ 50

Agent/SA set rate_ 0
Agent/SA set bucket_ 0
Agent/SA set packetSize_ 210

ADC set backoff_ true
ADC set dobump_ true
ADC/MS set backoff_ false

ADC set src_ -1
ADC set dst_ -1
ADC/MS set utilization_ 0.95
ADC/MSPK set utilization_ 0.95
ADC/Param set utilization_ 1.0
ADC/HB set epsilon_ 0.7
ADC/ACTO set s_ 0.002
ADC/ACTO set dobump_ false
ADC/ACTP set s_ 0.002
ADC/ACTP set dobump_ false


Est/TimeWindow set T_ 3
Est/ExpAvg set w_ 0.125
Est set period_ 0.5

ADC set bandwidth_ 0

SALink set src_ -1
SALink set dst_ -1

Est set src_ -1
Est set dst_ -1


Class IntServLink -superclass  SimpleLink
IntServLink instproc init { src dst bw delay q arg {lltype "DelayLink"} } {
	
	$self next $src $dst $bw $delay $q $lltype ; # SimpleLink ctor
	$self instvar queue_ link_

	$self instvar measclassifier_ signalmod_ adc_ est_ measmod_
	
	set ns_ [Simulator instance]
	
	#Create a suitable adc unit from larg with suitable params
	set adctype [lindex $arg 3]
	set adc_ [new ADC/$adctype]
	$adc_ set bandwidth_ $bw
	$adc_ set src_ [$src id]
	$adc_ set dst_ [$dst id]
	
	if { [lindex $arg 5] == "CL" } {
		#Create a suitable est unit 
		set esttype [lindex $arg 4]
		set est_ [new Est/$esttype]
		$est_ set src_ [$src id]
		$est_ set dst_ [$dst id]
		$adc_ attach-est $est_ 1
		
		#Create a Measurement Module 
		set measmod_ [new MeasureMod]
		$measmod_ target $queue_
		$adc_ attach-measmod $measmod_ 1
	}
	
	#Create the signalmodule
	set signaltype [lindex $arg 2]
	set signalmod_ [new $signaltype]
	$signalmod_ set src_ [$src id]
	$signalmod_ set dst_ [$dst id]
	$signalmod_ attach-adc $adc_
	$self add-to-head $signalmod_


	#Create a measurement classifier to decide which packets to measure
	$self create-meas-classifier
	$signalmod_ target $measclassifier_
	
	#Schedule to start the admission control object
	$ns_ at 0.0 "$adc_ start"
}
IntServLink instproc buffersize { b } {
	$self instvar est_ adc_
	$est_ setbuf [set b]
	$adc_ setbuf [set b]
}


#measClassifier is an instance of Classifier/Hash/Flow
#for right now
# FlowId 0 -> Best Effort traffic
# FlowId non-zero -> Int Serv traffic 

IntServLink instproc create-meas-classifier {} {
	$self instvar measclassifier_ measmod_ link_ queue_
	
	set measclassifier_ [new Classifier/Hash/Fid 1 ]
	#set slots for measclassifier
	set slot [$measclassifier_ installNext $queue_]
	$measclassifier_ set-hash auto 0 0 0 $slot 
	
	#Currently measure all flows with fid.ne.0 alone
	set slot [$measclassifier_ installNext $measmod_]
	$measclassifier_ set default_ 1
}

IntServLink instproc trace-sig { f } {
	$self instvar signalmod_ est_ adc_
	$signalmod_ attach $f
	$est_ attach $f
	$adc_ attach $f
	set ns [Simulator instance]
	$ns at 0.0 "$signalmod_ add-trace"
}

#Helper  function to output link utilization and bw estimate
IntServLink instproc trace-util { interval {f ""}} {
	$self instvar est_
	set ns [Simulator instance]
	if { $f != "" } {
		puts $f "[$ns now] [$est_ load-est] [$est_ link-utlzn]" 
	}
	$ns at [expr [$ns now]+$interval] "$self trace-util $interval $f" 
}
