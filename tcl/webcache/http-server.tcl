# Copyright (c) Xerox Corporation 1998. All rights reserved.
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
# Implementation of an HTTP server
#
# $Header: /cvsroot/nsnam/ns-2/tcl/webcache/http-server.tcl,v 1.11 2005/08/26 05:05:30 tomh Exp $


#
# PagePool
#

# Generage a new page, including size, age, and flags. Do NOT generate 
# modification time. That's the job of web servers.
PagePool instproc gen-page { pageid thismod } {
	set size [$self gen-size $pageid]
	# If $thismod == -1, we set age to -1, which means this page
	# never changes
	if {$thismod >= 0} {
		set age [expr [$self gen-modtime $pageid $thismod] - $thismod]
	} else {
		set age -1
	}
	return "size $size age $age modtime $thismod"
}

#
# Compound pagepool with a non-cacheable main page
#
Class PagePool/CompMath/noc -superclass PagePool/CompMath

PagePool/CompMath/noc instproc gen-page { pageid thismod } {
	set res [eval $self next $pageid $thismod]
	if {$pageid == 0} {
		return "$res noc 1"
	} else {
		return $res
	}
}


#
# web server codes
#
Http/Server instproc init args {
	eval $self next $args
	$self instvar node_ stat_
	$node_ color "HotPink"
	array set stat_ [list hit-num 0 mod-num 0 barrival 0]
}

Http/Server instproc set-page-generator { pagepool } {
	$self instvar pgtr_
	set pgtr_ $pagepool
}

Http/Server instproc gen-init-modtime { id } {
	$self instvar pgtr_ ns_
	if [info exists pgtr_] {
		return [$pgtr_ gen-init-modtime $id]
	} else {
		return [$ns_ now]
	}
}

# XXX 
# This method to calculate staleness time isn't scalable!!! We have to have
# a garbage collection method to release unused portion of modtimes_ and 
# modseq_. That's not implemented yet because it requires the server to know
# the oldest version held by all other clients.
Http/Server instproc stale-time { pageid modtime } {
	$self instvar modseq_ modtimes_ ns_
	for {set i $modseq_($pageid)} {$i >= 0} {incr i -1} {
		if {$modtimes_($pageid:$i) <= $modtime} {
			break
		}
	}
	if {$i < 0} {
		error "Non-existent modtime $modtime for page $pageid"
	}
	set ii [expr $i + 1]
	set t1 [expr abs($modtimes_($pageid:$i) - $modtime)]
	set t2 [expr abs($modtimes_($pageid:$ii) - $modtime)]
	if {$t1 > $t2} {
		incr ii
	}
	return [expr [$ns_ now] - $modtimes_($pageid:$ii)]
}

Http/Server instproc modify-page { pageid } {
	# Set Last-Modified-Time to current time
	$self instvar ns_ id_ stat_ pgtr_

	incr stat_(mod-num)
	set id [lindex [split $pageid :] end]

	# Change modtime and lifetime only, do not change page size
	set modtime [$ns_ now]
	if [info exists pgtr_] {
		set pginfo [$pgtr_ gen-page $id $modtime]
	} else {
		set pginfo "size 2000 age 50 modtime $modtime"
	}
	array set data $pginfo
	set age $data(age)
	$self schedule-nextmod [expr [$ns_ now] + $age] $pageid
	eval $self enter-page $pageid $pginfo

	$ns_ trace-annotate "S $id_ INV $pageid"
	$self evTrace S MOD p $pageid m [$ns_ now] n [expr [$ns_ now] + $age]

	$self instvar modtimes_ modseq_
	incr modseq_($pageid)
	set modtimes_($pageid:$modseq_($pageid)) $modtime
}

Http/Server instproc schedule-nextmod { time pageid } {
	$self instvar ns_
	$ns_ at $time "$self modify-page $pageid"
}

Http/Server instproc gen-page { pageid } {
	set pginfo [$self gen-pageinfo $pageid]
	eval $self enter-page $pageid $pginfo
	return $pginfo
}

# XXX Assumes page doesn't exists before. 
Http/Server instproc gen-pageinfo { pageid } {
	$self instvar ns_ pgtr_ 

	if [$self exist-page $pageid] {
		error "$self: shouldn't use gen-page for existing pages"
	}

	set id [lindex [split $pageid :] end]

	# XXX If a page never changes, set modtime to -1 here!!
	set modtime [$self gen-init-modtime $id]
	if [info exists pgtr_] {
		set pginfo [$pgtr_ gen-page $id $modtime]
	} else {
		set pginfo "size 2000 age 50 modtime $modtime"
	}
	array set data $pginfo
	set age $data(age)
	if {$modtime >= 0} {
		$self schedule-nextmod [expr [$ns_ now] + $age] $pageid
	}
	$self evTrace S MOD p $pageid m [$ns_ now] n [expr [$ns_ now] + $age]

	$self instvar modtimes_ modseq_
	set modseq_($pageid) 0
	set modtimes_($pageid:0) $modtime

	return [join $pginfo]
}

Http/Server instproc disconnect { client } {
	$self instvar ns_ clist_ node_
	set pos [lsearch $clist_ $client]
	if {$pos >= 0} {
		lreplace $clist_ $pos $pos
	} else { 
		error "Http/Server::disconnect: not connected to $server"
	}
	set tcp [[$self get-cnc $client] agent]
	$self cmd disconnect $client
	$tcp proc done {} "$ns_ detach-agent $node_ $tcp; delete $tcp"
	$tcp close
	#puts "server [$self id] disconnect"
}

Http/Server instproc alloc-connection { client fid } {
	Http instvar TRANSPORT_
	$self instvar ns_ clist_ node_ fid_

	lappend clist_ $client
	set snk [new Agent/TCP/$TRANSPORT_]
	$snk set fid_ $fid
	$ns_ attach-agent $node_ $snk
	$snk listen
	set wrapper [new Application/TcpApp $snk]
	$self cmd connect $client $wrapper
	return $wrapper
}

Http/Server instproc handle-request-GET { pageid args } {
	$self instvar ns_

	if [$self exist-page $pageid] {
		set pageinfo [$self get-page $pageid]
	} else {
		set pageinfo [$self gen-page $pageid]
	}

	lappend res [$self get-size $pageid]
	eval lappend res $pageinfo
}

Http/Server instproc handle-request-IMS { pageid args } {
	array set data $args
	set mt [$self get-modtime $pageid]
	if {$mt <= $data(modtime)} {
		# Send a not-modified since
		set size [$self get-invsize]
		# We don't need other information for a IMS of a 
		# valid page
		set pageinfo \
		  "size $size modtime $mt time [$self get-cachetime $pageid]"
		$self evTrace S SND p $pageid m $mt z $size t IMS-NM
	} else {
		# Page modified, send the new one
		set size [$self get-size $pageid]
		set pageinfo [$self get-page $pageid]
		$self evTrace S SND p $pageid m $mt z $size t IMS-M
	}

	lappend res $size
	eval lappend res $pageinfo
	return $res
}

Http/Server instproc get-request { client type pageid args } {
	$self instvar ns_ id_ stat_

	incr stat_(hit-num)
	array set data $args
	incr stat_(barrival) $data(size)
	unset data

	# XXX Here maybe we want to wait for a random time to model 
	# server response delay, it could be easily added in a derived class.

	set res [eval $self handle-request-$type $pageid $args]
	set size [lindex $res 0]
	set pageinfo [lrange $res 1 end]

	$self send $client $size \
		"$client get-response-$type $self $pageid $pageinfo"
}

Http/Server instproc set-parent-cache { cache } {
	# Dummy proc
}


#----------------------------------------------------------------------
# Http server modifying pages in the way as described in Pei Cao et al's 
# ICDCS'97 paper. Used to test the simulator
#----------------------------------------------------------------------

Class Http/Server/epa -superclass Http/Server

Http/Server/epa instproc start-update { interval } {
	$self instvar pm_itv_ ns_
	set pm_itv_ $interval
	$ns_ at [expr [$ns_ now] + $pm_itv_] "$self modify-page"
}

# Schedule next page modification using another way
Http/Server/epa instproc schedule-nextmod { time pageid } {
	$self instvar ns_ pm_itv_
	$ns_ at [expr [$ns_ now]+$pm_itv_] "$self modify-page $pageid"
}

# Change the page id to be modified. The pageid given in argument makes 
# no sense at all.
Http/Server/epa instproc modify-page args {
	$self instvar pgtr_
	set pageid $self:[$pgtr_ pick-pagemod]
	eval $self next $pageid
}

# Do not schedule modification during page generation.
Http/Server/epa instproc gen-pageinfo { pageid } {
	$self instvar ns_ pgtr_ 

	if [$self exist-page $pageid] {
		error "$self: shouldn't use gen-page for existing pages"
	}

	set id [lindex [split $pageid :] end]

	set modtime [$self gen-init-modtime $id]
	if [info exists pgtr_] {
		set pginfo [$pgtr_ gen-page $id $modtime]
	} else {
		set pginfo "size 2000 age 50 modtime $modtime"
	}
	array set data $pginfo
	set age $data(age)

	$self instvar modtimes_ modseq_
	set modseq_($pageid) 0
	set modtimes_($pageid:0) $modtime

	return [join $pginfo]
}


#----------------------------------------------------------------------
# Base Http invalidation server
#----------------------------------------------------------------------
Http/Server/Inval instproc modify-page { pageid } {
	$self next $pageid
	$self instvar ns_ id_
	$self invalidate $pageid [$ns_ now]
}

Http/Server/Inval instproc handle-request-REF { pageid args } {
	return [eval $self handle-request-GET $pageid $args]
}


#----------------------------------------------------------------------
# Old unicast invalidation Http server. For compatibility
# Server with single unicast invalidation
#----------------------------------------------------------------------
Class Http/Server/Inval/Ucast -superclass Http/Server/Inval

# We need to maintain a list of all caches who have gotten a page from this
# server.
Http/Server/Inval/Ucast instproc get-request { client type pageid args } {
        eval $self next $client $type $pageid $args

        # XXX more efficient representation?
        $self instvar cacheList_
        if [info exists cacheList_($pageid)] {
                set pos [lsearch $cacheList_($pageid) $client]
        } else {
                set pos -1
        }

        # If it's a new cache, put it there
        # XXX we should eventually have a timer for each cache entry, so 
        # we can get rid of old cache entries
        if {$pos < 0 && [regexp "Cache" [$client info class]]} {
                lappend cacheList_($pageid) $client
        }
}

Http/Server/Inval/Ucast instproc invalidate { pageid modtime } {
        $self instvar cacheList_ 

        if ![info exists cacheList_($pageid)] {
                return
        }
        foreach c $cacheList_($pageid) {
                # Send invalidation to every cache, assuming a connection 
                # exists between the server and the cache
                set size [$self get-invsize]

		# Mark invalidation packet as another fid
		set agent [[$self get-cnc $c] agent]
		set fid [$agent set fid_]
		$agent_ set fid_ [Http set PINV_FID_]
                $self send $c $size \
                        "$c invalidate $pageid $modtime"
		$agent_ set fid_ $fid
                $self evTrace S INV p $pageid m $modtime z $size
        }
}


#----------------------------------------------------------------------
# (Y)et another (U)ni(C)ast invalidation server
#
# It has a single parent cache. Whenever a page is updated in this server
# it informs the parent cache, which will in turn propagate the update
# (or invalidation) to the whole cache hierarchy.
#----------------------------------------------------------------------
Http/Server/Inval/Yuc instproc set-tlc { tlc } {
	$self instvar tlc_
	set tlc_ $tlc
}

Http/Server/Inval/Yuc instproc get-tlc { tlc } {
	$self instvar tlc_
	return $tlc_
}

Http/Server/Inval/Yuc instproc next-hb {} {
	Http/Server/Inval/Yuc instvar hb_interval_ 
	return [expr $hb_interval_ * [uniform 0.9 1.1]]
}

# XXX Must do this when the caching hierachy is ready
Http/Server/Inval/Yuc instproc set-parent-cache { cache } {
	$self instvar pcache_
	set pcache_ $cache

	# Send JOIN
	#puts "[$self id] joins cache [$pcache_ id]"
	$self send $pcache_ [$self get-joinsize] \
		"$pcache_ server-join $self $self"

	# Establish an invalidation connection using TCP
	Http instvar TRANSPORT_
	$self instvar ns_ node_

	set tcp [new Agent/TCP/$TRANSPORT_]
	$tcp set fid_ [Http set HB_FID_]
	$ns_ attach-agent $node_ $tcp
	set dst [$pcache_ setup-unicast-hb]
	set snk [$dst agent]
	$ns_ connect $tcp $snk
	#$tcp set dst_ [$snk set addr_] 
	$tcp set window_ 100

	set wrapper [new Application/TcpApp/HttpInval $tcp]
	$wrapper connect $dst
	$wrapper set-app $self

	$self add-inval-sender $wrapper

	# Start heartbeat after some time, otherwise TCP connection may 
	# not be well established...
	$self instvar ns_
	$ns_ at [expr [$ns_ now] + [$self next-hb]] "$self heartbeat"
}

Http/Server/Inval/Yuc instproc heartbeat {} {
	$self instvar pcache_ ns_

	$self cmd send-hb
	$ns_ at [expr [$ns_ now] + [$self next-hb]] \
		"$self heartbeat"
}

Http/Server/Inval/Yuc instproc get-request { cl type pageid args } {
	eval $self next $cl $type $pageid $args
	if {($type == "GET") || ($type == "REF")} {
		$self count-request $pageid
	}
}

Http/Server/Inval/Yuc instproc invalidate { pageid modtime } {
	$self instvar pcache_ id_ enable_upd_

	if ![info exists pcache_] {
		error "Server $id_ doesn't have a parent cache!"
	}

	# One more invalidation
	$self count-inval $pageid

	if [$self is-pushable $pageid] {
		$self push-page $pageid $modtime
		return
	}

	# Send invalidation to every cache, assuming a connection 
	# exists between the server and the cache
#	set size [$self get-invsize]
	# Mark invalidation packet as another fid
#	set agent [[$self get-cnc $pcache_] agent]
#	set fid [$agent set fid_]
#	$agent set fid_ [Http set PINV_FID_]
#	$self send $pcache_ $size "$pcache_ invalidate $pageid $modtime"
#	$agent set fid_ $fid

	$self cmd add-inv $pageid $modtime
	$self evTrace S INV p $pageid m $modtime 
}

Http/Server/Inval/Yuc instproc push-page { pageid modtime } {
	$self instvar pcache_ id_

	if ![info exists pcache_] {
		error "Server $id_ doesn't have a parent cache!"
	}
	# Do not send invalidation, instead send the new page to 
	# parent cache
	set size [$self get-size $pageid]
	set pageinfo [$self get-page $pageid]

	# Mark invalidation packet as another fid
	set agent [[$self get-cnc $pcache_] agent]
	set fid [$agent set fid_]
	$agent set fid_ [Http set PINV_FID_]
	$self send $pcache_ $size \
		"$pcache_ push-update $pageid $pageinfo"
	$agent set fid_ $fid
	$self evTrace S UPD p $pageid m $modtime z $size
}

Http/Server/Inval/Yuc instproc get-req-notify { pageid } {
	$self count-request $pageid
}

Http/Server/Inval/Yuc instproc handle-request-TLC { pageid args } {
	$self instvar tlc_
	array set data $args
	lappend res $data(size)	;# Same size of queries
	lappend res $tlc_
	return $res
}


#----------------------------------------------------------------------
# server + support for compound pages. 
# 
# A compound page is considered to be a frequently changing main page
# and several component pages which are usually big static images.
#
# XXX This is a naive implementation, which assumes single page and 
# fixed page size for all pages
#----------------------------------------------------------------------
Class Http/Server/Compound -superclass Http/Server

# Invalidation server for compound pages
Class Http/Server/Inval/MYuc -superclass \
		{ Http/Server/Inval/Yuc Http/Server/Compound}

