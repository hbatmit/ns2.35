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
# Implementation of web cache, client and server which support 
# multimedia objects.
#
# $Header: /cvsroot/nsnam/ns-2/tcl/webcache/http-mcache.tcl,v 1.6 2005/09/16 03:05:47 tomh Exp $

#
# Multimedia web client
#

# Override this method to change the default max size of the page pool
Http/Client/Media instproc create-pagepool {} {
	set pool [new PagePool/Client/Media]
	$self set-pagepool $pool
	return $pool
}

# Connection procedure:
# (1) Client ---(HTTP_REQUEST)--> Server
# (2) Client <--(HTTP_RESPONSE)-- Server
# (3) Client <----(RAP_DATA)----  Server
Http/Client/Media instproc get-response-GET { server pageid args } {
	eval $self next $server $pageid $args
	
	# XXX Enter the page into page pool, so that we can keep track of 
	# which segment has been received and know where to stop receiving. 
	if [$self exist-page $pageid] {
		error "Http/Client/Media: receives an \"active\" page!"
	}
	eval $self enter-page $pageid $args

	array set data $args
	if {[info exists data(pgtype)] && ($data(pgtype) == "MEDIA")} {
		# Create a multimedia connection to server
		$self media-connect $server $pageid
	}
}

# Don't accept an request if there is one stream still on transit
Http/Client/Media instproc send-request { server type pageid args } {
	$self instvar mmapp_ 
	if [info exists mmapp_($pageid)] {
		return
	}
	eval $self next $server $type $pageid $args
}

# Create a RAP connection to a server
Http/Client/Media instproc media-connect { server pageid } {

	# DEBUG ONLY
	#puts "Client [$self id] media-connect [$server id] $pageid"

	$self instvar mmapp_ ns_ node_ 
	Http instvar MEDIA_TRANSPORT_ MEDIA_APP_
	if [info exists mmapp_($pageid)] {
		puts "Media client [$self id] got a request for an existing stream"
		return
	}
	set agent [new Agent/$MEDIA_TRANSPORT_]
	$ns_ attach-agent $node_ $agent
	set app [new Application/$MEDIA_APP_ $pageid]
	$app attach-agent $agent
	$app target $self
	$server alloc-mcon $self $pageid $agent
	set mmapp_($pageid) $app
	$app set-layer [$self get-layer $pageid]
}

Http/Client/Media instproc media-disconnect { server pageid } {
	$self instvar mmapp_ ns_ node_

	if {![info exists mmapp_($pageid)]} {
		error "Media client [$self id] disconnect: not connected to \
server [$server id] with page $pageid"
	}
	set app $mmapp_($pageid)
	set agent [$app agent]
	$ns_ detach-agent $node_ $agent

	# DEBUG ONLY
#	puts "Client [$self id] disconnect from server [$server id]"

	# Disconnect the agent and app at the server side
	$server media-disconnect $self $pageid

	delete $agent
	delete $app
	unset mmapp_($pageid)

	$self stream-received $pageid
}


#
# Multimedia web server
#

# Create media pages instead of normal pages
Http/Server/Media instproc gen-page { pageid } {
	$self instvar pgtr_ 
	set pginfo [$self next $pageid]
	if [$pgtr_ is-media-page $pageid] {
		return [lappend pginfo pgtype MEDIA]
	} else {
		return $pginfo
	}
}

Http/Server/Media instproc create-pagepool {} {
	set pool [new PagePool/Client/Media]
	$self set-pagepool $pool
	# Set the pool size to "infinity" (2^31-1)
	$pool set max_size_ 2147483647
	return $pool
}

Http/Server/Media instproc medialog-on {} {
	$self instvar MediaLog_
	set MediaLog_ 1
}

# Allocate a media connection
Http/Server/Media instproc alloc-mcon { client pageid dst_agent } {
	$self instvar ns_ node_ mmapp_ 
	Http instvar MEDIA_TRANSPORT_ MEDIA_APP_

	set agent [new Agent/$MEDIA_TRANSPORT_]
	$ns_ attach-agent $node_ $agent
	set app [new Application/$MEDIA_APP_ $pageid]
	$app attach-agent $agent
	$app target $self 
	set mmapp_($client/$pageid) $app
	# Set layers
	$app set-layer [$self get-layer $pageid]

	# Associate $app with $client and $pageid
	$self register-client $app $client $pageid

	# DEBUG ONLY
	# Logging MediaApps, only do it for sender-side media apps
	$self instvar MediaLog_
	if [info exists MediaLog_] {
		set lf [$self log]
		if {$lf != ""} {
			$app log $lf
		}
	}
#	puts "Server [$self id] allocated a connection to client [$client id] using agent $agent"

	# Connect two RAP agents and start data transmission
	$ns_ connect $agent $dst_agent
	$agent start
}

Http/Server/Media instproc media-disconnect { client pageid } { 
	$self instvar mmapp_ ns_ node_

	# DEBUG ONLY
#	puts "At [$ns_ now] Server [$self id] disconnected from client [$client id]"

	if {![info exists mmapp_($client/$pageid)]} {
		error "Media server [$self id] disconnect: not connected to \
client [$client id] with page $pageid"
	}
	set app $mmapp_($client/$pageid)
	set agent [$app agent]
	$ns_ detach-agent $node_ $agent

	$self unregister-client $app $client $pageid

	# DEBUG ONLY 
#	puts "Server [$self id] deleting agent $agent"

	delete $agent
	delete $app
	unset mmapp_($client/$pageid)
}

# Tell the client that a stream has been completed. Assume that there is 
# an open connection between server and client
Http/Server/Media instproc finish-stream { app } {
	$self instvar mmapp_ 
	foreach n [array names mmapp_] {
		if {$mmapp_($n) == $app} {
			set tmp [split $n /]
			set client [lindex $tmp 0]
			set pageid [lindex $tmp 1]
			$self send $client [$self get-reqsize] \
				"$client media-disconnect $self $pageid"
			return
		}
	}
}

# If the page is a media page, set the response size to that of the request
Http/Server/Media instproc handle-request-GET { pageid args } {
	set pginfo [eval $self next $pageid $args]
	if {[$self get-pagetype $pageid] == "MEDIA"} {
		set pginfo [lreplace $pginfo 0 0 [$self get-reqsize]]
	}
	return $pginfo
}

Http/Server/Media instproc gen-pageinfo { pageid } {
	set pginfo [eval $self next $pageid]
	# Create contents for media page
	$self instvar pgtr_
	if [$pgtr_ is-media-page $pageid] {
		return [lappend pginfo pgtype MEDIA layer \
				[$pgtr_ get-layer $pageid]]
	} else {
		return $pginfo
	}
}

# Map a media application to the HTTP application at the other end
#Http/Server/Media instproc map-media-app { app } {
#	$self instvar mmapp_
#	foreach n [array names mmapp_] {
#		if {$mmapp_($n) == $app} {
#			return [lindex [split $n /] 0]
#		}
#	}
#}

# Handle prefetching of designated segments
Http/Server/Media instproc get-request { client type pageid args } {
	if {$type == "PREFSEG"} {
		# XXX allow only one prefetching stream from any single client
		# Records this client as doing prefetching. 
		# FAKE a connection to the client without prior negotiation
		set pagenum [lindex [split $pageid :] 1]
		set conid [lindex $args 0]
		set layer [lindex $args 1]
		set seglist [lrange $args 2 end]
		eval $self register-prefetch $client $pagenum $conid \
				$layer $seglist
		$client start-prefetch $self $pageid $conid
		$self evTrace S PREF p $pageid l $layer [join $seglist]
		# DEBUG only
#		$self instvar ns_
#		puts "At [$ns_ now] server [$self id] prefetches $args"
	} elseif {$type == "STOPPREF"} {
		set pagenum [lindex [split $pageid :] 1]
		set conid [lindex $args 0]
		# DEBUG only
#		$self instvar ns_
#		puts "At [$ns_ now] server [$self id] stops prefetching $pageid conid $conid"
		if [$self stop-prefetching $client $conid $pagenum] {
			# Tear down pref channel iff we don't have remaining 
			# clients sharing the channel
			$client media-disconnect $self $pageid $conid
		}
	} elseif {$type == "OFFLPREF"} {
		# Simply send the page back through TCP channel
		if ![$self exist-page $pageid] {
			error "Server [$self id] offline-prefetch non-existent page $pageid!"
		}
		set size [$self get-size $pageid]
		$self send $client $size "$client offline-complete $pageid"
	} else {
		eval $self next $client $type $pageid $args
	}
}



#
# Multimedia web cache
#

Http/Cache/Media instproc create-pagepool {} {
	set pool [new PagePool/Client/Media]
	$self set-pagepool $pool
	return $pool
}

Http/Cache/Media instproc start-prefetch { server pageid conid } {
	$self instvar pref_ ns_
	if [info exists pref_($server/$pageid)] {
		# We are already connected to the server
		if {[lsearch -exact $pref_($server/$pageid) $conid] == -1} {
#			puts "At [$ns_ now] cache [$self id] RE-REQUESTs prefetching to server [$server id] for page $pageid"
			lappend pref_($server/$pageid) $conid
		}
		return
	} else {
		# Whenever there is a prefetching request to the server, 
		# increase the "ref counter" by one. Then we delete it 
		# when it's 0.
		lappend pref_($server/$pageid) $conid
	}
#	puts "At [$ns_ now] cache [$self id] starts prefetching to [$server id] for page $pageid conid $conid"
	# XXX Do not use QA for prefetching!!
	Http instvar MEDIA_APP_
	set oldapp $MEDIA_APP_
	# XXX Do not use the default initial RTT of RAP. We must start sending
	# as soon as possible, then drop our rate if necessary. Instead, set
	# initial ipg_ and srtt_ to 10ms.
	set oldipg [Agent/RAP set ipg_]
	set oldsrtt [Agent/RAP set srtt_]
	Agent/RAP set ipg_ 0.01
	Agent/RAP set srtt_ 0.01
	set MEDIA_APP_ MediaApp
	$self media-connect $server $pageid
	set MEDIA_APP_ $oldapp
	Agent/RAP set ipg_ $oldipg
	Agent/RAP set srtt_ $oldsrtt
}

Http/Cache/Media instproc media-connect { server pageid } {
	$self instvar s_mmapp_ ns_ node_ 

	# DEBUG ONLY
#	puts "At [$ns_ now], cache [$self id] connects to [$server id] for page $pageid"

	Http instvar MEDIA_TRANSPORT_ MEDIA_APP_
	if [info exists s_mmapp_($server/$pageid)] {
		error "Media client [$self id] got a request for an existing \
stream"
	}
	set agent [new Agent/$MEDIA_TRANSPORT_]
	$ns_ attach-agent $node_ $agent
	set app [new Application/$MEDIA_APP_ $pageid]
	$app attach-agent $agent
	$app target $self
	$server alloc-mcon $self $pageid $agent
	set s_mmapp_($server/$pageid) $app
	$app set-layer [$self get-layer $pageid]
}

Http/Cache/Media instproc alloc-mcon { client pageid dst_agent } {
	$self instvar ns_ node_ c_mmapp_ 
	Http instvar MEDIA_TRANSPORT_ MEDIA_APP_
	if [info exists c_mmapp_($client/$pageid)] {
		error "Media cache [$self id] got a request for an existing \
stream $pageid from client [$client id]"
	}

	set agent [new Agent/$MEDIA_TRANSPORT_]
	$ns_ attach-agent $node_ $agent
	set app [new Application/$MEDIA_APP_ $pageid]
	$app attach-agent $agent
	$app target $self 
	set c_mmapp_($client/$pageid) $app
	# Set layers
	$app set-layer [$self get-layer $pageid]

	# Associate $app with $client and $pageid
	$self register-client $app $client $pageid

	# Logging MediaApps, only do it for sender-side media apps
	$self instvar MediaLog_
	if [info exists MediaLog_] {
		set lf [$self log]
		if {$lf != ""} {
			$app log $lf
		}
	}

	# Connect two RAP agents and start data transmission
	$ns_ connect $agent $dst_agent
	$agent start
}

# Turn on logging of media application
Http/Cache/Media instproc medialog-on {} {
	$self instvar MediaLog_
	set MediaLog_ 1
}

Http/Cache/Media instproc media-disconnect { host pageid args } {
	$self instvar c_mmapp_ s_mmapp_ ns_ node_ pref_ c_tbt_

	# Flags: is it client disconnection or server? 
	set cntdisco 0 
	set svrdisco 0

	set server [lindex [split $pageid :] 0]

	if {($host != $server) && [info exists c_mmapp_($host/$pageid)]} {
		# Disconnect from a client
		set app $c_mmapp_($host/$pageid)
		set agent [$app agent]
		$ns_ detach-agent $node_ $agent
		$self unregister-client $app $host $pageid
		# DEBUG ONLY
#		puts "At [$ns_ now] Cache [$self id] disconnect from \
#				client [$host id]"

		# Do NOT delete client-side agent and app until we've 
		# torn down prefetching connection to the server. 
		# XXX But first check that this client has indeed been 
		# involved in prefetching!
		if {[info exists pref_($server/$pageid)] && \
			[lsearch -exact $pref_($server/$pageid) $app] != -1} {
			# If we have concurrent requests and other clients
			# may still be using this prefetching channel. We
			# tell the server that this client has stopped so 
			# that server will remove all related state.
#			puts "At [$ns_ now] Cache [$self id] stops prefetching for conid $app, page $pageid, from server [$server id]"
			$self send $server [$self get-reqsize] "$server get-request $self STOPPREF $pageid $app"
			# Stop application-related timers
			set c_tbt_($host/$pageid) $app
			$app stop
		} else {
			delete $app
		}
		delete $agent
		unset c_mmapp_($host/$pageid)

		# Dump status of all pages after serving a client
		$self instvar pool_
		foreach p [lsort [$pool_ list-pages]] {
			$self dump-page $p
		}
		set cntdisco 1

	} elseif [info exists s_mmapp_($host/$pageid)] {
		# XXX This assumes that we are NOT connecting to another cache.
		# Stop prefetching when we requested that at the server. 
		#
		# Also, we assume that while the cache is downloading from 
		# the server during the first retrieval, no prefetching should
		# be used. This is guaranteed by the way MEDIAREQ_CHECKSEG is 
		# handled in mcache.cc:680.
		#
		# Note that (1) this always happens AFTER client disconnection,
		# (2) whenever there is a prefetching connection, there's 
		# always a corresponding entry in s_mmapp_().
		#
		# If we are disconnecting a prefetching channel, check if 
		# we still have other clients sharing the channel. If not,
		# then we tear down the channel. 
		set svrdisco 1
		if [info exists pref_($server/$pageid)] {
			# By default we don't tear down pref channel
			set teardown 0
			# Actually the MediaApp for the client
			set conid [lindex $args 0]
			set pos [lsearch -exact $pref_($server/$pageid) $conid]
			if {$pos == -1} {
				error "media-disconnect cannot find $conid!!"
			}
			set pref_($server/$pageid) [lreplace \
				$pref_($server/$pageid) $pos $pos]
			if {[llength $pref_($server/$pageid)] == 0} {
				$self evTrace E STP s [$server id] p $pageid
				unset pref_($server/$pageid)
				# Now we can tear down the pref channel
				set teardown 1
			}
			# Tear down client-side connection (i.e.,conid)
#			puts "At [$ns_ now] cache [$self id] stops prefetching\
# to [$server id] for client $conid"
			delete $conid
		} else {
			# If no pref, tear down by default
			set teardown 1
		}
		if {$teardown} {
			# Disconnect from a server
			set app $s_mmapp_($host/$pageid)
			set agent [$app agent]
			$ns_ detach-agent $node_ $agent
			$host media-disconnect $self $pageid
			# DEBUG ONLY
#			puts "At [$ns_ now] Cache [$self id] disconnect from \
#				server [$host id]"
			delete $agent
			delete $app
			unset s_mmapp_($host/$pageid)
		}
		# Use the TCP channel between the cache and the server to 
		# transmit the entire file. Note since we no longer do 
		# online prefetching, this channel is pretty much empty
		# and we can occupy it as long as we want. 
		$self instvar firstreq_
		if {([$self get-pref-style] == "OFFLINE_PREF") && \
				[info exists firstreq_($pageid)]} { 
			$self send $server [$self get-reqsize] \
				"$server get-request $self OFFLPREF $pageid"
		}
		if [info exists firstreq_($pageid)] {
			unset firstreq_($pageid)
		}
	} else {
		error "At [$ns_ now] Media cache [$self id] tries to \
			disconnect from a non-connected host [$host id]"
	}

	# XXX Only do this when we disconnect from a server. 
	if {$svrdisco == 1} {
		$self stream-received $pageid
	}
}

# Tell the client that a stream has been completed. Assume that there is 
# an open connection between server and client
Http/Cache/Media instproc finish-stream { app } {
	$self instvar c_mmapp_ s_mmapp_
	foreach n [array names c_mmapp_] {
		if {$c_mmapp_($n) == $app} {
			set tmp [split $n /]
			set client [lindex $tmp 0]
			set pageid [lindex $tmp 1]
			$self send $client [$self get-reqsize] \
				"$client media-disconnect $self $pageid"
			return
		}
	}
}

Http/Cache/Media instproc get-response-GET { server pageid args } {
	# Whether this is the first request. If offline prefetching is used, 
	# media-disconnect{} uses this information to decide whether to 
	# prefetch the entire stream. 
	$self instvar firstreq_
	if ![$self exist-page $pageid] {
		set firstreq_($pageid) 1
	}

	eval $self next $server $pageid $args

	# DEBUG ONLY
#	puts "Cache [$self id] gets response"

	array set data $args
	if {[info exists data(pgtype)] && ($data(pgtype) == "MEDIA")} {
		# Create a multimedia connection to server
		$self media-connect $server $pageid
	}
}

# XXX If it's a media page, change page size and send back a small response. 
# This response is intended to establish a RAP connection from client to 
# cache. 
Http/Cache/Media instproc answer-request-GET { cl pageid args } {
	array set data $args
	if {[info exists data(pgtype)] && ($data(pgtype) == "MEDIA")} {
		set size [$self get-reqsize]
	} else {
		set size $data(size)
	}
	$self send $cl $size \
		"$cl get-response-GET $self $pageid $args"
	$self evTrace E SND c [$cl id] p $pageid z $data(size)
}

# $args is a list of segments, each of which is a list of 
# two elements: {start end}
Http/Cache/Media instproc pref-segment {conid pageid layer args} {
	# XXX directly send a request to the SERVER. Assuming that 
	# we are not going through another cache. 
	set server [lindex [split $pageid :] 0]
	set size [$self get-reqsize]
	$self send $server $size "$server get-request $self PREFSEG \
		$pageid $conid $layer [join $args]"
}

Http/Cache/Media instproc set-repl-style { style } {
	$self instvar pool_
	$pool_ set-repl-style $style
}

