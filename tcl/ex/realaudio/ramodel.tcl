##
set seed           0
set packetsize     245
set nn             300             ;# number of nodes
set cnn            [expr $nn -2 ]   ;# number of clients
set plottime 15000.0

ns-random $seed

##user start time from a poisson distribution
set starttime(2) 0

#set tmp [new RandomVariable/Exponential] ;# Poisson process
#$tmp set avg_ 5.4674 ;# average arrival interval

#artificially syncronize flow start time
#every flow starts at multiple of 1.8s
set tmp [new RandomVariable/Empirical]
$tmp loadCDF userintercdf1

for {set i 3} {$i < $nn} {incr i} {
    set p [$tmp value]
    set i1 [expr $i - 1 ]
    set starttime($i) [expr $starttime($i1) + $p ]
}

##number of sequential flow per user
set rv0 [new RandomVariable/Empirical]
$rv0 loadCDF sflowcdf

##flow duration
set rv1 [new RandomVariable/Empirical]
$rv1 loadCDF flowdurcdf

for {set i 2} {$i < $nn} {incr i} {
  set q [$rv0 value]
  set sflow($i) [expr int($q) ]
  puts "node $i has $sflow($i) flow "
  set p [$rv1 value]
  set dur($i) [expr $p * 60 ]
  puts "node $i duration : $dur($i)"
}


for {set i 2} {$i < $nn} {incr i} {
    set flowstoptime($i) [expr $starttime($i) + $dur($i) ]
}


set ns [new Simulator]

for {set i 0} {$i < $nn} {incr i} {
set n($i) [$ns node]
}

set f [open /usr/RAtrace/newout.tr w]
$ns trace-all $f

$ns duplex-link $n(0) $n(1) 1.5Mb 10ms DropTail

for {set j 2} {$j < $nn} {incr j} {
    $ns duplex-link $n(0) $n($j) 10Mb 5ms DropTail
}

set rv2 [new RandomVariable/Empirical]
$rv2 loadCDF ontimecdf
set rv3 [new RandomVariable/Empirical]
$rv3 loadCDF fratecdf

for {set i 2} {$i < $nn} {incr i} {
  set s($i) [new Agent/UDP]
  $ns attach-agent $n(1) $s($i)
  set null($i) [new Agent/Null]
  $ns attach-agent $n($i) $null($i)
  $ns connect $s($i) $null($i)

  set realaudio($i) [new Application/Traffic/RealAudio]
  $realaudio($i) set packetSize_ $packetsize

  $realaudio($i) set burst_time_ 0.05ms
  $realaudio($i) set idle_time_ 1800ms


  set flow_rate  [$rv3 value]  
  set r [ format "%fk"  $flow_rate ]

  puts "node $i flow rate $r"

  $realaudio($i) set rate_ $r
  $realaudio($i) attach-agent $s($i)
}



for {set i 2} {$i < $nn} {incr i} {

      $ns at $starttime($i) "$realaudio($i) start"
      $ns at $flowstoptime($i) "$realaudio($i) stop"

      puts "node $i starttime $starttime($i)"
      puts "node $i stoptime $flowstoptime($i)"
  

      ##schedule for next flow
      for {set h 2} {$h <= $sflow($i)} {incr h} {

          set starttime($i) [expr $flowstoptime($i) + 0.001 ]
          set p [$rv1 value]
          set dur($i) [expr $p * 60 ]
          puts "node $i duration : $dur($i)"

	  set flowstoptime($i) [expr $starttime($i) + $dur($i) ]

          set realaudio($i) [new Application/Traffic/RealAudio]
          $realaudio($i) set packetSize_ $packetsize
          $realaudio($i) set burst_time_ 0.05ms
          $realaudio($i) set idle_time_ 1800ms

          set flow_rate  [$rv3 value]  
          set r [ format "%fk"  $flow_rate ]

          puts "node $i flow rate $r"

          $realaudio($i) set rate_ $r
          $realaudio($i) attach-agent $s($i)
          
      }

}

$ns at $plottime "close $f"
$ns at $plottime "finish tg"

proc finish file {
 

        #exec rm -f  out.time.tr
        #
        #
	exec awk {
		{
			if (($1 == "+") && ($3 == 1) ) \
			     print $2, $10
		}
	} /usr/RAtrace/newout.tr > RA.time.tr




}

$ns run


exit 0
