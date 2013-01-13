#
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License,
#  version 2, as published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
#
#  The copyright of this module includes the following
#  linking-with-specific-other-licenses addition:
#
#  In addition, as a special exception, the copyright holders of
#  this module give you permission to combine (via static or
#  dynamic linking) this module with free software programs or
#  libraries that are released under the GNU LGPL and with code
#  included in the standard release of ns-2 under the Apache 2.0
#  license or under otherwise-compatible licenses with advertising
#  requirements (or modified versions of such code, with unchanged
#  license).  You may copy and distribute such a system following the
#  terms of the GNU GPL for this module and the licenses of the
#  other code concerned, provided that you include the source code of
#  that other code when and as the GNU GPL requires distribution of
#  source code.
#
#  Note that people who make modified versions of this module
#  are not obligated to grant this special exception for their
#  modified versions; it is their choice whether to do so.  The GNU
#  General Public License gives permission to release a modified
#  version without this exception; this exception also makes it
#  possible to release a modified version which carries forward this
#  exception.
# 
# $Header: /cvsroot/nsnam/ns-2/tcl/ex/simple-webcache-comp.tcl,v 1.2 2005/09/16 03:05:41 tomh Exp $
#
# Example script illustrating the usage of complex page in webcache. 
# Thanks to Xiaowei Yang <yxw@mercury.lcs.mit.edu> for motivation and 
# an original version of this script.

Agent/TCP/FullTcp set segsize_ 1460
Http set TRANSPORT_ FullTcp

set ns [new Simulator]

set f [open "comp.tr" w]
$ns namtrace-all $f

set log [open "comp.log" w]

$ns rtproto Session
set node(c) [$ns node]
set node(r) [$ns node]
set node(s) [$ns node]
$ns duplex-link $node(s) $node(r) 100Mb 25ms DropTail
$ns duplex-link $node(r) $node(c) 10Mb 25ms DropTail

[$ns link $node(s) $node(r)] set queue-limit 10000
[$ns link $node(r) $node(s)] set queue-limit 10000
[$ns link $node(c) $node(r)] set queue-limit 10000
[$ns link $node(r) $node(c)] set queue-limit 10000

# Use PagePool/CompMath to generate compound page, which is a page 
# containing multiple embedded objects.
set pgp [new PagePool/CompMath]
$pgp set main_size_ 1000	;# Size of main page
$pgp set comp_size_ 5000	;# Size of embedded object
$pgp set num_pages_ 3		;# Number of objects per compoud page

# Average age of component object: 100s
set tmp [new RandomVariable/Constant]
$tmp set val_ 100
$pgp ranvar-obj-age $tmp

# Average age of the main page: 50s
set tmp [new RandomVariable/Constant]
$tmp set val_ 50
$pgp ranvar-main-age $tmp

set server [new Http/Server/Compound $ns $node(s)]
$server set-page-generator $pgp
$server log $log

set client [new Http/Client/Compound $ns $node(c)]
set tmp [new RandomVariable/Constant]
$tmp set val_ 10
$client set-interval-generator $tmp
$client set-page-generator $pgp
$client log $log

set startTime 1 ;# simulation start time
set finishTime 500 ;# simulation end time
$ns at $startTime "start-connection"
$ns at $finishTime "finish"

proc start-connection {} {
	global ns server  client
	$client connect $server

	# Comment out following line to continuously send requests
	$client start-session $server $server

	# Comment out following line to send out ONE request
	# DON't USE with start-session!!!
	#set pageid $server:[lindex [$client gen-request] 1]
	#$client send-request $server GET $pageid
}

proc finish {} {
	global ns log f client server

	$client stop-session $server
	$client disconnect $server

	$ns flush-trace
	flush $log
	close $log
	exit 0
}

$ns run
