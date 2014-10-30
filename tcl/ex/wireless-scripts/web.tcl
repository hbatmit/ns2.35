# Web traffic generator by Sally Floyd

proc add_web_nodes {secondDelay variable} {
  global ns opt
  global nodes
  global s_
  global r_
  global count

  set i $count
        if {$variable == 0} {
                set delay2 [expr $secondDelay]
                set delay2a [expr $secondDelay]
        } else {
                ## for traffic from the web traffic generator, 
                ## each path ends up with roughly the same traffic fraction
                ## So, to get the right distribution, we use more short delays.
                set num [expr (($i)%10)/9.0]
                ## num is a number between 0 and 1
                set delay2 [expr 2*$secondDelay*pow($num,4)]
                set delay2a [expr 2*$secondDelay*pow($num,4)]
        }
  set s_($i) [$ns node]
  set r_($i) [$ns node]
  if {$opt(printRTTs) == 1} {
    puts "link delay web [format "%.2f" $delay2] [format "%.2f" $delay2a]"
  }
  set speed 1000
  if {$opt(accessLink) == 1} {
    if {$i%3==0} { set speed 100}
    if {$i%3==0} { set speed 10}
    if {$i%3==0} { set speed 1}
  }
  $ns duplex-link $s_($i) $nodes(ms) 1000Mb [expr $delay2]ms DropTail
  $ns duplex-link $r_($i) $nodes(bs) [expr $speed]Mb [expr $delay2a]ms DropTail
  incr count
}
# interpage: interPage
# pagesize in # of objects: pageSize, rvPageSize_, new WebPage
# interobject: interObj
# objectsize (in # of pkts, I think): objSize
proc add_web_traffic {secondDelay nums interpage pagesize objectsize shape forward} {
  global ns opt
  global s_
  global r_
  global count
  global pool

        PagePool/WebTraf set FID_ASSIGNING_MODE_ 2
        # gen/ns_tcl.cc, for 2, use sameFid_ 
        # for 1, increment FID.
        # for 0, use id_.
        set pool [new PagePool/WebTraf]
        $pool set-num-client $opt(numWebNodes)
        $pool set-num-server $opt(numWebNodes)
        # $pool set sameFid_ $count
        $pool set sameFid_ 2

        for {set i 0} {$i < $opt(numWebNodes)} {incr i} {
                if {$forward == 1} {
                        add_web_nodes $secondDelay $opt(rtts)
                        $pool set-server $i $s_([expr $count - 1])
                        $pool set-client $i $r_([expr $count - 1])
                } elseif {$forward == 0} {
                        $pool set-client $i $s_([expr $count - 1])
                        $pool set-server $i $r_([expr $count - 1])
                }
        }
        $pool set-num-session $nums
        for {set i 0} {$i < $nums} {incr i} {
          set interPage [new RandomVariable/Exponential]
          $interPage set avg_ $interpage
          set pageSize [new RandomVariable/Constant]
          $pageSize set val_ $pagesize
          set interObj [new RandomVariable/Exponential]
          $interObj set avg_ [expr 0.01]
          set objSize [new RandomVariable/ParetoII]
          $objSize set avg_ $objectsize
          $objSize set shape_ $shape
          set tmp [new RandomVariable/Uniform]
          set startTime [expr 1.0 * [$tmp value] * $opt(stop) ]
          # Poisson start times for sessions.
          # when does a web session end?
          # puts [format "session $i startTime %0.3f" $startTime ]
          $pool create-session $i $opt(numPage) $startTime $interPage $pageSize $interObj $objSize
          # All of the web traffic uses the same packet size.
          # webcache/webtraf.cc
          # doc/webcache.tex
          # Feldmann99a, Section 2.4, paragraph 3-4 and the appendix A.1.
          # empweb/empftp.cc
          # doc/session.tex
          # When does each web session start?
        }
}
