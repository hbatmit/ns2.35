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
# HTTP agents: server, client, cache
#
# $Header: /cvsroot/nsnam/ns-2/tcl/webcache/http-agent.tcl,v 1.11 2005/08/26 05:05:30 tomh Exp $


Http set id_ 0	;# required by TclCL
# Type of Tcp agent. Can be SimpleTcp or FullTcp
# Default should be set to FullTcp, in case for inadvertent bites. :(
Http set TRANSPORT_ FullTcp
Http set HB_FID_ 40
Http set PINV_FID_ 41

# XXX invalidation message size should be proportional to the number of
# invalidations inside the message
Http set INVSize_ 43	;# unicast invalidation
Http set REQSize_ 43	;# Request
Http set REFSize_ 50	;# Refetch request
Http set IMSSize_ 50	;# If-Modified-Since
Http set JOINSize_ 10	;# Server join/leave
Http set HBSize_ 1	;# Used by Http/Server/Inval only
Http set PFSize_ 1	;# Pro forma
Http set NTFSize_ 10	;# Request Notification
Http set MPUSize_ 10	;# Mandatory push request

Http/Server set id_ 0
Http/Server/Inval set id_ 0
Http/Server/Inval/Yuc set hb_interval_ 60
Http/Server/Inval/Yuc set enable_upd_ 0
Http/Server/Inval/Yuc set Ca_ 1
Http/Server/Inval/Yuc set Cb_ 4
Http/Server/Inval/Yuc set push_thresh_ 4
Http/Server/Inval/Yuc set push_low_bound_ 0
Http/Server/Inval/Yuc set push_high_bound_ 8

Http/Cache set id_ 0
Http/Cache/Inval set id_ 0
Http/Cache/Inval/Mcast set hb_interval_ 60
Http/Cache/Inval/Mcast set upd_interval_ 5
Http/Cache/Inval/Mcast set enable_upd_ 0
Http/Cache/Inval/Mcast set Ca_ 1
Http/Cache/Inval/Mcast set Cb_ 4
Http/Cache/Inval/Mcast set push_thresh_ 4
Http/Cache/Inval/Mcast set push_low_bound_ 0
Http/Cache/Inval/Mcast set push_high_bound_ 8
Http/Cache/Inval/Mcast/Perc set direct_request_ 0

PagePool/CompMath set num_pages_ 1
PagePool/CompMath set main_size_ 1024
PagePool/CompMath set comp_size_ 10240

# Transport protocol used for multimedia connections
Http set MEDIA_TRANSPORT_ RAP
# Application-level handler for multimedia connections. 
# Currently there are two available:
# - MediaApp: simply extract data from packets and pass it to cache
# - QA: do quality adaptation
Http set MEDIA_APP_ MediaApp
# 1K per multimedia segment
Application/MediaApp set segmentSize_ 1024
Application/MediaApp set MAX_LAYER_ 10
# Constants related to quality adaptation
Application/MediaApp/QA set LAYERBW_ 2500 ;# Byte per-second
Application/MediaApp/QA set MAXACTIVELAYERS_ 10
Application/MediaApp/QA set SRTTWEIGHT_ 0.95
Application/MediaApp/QA set SMOOTHFACTOR_ 4
Application/MediaApp/QA set MAXBKOFF_ 100
Application/MediaApp/QA set debug_output_ 0
# Prefetching lookahead SRTT 200ms
Application/MediaApp/QA set pref_srtt_ 0.6
# 100M buffer size at cache/server/client
PagePool/Client/Media set max_size_ 104857600 


Http instproc init { ns node } {
	$self next
	$self instvar ns_ node_ id_ pool_
	set ns_ $ns
	set node_ $node
	$self set id_ [$node_ id]
	set pool_ [$self create-pagepool]
}

Http instproc create-pagepool {} {
	set pool [new PagePool/Client]
	$self set-pagepool $pool
	return $pool
}

Http instproc addr {} {
	$self instvar node_ 
	return [$node_ node-addr]
}

Http set fid_ -1
Http instproc getfid {} {
	$self instvar fid_
	set fid_ [Http set fid_]
	Http set fid_ [incr fid_]
}

Http instproc get-mpusize {} {
	return [Http set MPUSize_]
}

Http instproc get-ntfsize {} {
	return [Http set NTFSize_]
}

Http instproc get-pfsize {} {
	return [Http set PFSize_]
}

Http instproc get-hbsize {} {
	return [Http set HBSize_]
}

Http instproc get-imssize {} {
	return [Http set IMSSize_]
}

Http instproc get-invsize {} {
	return [Http set INVSize_]
}

# Generate request packet size. Should be constant because it's small
Http instproc get-reqsize {} {
	return [Http set REQSize_]
}

Http instproc get-refsize {} {
	return [Http set REFSize_]
}

Http instproc get-joinsize {} {
	return [Http set JOINSize_]
}

# At startup, connect to a server, the server may be a cache
Http instproc connect { server } {
	Http instvar TRANSPORT_
	$self instvar ns_ slist_ node_ fid_ id_

	lappend slist_ $server
	set tcp [new Agent/TCP/$TRANSPORT_]
	$tcp set fid_ [$self getfid]
	$ns_ attach-agent $node_ $tcp

	set ret [$server alloc-connection $self $fid_]
	set snk [$ret agent]
	$ns_ connect $tcp $snk
	#$tcp set dst_ [$snk set addr_]
	$tcp set window_ 100

	# Use a wrapper to implement application data transfer
	set wrapper [new Application/TcpApp $tcp]
	$self cmd connect $server $wrapper
	$wrapper connect $ret
	#puts "HttpApp $id_ connected to server [$server id]"
}

Http instproc stat { name } {
	$self instvar stat_
	return $stat_($name)
}


# Used for mandatory push refreshments
Http/Client set hb_interval_ 60

Http/Client instproc init args {
	eval $self next $args
	$self instvar node_ stat_
	$node_ color "SteelBlue"
	array set stat_ [list req-num 0 stale-num 0 stale-time 0 rep-time 0 \
		rt-min 987654321 rt-max 0 st-min 987654321 st-max 0]
}

# XXX Assume that it's always client disconnects from server, not vice versa
Http/Client instproc disconnect { server } {
	$self instvar ns_ slist_ 
	set pos [lsearch $slist_ $server]
	if {$pos >= 0} {
		lreplace $slist_ $pos $pos
	} else { 
		error "Http::disconnect: not connected to $server"
	}

	# Cleanup of all pending requests and states
	$self instvar ns_ node_ cache_
	$self stop-session $server

	# XXX Is this the right behavior? Should we wait for FIN etc.?
	set tcp [[$self get-cnc $server] agent]
	$self cmd disconnect $server
	$server disconnect $self
	$tcp proc done {} "$ns_ detach-agent $node_ $tcp; delete $tcp"
	$tcp close
}

# Meta-data to be sent in a request
# XXX pageid should always be given from the users, because client may 
# connect to a cache, hence it doesn't know the server name.
Http/Client instproc send-request { server type pageid args } {
	$self instvar ns_ pending_ 	;# unansewered requests

	# XXX Do not set pending states for an non-existent connection
	if ![$self cmd is-connected $server] {
		return
	}

	if ![info exists pending_($pageid)] { 
		# XXX Actually we should use set, because only one request
		# is allowed for a page simultaneously
		lappend pending_($pageid) [$ns_ now]
	} else {
		# If the page is being requested, do not send another request
		return
	}

	set size [$self get-reqsize]
	$self send $server $size \
	    "$server get-request $self $type $pageid size $size [join $args]"
	$self evTrace C GET p $pageid s [$server id] z $size
	$self instvar stat_ simStartTime_
	if [info exists simStartTime_] {
		incr stat_(req-num)
	}

	$self mark-request $pageid
}

Http/Client instproc mark-request { pageid } {
	# Nam state coloring
	$self instvar node_ marks_ ns_
	$node_ add-mark $pageid:[$ns_ now] "purple"
	lappend marks_($pageid) $pageid:[$ns_ now]
}

# The reason that "type" is here is for Http/Cache to work. Client doesn't 
# check the reason of the response
Http/Client instproc get-response-GET { server pageid args } {
	$self instvar pending_ id_ ns_ stat_ simStartTime_

	if ![info exists pending_($pageid)] {
		error "Client $id_: Unrequested response page $pageid from server [$server id]"
	}

	array set data $args

	# Check stale hits
	set origsvr [lindex [split $pageid :] 0]
	set modtime [$origsvr get-modtime $pageid]
	set reqtime [lindex $pending_($pageid) 0]
	set reqrtt [expr [$ns_ now] - $reqtime]
	
	#
	# XXX If a stale hit occurs because a page is modified during the RTT
	# of the request, we should *NOT* consider it a stale hit. We 
	# implement it by ignoring all stale hits whose modification time is
	# larger than the request time. 
	#
	if {$modtime > $data(modtime)} {
		# Staleness is the time from now to the time it's last modified
		set tmp [$origsvr stale-time $pageid $data(modtime)]
		if {$tmp > $reqrtt/2} {
			# We have a real stale hit
			$self evTrace C STA p $pageid s [$origsvr id] l $tmp
			if [info exists simStartTime_] {
				incr stat_(stale-num)
				set stat_(stale-time) [expr \
					$stat_(stale-time) + $tmp]
				if {$stat_(st-min) > $tmp} {
					set stat_(st-min) $tmp
				}
				if {$stat_(st-max) < $tmp} {
					set stat_(st-max) $tmp
				}
			}
		}
	}

	# Assume this response is for the very first request we've sent. 
	# Because we'll average the response time at the end, which 
	# request this response actually corresponds to doesn't matter.
	$self evTrace C RCV p $pageid s [$server id] l $reqrtt z $data(size)
	if [info exists simStartTime_] {
		set stat_(rep-time) [expr $stat_(rep-time) + $reqrtt]
		if {$stat_(rt-min) > $reqrtt} {
			set stat_(rt-min) $reqrtt
		}
		if {$stat_(rt-max) < $reqrtt} {
			set stat_(rt-max) $reqrtt
		}
	}

	set pending_($pageid) [lreplace $pending_($pageid) 0 0]
	if {[llength $pending_($pageid)] == 0} {
		unset pending_($pageid)
	}
	$self mark-response $pageid
}

Http/Client instproc mark-response { pageid } {
	$self instvar node_ marks_ ns_
	set mk [lindex $marks_($pageid) 0]
	$node_ delete-mark $mk
	set marks_($pageid) [lreplace $marks_($pageid) 0 0]
}

Http/Client instproc get-response-REF { server pageid args } {
	eval $self get-response-GET $server $pageid $args
}

Http/Client instproc get-response-IMS { server pageid args } {
	eval $self get-response-GET $server $pageid $args
}

# Generate the time when next request will occur
# It's either a TracePagePool or a MathPagePool
#
# XXX both TracePagePool and MathPagePool should share the same C++ 
# interface and OTcl interface
Http/Client instproc set-page-generator { pagepool } {
	$self instvar pgtr_ 	;# Page generator
	set pgtr_ $pagepool
}

Http/Client instproc set-interval-generator { ranvar } {
	$self instvar rvInterPage_
	set rvInterPage_ $ranvar
}

# XXX PagePool::gen-pageid{} *MUST* precede gen-req-itvl{}, because the 
# former may responsible for loading a new page from the trace file if 
# PagePool/ProxyTrace is used
Http/Client instproc gen-request {} {
	$self instvar pgtr_ rvInterPage_ id_

	if ![info exists pgtr_] {
		error "Http/Client requires a page generator (pgtr_)!"
	}

	# XXX 
	# rvInterPage_ should only be set when PagePool/ProxyTrace is 
	# *NOT* used
	if [info exists rvInterPage_] {
		return [list [$rvInterPage_ value] [$pgtr_ gen-pageid $id_]]
	} else {
		return [$pgtr_ gen-request $id_]
	}
}

Http/Client instproc next-request { server pageid } {
	$self instvar ns_ cache_ nextreq_

	if [info exists cache_] {
		$self send-request $cache_ GET $pageid
	} else {
		$self send-request $server GET $pageid
	}

	# Prepare for the next request
	set req [$self gen-request]
	set pageid $server:[lindex $req 1]
	set itvl [lindex $req 0]
	if {$itvl >= 0} {
		set nextreq_([$server id]) [$ns_ at [expr [$ns_ now] + $itvl] \
			"$self next-request $server $pageid"]
	} ;# otherwise it's the end of the request stream 
}

Http/Client instproc set-cache { cache } {
	$self instvar cache_
	set cache_ $cache
}

# Assuming everything is setup, this function starts sending requests
# Sending a request to $cache, the original page should come from $server
#
# Populate a cache with all available pages
# XXX how would we distribute pages spatially when we have a hierarchy 
# of caches? Or, how would we distribute client requests spatially?
# It should be used in single client, single cache and single server case
# *ONLY*.
Http/Client instproc start-session { cache server } {
	$self instvar ns_ cache_ simStartTime_

	$self instvar simStartTime_ pgtr_
	set simStartTime_ [$ns_ now]
	if [info exists pgtr_] {
		if {[$pgtr_ get-start-time] > $simStartTime_} {
			$pgtr_ set-start-time $simStartTime_
		}
	}

	#puts "Client [$self id] starts request session at [$ns_ now]"

	set cache_ $cache

	# The first time invocation we don't send out a request 
	# immediately. Instead, we find out when will the first
	# request happen, then schedule it using next-request{}
	# Then every time next-request{} is called later, it
	# sends out a request and schedule the next.
	set req [$self gen-request]
	set pageid $server:[lindex $req 1]
	set itvl [lindex $req 0]
	if {$itvl >= 0} {
		$ns_ at [expr [$ns_ now] + $itvl] \
			"$self next-request $server $pageid"
	} ;# otherwise it's the end of the request stream 
}

# Stop sending further requests, and clean all pending requests
Http/Client instproc stop-session { server } {
	$self instvar ns_ nextreq_ pending_ cache_
	set sid [$server id]

	# Clean up all pending requests
	if [info exists nextreq_($sid)] {
		$ns_ cancel $nextreq_($sid)
	}
	if {![info exists pending_]} {
		return
	}
	# XXX If a client either connects to a *SINGLE* cache 
	# or a *SINGLE* server, then we remove all pending requests 
	# when we are disconnected. 
	if {[info exists cache_] && ($server == $cache_)} {
		unset pending_
	} else {
		# Only remove pending requests related to $server
		foreach p [array names pending_] {
			if {$server == [lindex [split $p :] 0]} {
				unset pending_($p)
			}
		}
	}
}

Http/Client instproc populate { cache server } {
	$self instvar pgtr_ curpage_ status_ ns_

	if ![info exists status_] {
		set status_ "POPULATE"
		set curpage_ 0
	}

	if [info exists pgtr_] {
		# Populate cache with all pages incrementally
		if {$curpage_ < [$pgtr_ get-poolsize]} {
			$self send-request $cache GET $server:$curpage_
			incr curpage_
			$ns_ at [expr [$ns_ now] + 1] \
				"$self populate $cache $server"
#			if {$curpage_ % 1000 == 0} {
#				puts "Client [$self id] populated $curpage_"
#			}
			return
		}
	}

	# Start sending requests
	$ns_ at [expr [$ns_ now] + 10] "$self start-session $cache $server"
}

Http/Client instproc start { cache server } {
	$self instvar cache_
	set cache_ $cache
	$self populate $cache $server
}

Http/Client instproc request-mpush { page } {
	$self instvar mpush_refresh_ ns_ cache_
	$self send $cache_ [$self get-mpusize] \
		"$cache_ request-mpush $page"
	Http/Client instvar hb_interval_
	set mpush_refresh_($page) [$ns_ at [expr [$ns_ now] + $hb_interval_] \
		"$self send-refresh-mpush $page"]
}

Http/Client instproc send-refresh-mpush { page } {
	$self instvar mpush_refresh_ ns_ cache_
	$self send $cache_ [$self get-mpusize] "$cache_ refresh-mpush $page"
	#puts "[$ns_ now]: Client [$self id] send mpush refresh"
	Http/Client instvar hb_interval_
	set mpush_refresh_($page) [$ns_ at [expr [$ns_ now] + $hb_interval_] \
		"$self send-refresh-mpush $page"]
}

# XXX We use explicit teardown. 
Http/Client instproc stop-mpush { page } {
	$self instvar mpush_refresh_ ns_ cache_

	if [info exists mpush_refresh_($page)] {
		# Stop sending the periodic refreshment
		$ns_ cancel $mpush_refresh_($page)
		#puts "[$ns_ now]: Client [$self id] stops mpush"
	} else {
		error "no mpush to cancel!"
	}
	# Send explicit message up to tear down
	$self send $cache_ [$self get-mpusize] "$cache_ stop-mpush $page"
}


#----------------------------------------------------------------------
# Client which is capable of handling compound pages
#----------------------------------------------------------------------
Class Http/Client/Compound -superclass Http/Client

Http/Client/Compound instproc set-interobj-generator { ranvar } {
	$self instvar rvInterObj_
	set rvInterObj_ $ranvar
}

# Generate next page request and the number of embedded objects for 
# this page. 
# Use rvInterPage_ to generate interval between pages (inactive OFF), and 
# use rvInterObj_ to generate interval between embedded objects (active OFF)
Http/Client/Compound instproc next-request { server pageid } {
	# First schedule next page request
	eval $self next $server $pageid

}

# Request the next embedded object(s)
Http/Client/Compound instproc next-obj { server args } {
	$self instvar pgtr_ cache_ req_objs_ ns_ rvInterObj_

	if ![llength $args] {
		# No requests to make
		return
	}

	if [info exists cache_] {
		set dest $cache_
	} else {
		set dest $server
	}

	set pageid [lindex $args 0]
	set mpgid [$pgtr_ get-mainpage $pageid] ;# main page id
	set max 0
	set origsvr [lindex [split $pageid :] 0]

	foreach pageid $args {
		set id [lindex [split $pageid :] 1]
		if {$max < $id} {
			set max $id
		}
		incr req_objs_($mpgid) -1
		$self send-request $dest GET $pageid
	}
	if {$req_objs_($mpgid) <= 0} {
		# We have requested all objects for this page, done.
		return
	}

	# Schedule next requests. Assuming we get embedded objects according
	# to the ascending order of their page id
	set objid [join [$pgtr_ get-next-objs $origsvr:$max]]
	puts "At [$ns_ now], client [$self id] get objs $objid"
	if [info exists rvInterObj_] {
		$ns_ at [expr [$ns_ now] + [$rvInterObj_ value]] \
			"$self next-obj $server $objid"
	} else {
		$self next-obj $server $objid
	}
}

# XXX We need to override get-response-GET because we need to recompute
#     the RCV time, and the STA time, etc. 
# XXX Allow only *ONE* compound page.
Http/Client/Compound instproc get-response-GET { server pageid args } {
	$self instvar pending_ id_ ns_ recv_objs_ max_stale_ stat_ \
			simStartTime_ pgtr_

	if ![info exists pending_($pageid)] {
		error "Client $id_: Unrequested response page $pageid from server/cache [$server id]"
	}

	# Check if this is the main page
	if [$pgtr_ is-mainpage $pageid] {
		set mpgid $pageid
		# Get all the embedded objects, do "active OFF" delay 
		# if available
		$self instvar req_objs_ recv_objs_ rvInterObj_
		# Objects that are to be received
		set recv_objs_($pageid) [$pgtr_ get-obj-num $pageid] 
		# Objects that have been requested
		set req_objs_($pageid) $recv_objs_($pageid) 
		set objid [join [$pgtr_ get-next-objs $pageid]]
		if [info exists rvInterObj_] {
			$ns_ at [expr [$ns_ now] + [$rvInterObj_ value]] \
					"$self next-obj $server $objid"
		} else {
			eval $self next-obj $server $objid
		}
	} else {
		# Main page id
		set mpgid [$pgtr_ get-mainpage $pageid]
	}

	array set data $args

	# Check stale hits and record maximum stale hit time
	set origsvr [lindex [split $pageid :] 0]
	set modtime [$origsvr get-modtime $pageid]
	set reqtime [lindex $pending_($pageid) 0]
	set reqrtt [expr [$ns_ now] - $reqtime]
	# XXX If a stale hit occurs because a page is modified during the RTT
	# of the request, we should *NOT* consider it a stale hit. We 
	# implement it by ignoring all stale hits whose modification time is
	# larger than the request time. 
	#
	# See Http/Client::get-response-GET{}
	if {$modtime > $data(modtime)} {
		$self instvar ns_
		# Staleness is the time from now to the time it's last modified
		set tmp [$origsvr stale-time $pageid $data(modtime)]
		if {$tmp > $reqrtt/2} {
			if ![info exists max_stale_($mpgid)] {
				set max_stale_($mpgid) $tmp
			} elseif {$max_stale_($mpgid) < $tmp} {
				set max_stale_($mpgid) $tmp
			}
		}
	}

	# Wait for the embedded objects if this is a main page
	if [$pgtr_ is-mainpage $pageid] {
		return
	}

	# Delete pending record of all embedded objects, but not the main page;
	# we need it later to compute the response time, etc.
	# XXX assuming only one request per object
	$self evTrace C RCV p $pageid s [$server id] l $reqrtt z $data(size)
	unset pending_($pageid)

	# Check if we have any pending embedded objects
	incr recv_objs_($mpgid) -1
	if {$recv_objs_($mpgid) > 0} {
		# We are waiting for more objects to come
		return
	}

	# Now we've received all objects
	$self instvar pgtr_
	# Record response time for the entire compound page
	set reqtime [lindex $pending_($mpgid) 0]
	$self evTrace C RCV p $mpgid s [$origsvr id] l \
			[expr [$ns_ now] - $reqtime] z $data(size)
	# We are done with this page
	unset pending_($mpgid)

	if [info exists simStartTime_] {
		set tmp [expr [$ns_ now] - $reqtime]
		set stat_(rep-time) [expr $stat_(rep-time) + $tmp]
		if {$stat_(rt-min) > $tmp} {
			set stat_(rt-min) $tmp
		}
		if {$stat_(rt-max) < $tmp} {
			set stat_(rt-max) $tmp
		}
		unset tmp
	}
	if [info exists max_stale_($mpgid)] {
		$self evTrace C STA p $mpgid s [$origsvr id] \
				l $max_stale_($mpgid)
		if [info exists simStartTime_] {
			incr stat_(stale-num)
			set stat_(stale-time) [expr \
				$stat_(stale-time) + $max_stale_($mpgid)]
			if {$stat_(st-min) > $max_stale_($mpgid)} {
				set stat_(st-min) $max_stale_($mpgid)
			}
			if {$stat_(st-max) < $max_stale_($mpgid)} {
				set stat_(st-max) $max_stale_($mpgid)
			}
		}
		unset max_stale_($mpgid)
	}
	$self mark-response $mpgid
}

Http/Client/Compound instproc mark-request { pageid } {
	set id [lindex [split $pageid :] end]
	if {$id == 0} {
		$self next $pageid
	}
}

