#!/usr/bin/tclsh

global pageIdx
global server
global nSvr
global client
global nClnt
global sessionIdx
global PageThreshold
global SessionThreshold
global outp

source util.tcl

if { $argc != 2} {
   puts "usage: main.tcl <tcpdump file> <log file>"
   exit
} else {
  set arg [split $argv " " ]
  set tfile [lindex $arg 0]
  set logfile [lindex $arg 1]
}

set logf [open $logfile w ]
set srcf [open server.log w ]

set defaultInterval 100
set PageThreshold 1
set SessionThreshold 600

set timezero 0.0
set isrc ""
set idst ""
set t 0
set nSvr 0
set nClnt 0

set d [exec date +%X ]
puts $logf "Initialization $d"

catch { exec /bin/rm -rf CLIENT*} res
catch { exec /bin/rm -rf *dat*} res
catch { exec /bin/rm -rf *.tmp} res
catch { exec tcpdump -S -n -r  $tfile src port 80 and tcp | awk -f util1.awk  | sort +0 > www1.dmp} res
catch { exec tcpdump -S -n -r  $tfile dst port 80 and tcp | awk -f util1.awk  > www-requestI.dmp} res

set outp [open "config.log" w ]

set d [exec date +%X ]
puts $logf "Parsing tcpdump file $tfile $d"

set oldClient ""
set fi [open "www1.dmp" r ]
while {[gets $fi line] >= 0} {

  set startTime  [tcpdtimetosecs [lindex $line 1]]

  set src [lindex $line 3]
  set dst [lindex $line 0]
  set port [lindex $line 2]
  set flag [lindex $line 4]
  set seqno [lindex $line 5]
  if { $seqno =="ack"} {
   set ack  [lindex $line 6]
   set win  [lindex $line 7]
   set seqno ""
  } else {
   set ack  [lindex $line 7]
   set win  [lindex $line 9]
  }

   #calculate the number of client
   set c $dst 
   set p 0
   for {set i 0} {$i< $nClnt} {incr i} {
      if { $c == $client($i)} {
         set p 1
      }
   }
   #a new client address
   if { $p == 0 } {  
     set client($nClnt) $c
     set nClnt [expr $nClnt + 1]
     set clientDir [format "CLIENT%04d" $nClnt]
     set t 0
     set sessionIdx($nClnt) 0
   }

   #calculate the number of server
   set p 0
   for {set i 0} {$i< $nSvr} {incr i} {
      if { $src == $server($i)} {
         set p 1
      }
   }
   if { $p == 0 } {
     set server($nSvr) $src
     set server_cnt($nSvr) 0
     set nSvr [expr $nSvr + 1]

     puts $srcf $src
   }



  set diff [expr $startTime - $t]

  if { $diff > $SessionThreshold } {

     if { $sessionIdx($nClnt) != 0 } {
        puts $outp "INTERSESSION $diff"
     }
     set sessionIdx($nClnt) [expr $sessionIdx($nClnt) + 1]
     set pageIdx($nClnt,$sessionIdx($nClnt)) 0
  }

  if { $diff > $PageThreshold } {

     if { $diff < $SessionThreshold && $t != 0 } {
         puts $outp "INTERPAGE $diff"
     }

     set pageIdx($nClnt,$sessionIdx($nClnt)) [expr $pageIdx($nClnt,$sessionIdx($nClnt)) + 1]

     set sessDir [format "SESSION%04d" $sessionIdx($nClnt)]
     set pageDir [format "PAGE%06d" $pageIdx($nClnt,$sessionIdx($nClnt))]
     set pageName [format "page%06d" $pageIdx($nClnt,$sessionIdx($nClnt))]
     catch {exec mkdir $clientDir} res
     catch {exec mkdir $clientDir/$sessDir} res
     catch {exec mkdir $clientDir/$sessDir/$pageDir} res
     if [info exists fo] {
        close $fo
     }
     set fo [open $clientDir/$sessDir/$pageDir/$pageName w ]
  }

  set dst_port [format "%s:%s" $dst $port]

  if { $seqno ==""} {
     puts $fo "$startTime $src $dst_port $flag ack $ack "
  } else {
     set s [split $seqno ":()"]
  puts $fo "$startTime $src $dst_port $flag [lindex $s 0] [lindex $s 2] ack $ack "
  }

  set t $startTime

}

close $fo
close $fi
close $srcf


puts $logf "number of client = $nClnt"

puts $outp "NUMSERVER $nSvr"
puts $outp "NUMCLIENT $nClnt"
puts $outp "NUMSESSION $sessionIdx($nClnt)"

set objf [open object.startTime w]
set svrf [open server.popularity.cdf w]
 
for {set p 1} {$p<=$nClnt} {incr p} {
 
set d [exec date +%X ]
puts $logf "parsing client $p $d"

for {set m 1} {$m<=$sessionIdx($p)} {incr m} {

puts $logf "parsing session $m $d"

 puts $outp "NUMPAGE $pageIdx($p,$m)"

 for {set j 1} {$j<=$pageIdx($p,$m)} {incr j} {
   set numObjPerPage 0

   set clientDir [format "CLIENT%04d" $p]
   set sessDir [format "SESSION%04d" $m]
   set pageDir [format "PAGE%06d" $j]
   set pageName [format "page%06d" $j]
   set pageFile [format "%s" $clientDir/$sessDir/$pageDir/$pageName ]
   set pageFileSorted [format "%s" $clientDir/$sessDir/$pageDir/$pageName.sort ]
   catch { exec cat $pageFile | awk -f util2.awk | sort > $pageFileSorted } res

   if [info exists fpi] {
     close $fpi
   }

   set fpi [open $pageFileSorted r] 


   set isrc ""
   set idst ""
   set k 0

   #number of TCP connection
   while {[gets $fpi line] >= 0} {

   set src [lindex $line 0]
   set dst [lindex $line 1]
   set startTime  [lindex $line 2]

   set field3 [lindex $line 3]
   set field4 [lindex $line 4]
   set field5 [lindex $line 5]
   set field6 [lindex $line 6]
   set field7 [lindex $line 7]
   set field8 [lindex $line 8]

   if { $src != $isrc || $dst != $idst } {
      if { $k != 0 } {
         close $outf
      }
      set k [expr $k + 1]
      set dumpFile [format "%04d.dmp" $k]
      set outf [open $clientDir/$sessDir/$pageDir/$dumpFile w ]
      set isrc $src
      set idst $dst
     }

   puts $outf "$src $dst $startTime $field3 $field4 $field5 $field6 $field7 $field8"
   }
   puts $outp "NUMCONNECTION $k"
   if [info exists outf] {
      close $outf
   }

   #number of objects requested in each connection
   for {set n 1} {$n<=$k} {incr n} {
      set dumpFile [format "%04d.dmp" $n]
      set inf [open $clientDir/$sessDir/$pageDir/$dumpFile r ]
      set oldAck 0
      set numObj 0
      set objSize 0
      while {[gets $inf line] >= 0} {
         set src [lindex $line 0]
         set startTime  [lindex $line 2]
         set size [lindex $line 5]
         set ackSeq [lindex $line 7]
         if { $size > 0 && $size != "ack" } {
           if { $ackSeq > $oldAck } {
             if { $objSize > 0 } {
                 puts $outp "OBJSIZE $objSize"
                 set numObjPerPage [expr $numObjPerPage + 1]
             }
             puts $objf "$clientDir/$sessDir/$pageDir/$dumpFile $startTime"
             set objSize $size
             set numObj [expr $numObj + 1]

             #counting the unique server
             for {set t 0} {$t< $nSvr} {incr t} {
                  if { $src == $server($t)} {
                    set server_cnt($t) [expr $server_cnt($t) + 1]
                  }
             }

           } else {
             if { $ackSeq == $oldAck && $size > 0 } {
                set objSize [expr $objSize + $size]
             }
           }
           set oldAck $ackSeq
         }
      }

      close $inf

      if { $objSize > 0 } {
          set numObjPerPage [expr $numObjPerPage + 1]
          puts $outp "OBJSIZE $objSize"
      }
      if { $numObj > 1 } {
          puts $outp "PERSISTCONN"
      }
   }
   if { $numObjPerPage > 0 } {
     puts $outp "NUMOBJPERPAGE $numObjPerPage"
   }
}
}
}

#output CDF of server popularity
set totalRef 0
for {set t 0} {$t< $nSvr} {incr t} {
    set totalRef [expr $totalRef + $server_cnt($t)] 
}
set cumRef 0
for {set t 0} {$t< $nSvr} {incr t} {
    set cumRef [expr $cumRef + $server_cnt($t)]
    puts $svrf "$t $cumRef [expr ($cumRef * 1.0) / ($totalRef * 1.0)]" 
}


close $objf 
close $svrf 

set d [exec date +%X ]
puts $logf "Calculate request size $d "

#
#calculate request size for each connection
#
set timezero 0.0


set fri [open "www-requestI.dmp" r ]
set fro [open "www-requestO.dmp" w ]


while {[gets $fri line] >= 0} {

set startTime  [tcpdtimetosecs [lindex $line 1]]

set src [lindex $line 3]
set dst [lindex $line 0]
set port [lindex $line 2]
set flag [lindex $line 4]
set seqno [lindex $line 5]
if { $seqno =="ack"} {
 set ack  [lindex $line 6]
 set win  [lindex $line 8]
 set seqno ""
} else {
 set ack  [lindex $line 7]
 set win  [lindex $line 9]
}


set dst_port [format "%s:%s" $dst $port]

if { $seqno !=""  } {
   set s [split $seqno ":()"]
   if { [lindex $s 2] > 0  } {
      puts $fro "$src $dst_port $ack $startTime $flag [lindex $s 0] [lindex $s 2]"
   }
}

}

close $outp
close $fro
close $fri

catch { exec sort www-requestO.dmp > www-request.sorted } res

set fi [open www-request.sorted r]
set fo [open request.size.dat w]

set isrc ""
set idst ""
set iack 0
set requestSize 0

set maxREQUEST 0
set cntREQUEST 0
set minREQUEST 1000

while {[gets $fi line] >= 0} {
   set src [lindex $line 0]
   set dst [lindex $line 1]
   set size [lindex $line 6]
   set ackNo [lindex $line 2]

   if { $src != $isrc || $dst != $idst || $ackNo != $iack } {
      if { $iack != 0 } {
         puts $fo $requestSize
         set cntREQUEST [expr $cntREQUEST + 1]
         set maxREQUEST [max $maxREQUEST $requestSize]
         set minREQUEST [expr floor([min $minREQUEST $requestSize])]
      }
      set requestSize $size
   } else {
      set requestSize [expr $requestSize + $size]
   }
   set isrc $src
   set idst $dst
   set iack $ackNo
}
puts $fo $requestSize
set maxREQUEST [max $maxREQUEST $requestSize]
set minREQUEST [expr floor([min $minREQUEST $requestSize])]

set d [exec date +%X ]
puts $logf "Calculate object inter-arrival time $d"

catch { exec sort +1 object.startTime -o object.start } res

#calculate object inter-arrival time
close $fi
close $fo
set fi [open "object.start" r ]
set fo [open "obj.inter.dat" w ]

set maxOBJINTER 0
set cntOBJINTER 0
set minOBJINTER $PageThreshold

set iTime 0
while {[gets $fi line] >= 0} {
   set time  [lindex $line 1]
   set diff [expr $time - $iTime]
   if { $iTime != 0 && $diff < $PageThreshold && $diff > 0 } {
      puts $fo $diff
      set cntOBJINTER [expr $cntOBJINTER + 1]
      set maxOBJINTER [max $maxOBJINTER $diff]
      set minOBJINTER [min $minOBJINTER $diff]
   }
   set iTime $time
}

close $fi
close $fo

set d [exec date +%X ]
puts $logf "Calculate CDF files $d"

set fi [open "config.log" r ]
set fo1 [open "page.per.session.dat" w ]
set fo2 [open "object.size.dat" w ]
set fo3 [open "page.inter.dat" w ]
set fo4 [open "session.inter.dat" w ]
set fo5 [open "persist.cdf" w ]
set fo6 [open "obj.per.page.dat" w ]

set numPer 0
set numConn 0
set cntNUMPAGE 0
set cntOBJSIZE 0
set cntINTERPAGE 0
set cntINTERSESSION 0 
set cntNUMOBJPERPAGE 0
set maxNUMPAGE 0
set maxOBJSIZE 0
set maxINTERPAGE 0
set maxINTERSESSION 0 
set maxNUMOBJPERPAGE 0 
set minNUMPAGE 100
set minNUMOBJPERPAGE 100
set minOBJSIZE 1000
set minINTERPAGE $PageThreshold
set minINTERSESSION $SessionThreshold 
while {[gets $fi line] >= 0} {
   set keyword  [lindex $line 0]
   set value  [lindex $line 1]
   set svrRef  [lindex $line 2]
   if { $keyword == "NUMPAGE" } {
       puts $fo1 $value
       set cntNUMPAGE [expr $cntNUMPAGE + 1]
       set maxNUMPAGE [max $maxNUMPAGE $value]
       set minNUMPAGE [expr floor([min $minNUMPAGE $value])]
   }
   if { $keyword == "PERSISTCONN" } {
       set numPer [expr $numPer + 1]
   }
   if { $keyword == "NUMCONNECTION" } {
       set numConn [expr $numConn + 1]
   }
   if { $keyword == "OBJSIZE" } {
       puts $fo2 $value
       set cntOBJSIZE [expr $cntOBJSIZE + 1]
       set maxOBJSIZE [max $maxOBJSIZE $value]
       set minOBJSIZE [expr floor([min $minOBJSIZE $value])]
   }
   if { $keyword == "INTERPAGE" } {
       puts $fo3 $value
       set cntINTERPAGE [expr $cntINTERPAGE + 1]
       set maxINTERPAGE [max $maxINTERPAGE $value]
   }
   if { $keyword == "INTERSESSION" } {
       puts $fo4 $value
       set cntINTERSESSION [expr $cntINTERSESSION + 1]
       set maxINTERSESSION [max $maxINTERSESSION $value]
   }
   if { $keyword == "NUMOBJPERPAGE" } {
       puts $fo6 $value
       set cntNUMOBJPERPAGE [expr $cntNUMOBJPERPAGE + 1]
       set maxNUMOBJPERPAGE [max $maxNUMOBJPERPAGE $value]
       set minNUMOBJPERPAGE [expr floor([min $minNUMOBJPERPAGE $value])]
   }
}

set incrNUMPAGE [expr ([expr $maxNUMPAGE - $minNUMPAGE] * 1.0) / ($defaultInterval * 1.0)]
set incrOBJSIZE [expr ([expr $maxOBJSIZE - $minOBJSIZE] * 1.0) / ($defaultInterval * 1.0)]
set incrINTERPAGE [expr ([expr $maxINTERPAGE - $minINTERPAGE] * 1.0) / ($defaultInterval * 1.0)]
set incrINTERSESSION [expr ([expr $maxINTERSESSION - $minINTERSESSION] * 1.0) / ($defaultInterval * 1.0)]
set incrREQUEST [expr ([expr $maxREQUEST - $minREQUEST] * 1.0) / ($defaultInterval * 1.0)]
set incrOBJINTER [expr ([expr $maxOBJINTER - $minOBJINTER] * 1.0) / ($defaultInterval * 1.0)]
set incrNUMOBJPERPAGE [expr ([expr $maxNUMOBJPERPAGE - $minNUMOBJPERPAGE] * 1.0) / ($defaultInterval * 1.0)]

set persistRatio [expr ( $numPer * 1.0) / ($numConn * 1.0) ]
puts $fo5 "0 [expr $numConn - $numPer] [expr 1.0 - $persistRatio]"
puts $fo5 "1 $numConn 1.0"

close $fi
close $fo1
close $fo2
close $fo3
close $fo4
close $fo5
close $fo6

catch { exec sort -g page.per.session.dat -o page.per.session.dat.sorted } res
catch { exec sort -g object.size.dat -o object.size.dat.sorted } res
catch { exec sort -g page.inter.dat -o page.inter.dat.sorted } res
catch { exec sort -g session.inter.dat -o session.inter.dat.sorted  } res
catch { exec sort -g request.size.dat -o request.size.dat.sorted  } res
catch { exec sort -g obj.inter.dat -o obj.inter.dat.sorted  } res
catch { exec sort -g obj.per.page.dat -o obj.per.page.dat.sorted  } res

outputPDF $minNUMPAGE $incrNUMPAGE "page.per.session.dat" 
outputPDF $minOBJSIZE $incrOBJSIZE "object.size.dat" 
outputPDF $minINTERPAGE $incrINTERPAGE "page.inter.dat" 
outputPDF $minREQUEST $incrREQUEST "request.size.dat" 
outputPDF $minOBJINTER $incrOBJINTER "obj.inter.dat" 
outputPDF $minNUMOBJPERPAGE $incrNUMOBJPERPAGE "obj.per.page.dat" 
if { $cntINTERSESSION > 0 } {
   outputPDF $minINTERSESSION $incrINTERSESSION "session.inter.dat" 
   outputCDF "session.inter.dat"
} else {
  puts "Warning: only one user session?"
}

outputCDF "page.per.session.dat"
outputCDF "object.size.dat"
outputCDF "page.inter.dat"
outputCDF "request.size.dat"
outputCDF "obj.inter.dat"
outputCDF "obj.per.page.dat"
