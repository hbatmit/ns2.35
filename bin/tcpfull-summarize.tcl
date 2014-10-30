#
# program to process a (full) tcp trace file and summarize
# various things (dataful acks, pure acks, etc).
#
# invoke with "tclsh thisfile infile outprefix"
#

set seqmod 0	; # seq number mod value (0: none)
set seqscale 1	; # amount to scale seq #s by
set flowfactor 536; # amount to scale flow # by

proc scaleseq { fid seqno } {
	global seqmod seqscale flowfactor
	if { $seqmod == 0 } {
		set val [expr $fid * $flowfactor + $seqno * $seqscale]
	} else {
		set val [expr $fid * $flowfactor + ($seqno % $seqmod) * $seqscale]
	}
	return $val
}

proc forward_segment { fid time seqno } {
	global segchan
	puts $segchan "$time [scaleseq $fid $seqno]"
}

proc forward_emptysegment { fid time seqno } {
	global emptysegchan
	puts $emptysegchan "$time [scaleseq $fid $seqno]"
}
proc backward_dataful_ack { fid time ackno } {
	global ackchan
	puts $ackchan "$time [scaleseq $fid $ackno]"
}
proc backward_pure_ack { fid time ackno } {
	global packchan
	puts $packchan "$time [scaleseq $fid $ackno]"
}
proc drop_pkt { fid time ackno } {
	global dropchan
	puts $dropchan "$time [scaleseq $fid $ackno]"
}

proc ctrl { fid time tflags seq } {
	global ctrlchan
	puts $ctrlchan "$time [scaleseq $fid $seq]"
}

proc ecnecho_pkt { fid time ackno } {
	global ecnchan
	puts $ecnchan "$time [scaleseq $fid $ackno]"
}

proc cong_act { fid time seqno } {
	global cactchan
	puts $cactchan "$time [scaleseq $fid $seqno]"
}

proc salen { fid time ackno } {
	global sachan
	puts $sachan "$time [scaleseq $fid $ackno]"
}

proc parse_line line {
	global synfound active_opener passive_opener seqmod

	set okrecs { - + d }
	if { [lsearch $okrecs [lindex $line 0]] < 0 } {
		return
	}
	set sline [split $line " "]
	if { [llength $sline] < 15 } {
		puts stderr "apparently not in extended full-tcp format!"
		exit 1
	}
	set field(op) [lindex $sline 0]
	set field(time) [lindex $sline 1]
	set field(tsrc) [lindex $sline 2]
	set field(tdst) [lindex $sline 3]
	set field(ptype) [lindex $sline 4]
	set field(len) [lindex $sline 5]
	set field(pflags) [lindex $sline 6]
	set field(fid) [lindex $sline 7]
	set field(src) [lindex $sline 8]
	set field(srcaddr) [lindex [split $field(src) "."] 0]
	set field(srcport) [lindex [split $field(src) "."] 1]
	set field(dst) [lindex $sline 9]
	set field(dstaddr) [lindex [split $field(dst) "."] 0]
	set field(dstaddr) [lindex [split $field(dst) "."] 1]
	set field(seqno) [lindex $sline 10]
	set field(uid) [lindex $sline 11]
	set field(tcpackno) [lindex $sline 12]
	set field(tcpflags) [lindex $sline 13]
	set field(tcphlen) [lindex $sline 14]
	set field(salen) [lindex $sline 15]

	set fid $field(fid)
	if { ![info exists synfound($fid)] && [expr $field(tcpflags) & 0x02] } {
		global reverse
		set synfound($fid) 1
		if { [info exists reverse] && $reverse } {
			set active_opener($fid) $field(dst)
			set passive_opener($fid) $field(src)
		} else {
			set active_opener($fid) $field(src)
			set passive_opener($fid) $field(dst)
		}
	}

	set interesting 0

	if { $field(op) == "+" && [lsearch "tcp ack" $field(ptype)] >= 0} {
		set interesting 1
	}

	if { $interesting && [expr $field(tcpflags) & 0x03] } {
		# either SYN or FIN is on
		if { $field(src) == $active_opener($fid) && \
		    $field(dst)  == $passive_opener($fid) } {
			ctrl $field(fid) $field(time) $field(tcpflags) \
				[expr $field(seqno) + $field(len) - $field(tcphlen)]
		} elseif { $field(src) == $passive_opener($fid) && \
		    $field(dst) == $active_opener($fid) } {
			ctrl $field(fid) $field(time) $field(tcpflags) \
				$field(tcpackno)
		}
	}

	if { $interesting && $field(src) == $active_opener($fid) && $field(dst) == $passive_opener($fid) } {
		set topseq [expr $field(seqno) + $field(len) - $field(tcphlen)]
		if { $field(len) > $field(tcphlen) } {
			forward_segment $field(fid) $field(time) $topseq
		} else {
			forward_emptysegment $field(fid) $field(time) $topseq
		}
		if { [string index $field(pflags) 3] == "A" } {
			cong_act $field(fid) $field(time) $topseq
		}
		return
	}

	if { $interesting && $field(len) > $field(tcphlen) &&
	    $field(src) == $passive_opener($fid) && $field(dst) == $active_opener($fid) } {
		# record acks for the forward direction that have data
		backward_dataful_ack $field(fid) $field(time) $field(tcpackno)
		if { [string index $field(pflags) 0] == "C"  &&
			[string last N $field(pflags)] >= 0 } {
			ecnecho_pkt $field(fid) $field(time) $field(tcpackno)
		}
		return
	}

	if { $interesting && $field(len) == $field(tcphlen) &&
	    $field(src) == $passive_opener($fid) && $field(dst) == $active_opener($fid) } {
		# record pure acks for the forward direction
		backward_pure_ack $field(fid) $field(time) $field(tcpackno)
		if { [string index $field(pflags) 0] == "C" &&
			[string last N $field(pflags)] >= 0 } {
			ecnecho_pkt $field(fid) $field(time) $field(tcpackno)
		}
		if { $field(salen) > 0 } {
			salen $field(fid) $field(time) $field(tcpackno)
		}
		return
	}

	if { $field(op) == "d" && $field(src) == $active_opener($fid) &&
	    $field(dst) == $passive_opener($fid) } {
		drop_pkt $field(fid) $field(time) \
			[expr $field(seqno) + $field(len) - $field(tcphlen)]
		return
	}

}

proc parse_file chan {
	while { [gets $chan line] >= 0 } {
		parse_line $line
	}
}

proc dofile { infile outfile } {
	global ackchan packchan segchan dropchan ctrlchan emptysegchan ecnchan cactchan sachan

        set ackstmp $outfile.acks ; # data-full acks
        set segstmp $outfile.p; # segments
	set esegstmp $outfile.es; # empty segments
        set dropstmp $outfile.d; # drops
        set packstmp $outfile.packs; # pure acks
	set ctltmp $outfile.ctrl ; # SYNs + FINs
	set ecntmp $outfile.ecn ; # ECN acks
	set cacttmp $outfile.cact; # cong action
	set satmp $outfile.sack; # sack info
        exec rm -f $ackstmp $segstmp $esegstmp $dropstmp $packstmp $ctltmp $ecntmp $cacttmp $satmp

	set ackchan [open $ackstmp w]
	set segchan [open $segstmp w]
	set emptysegchan [open $esegstmp w]
	set dropchan [open $dropstmp w]
	set packchan [open $packstmp w]
	set ctrlchan [open $ctltmp w]
	set ecnchan [open $ecntmp w]
	set cactchan [open $cacttmp w]
	set sachan [open $satmp w]

	set inf [open $infile r]

	parse_file $inf

	close $ackchan
	close $segchan
	close $emptysegchan
	close $dropchan
	close $packchan
	close $ctrlchan
	close $ecnchan
	close $cactchan
	close $sachan
}

proc getopt {argc argv} { 
        global opt
        lappend optlist s n m r

        for {set i 0} {$i < $argc} {incr i} {
                set arg [lindex $argv $i]
                if {[string range $arg 0 0] != "-"} continue

                set name [string range $arg 1 end]
                set opt($name) [lindex $argv [expr $i+1]]
        }
}

getopt $argc $argv
set base 0

if { [info exists opt(s)] && $opt(s) != "" } {
	global seqscale base opt argc argv
	set seqscale $opt(s)
	incr argc -2
	incr base 2
}

if { [info exists opt(n)] && $opt(n) != "" } {
	global flowfactor base opt argc argv
	set flowfactor $opt(n)
	incr argc -2
	incr base 2
}

if { [info exists opt(m)] && $opt(m) != "" } {
	global seqmod base opt argc argv
	set seqmod $opt(m)
	incr argc -2
	incr base 2
}

if { [info exists opt(r)] && $opt(r) != "" } {
	global reverse base argc argv
	set reverse 1
	incr argc -1
	incr base
}

if { $argc < 2 || $argc > 3 } {
	puts stderr "usage: tclsh tcpfull-suumarize.tcl \[-m wrapamt\] \[-r\] \[-n flowfactor\] \[-s seqscale\] tracefile outprefix \[reverse\]"
	exit 1
} elseif { $argc == 3 } {
	if { [lindex $argv [expr $base + 2]] == "reverse" } {
		global reverse
		set reverse 1
	}
}
dofile [lindex $argv [expr $base]] [lindex $argv [expr $base + 1]]
exit 0
