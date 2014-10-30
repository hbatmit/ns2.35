source ../../../ex/asym/util.tcl

set WebCScount 0

Class WebCS

proc createTransfer {ns src dst tcpargs tcptrace sinkargs session sessionargs doneargs xfersz} {
	set tcp [eval createTcpSource $tcpargs]
	$tcp proc done {} " $doneargs"
	if {$xfersz < 1} {
		$tcp set packetSize_ [expr int($xfersz*1000)]
	}
	set sink [eval createTcpSink $sinkargs]
	set ftp [createFtp $ns $src $tcp $dst $sink]
	if {$session} {
		set newSessionFlag false
		set newSessionFlag [eval setupTcpSession $tcp $sessionargs]
		setupTcpTracing $tcp $tcptrace $newSessionFlag
	}
	setupTcpTracing $tcp $tcptrace $session
	return $ftp
}

proc numberOfImages {} {
	global numimg

	return $numimg
}

proc htmlRequestSize {} {
	global reqsize

	return $reqsize
}

proc htmlReplySize {} {
	global htmlsize

	return $htmlsize
}

proc imageRequestSize {} {
	global reqsize

	return $reqsize
}

proc imageResponseSize {} {
	global imgsize

	return $imgsize
}

WebCS instproc init {ns client server tcpargs tcptrace sinkargs {phttp 0} {httpseq 0} {session false} {sessionargs ""}} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar numImgReq_
	$self instvar starttime_ endtime_
	$self instvar phttp_ httpseq_ session_
	$self instvar finish_
	global WebCScount

	incr WebCScount
	set ns_ $ns
	set phttp_ $phttp
	set httpseq_ $httpseq
	set session_ $session
	# compute transfer sizes
	set numImg_ [numberOfImages]
	set htmlReqsz_ [htmlRequestSize]
	set htmlReplsz_ [htmlReplySize]
	set imgReqsz_ [imageRequestSize]
	set imgReplsz_ [imageResponseSize]

	# create "ftp's" for each transfer
	set ftpCS1_ [createTransfer $ns $client $server "TCP/Newreno" $tcptrace $sinkargs $session $sessionargs "$self htmlReqDone" $htmlReqsz_]
	set ftpSC1_ [createTransfer $ns $server $client $tcpargs $tcptrace $sinkargs $session $sessionargs "$self htmlReplDone" $htmlReplsz_]
	# XXX assume request to be of fixed size regardless of numImg_
	# unless HTTP with sequential connections is being employed
	if {$httpseq} {
		set numImgReq_ $numImg_
		for {set i 0} {$i < $numImg_} {incr i 1} {
			set ftpCS2_($i) [createTransfer $ns $client $server "TCP/Newreno" $tcptrace $sinkargs $session $sessionargs "$self imgReqDone" $imgReqsz_]
		}
	} else {
		set numImgReq_ 1
		set ftpCS2_(0) $ftpCS1_
	}
	for {set i 0} {$i < $numImg_} {incr i 1} {
		if {$phttp} {
			set ftpSC2_($i) $ftpSC1_
#			if {$i == 0} {
#				set ftpSC2_($i) [createTransfer $ns $server $client $tcpargs $tcptrace $sinkargs $session $sessionargs "$self imgReplDone" $imgReplsz_]
#			} else {
#				set ftpSC2_($i) $ftpSC2_(0)
#			}
		} else {
			set ftpSC2_($i) [createTransfer $ns $server $client $tcpargs $tcptrace $sinkargs $session $sessionargs "$self imgReplDone" $imgReplsz_]
		}
	}
}

WebCS instproc start {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar numImgReq_
	$self instvar starttime_ endtime_
	$self instvar phttp_ httpseq_ session_

	set starttime_ [$ns_ now]
	set numImgRecd_ 0
	set numImgRepl_ 0
	if {$htmlReqsz_ < 1} {
		$ftpCS1_ producemore 1
	} else {
		$ftpCS1_ producemore $htmlReqsz_
	}
}

WebCS instproc htmlReqDone {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar numImgReq_
	$self instvar starttime_ endtime_
	$self instvar phttp_ httpseq_ session_

	$ftpSC1_ producemore $htmlReplsz_
}

WebCS instproc htmlReplDone {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar numImgReq_
	$self instvar starttime_ endtime_
	$self instvar phttp_ httpseq_ session_

	[$ftpCS1_ agent] proc done {} "$self imgReqDone"
	[$ftpSC1_ agent] proc done {} "$self imgReplDone"
	if {$imgReqsz_ < 1} {
		$ftpCS2_($numImgRecd_) producemore 1
	} else {
		$ftpCS2_($numImgRecd_) producemore $imgReqsz_
	}
}

WebCS instproc imgReqDone {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar numImgReq_
	$self instvar starttime_ endtime_
	$self instvar phttp_ httpseq_ session_

	if {$httpseq_} {
		$ftpSC2_($numImgRepl_) producemore $imgReplsz_
		incr numImgRepl_
	} else {
		for {} {$numImgRepl_ < $numImg_} {incr numImgRepl_ 1} {
			
			$ftpSC2_($numImgRepl_) producemore $imgReplsz_
		}
	}
}
	
	
WebCS instproc imgReplDone {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar numImgReq_
	$self instvar starttime_ endtime_
	$self instvar phttp_ httpseq_ session_

	incr numImgRecd_
	if {($numImgRecd_ == $numImg_) || $phttp_} {
		$self end
	} elseif {$httpseq_} {
		$self htmlReplDone
	}
}

WebCS instproc end {} {
	$self instvar ns_ 
	$self instvar ftpCS1_ ftpCS2_ 
	$self instvar ftpSC1_ ftpSC2_
	$self instvar numImg_ htmlReqsz_ htmlReplsz_ imgReqsz_ imgReplsz_
	$self instvar numImgRecd_ numImgRepl_
	$self instvar numImgReq_
	$self instvar starttime_ endtime_
	$self instvar phttp_ httpseq_ session_
	$self instvar finish_
	global WebCScount

	set endtime_ [$ns_ now]
	puts -nonewline stderr "[format "%.3f " [expr $endtime_ - $starttime_]]"
	if {$phttp_} {
		set nrexmit [[$ftpSC2_(0) agent] set nrexmit_]
	} elseif {$session_} {
		set nrexmit [[[$ftpSC2_(0) agent] session] set nrexmit_]
	} else {
		set nrexmit 0
		for {set i 0} {$i < $numImg_} {incr i} {
			incr nrexmit [[$ftpSC2_($i) agent] set nrexmit_]
		}
	}
	puts -nonewline stderr "$nrexmit "
	flush stderr
	incr WebCScount -1
	if {$WebCScount == 0} {
		eval "$finish_"
	}
#	$ns_ at [expr [$ns_ now] + 3] "$self start"
#	[$ftpCS1_ agent] proc done {} "$self htmlReqDone"
#	[$ftpSC1_ agent] proc done {} "$self htmlReplDone"
}

