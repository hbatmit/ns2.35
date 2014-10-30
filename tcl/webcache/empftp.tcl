# Copyright (C) 1999 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 

PagePool/EmpFtpTraf set debug_ false
PagePool/EmpFtpTraf set TCPTYPE_ Reno
PagePool/EmpFtpTraf set TCPSINKTYPE_ TCPSink   ;#required for SACK1 Sinks.

PagePool/EmpFtpTraf set FID_ASSIGNING_MODE_ 0 
PagePool/EmpFtpTraf set VERBOSE_ 0

PagePool/EmpFtpTraf instproc send-file { id svr clnt stcp ssnk size color} {
	set ns [Simulator instance]

	$ns attach-agent $svr $stcp
	$ns attach-agent $clnt $ssnk
	$ns connect $stcp $ssnk
	
	if {[PagePool/EmpFtpTraf set FID_ASSIGNING_MODE_] == 0} {
#		$stcp set fid_ $id
		$stcp set fid_ $color
	}

	$stcp proc done {} "$self done-send $id $clnt $svr $stcp $ssnk $size [$ns now] [$stcp set fid_]"
	
        puts "req ftp clnt [$clnt id] svr [$svr id] [$ns now] obj $id size $size"

	# Send request packets based on empirical distribution
	$stcp advanceby $size
}


PagePool/EmpFtpTraf instproc done-send { id clnt svr stcp ssnk size {startTime 0} {fid 0}} {
	set ns [Simulator instance]

	if {[PagePool/EmpFtpTraf set VERBOSE_] == 1} {
		puts "done-send - file:$id srv:[$svr id] clnt:[$clnt id] $size $startTime [$ns now] $fid"
	}

	# Recycle server-side TCP agents
	$ns detach-agent $clnt $ssnk
	$ns detach-agent $svr $stcp
	$stcp reset
	$ssnk reset
	$self recycle $stcp $ssnk
}



# XXX Should allow customizable TCP types. Can be easily done via a 
# class variable
PagePool/EmpFtpTraf instproc alloc-tcp { size } {


	set tcp [new Agent/TCP/[PagePool/EmpFtpTraf set TCPTYPE_]]

        $tcp set window_ $size

	set fidMode [PagePool/EmpFtpTraf set FID_ASSIGNING_MODE_]
	if {$fidMode == 1} {
		$self instvar maxFid_
		$tcp set fid_ $maxFid_
		incr maxFid_
	} elseif  {$fidMode == 2} {
		$self instvar sameFid_
		$tcp set fid_ $sameFid_
	}

	
	return $tcp
}

PagePool/EmpFtpTraf instproc alloc-tcp-sink {} {
	return [new Agent/[PagePool/EmpFtpTraf set TCPSINKTYPE_]]
}

