proc Pre { tfile } { 
    global timezero 
    set f [open "|tcpdump -n -r $tfile src port 80 and tcp" r]
    set line [gets $f]
    catch "close $f"
    set aslist [split $line " "]
    set timezero [tcpdtimetosecs [lindex $aslist 0]]
}

proc max {arg1 arg2}  {
    if { $arg1 > $arg2 } { return $arg1 }
    return $arg2 
}

proc min {arg1 arg2}  {
    if { $arg1 > $arg2 } { return $arg2 }
    return $arg1 
}

proc tcpdtimetosecs tcpdtime  {
    global timezero 

    # convert time to seconds
    set t [split $tcpdtime ":."]
    regsub {^0*([0-9])} [lindex $t 0] {\1} hours
    regsub {^0*([0-9])} [lindex $t 1] {\1} minutes
    regsub {^0*([0-9])} [lindex $t 2] {\1} seconds
    set micsec [lindex $t 3]
    append mtim "0." $micsec

    set tim [expr 1.0*$hours*3600 + 1.0*$minutes*60 + 1.0*$seconds ]
    set tim [expr $tim + 1.0*$mtim]


    ##  check for time wrap
    ##
    if { $tim < $timezero } { set tim [expr $tim + 86400.0] }
    return [expr $tim - $timezero]
}

proc outputPDF {cur_epoch incr tfile }  {

set count 0
set cur_time  0
set tfileI [format "%s.sorted" $tfile]
set tfileO [format "%s.pdf" $tfile]

set fi [open $tfileI r ]
set fo [open $tfileO w ]

while {[gets $fi line] >= 0} {

set cur_time [lindex $line 0]


if {$cur_time > $cur_epoch} {
   while {$cur_epoch < $cur_time} {

     puts $fo "$cur_epoch   $count"

     set cur_epoch [expr $cur_epoch + $incr]
     set count 0
   }
   if {$cur_time <= $cur_epoch} {
      set count 1
   } else {
      set count 0
   }
} else {
   set count [expr $count + 1]
}

}

puts $fo "$cur_epoch   $count"


close $fo
}

proc outputCDF {tfile}  {

set tfileI [format "%s.pdf" $tfile]
set tfileO [format "%s.cdf" $tfile]

set fi [open $tfileI r ]
set fo [open $tfileO w ]

set totalCnt 0
set cumCnt 0
while {[gets $fi line] >= 0} {
   set cnt [lindex $line 1]
   set totalCnt [expr $totalCnt + $cnt]
}
close $fi
set fi [open $tfileI r ]
while {[gets $fi line] >= 0} {
   set value [lindex $line 0]
   set cnt [lindex $line 1]
   set cumCnt [expr $cumCnt + $cnt]
   puts $fo "$value $cumCnt [expr ($cumCnt * 1.0) / ($totalCnt * 1.0)]"
}

}
