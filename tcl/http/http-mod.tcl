# Part of the work for the summer intern at AT&T Labs-Research
# Code contributed by Polly Huang, http://www-scf.usc.edu/~bhuang
# phuang@research.att.com, huang@isi.edu
# Ported from UCB Empirical HTTP code, http.tcl

#puts "WARNING: Obsoleted by PagePool/WebTraf."
#puts "See ~ns/tcl/webcache/webtraf.{h,cc} and web-traffic.tcl in tcl/ex"

##################### Class: HttpSession #######################
Agent/CBR set maxpkts_ 0
Class HttpSession
HttpSession set sessionId_ 1

HttpSession instproc init { ns numPage sessionSrc } {
    $self instvar httpPages_ numPage_ interPage_ pageSize_  donePage_
    $self instvar ns_ sessionId_ sessionSrc_
    $self instvar tcpType_ tcpSinkType_

    set ns_ $ns
    set numPage_ $numPage
    set donePage_ 0
    set sessionId_ [HttpSession set sessionId_]
    HttpSession set sessionId_ [expr $sessionId_ + 1]
    set sessionSrc_ $sessionSrc
    set tcpType_ TCP/Reno
    set tcpSinkType_ TCPSink

    # default interPage_ interval to 1 second/page
    if ![info exist interPage_] {
	set interPage_ [new RandomVariable/Constant]
	$interPage_ set val_ 1
    }
    # default pageSize_ to 2 objects/page
    if ![info exist pageSize_] {
	set pageSize_ [new RandomVariable/Constant]
	$pageSize_ set val_ 2
    }
}

HttpSession instproc disable-reliability {} {
    $self instvar disable_reliability_

    set disable_reliability_ 1
}

HttpSession instproc disable-flow-control windowInit {
    $self instvar disable_flow_control_ windowInit_    

    set disable_flow_control_ 1
    set windowInit_ $windowInit
}

HttpSession instproc createPage {} {
    $self instvar httpPages_ numPage_ pageSize_ 
    $self instvar ns_ sessionId_ sessionSrc_
    $self instvar tcpType_ tcpSinkType_
    $self instvar disable_reliability_ disable_flow_control_ windowInit_

    for {set i 0} {$i < $numPage_} {incr i} {
	set httpPages_($i) [new HttpPage $ns_ $sessionId_]
	$httpPages_($i) set numObject_ [$pageSize_ value]
	$httpPages_($i) set pageSrc_ $sessionSrc_
	$httpPages_($i) set sessionManager_ $self
	# puts "HttpSession::createPage:$tcpType_ $tcpSinkType_"
	$httpPages_($i) set tcpType_ $tcpType_
	$httpPages_($i) set tcpSinkType_ $tcpSinkType_
	if {[info exist disable_reliability_] && $disable_reliability_} {
	    $httpPages_($i) set disable_reliability_ 1
	    # puts "HttpSession::createPage: disable_reliability_ $disable_reliability_"
	}
	if {[info exist disable_flow_control_] && $disable_flow_control_} {
	    $httpPages_($i) set disable_flow_control_ 1
	    $httpPages_($i) set windowInit_ $windowInit_
	    # puts "HttpSession::createPage: disable_flow_control_ $disable_flow_control_ windowInit_ $windowInit_"
	}
    }
}

HttpSession instproc start {} {
    $self instvar httpPages_ numPage_ interPage_ 
    $self instvar ns_ 

    set launchTime [$ns_ now]
    for {set i 0} {$i < $numPage_} {incr i} {
	$ns_ at $launchTime "$httpPages_($i) start"
	set launchTime [expr $launchTime + [$interPage_ value]]
    }
}


HttpSession instproc setDistribution { var distribution args } {
    $self instvar httpPages_

    ## Create random model object
    set model [new RandomVariable/$distribution]
    switch $distribution {
	Constant {$model set val_ [lindex $args 0]}
	Uniform  {
	    $model set max_ [lindex $args 0] 
	    $model set min_ [lindex $args 1]
	}
	Exponential {$model set avg_ [lindex $args 0]}
	Pareto  {
	    $model set avg_ [lindex $args 0] 
	    $model set shape_ [lindex $args 1]
	}
	ParetoII  {
	    $model set avg_ [lindex $args 0] 
	    $model set shape_ [lindex $args 1]
	}
	TraceDriven  {$model set filename_ [lindex $args 0]}
    }

    ## Assign variables with the random model
    switch $var {
	interPage_ {$self set $var $model}
	pageSize_  {$self set $var $model}
	interObject_ {
	    foreach index [array name httpPages_] {
		$httpPages_($index) set $var $model
		$self set interObject_ $model
	    }
	}
	objectSize_  {
	    foreach index [array name httpPages_] {
		$httpPages_($index) set $var $model
		$self set objectSize_ $model
	    }
	}
    }
}

HttpSession instproc doneOnePage {} {
    $self instvar interPage_ pageSize_ numPage_ donePage_ 
    $self instvar interObject_ objectSize_

    incr donePage_
    # puts "doneOnePage: $numPage_ $donePage_"
    if {$donePage_ == $numPage_} {    
	delete $interPage_ 
	delete $pageSize_
	if {[info exist interObject_]} {
	    delete $interObject_
	}
	if {[info exist objectSize_]} {
	    delete $objectSize_
	}
	delete $self
    }
}

##################### Class: HttpPage ###########################
Class HttpPage
HttpPage set pageId_ 1

HttpPage instproc init { ns sessionId } {
    $self instvar httpObjects_ numObject_ interObject_ objectSize_ 
    $self instvar ns_ sessionId_ pageId_ curObject_ doneObject_
    $self instvar tcpType_ tcpSinkType_

    set ns_ $ns
    set sessionId_ $sessionId
    set pageId_ [HttpPage set pageId_]
    HttpPage set pageId_ [expr $pageId_ + 1]
    set tcpType_ TCP/Reno
    set tcpSinkType_ TCPSink

    # default numObject_ to 1 object/session
    set numObject_ 1
    set curObject_ 0
    set doneObject_ 0
    # default interObject_ interval to 1 second/object
    if ![info exist interObject_] {
	set interObject_ [new RandomVariable/Constant]
	$interObject_ set val_ 0.5
    }
    # default objectSize_ to 5 packets/object
    if ![info exist objectSize_] {
	set objectSize_ [new RandomVariable/Constant]
	$objectSize_ set val_ 5
    }
}

HttpPage instproc start {} {
    $self instvar httpObjects_ numObject_ interObject_ objectSize_ 
    $self instvar ns_ pageSrc_ sessionManager_ pageId_ sessionId_
    $self instvar curObject_ tcpType_ tcpSinkType_
    $self instvar disable_reliability_ disable_flow_control_ windowInit_

    if {$curObject_ < $numObject_} {
	set httpObjects_($curObject_) [new HttpObject $ns_ $pageSrc_ [$ns_ pickdst] $pageId_ $sessionId_ $tcpType_ $tcpSinkType_]
	$httpObjects_($curObject_) set numPacket_ [$objectSize_ value]
	$httpObjects_($curObject_) set pageManager_ $self
	$httpObjects_($curObject_) set sessionManager_ $sessionManager_
	if {[info exist disable_reliability_] && $disable_reliability_} {
	    $httpObjects_($curObject_) set disable_reliability_ 1
	    # puts "HttpPage::start: disable_reliability $disable_reliability_"
	}
	if {[info exist disable_flow_control_] && $disable_flow_control_} {
	    $httpObjects_($curObject_) set disable_flow_control_ 1
	    $httpObjects_($curObject_) set windowInit_ $windowInit_
	    # puts "HttpPage::start: disable_flow_control_ $disable_flow_control_ windowInit_ $windowInit_"
	}
	$httpObjects_($curObject_) start
	incr curObject_
	$ns_ at [expr [$ns_ now] + [$interObject_ value]] "$self start"
    }
}

HttpPage instproc doneOneObject {} {
    $self instvar interObject_ objectSize_ doneObject_ numObject_
    $self instvar sessionManager_

    incr doneObject_
    # puts "doneOneObject: $numObject_ $doneObject_"
    if {$doneObject_ == $numObject_} {
	#delete $interObject_
	#delete $objectSize_
	$sessionManager_ doneOnePage
	delete $self
    }
}

##################### Class: HttpObject ##############################
Class HttpObject -superclass InitObject
HttpObject set objectId_ 1

HttpObject instproc init { ns src dst pageId sessionId tcpType tcpSinkType} {
    $self instvar numPacket_ ns_ tcpType_ tcpSinkType_
    $self instvar clientSrc_ serverSrc_  clientSink_ serverSink_
    $self instvar clientNode_ serverNode_ clientTCP_ serverTCP_
    $self instvar sessionManager_ objectSrc_ objectId_ pageId_ sessionId_
    $self instvar clientSinkRcvPktCount_

    set ns_ $ns
    set pageId_ $pageId
    set sessionId_ $sessionId
    set objectId_ [HttpObject set objectId_]
    HttpObject set objectId_ [expr $objectId_ + 1]

    # default numObject_ to 1 object/session
    set numPacket_ 1
    set tcpType_ $tcpType
    set tcpSinkType_ $tcpSinkType
    set clientNode_ $src
    set serverNode_ $dst
#    $clientNode_ set idleTCP_ ""
#    $clientNode_ set idleTCPSink_ ""
#    $serverNode_ set idleTCP_ ""
#    $serverNode_ set idleTCPSink_ ""

    set clientSinkRcvPktCount_ 0

    # setup TCP connection
    set clientTCP_ [$clientNode_ pickTCP TCP/Reno]
    # puts "clientTCP $clientTCP_"

    # trace client TCP info
    $ns instvar clientchan_
    if [info exist clientchan_] {
	$clientTCP_ set trace_all_oneline_ true
	$clientTCP_ trace cwnd_
	$clientTCP_ attach [$ns set clientchan_]
    }

    set serverTCP_ [$serverNode_ pickTCP $tcpType_]
    # puts "serverTCP $serverTCP_"

    # trace server TCP info
    $ns instvar serverchan_
    if [info exist serverchan_] {
	$serverTCP_ set trace_all_oneline_ true
	$serverTCP_ trace cwnd_
	$serverTCP_ attach [$ns set serverchan_]
    }

    $clientTCP_ set fid_ $objectId_
    $serverTCP_ set fid_ $objectId_

    set clientSink_ [$serverNode_ pickTCPSink TCPSink]
    set serverSink_ [$clientNode_ pickTCPSink $tcpSinkType_]
    set clientSrc_ [$self newXfer FTP $clientNode_ $serverNode_ $clientTCP_ $clientSink_]
    set serverSrc_ [$self newXfer FTP $serverNode_ $clientNode_ $serverTCP_ $serverSink_]

    $clientTCP_ proc done {} "$self doneRequest"
    $serverTCP_ proc done {} "$self doneReply"
}

HttpObject instproc start {} {
    $self instvar numPacket_ ns_ pageManager_ sessionManager_
    $self instvar clientNode_ clientTCP_ clientSrc_ serverTCP_
    $self instvar objectId_ pageId_ sessionId_
    $self instvar clientSink_ serverSink_
    $self instvar disable_reliability_ disable_flow_control_ windowInit_

    # puts "$numPacket_ \t $objectId_ \t $pageId_ \t $sessionId_ \t [$ns_ now]"

    if {[info exist disable_reliability_] && $disable_reliability_} {
	$clientTCP_ disable-reliability
	$clientSink_ disable-reliability 
	$serverTCP_ disable-reliability
	$serverSink_ disable-reliability 
    }
    if {[info exist disable_flow_control_] && $disable_flow_control_} {
	$clientTCP_ disable-flow-control
	$serverTCP_ disable-flow-control
	$clientTCP_ set windowInit_ $windowInit_
	$serverTCP_ set windowInit_ $windowInit_
    }

    $clientSrc_ producemore 1
}

HttpObject instproc newXfer {type src dst sa da} {
	$self instvar ns_
	$ns_ attach-agent $src $sa
	$ns_ attach-agent $dst $da
	$ns_ connect $sa $da
        set app [new Application/$type]
        $app attach-agent $sa
	return $app
}

HttpObject instproc doneRequest {} {
    $self instvar numPacket_ ns_
    $self instvar clientSrc_ serverSrc_  clientSink_ serverSink_
    $self instvar clientNode_ serverNode_ clientTCP_ serverTCP_

    # puts "doneRequest: server([$serverNode_ id]) replyin obj size($numPacket_) [$ns_ now]"
    $clientNode_ instvar idleTCP_
    $serverNode_ instvar idleTCPSink_

    if {![info exists idleTCP_] || [lsearch $idleTCP_ $clientTCP_] < 0} {
	lappend idleTCP_ $clientTCP_
	# puts "[$clientNode_ id] TCP doneRequest: append $clientTCP_ => $idleTCP_"
    } else {
	puts "[$clientNode_ id] doneRequest: using idle TCP $clientTCP_, $idleTCP_"
	exit
    }

    if {![info exists idleTCPSink_] || [lsearch $idleTCPSink_ $clientSink_] < 0} {
	lappend idleTCPSink_ $clientSink_
	# puts "[$serverNode_ id] TCPSInk doneRequest: append $clientSink_ => $idleTCPSink_"
    } else {
	puts "[$serverNode_ id] doneRequest: using idle TCP Sink $clientSink_, $idleTCPSink_"
	exit
    }
    # puts "$serverSrc_ [expr int(ceil($numPacket_))]"
    $serverSrc_ producemore [expr int(ceil($numPacket_))]
}

HttpObject instproc doneReply {} {
    $self instvar numPacket_ ns_
    $self instvar clientSrc_ serverSrc_  clientSink_ serverSink_
    $self instvar clientNode_ serverNode_ clientTCP_ serverTCP_ objectId_
    $self instvar pageManager_

    # puts "$objectId_ doneReply: server([$serverNode_ id]) client([$clientNode_ id]) replied obj size($numPacket_) [$ns_ now]"
    $serverNode_ instvar idleTCP_
    $clientNode_ instvar idleTCPSink_

    if {![info exists idleTCP_] || [lsearch $idleTCP_ $serverTCP_] < 0} {
	lappend idleTCP_ $serverTCP_
	# puts "[$serverNode_ id] TCP doneReply: append $serverTCP_ => $idleTCP_"
    } else {
	puts "[$serverNode_ id] doneReply: using idle TCP $serverTCP_, $idleTCP_"
	exit
    }

    if {![info exists idleTCPSink_] || [lsearch $idleTCPSink_ $serverSink_] < 0} {
	lappend idleTCPSink_ $serverSink_
	# puts "[$clientNode_ id] TCPSink doneReply: append $serverSink_ => $idleTCPSink_"
    } else {
	puts "[$clientNode_ id] doneReply: using idle TCP Sink $serverSink_, $idleTCPSink_"
	exit
    }
    $pageManager_ doneOneObject
    delete $self
}

#####################################################################
Node instproc pickTCP { type } {
    $self instvar idleTCP_
    if [info exist idleTCP_] {
	set i 0
	foreach TCP $idleTCP_ {
	    if {[$TCP info class] == "Agent/$type"} {
		set idleTCP_ [lreplace $idleTCP_ $i $i]
		# puts "[$self id] TCP pick(found): $TCP, $idleTCP_"
		$TCP reset
		return $TCP
	    }
	    incr i
	}
    }
    set TCP [new Agent/$type] 
    if [info exist idleTCP_] {
	# puts "[$self id] TCP pick(new): $TCP , $idleTCP_"
    } else {
	# puts "[$self id] TCP pick(new): $TCP"
    }
    return $TCP
}

Node instproc pickTCPSink { type } {
    $self instvar idleTCPSink_
    if [info exist idleTCPSink_] {
	set i 0
	foreach Sink $idleTCPSink_ {
	    if {[$Sink info class] == "Agent/$type"} {
		set idleTCPSink_ [lreplace $idleTCPSink_ $i $i]
		# puts "[$self id] TCPSink pick(found): $Sink, $idleTCPSink_"
		$Sink reset
		return $Sink
	    }
	    incr i
	}
    }
    set Sink [new Agent/$type]
    if [info exist idleTCPSink_] {
	# puts "[$self id] TCPSink pick(new): $Sink, $idleTCPSink_"
    } else {
	# puts "[$self id] TCPSink pick(new): $Sink"
    }
    return $Sink
}

#####################################################################
Simulator instproc picksrc {} {
    $self instvar Node_ src_
    global defaultRNG

    if {![info exist src_] || [llength $src_] == 0} {
	set tmp [$defaultRNG integer [Node set nn_]]
	return $Node_($tmp)
    } else {
	set round [llength $src_]
	set tmp [$defaultRNG integer $round]
	return $Node_([lindex $src_ $tmp])
    }
}

Simulator instproc roundrobinsrc {} {
    $self instvar Node_ src_ roundrobin_
    global defaultRNG

    if {![info exist src_] || [llength $src_] == 0} {
	set round [Node set nn_]
    } else {
	set round [llength $src_]
    }
    if ![info exist roundrobin_] {
	set roundrobin_ [$defaultRNG integer $round]
    }
    set roundrobin_ [expr [expr $roundrobin_ + 1] % $round]

    if {![info exist src_] || [llength $src_] == 0} {
	return $Node_($roundrobin_)
    } else {
	# puts "roundrobin: $roundrobin_"
	return $Node_([lindex $src_ $roundrobin_])
    }
}

Simulator instproc pickdst {} {
    $self instvar Node_ dst_
    global defaultRNG

    if {![info exist dst_] || [llength $dst_] == 0} {
	set round 0
	foreach index [array names Node_] {
	    incr round
	}
	set tmp [$defaultRNG integer $round]
	# puts "$round $tmp"
	return $Node_($tmp)
    } else {
	set round [llength $dst_]
	set tmp [$defaultRNG integer $round]
	# puts "$round $tmp"
	return $Node_([lindex $dst_ $tmp])
    }
}
