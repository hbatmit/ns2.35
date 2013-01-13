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
#  $Header: /cvsroot/nsnam/ns-2/tcl/ex/simple-webcache.tcl,v 1.2 2005/09/16 03:05:42 tomh Exp $

set ns [new Simulator]

# Create topology/routing
set node(c) [$ns node] 
set node(e) [$ns node]
set node(s) [$ns node]
$ns duplex-link $node(s) $node(e) 1.5Mb 50ms DropTail
$ns duplex-link $node(e) $node(c) 10Mb 2ms DropTail 
$ns rtproto Session

# HTTP logs
set log [open "http.log" w]

# Use PagePool/Math
set pgp [new PagePool/Math]
set tmp [new RandomVariable/Constant] ;# Size generator
$tmp set val_ 1024  ;# average page size
$pgp ranvar-size $tmp
set tmp [new RandomVariable/Exponential] ;# Age generator
$tmp set avg_ 5 ;# average page age
$pgp ranvar-age $tmp

set server [new Http/Server $ns $node(s)]
$server set-page-generator $pgp
$server log $log

set cache [new Http/Cache $ns $node(e)]
$cache log $log

set client [new Http/Client $ns $node(c)]
set tmp [new RandomVariable/Exponential] ;# Poisson process
$tmp set avg_ 5 ;# average request interval
$client set-interval-generator $tmp
$client set-page-generator $pgp
$client log $log

set startTime 1 ;# simulation start time
set finishTime 50 ;# simulation end time
$ns at $startTime "start-connection"
$ns at $finishTime "finish"

proc start-connection {} {
	global ns server cache client
	$client connect $cache
	$cache connect $server
	$client start-session $cache $server
}

proc finish {} {
	global ns log
	$ns flush-trace
	flush $log
	close $log
	exit 0
}

$ns run
