# Author : Hari Balakrishnan
# Class that encapsulates all statistics collection.

Class StatCollector 

StatCollector instproc init {id ctype} {
    $self set srcid_ $id
    $self set ctype_ $ctype;    # type of congestion control / tcp
    $self set numbytes_ 0
    $self set ontime_ 0.0;     # total time connection was in ON state
    $self set cumrtt_ 0.0
    $self set rtt_samples_ {}
    $self set nsamples_ 0
    $self set nconns_ 0
}

StatCollector instproc showstats {final} {
  global opt
  set res [$self results]
  set id [$self set srcid_]
  set totalbytes [lindex $res 0]
  set totaltime [lindex $res 1]
  set totalrtt [lindex $res 2]
  set nsamples [lindex $res 3]
  set nconns [lindex $res 4]
  set all_rtt_samples [lindex $res 5]

  if { $nsamples > 0 } {
      set avgrtt [expr 1000*$totalrtt/$nsamples]
      set reqd_index [expr round (floor ($nsamples * 0.95)) ]
      set sorted [lsort $all_rtt_samples]
      set rtt95th [expr [lindex $sorted $reqd_index] * 1000]
  } else {
      set avgrtt 0.0
      set rtt95th 0.0
  }

  if { ($totaltime > 0.0) && ($nsamples > 0) } {
      set throughput [expr 8.0 * $totalbytes / $totaltime]
      set utility [expr log($throughput) - [expr log($rtt95th)]]
      if { $final == True } {
          puts [ format "FINAL id %d bytes %d mbps %.3f avgrtt %.1f rtt-95th %.1f on_percentage %.4f utility %.2f num_connections %d nsamples %d" $id $totalbytes [expr $throughput/1000000.0] $avgrtt $rtt95th [expr 100.0*$totaltime/$opt(duration)] $utility $nconns $nsamples ]
      } else {
          puts [ format "----- id %d bytes %d mbps %.3f avgrtt %.1f rtt-95th %.1f on_percentage %.4f utility %.2f num_connections %d nsamples %d" $id $totalbytes [expr $throughput/1000000.0] $avgrtt $rtt95th [expr 100.0*$totaltime/$opt(duration)] $utility $nconns $nsamples ]
      }
  }
}

StatCollector instproc update {newbytes newtime cumrtt nsamples rtt_samples} {
    global ns opt
    $self instvar srcid_ numbytes_ ontime_ cumrtt_ nsamples_ nconns_ rtt_samples_
    incr numbytes_ $newbytes
    set ontime_ [expr $ontime_ + $newtime]
    set cumrtt_ [expr $cumrtt_ + $cumrtt]
    incr nsamples_ $nsamples
    set rtt_samples_ [concat $rtt_samples_ $rtt_samples]
    incr nconns_
    puts "[$ns now]: updating stats for $srcid_: $newbytes $newtime $cumrtt $nsamples"
    puts "[$ns now]: Total so far: $numbytes_ $ontime_ $cumrtt_ $nsamples_"
    if { $opt(partialresults) } {
        $self showstats False
    }
}

StatCollector instproc results { } {
    $self instvar numbytes_ ontime_ cumrtt_ nsamples_ nconns_ rtt_samples_
    return [list $numbytes_ $ontime_ $cumrtt_ $nsamples_ $nconns_ $rtt_samples_]
}
