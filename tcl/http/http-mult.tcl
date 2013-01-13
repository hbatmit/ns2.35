#
# Copyright (c) 1997 Regents of the University of California.
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
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
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
# This file was contributed by Curtis Villamizar <curtis@ans.net>, May 1997.
# Maintainer: John Heidemann <johnh@isi.edu>.
#

#
# WARNING:  This code uses the compatibility library and so should not
# be used as an example.  Hopefully at some time in the future it will
# be updated.
#

#
#	Start with the idea of a four node test environment.
#
#  cN -------- ispN -------- nsp1 -------- nsp2 -------- sN
#      28.8kb        1.54mb        44.2mb        1.54mb
#     modemdelay    ispdelay      netdelay      taildelay
#
#	Assume there are http N clients c(n) and N servers s(n) and
# each of the clients is served by a different providers isp(n).  The
# delay from the modem is fixes at modemdelay.  There is a random
# delay in the range of zero to ispdelay between the ISP and the
# bottleneck link nsp1-nsp2.  The bottleneck has a delay of netdelay.
# There is also a random delay between the bottleneck link and each
# server in the range of taildelay_lo to taildelay_hi.
#

proc create_testnet {} {
    global testnet flows background

    # use a fixed seed - change if multiple tests needed
    ns-random $testnet(seed)

    set testnet(nsp1) [ns node]
    set testnet(nsp2) [ns node]
    if {$testnet(verbose)} {
	puts "\ttestnet(nsp1)\t$testnet(nsp1)"
	puts "\ttestnet(nsp2)\t$testnet(nsp2)"
    }

    set netlink [ns_duplex $testnet(nsp1) $testnet(nsp2) \
	    $testnet(netspeed) $testnet(netdelay) $testnet(netqtype)]
    if {$testnet(netqtype) == "red"} {
	set redlink [ns link $testnet(nsp1) $testnet(nsp2)]
	$redlink set thresh [expr $testnet(netqueue) * 0.25]
	$redlink set maxthresh [expr $testnet(netqueue) * 0.85]
	$redlink set q_weight 0.001
	$redlink set wait_ 1
	$redlink set dropTail_ 1
    } elseif {$testnet(netqtype) == "sfq"} {
	set sfqlink [ns link $testnet(nsp1) $testnet(nsp2)]
	$sfqlink set limit $testnet(netqueue)
	$sfqlink set buckets [expr $testnet(netqueue) >> 2]
    } else {
	[lindex $netlink 0] set queue-limit $testnet(netqueue)
	[lindex $netlink 1] set queue-limit $testnet(netqueue)
    }
    if {$testnet(quiet) == 0} {
	puts [format "creating %d links for clickers" $testnet(clickers)]
    }
    for {set pair 0} {$pair < $testnet(numisp)} {incr pair} {
	set ispdelay [format "%dms" \
		[expr ( $testnet(ispdelay) * ( [ns-random] >> 16 ) ) >> 16]]
	set isp [format "isp%d" $pair]
	set testnet($isp) [ns node]
	if {$testnet(verbose) != 0} {
	    puts "\ttestnet(isp=$isp)\t$testnet($isp)"
	}
    }
    for {set pair 0} {$pair < $testnet(clickers)} {incr pair} {
	set client [format "c%d" $pair]
	set server [format "s%d" $pair]
	set testnet($server) [ns node]
	if {$testnet(verbose) != 0} {
	    puts "\ttestnet(server=$server)\t$testnet($isp)"
	}
	set isp [format "isp%d" [expr $pair % $testnet(numisp)]]
	set dest [format "d%d" $pair]
	set taildelay [format "%dms" \
		[expr $testnet(taildelay_lo) + \
		( ( ( $testnet(taildelay_hi) - $testnet(taildelay_lo) ) \
		* ( [ns-random] >> 16 ) ) >> 16 )]]
	if {$testnet(doproxy) == 0} {
	    set testnet($client) [ns node]
	    if {$testnet(verbose) != 0} {
		puts "\ttestnet(client=$client)\t$testnet($client)"
	    }
	    set modemlink \
		    [ns_duplex $testnet($client) $testnet($isp) \
		    $testnet(modemspeed) $testnet(modemdelay) \
		    $testnet(modemqtype)]
	    [lindex $modemlink 0] set queue-limit $testnet(modemqueue)
	    [lindex $modemlink 1] set queue-limit $testnet(modemqueue)
	    ns_duplex $testnet($isp) $testnet(nsp1) \
		    $testnet(ispspeed) $ispdelay $testnet(netqtype)
	    set testnet($dest) $testnet($client)
	} else {
	    # this is a hack - make the isp the actual client
	    ns_duplex $testnet($isp) $testnet(nsp1) \
		    $testnet(ispspeed) $ispdelay $testnet(netqtype)
	    set testnet($dest) $testnet($isp)
	}
	ns_duplex $testnet(nsp2) $testnet($server) \
		$testnet(ispspeed) $taildelay $testnet(netqtype)
    }
    if {$testnet(quiet) == 0} {
	puts [format "creating %d links for background" $background(nflows)]
    }
    set ident [expr $testnet(clickers) * (1 + $flows(inlines_needed))]
    for {set pair 0} {$pair < $background(numisp)} {incr pair} {
	set client [format "c%d" $ident]
	set server [format "s%d" $ident]
	set isp [format "isp%d" $ident]
	incr ident;
	set testnet($client) [ns node]
	set testnet($server) [ns node]
	if {$testnet(verbose) != 0} {
	    puts "\ttestnet(client=$client)\t$testnet($client)"
	    puts "\ttestnet(server=$server)\t$testnet($server)"
	}
	set ispdelay [format "%dms" \
		[expr ( $testnet(ispdelay) * ( [ns-random] >> 16 ) ) >> 16]]
	set taildelay [format "%dms" \
		[expr $testnet(taildelay_lo) + \
		( ( ( $testnet(taildelay_hi) - $testnet(taildelay_lo) ) \
		* ( [ns-random] >> 16 ) ) >> 16 )]]
	if {$testnet(bkgproxy) == 0} {
	    set testnet($isp) [ns node]
	    if {$testnet(verbose) != 0} {
		puts "\ttestnet(isp=$isp)\t$testnet($isp)"
	    }
	    set modemlink \
		    [ns_duplex $testnet($client) $testnet($isp) \
		    $testnet(modemspeed) $testnet(modemdelay) \
		    $testnet(modemqtype)]
	    [lindex $modemlink 0] set queue-limit $testnet(modemqueue)
	    [lindex $modemlink 1] set queue-limit $testnet(modemqueue)
	    ns_duplex $testnet($isp) $testnet(nsp1) \
		    $testnet(ispspeed) $ispdelay $testnet(netqtype)
	} else {
	    ns_duplex $testnet($client) $testnet(nsp1) \
		    $testnet(ispspeed) $ispdelay $testnet(netqtype)
	}
	ns_duplex $testnet(nsp2) $testnet($server) \
		$testnet(ispspeed) $taildelay $testnet(netqtype)
    }
    if {$testnet(quiet) == 0} {
	puts "testnet topology completed"
    }
}

proc trigger { xresults } {
    global testnet flows

    # NEEDSWORK:  should we really indirect once down results like this?
    set results "[lindex $xresults 0]"

    set type [lindex $results 0]
    if { $type != "-" && $type != "d" } {
	return;
    }
    set id [lindex $results 7]
    if {$id > $testnet(clickers) * (1 + $flows(inlines_needed))} {
	return;
    }
    set counter [format "count%d" $id]
    if { [info exists flows($counter)] } {
	incr flows($counter)
    } else {
	set flows($counter) 1
    }
    set last last$id
    set flows($last) [ns now]
    if { $type != "-" } {
	return;
    }
    set base [expr $id - ( $id % $flows(flows_per) )]
    set got [expr 1 + [lindex $results 10]]
    set isrunning isrunning$id
    if {$flows($isrunning)} {
	if { $id == $base } {
	    if { $got == $testnet(httpsize) } {
		if {$testnet(quiet) == 0} {
		    puts [format "http flow completed at %s" \
			    [lindex $results 1]]
		}
		set flows_running [format "flows_running%d" $base]
		incr flows($flows_running) -1
		incr flows(total_running) -1
		set flows($isrunning) 0
	    }
	} else {
	    if { $got == $testnet(inlinesize) } {
		if {$testnet(quiet) == 0} {
		    puts [format "inline%d completed at %s" \
			    $id [lindex $results 1]]
		}
		set flows_running [format "flows_running%d" $base]
		incr flows($flows_running) -1
		set inlines_running [format "inlines_running%d" $base]
		incr flows($inlines_running) -1
		incr flows(total_running) -1
		set flows($isrunning) 0
	    }
	}
    }
    set basecount count$base
    if { $flows($basecount) > 1 } {
	set inlines_started [format "inlines_started%d" $base]
	set flows_running [format "flows_running%d" $base]
	set inlines_running [format "inlines_running%d" $base]
	while {$flows($inlines_started) < $flows(inlines_needed) \
		&& $flows($flows_running) < $flows(flows_allowed) \
		&& $flows($inlines_running) < $flows(inlines_allowed) } {
	    incr flows($inlines_started)
	    incr flows($flows_running)
	    incr flows($inlines_running)
	    incr flows(total_running)
	    set ident [expr $base + $flows($inlines_started)]
	    set nextflow [format "tcp%d" $ident]
	    if {$testnet(quiet) == 0} {
		puts [format "at %7.3f start %s : %d running" \
			[lindex $results 1] $nextflow $flows(total_running)]
	    }
	    if {$flows(persist)} {
		for {set j $base} {$j < $ident} {incr j} {
		    set isrunning isrunning$j
		    if {$flows($isrunning) == 0} {
			set thisflow flow$ident
			set otherflow flow$j
			$flows($thisflow) persist $flows($otherflow)
			break
		    }
		}
	    }
	    $flows($nextflow) start
	    incr testnet(needed) $testnet(inlinesize)
	    set isrunning isrunning$ident
	    set flows($isrunning) 1
	    set first first$ident
	    set flows($first) [ns now]
	    set last last$ident
	    set flows($last) flows($first)
	}
    }
}

proc openTrace { stopTime testName } {
    exec rm -f out.tr
    set traceFile [open out.tr w]
    ns at $stopTime "stopsource $traceFile $testName"
    set T [ns trace]
    $T attach $traceFile
    return $T
}

proc stopsource { traceFile testName } {
    global flows testnet
    set flows(startnew) 0
    if {$flows(total_running) == 0} {
	close $traceFile
	finish $testName"
	    # "  hack for emacs fontification
	exit 0
    } else {
	if {$testnet(quiet) == 0} {
	    puts [format "at [ns now] %d flows still running" \
		    $flows(total_running)]
	}
	ns at [expr [ns now] + 100] "stopsource $traceFile $testName"
    }
}

proc finish file {
    global testnet flows

    set total 0
    set ident 0
    for {set pair 0} {$pair < $testnet(clickers)} {incr pair} {
	set first first$ident
	set firstpkt $flows($first)
	set lastpkt $firstpkt
	set lastimg $firstpkt
	for { set i 0 } { $i <= $flows(inlines_needed) } { incr i } {
	    set counter [format "count%d" $ident]
	    set got $flows($counter)
	    incr total $got
	    set last last$ident
	    set elapsed [expr $flows($last) - $firstpkt]
	    if {$testnet(quiet) == 0} {
		puts [format "flow %3d %d : %3d packets, elapsed %7.3f" \
			$ident $i $got $elapsed]
	    }
	    if {$flows($last) > $lastpkt} {
		set lastpkt $flows($last)
	    }
	    if {$i <= 1 && $flows($last) > $lastimg} {
		set lastimg $flows($last)
	    }
	    incr ident
	}
	set elapsed [expr $lastpkt - $firstpkt]
	set elapsimg [expr $lastimg - $firstpkt]
	if {$testnet(quiet) == 0} {
	    puts [format "clicker %3d : elapsed %7.3f %7.3f" \
		    $pair $elapsimg $elapsed]
	}
	set bucket [expr int((10 * $elapsed) + 0.999)]
	if {[info exists histogram($bucket)]} {
	    incr histogram($bucket)
	} else {
	    set histogram($bucket) 1
	}
	set bucket [expr int((10 * $elapsimg) + 0.999)]
	if {[info exists histimg($bucket)]} {
	    incr histimg($bucket)
	} else {
	    set histimg($bucket) 1
	}
    }
    set needed $testnet(needed)
    set discard [expr $total - $needed]
    puts [format "%d sent : %d needed : %d discarded : %d %%" \
	    $total $needed $discard [expr 100 * $discard / $needed]]
    puts "histogram of http plus first image times"
    foreach bucket [lsort -integer [array names histimg]] {
	puts [format "1st image  seconds:occurances  %6.1f : %4d" \
		[expr 0.1 * $bucket] $histimg($bucket)]
    }
    puts "histogram of complete clicker transfer time"
    foreach bucket [lsort -integer [array names histogram]] {
	puts [format "clicker  seconds:occurances  %6.1f : %4d" \
		[expr 0.1 * $bucket] $histogram($bucket)]
    }
}

proc init_tcp_flow {size pair ident} {
    global testnet flows

    set taskid [format "tcp%d" $ident ]
    set flowid [format "flow%d" $ident]
    set dest [format "d%d" $pair]
    set server [format "s%d" $pair]
    set flow [ns_create_connection tcp-reno \
	    $testnet($server) tcp-sink $testnet($dest) $ident]
    $flow set window $testnet(window)
    $flow set packet-size $testnet(mss)
    $flow set maxcwnd $testnet(window)
    set flows($flowid) $flow 
    set flows($taskid) [$flow source ftp]
    $flows($taskid) set maxpkts_ $size
}

proc setup_http_test {} {
    global testnet flows background

    create_testnet

    set trigger $testnet(starttime)
    for {set pair 0} {$pair < $testnet(clickers)} {incr pair} {
	set base [expr $pair * $flows(flows_per)]
	set ident $base
	set inlines_started [format "inlines_started%d" $base]
	set flows_running [format "flows_running%d" $base]
	set inlines_running [format "inlines_running%d" $base]
	set counter [format "count%d" $ident]
	set flows($inlines_started) 0
	set flows($flows_running) 0
	set flows($inlines_running) 0
	init_tcp_flow $testnet(httpsize) $pair $ident
	set flows($counter) 0
	for { set i 1 } { $i <= $flows(inlines_needed) } { incr i } {
	    incr ident
	    init_tcp_flow $testnet(inlinesize) $pair $ident
	}
	ns at $trigger "start_http $base"
	set addms [expr ($testnet(clickdelay) * ([ns-random] >> 16)) >> 16]
	set trigger [expr $trigger + (0.001 * $addms)]
		
    }
    set testnet(needed) 0
    set flows(total_running) 0

    set trigger $background(start)
    set offset [expr $testnet(clickers) * (1 + $flows(inlines_needed))]
    for {set pair 0} {$pair < $background(nflows)} {incr pair} {
	set ident [expr $offset + ($pair % $background(numisp))]
	set client [format "c%d" $ident]
	set server [format "s%d" $ident]
	set isp [format "isp%d" $ident]
	set ident [expr $offset + $pair + 1]
	set flow [ns_create_connection tcp-reno \
		$testnet($server) tcp-sink $testnet($client) $ident]
	$flow set window $background(window)
	$flow set packet-size $background(mss)
	$flow set maxcwnd $background(window)
	set tcp_id [$flow source ftp]
	set bkgsize [expr $background(minsize) \
		+ ((($background(maxsize) - $background(minsize)) \
		* ([ns-random] >> 16)) >> 16)]
	$tcp_id set maxpkts_ $bkgsize
	if {$testnet(quiet) == 0} {
	    puts [format "at %7.3f start background flow %d - %d" \
		    $trigger $pair $bkgsize]
	}
	ns at $trigger "$tcp_id start"
	set addms [expr ($background(delay) * ([ns-random] >> 16)) >> 16]
	set trigger [expr $trigger + (0.001 * $addms)]
    }

    # trace only the NSP bottleneck link

    set traceme [openTrace $testnet(testlimit) test_http]
    set bottleneck [ns link $testnet(nsp2) $testnet(nsp1)]
    $bottleneck trace $traceme
    $bottleneck callback { trigger }

    if {$testnet(gen_map)} {
	ns gen-map
    }
}

proc start_http { base } {
    global testnet flows

    set flows_running [format "flows_running%d" $base]
    incr flows($flows_running)
    set flow tcp$base
    if {$testnet(quiet) == 0} {
	puts [format "at %7.3f start http%d" [ns now] $base]
    }
    $flows($flow) start
    incr testnet(needed) $testnet(httpsize)
    incr flows(total_running)
    set isrunning isrunning$base
    set flows($isrunning) 1
    set first first$base
    set flows($first) [ns now]
    set last last$base
    set flows($last) flows($first)
}

proc set_globals {} {
    global testnet flows background

    set testnet(starttime) 10.0
    set testnet(netdelay) 35ms
    set testnet(modemdelay) 50ms
    set testnet(ispdelay) 25
    set testnet(taildelay_lo) 1
    set testnet(taildelay_hi) 25
    set testnet(netspeed) 512kb
    set testnet(modemspeed) 28.8kb
    set testnet(ispspeed) 1.54mb
    set testnet(window) 64
    set testnet(mss) 512
    set testnet(httpsize) 6
    set testnet(inlinesize) 40
    set testnet(modemqueue) 24
    set testnet(netqueue) 40
    set testnet(clickers) 10
    set testnet(clickdelay) 500
    set testnet(testlimit) 500.0
    set testnet(quiet) 0
    set testnet(verbose) 1
    set testnet(seed) 1
    set testnet(netqtype) drop-tail
    set testnet(modemqtype) drop-tail
    set testnet(doproxy) 0
    set testnet(bkgproxy) 0
    set testnet(numisp) 2
    set testnet(gen_map) 0

    set flows(inlines_started) 0
    set flows(inlines_needed) 3
    set flows(flows_running) 0
    set flows(flows_allowed) 4
    set flows(inlines_running) 0
    set flows(inlines_allowed) $flows(flows_allowed)
    set flows(flows_per) [expr $flows(inlines_needed) + 1]
    set flows(persist) 0

    set background(nflows) 150
    set background(start) 0.0
    set background(window) 64
    set background(mss) 512
    set background(minsize) 2
    set background(maxsize) 120
    set background(delay) 2000
    set background(numisp) [expr $background(nflows) >> 4]
}

proc process_args {} {
    global argc argv testnet flows background

    for {set i 0} {$i < $argc} {incr i} {
	set arg [lindex $argv $i]
	switch x$arg {
	    x-delayN {
		incr i
		set testnet(netdelay) [lindex $argv $i]
	    }
	    x-delayM {
		incr i
		set testnet(modemdelay) [lindex $argv $i]
	    }
	    x-delayI {
		incr i
		set testnet(ispdelay) [lindex $argv $i]
	    }
	    x-delayTlo {
		incr i
		set testnet(taildelay_lo) [lindex $argv $i]
	    }
	    x-delayThi {
		incr i
		set testnet(taildelay_hi) [lindex $argv $i]
	    }
	    x-delayT {
		incr i
		set testnet(taildelay_hi) [lindex $argv $i]
		set testnet(taildelay_lo) 0
	    }
	    x-netspeed {
		incr i
		set testnet(netspeed) [lindex $argv $i]
	    }
	    x-ispspeed {
		incr i
		set testnet(ispspeed) [lindex $argv $i]
	    }
	    x-modemspeed {
		incr i
		set testnet(modemspeed) [lindex $argv $i]
	    }
	    x-window {
		incr i
		set testnet(window) [lindex $argv $i]
		if {$testnet(bkgproxy)} {
		    set background(window) [lindex $argv $i]
		}
	    }
	    x-mss {
		incr i
		set testnet(mss) [lindex $argv $i]
	    }
	    x-httpsize {
		incr i
		set testnet(httpsize) [lindex $argv $i]
	    }
	    x-inlinesize {
		incr i
		set testnet(inlinesize) [lindex $argv $i]
	    }
	    x-modemqueue {
		incr i
		set testnet(modemqueue) [lindex $argv $i]
	    }
	    x-netqueue {
		incr i
		set testnet(netqueue) [lindex $argv $i]
	    }
	    x-testlimit {
		incr i
		set testnet(testlimit) [lindex $argv $i]
	    }
	    x-clickers {
		incr i
		set testnet(clickers) [lindex $argv $i]
	    }
	    x-clickdelay {
		incr i
		set testnet(clickdelay) [lindex $argv $i]
	    }
	    x-inlines {
		incr i
		set flows(inlines_needed) [lindex $argv $i]
	    }
	    x-maxflow {
		incr i
		set flows(flows_allowed) [lindex $argv $i]
	    }
	    x-maxinline {
		incr i
		set flows(inlines_allowed) [lindex $argv $i]
	    }
	    x-bkg-flows {
		incr i
		set background(nflows) [lindex $argv $i]
	    }
	    x-bkg-window {
		incr i
		set background(window) [lindex $argv $i]
	    }
	    x-bkg-mss {
		incr i
		set background(mss) [lindex $argv $i]
	    }
	    x-bkg-minsize {
		incr i
		set background(minsize) [lindex $argv $i]
	    }
	    x-bkg-maxsize {
		incr i
		set background(maxsize) [lindex $argv $i]
	    }
	    x-bkg-delay {
		incr i
		set background(delay) [lindex $argv $i]
	    }
	    x-start {
		incr i
		set testnet(starttime) [lindex $argv $i]
	    }
	    x-bkg-start {
		incr i
		set background(start) [lindex $argv $i]
	    }
	    x-numisp {
		incr i
		set testnet(numisp) [lindex $argv $i]
	    }
	    x-bkgisp {
		incr i
		set background(numisp) [lindex $argv $i]
	    }
	    x-seed {
		incr i
		set testnet(seed) [lindex $argv $i]
	    }
	    x-modemred {
		set testnet(modemqtype) red
	    }
	    x-netred {
		set testnet(netqtype) red
	    }
	    x-red {
		set testnet(netqtype) red
		set testnet(modemqtype) red
	    }
	    x-sfq {
		set testnet(netqtype) sfq
	    }
	    x-proxy {
		set testnet(doproxy) 1
	    }
	    x-bkgproxy {
		set testnet(bkgproxy) 1
		set background(window) 16
	    }
	    x-allproxy {
		set testnet(doproxy) 1
		set testnet(bkgproxy) 1
		set background(window) 16
	    }
	    x-tail {
		set testnet(netqtype) drop-tail
		set testnet(modemqtype) drop-tail
	    }
	    x-persist {
		set flows(persist) 1
	    }
	    x-nopersist {
		set flows(persist) 0
	    }
	    x-quiet {
		set testnet(quiet) 1
	    }
	    x-ns-gen-map {
		set testnet(gen_map) 1
	    }
	    default {
		puts [format "unrecognized argument: %s" [lindex $argv $i]]
		exit 1
	    }
	}
    }
}

proc report_conditions {} {
    global testnet flows background

    if {$flows(inlines_allowed) > $flows(flows_allowed)} {
	set flows(inlines_allowed) $flows(flows_allowed)
    }
    puts [format "delays: net %s, modem %s, isp 0-%dms, tail %d-%dms" \
	    $testnet(netdelay) $testnet(modemdelay) $testnet(ispdelay) \
	    $testnet(taildelay_lo) $testnet(taildelay_hi)]
    puts [format "link speeds: net %s, modem %s, isp %s" \
	    $testnet(netspeed) $testnet(modemspeed) $testnet(ispspeed)]
    puts [format "queues: modem %d, net %d, (isp and tail not congested)" \
	    $testnet(modemqueue) $testnet(netqueue)]
    if {$flows(persist)} {
	set ispersistant "HTTP 1.1 (persistent)"
    } else {
	set ispersistant "pre- HTTP 1.1"
    }
    puts [format "window %d, mss %d, %s, http %d(%d), inline %d(%d)" \
	    $testnet(window) $testnet(mss) $ispersistant \
	    $testnet(httpsize) [expr $testnet(mss) * $testnet(httpsize)] \
	    $testnet(inlinesize) [expr $testnet(mss) * $testnet(inlinesize)]]
    puts [format "sources: %d clickers, spaced 0-%dms, seed for random= %d" \
	    $testnet(clickers) $testnet(clickdelay) $testnet(seed)]
    puts [format \
	    "%d inlines per page, %d flows per client, %d inlines running" \
	    $flows(inlines_needed) $flows(flows_allowed) \
	    $flows(inlines_allowed)]
    puts [format \
	    "%d bkg flows, window %d, mss %d, size %d-%d pkts, every 0-%dms" \
	    $background(nflows) $background(window) $background(mss) \
	    $background(minsize) $background(maxsize) $background(delay)]
    puts [format "start bkg at %s, test at %s, run test for %s seconds" \
	    $background(start) $testnet(starttime) $testnet(testlimit)]
    if {$testnet(doproxy)} {
	set doingproxy "proxy HTTP"
    } else {
	set doingproxy "no proxy HTTP"
    }
    if {$testnet(bkgproxy) > 1} {
	set dobkgproxy "bkg proxy HTTP"
    } else {
	set dobkgproxy "no bkg proxy"
    }
    puts [format "queue: modem= %s, net= %s, %s, %s" \
	    $testnet(modemqtype) $testnet(netqtype) $doingproxy $dobkgproxy]
}

set_globals
process_args
report_conditions
setup_http_test
ns run
