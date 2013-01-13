#
# routines to perform post processing, mostly for plotting
#

Class PostProcess

PostProcess instproc init { l lf lg gf gg redqf} {
        $self instvar linkflowfile_ linkgraphfile_
        $self instvar flowfile_ graphfile_
        $self instvar redqfile_
	$self instvar label_ post_

	set label_ $l
        set linkflowfile_ $lf
        set linkgraphfile_ $lg
       set flowfile_ $gf
        set graphfile_ $gg
	set redqfile_ $redqf

	set format_ "xgraph"	; # default
}


#
# Plot the queue size and average queue size, for RED gateways.
#
PostProcess instproc plot_queue {} {

	$self instvar redqfile_
	$self instvar label_ format_

	#TestSuite instproc plotQueue file
	
	set awkCode {
		{
			if ($1 == "Q" && NF>2) {
				print $2, $3 >> "temp.q";
				set end $2
			}
			else if ($1 == "a" && NF>2)
				print $2, $3 >> "temp.a";
		}
	}

	set graphFile $redqfile_.xgr
	
	set f [open $graphFile w]
	puts $f "TitleText: $label_"
	puts $f "Device: Postscript"

	exec rm -f temp.q temp.a 
	exec touch temp.a temp.q
	
	exec awk $awkCode $redqfile_
	
	puts $f \"queue
	exec cat temp.q >@ $f  
	puts $f \n\"ave_queue
	exec cat temp.a >@ $f
	
	###puts $f \n"thresh
	###puts $f 0 [[ns link $r1 $r2] get thresh]
	###puts $f $end [[ns link $r1 $r2] get thresh]
	
	close $f
	puts "running xgraph for queue dynamics ...."
	if { $format_ == "xgraph" } {
		exec xgraph -bb -tk -x time -y queue $graphFile &
		return
	}
	puts stderr "graph format $format_ unknown"
}


#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [pkts]

PostProcess instproc unforcedmakeawk arg {
    if { $arg == "unforced" } {
	set awkCode {
	    BEGIN { prev=-1; print "\"flow 0"; }
	    {
		if ($5 != prev) {
		    print " "; print "\"flow " $5;
		    if ($13 > 0 && $14 > 0) {
			print 100.0 * $9/$13, 100.0 * $10 / $14;
		    }
		    prev = $5;
		}
		else if ($13 > 0 && $14 > 0) {
		    print 100.0 * $9 / $13, 100.0 * $10 / $14;
		}
	    }
	}
    } elseif { $arg == "forced" } {
	set awkCode {
	    BEGIN { prev=-1; print "\"flow 0" }
	    {
		if ($5 != prev) {
		    print " "; print "\"flow " $5;
		    if ($13 > 0 && ($16-$14) > 0) {
			print 100.0 * $9/$13, 100.0 * ($18-$10) / ($16-$14);
		    }
		    prev = $5;
		}
		else if ($13 > 0 && ($16-$14) > 0) {
		    print 100.0 * $9 / $13, 100.0 * ($18-$10) / ($16-$14);
		}
	    }
	}
    } else {
	puts stderr "Error: unforcedmakeawk: arg $arg unknown."
	return {}
    }
    return $awkCode
}

#
# awk code used to produce:
#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [pkts]
# Make sure that N > 2.3 / P^2, for N = # drops this category [pkts],
#       P = y axis value
PostProcess instproc unforcedmakeawk1 { } {
        return {
            BEGIN { print "\"flow 0" }
            {
                if ($5 != prev) {
                        print " "; 
			print "\"flow " $5;
			drops = 0; flow_drops = 0; arrivals = 0;
			flow_arrivals = 0;
			byte_arrivals = 0; flow_byte_arrivals = 0;
		}
		drops += $14;
		flow_drops += $10;
		arrivals += $12;
		byte_arrivals += $13;
		flow_arrivals += $8;
		flow_byte_arrivals += $9;
		p = flow_arrivals/arrivals;
		if (p*p*drops >= 2.3) {
		  print 100.0 * flow_byte_arrivals/byte_arrivals, 
		    100.0 * flow_drops / drops;
		  drops = 0; flow_drops = 0; arrivals = 0;
		  flow_arrivals = 0;
		  byte_arrivals = 0; flow_byte_arrivals = 0;
		} else {
		    printf "p: %8.2f drops: %d\n", p, drops
		}
		prev = $5
            }
        }
}

#printf "prev=%d,13=%d,17=%d,15=%d\n",prev,$13,$17,$15;
#
# awk code used to produce:
#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [bytes]
PostProcess instproc forcedmakeawk arg {
    if { $arg == "forced" } {
	set awkCode {
	    BEGIN { prev=-1; print "\"flow 0"; }
	    {
		if ($5 != prev) {
		    print " "; print "\"flow " $5;
		    if ($13 > 0 && ($17-$15) > 0) {
			print 100.0 * $9/$13, 100.0 * ($19-$11) / ($17-$15);
			prev = $5;
		    }
		}
		else if ($13 > 0 && ($17-$15) > 0) {
		    print 100.0 * $9 / $13, 100.0 * ($19-$11) / ($17-$15);
		}
	    }
	}
    } elseif { $arg == "unforced" } {
	set awkCode {
	    BEGIN { prev=-1; print "\"flow 0"; }
	    {
		if ($5 != prev) {
		    print " "; print "\"flow " $5;
		    if ($13 > 0 && $15 > 0) {
			print 100.0 * $9/$13, 100.0 * $11 / $15;
			prev = $5;
		    }
		}
		else if ($13 > 0 && $15 > 0) {
		    print 100.0 * $9 / $13, 100.0 * $11 / $15;
		}
	    }
	}
    } else {
	puts stderr "Error: forcedmakeawk: arg $arg unknown."
	return {}
    }
    return $awkCode
}

#
# awk code used to produce:
#      x axis: # arrivals for this flow+category / # total arrivals [bytes]
#      y axis: # drops for this flow / # drops [pkts and bytes combined]
PostProcess instproc allmakeawk { } {
    set awkCode {
	BEGIN { prev=-1; frac_bytes=0; frac_packets=0; frac_arrivals=0; cat0=0; cat1=0}
	{
	    if ($5 != prev) {
		print " "; print "\"flow "$5;
		prev = $5
	    }
	    if ($1 != prevtime && cat1 + cat0 > 0) {
		if (frac_packets + frac_bytes > 0) {
		    cat1_part = frac_packets * cat1 / ( cat1 + cat0 ) 
		    cat0_part = frac_bytes * cat0 / ( cat1 + cat0 ) 
		    print 100.0 * frac_arrivals, 100.0 * ( cat1_part + cat0_part )
		}
		frac_bytes = 0; frac_packets = 0; frac_arrivals = 0;
		cat1 = 0; cat0 = 0;
		prevtime = $1
	    }
	    if ($14 > 0) {
		frac_packets = $10/$14;
	    }
	    else {
		frac_packets = 0;
	    }
	    if (($17-$15) > 0) {
		frac_bytes = ($19-$11)/($17-$15);
	    }
	    else {
		frac_bytes = 0;
	    }
	    if ($13 > 0) {
		frac_arrivals = $9/$13;
	    }
	    else {
		frac_arrivals = 0;
	    }
	    cat0 = $16-$14;
	    cat1 = $14;
	    prevtime = $1
	}
	END {
	    if (frac_packets + frac_bytes > 0) {
		cat1_part = frac_packets * cat1 / ( cat1 + cat0 ) 
		cat0_part = frac_bytes * cat0 / ( cat1 + cat0 ) 
		print 100.0 * frac_arrivals, 100.0 * ( cat1_part + cat0_part )
	    }
	}
    }
    return $awkCode
}

#--------------------------------------------------------------

PostProcess instproc create_flow_graph { graphtitle in out awkprocedure } {
        puts "removing graph file: $out"
        exec rm -f $out 
        set outdesc [open $out w] 

        #       
        # this next part is xgraph specific
        #       
        puts $outdesc "TitleText: $graphtitle"
        puts $outdesc "Device: Postscript"
        puts "writing flow data to $out ..."

	catch {exec sort -n +1 -o $in $in} result
	exec awk [$awkprocedure] $in >@ $outdesc
	close $outdesc
}

# plot drops vs. arrivals
PostProcess instproc finish_flow {} {
        $self instvar format_
        $self instvar label_ linkflowfile_ linkgraphfile_

	$self create_flow_graph $label_ $linkflowfile_ $linkgraphfile_ $awkprocedure
	puts "running xgraph for comparing drops and arrivals..."
	if { $format_ == "xgraph" } {
	    exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards" $linkgraphfile_ &
	} 
	puts stderr "graph format $format_ unknown"
}

# plot drops vs. arrivals, for unforced drops.
PostProcess instproc plot_dropsinpackets { name flowgraphfile } {
        $self instvar format_
        $self instvar label_ linkflowfile_ linkgraphfile_

    $self create_flow_graph $label_ $linkflowfile_ $linkgraphfile_ \
	"$self unforcedmakeawk unforced"
    puts "running xgraph for comparing drops and arrivals..."
    if { $format_ == "xgraph" } {
	exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards (in packets).  Queue in SETME" $linkgraphfile_ &
    } 
    puts stderr "graph format $format_ unknown"
}

# plot drops vs. arrivals, for unforced drops.
PostProcess instproc plot_dropsinpackets1 { name flowgraphfile } {
        $self instvar format_
        $self instvar label_ linkflowfile_ linkgraphfile_

    $self create_flow_graph $label_ $linkflowfile_ $linkgraphfile_ \
	"$self unforcedmakeawk1"
    puts "running xgraph for comparing drops and arrivals..."
    if { $format_ == "xgraph" } {
	exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards (in packets).  Queue in SETME" $linkgraphfile_ &
    } 
    puts stderr "graph format $format_ unknown"
}

# plot drops vs. arrivals, for forced drops.
PostProcess instproc plot_dropsinbytes { name flowgraphfile } {
        $self instvar format_
        $self instvar label_ linkflowfile_ linkgraphfile_

    $self create_flow_graph $label_ $linkflowfile_ $linkgraphfile_ \
	"$self forcedmakeawk forced"
    puts "running xgraph for comparing drops and arrivals..."
    if { $format_ == "xgraph" } {
	exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards (in packets).  Queue in SETME" $linkgraphfile_ &
    } 
    puts stderr "graph format $format_ unknown"
}

# plot drops vs. arrivals, for combined metric drops.
PostProcess instproc plot_dropscombined { name flowgraphfile } {
        $self instvar format_
        $self instvar label_ linkflowfile_ linkgraphfile_

    $self create_flow_graph $label_ $linkflowfile_ $linkgraphfile_ \
	"$self allmakeawk"
    puts "running xgraph for comparing drops and arrivals..."
    if { $format_ == "xgraph" } {
	exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards (in packets).  Queue in SETME" $linkgraphfile_ &
    } 
    puts stderr "graph format $format_ unknown"
}

#--------------------------------------------------------------------------

# awk code used to produce:
#      x axis: time
#      y axis: per-flow drop ratios
PostProcess instproc time_awk { } {
        set awkCode {
            BEGIN { print "\"flow 0"}
            {
                if ($1 != prevtime && prevtime > 0){
			if (cat1 + cat0 > 0) {
			  cat1_part = frac_packets * cat1 / ( cat1 + cat0 )
			  cat0_part = frac_bytes * cat0 / ( cat1 + cat0 )
                          print prevtime, 100.0 * ( cat1_part + cat0_part )
			}
			frac_bytes = 0; frac_packets = 0; 
			cat1 = 0; cat0 = 0;
			prevtime = $1
		}
		if ($5 != prev) {
			print " "; print "\"flow "prev;
			prev = $5
		}
		if ($3==0) { 
			if ($15>0) {frac_bytes = $11 / $15}
			else {frac_bytes = 0}
			cat0 = $14
		} if ($3==1) { 
			if ($14>0) {frac_packets = $10 / $14}
			else {frac_packets = 0}
			cat1 = $14
		}
		prevtime = $1
            }
	    END {
		cat1_part = frac_packets * cat1 / ( cat1 + cat0 )
		cat0_part = frac_bytes * cat0 / ( cat1 + cat0 )
                print prevtime, 100.0 * ( cat1_part + cat0_part )
	    }
        }
        return $awkCode
}

# plot time vs. per-flow drop ratio
PostProcess instproc create_time_graph { graphtitle graphfile } {
        global flowfile awkprocedure

        exec rm -f $graphfile
        set outdesc [open $graphfile w]
        #
        # this next part is xgraph specific
        #
        puts $outdesc "TitleText: $graphtitle"
        puts $outdesc "Device: Postscript"

        puts "writing flow xgraph data to $graphfile..."

        exec sort -n +1 -o $flowfile $flowfile
        exec awk [time_awk] $flowfile >@ $outdesc
        close $outdesc
}

# Plot per-flow bandwidth vs. time.
PostProcess instproc plot_dropmetric { name } {
	$self instvar format_ linkgraphfile_
	$self create_time_graph $name $linkgraphfile_
	puts "running time xgraph for plotting arrivals..."
	if { $format_ == "xgraph" } {
	  exec xgraph -bb -tk -m -ly 0,100 -x "time" -y "Bandwidth(%)" $timegraphfile &
	}
	puts stderr "graph format $format_ unknown"
}

#--------------------------------------------------------------------------

# awk code used to produce:
#      x axis: time
#      y axis: per-flow bytes
PostProcess instproc byte_awk { } {
	puts "In byte_awk"
        set awkCode {
		BEGIN { new = 1; prev = -1 }
		{
			class = $1;
			time = $2;
			bytes = $3;
			if (class != prev) {
				prev = class;
				if (new==1) {new=0;}
				else {print " "; }
				print "\"flow "prev;
			}
			if (bytes > oldbytes[class]) {
				if (oldtime[class]==0) {
					interval = $4;
				} else { interval = time - oldtime[class]; }
				if (interval > 0) {
					bitsPerSecond = 8*(bytes - oldbytes[class])/interval;
				}
				print time, 100*bitsPerSecond/(bandwidth*1000);
				#print time, 100*bitsPerSecond/(bandwidth*1000);
			}
			oldbytes[class] = bytes;
			oldtime[class] = time;
		}
        }
        return $awkCode
}

PostProcess instproc reclass_awk { } {
	set awkCode {
		{
		print " ";
		printf "\"%s\n", $3
		print $1, 0;
		print $1, 100;
		}
	}
}

# plot time vs. per-flow bytes
PostProcess instproc create_bytes_graph { graphtitle in out bandwidth } {
        set tmpfile /tmp/fg1[pid]
	# print: time class bytes interval
	#change $4d to %4d - the last token - ratul
	set awkCode {
		{ printf "%4d %8d %16d %4d\n", $4, $2, $6, $7; }
	}

	puts "removing graph file: $out"
        exec rm -f $out
        set outdesc [open $out w]
        #
        # this next part is xgraph specific
        #
        puts $outdesc "TitleText: $graphtitle"
        puts $outdesc "Device: Postscript"

        exec rm -f $tmpfile 
        puts "writing flow data to $out ... using $in"
	exec awk $awkCode $in | sort -n -s > $tmpfile
        exec awk [$self byte_awk] bandwidth=$bandwidth $tmpfile >@ $outdesc
	exec rm -f $tmpfile
        close $outdesc
}

# Plot per-flow bytes vs. time.
PostProcess instproc plot_bytes { bandwidth } {
	$self instvar format_
	$self instvar label_ linkflowfile_ linkgraphfile_
	$self create_bytes_graph $label_ $linkflowfile_ $linkgraphfile_ $bandwidth 
	puts "running $format_ for plotting per-flow bytes..."
	if { $format_ == "xgraph" } {
		puts "In graph drawing"
		exec xgraph -bb -tk -m -ly 0,100 -x "time" -y "Bandwidth(%)" $linkgraphfile_ &
		return
	}
	if { $format_ == "" } {
		puts stderr "output format not defined"
		return
	}
	puts stderr "output format $format_ not supported"
}
#--------------------------------------------------------

# awk code used to produce:
#      x axis: time
#      y axis: aggregate drop ratios in packets
PostProcess instproc frac_awk { } {
        set awkCode {
            {
                if ($1 > prevtime){
                        if (prevtime > 0) print prevtime, 100.0 * frac
			prevtime = $1
			frac = $16/$12
		}
            }
	    END { print prevtime, 100.0 * frac }
        }
        return $awkCode
}

# plot time vs. aggregate drop ratio
PostProcess instproc create_frac_graph { graphtitle graphfile } {

        exec rm -f $graphfile
        set outdesc [open $graphfile w]
        #
        # this next part is xgraph specific
        #
        puts $outdesc "TitleText: $graphtitle"
        puts $outdesc "Device: Postscript"

        puts "writing flow xgraph data to $graphfile..."

        exec sort -n +1 -o $flowfile $flowfile
        exec awk [$self frac_awk] $flowfile >@ $outdesc
        close $outdesc
}

# plot true average of arriving packets that are dropped
PostProcess instproc plot_dropave { name } {
	$self instvar format_
	$self create_frac_graph $name $fracgraphfile 
	puts "running time xgraph for plotting drop ratios..."
	if { $format_ == "xgraph" } {
	  exec xgraph -bb -tk -m -x "time" -y "Drop_Fraction(%)" $fracgraphfile &
	}
    puts stderr "graph format $format_ unknown"
}

#--------------------------------------------------------------------

# plot tcp-friendly bandwidth 
# "factor" is packetsize/rtt, for packetsize in bytes and rtt in msec.
# bandwidth is in Kbps, goodbandwidth is in Bps
PostProcess instproc create_friendly_graph { graphtitle graphfile ratiofile bandwidth } {
	set awkCode {
		BEGIN { print "\"reference"; drops=0; packets=0;}
		{
		drops = $6 - drops;
		packets = $4 - packets;
		rtt = 0.06
		if (drops > 0) {
		  dropratio = drops/packets;
		  goodbandwidth = 1.22*factor/sqrt(dropratio);
		  print $2, 100*goodbandwidth*8/(bandwidth*1000);
		}
		drops = $6; packets = $4;
		}
	}
	set packetsize 1500
	set rtt 0.06
	set factor [expr $packetsize / $rtt]

        exec rm -f $graphfile
        set outdesc [open $graphfile w]
        #
        # this next part is xgraph specific
        #
        puts $outdesc "TitleText: $graphtitle"
        puts $outdesc "Device: Postscript"

        puts "writing friendly xgraph data to $graphfile..."

	exec cat Ref >@ $outdesc
	exec awk $awkCode bandwidth=$bandwidth factor=$factor $ratiofile >@ $outdesc
        close $outdesc
}

# Plot tcp-friendly bandwidth.
PostProcess instproc plot_friendly { name bandwidth } {
	$self instvar format_ 
	puts "beginning time xgraph for tcp-friendly bandwidth..."
	$self create_friendly_graph $name $friendlygraphfile $ratiofile $bandwidth
	puts "running time xgraph for tcp-friendly bandwidth..."
	if { $format_ == "xgraph" } {
	  exec xgraph -bb -tk -m -ly 0,200 -x "time" -y "Bandwidth(%)" $friendlygraphfile &
	}
    puts stderr "graph format $format_ unknown"
}

#----------------------------------------------------------------

# x: flow id
# y: total number of bytes received in the simulation

PostProcess instproc plot_cumBytes {infile outfile} {
	$self instvar format_ label_
	set awkcode {
		{ bytes[$4] = $6 }
		END { for (i in bytes) printf("%d %d\n", i, bytes[i])}
	}

	set outdesc [open $outfile w]
	puts $outdesc "TitleText: $label_"
	puts $outdesc "Device: Postscript"

	exec awk $awkcode $infile >@ $outdesc
	close $outdesc

	if { $format_ == "xgraph" } {
		exec xgraph -bb -tk -bar -brw 0.5 -nl -x "flowID" -y "Bytes Sent" $outfile &
	}

}

PostProcess instproc plot_tcpRate {file} {

	$self instvar format_	

	if { $format_ == "xgraph" } {
		exec xgraph -bb -tk -bar -brw 0.5 -nl -x "flowID" -y "Rate" $file &
	}

}
