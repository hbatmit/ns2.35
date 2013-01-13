TestSuite instproc June01defaults {} {
	 Agent/TCP set windowInit_ 2
	 Agent/TCP set singledup_ 1
	 Agent/TCP set minrto_ 1
	 Agent/TCP set syn_ true
	 Agent/TCP set delay_growth_ true
}

TestSuite instproc dropPkts { link fid pkts {delayPkt no} {delay -1}} {
    set emod [new ErrorModule Fid]
    set errmodel1 [new ErrorModel/List]
    if {$delayPkt != "no"} {
        $errmodel1 set delay_pkt_ $delayPkt
    }
    if {$delay != -1} {
        $errmodel1 set delay_ $delay
    }
    $errmodel1 droplist $pkts
    $link errormodule $emod
    $emod insert $errmodel1
    $emod bind $errmodel1 $fid
}

TestSuite instproc dropPktsPeriodic { link fid offset period {delayPkt no} {delay -1}} {
    set emod [new ErrorModule Fid]
    set errmodel1 [new ErrorModel/Periodic]
    $errmodel1 unit pkt
    $errmodel1 set offset_ $offset
    $errmodel1 set period_ $period
    if {$delayPkt != "no"} {
        $errmodel1 set delay_pkt_ $delayPkt
    }
    if {$delay != -1} {
        $errmodel1 set delay_ $delay
    }
    $link errormodule $emod
    $emod insert $errmodel1
    $emod bind $errmodel1 $fid
}

