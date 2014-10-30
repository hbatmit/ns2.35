#
# cplot -- "cooked" plot
#	merge multiple cooked trace files together, eventually
#	to produce a final plot:
#
# Usage: cplot package graph-title cfile1 cname1 [cfile2 cname 2] ...
#
# this will take cooked trace files cfile{1,2,...}
# and merge them into a combined graph of the type defined in
# "package".  for now, package is either xgraph or gnuplot
#
# Sources for gnuplot, as of 10/20/97:
# 	gnuplot3.5: ftp://ftp.dartmouth.edu/pub/gnuplot/
#	gnuplot3.6beta: ftp://cmpc1.phys.soton.ac.uk/pub
#	gnuplot3.6beta-mirror: http://www.nas.nasa.gov/~woo/gnuplot/beta/

set labelproc(xgraph) xgraph_label
set labelproc(gnuplot) gnuplot_label
set headerproc(xgraph) xgraph_header
set headerproc(gnuplot) gnuplot_header
set filext(xgraph) xgr
set filext(gnuplot) plt

set package default; # graphics package to use

if { $argc < 4 || [expr $argc & 1] } {
	puts stderr "Usage: tclsh cplot graphics-package graph-title cfile1 cname1 \[cfile2 cname2\] ..."
	exit 1
}

proc init {} {
	global tmpchan tmpfile
	set tmpfile /tmp/[pid].tmp
	set tmpchan [open $tmpfile w+]
}

proc cleanup {} {
	global tmpchan tmpfile package filext
	seek $tmpchan 0 start
	exec cat <@ $tmpchan >@ stdout
	close $tmpchan
	exec rm -f $tmpfile
}

proc run {} {
	global labelproc headerproc package argv tmpchan
	init
	set package [lindex $argv 0]
	set title [lindex $argv 1]
	if { ![info exists labelproc($package)] } {
		puts stderr "cplot: invalid output package $package, known packages: [array names labelproc]"
		exit 1
	}
	set ifile 2
	set iname 3
	$headerproc($package) $tmpchan $title
	while {1} {
		set fname [lindex $argv $ifile]
		set label [lindex $argv $iname]
		if { $fname == "" || $label == "" } {
			break
		}
		do_file $fname $label $package $tmpchan
		incr ifile 2
		incr iname 2
	}
	cleanup
}

proc do_file { fname label graphtype tmpchan } {
	global labelproc
	$labelproc($graphtype) $tmpchan $label $fname
}

#
# xgraph-specific stuff
#

proc xgraph_header { tmpchan title } {
        puts $tmpchan "TitleText: $title"
        puts $tmpchan "Device: Postscript"
	puts $tmpchan "BoundBox: true"
	puts $tmpchan "Ticks: true"
	puts $tmpchan "Markers: true"
	puts $tmpchan "NoLines: true"
	puts $tmpchan "XUnitText: time"
	puts $tmpchan "YUnitText: sequence/ack number"
}

proc xgraph_label { tmpchan label fname } {
	puts $tmpchan \n\"$label
	exec cat $fname >@ $tmpchan
}

#
# gnuplot-specific stuff
#

proc gnuplot_header { tmpchan title } {
	puts $tmpchan "set title '$title'"
	puts $tmpchan "set xlabel 'time'"
	puts $tmpchan "set ylabel 'sequence/ack number'"
	puts $tmpchan "set grid"

	global gnu_first_time gnu_label_index
	set gnu_first_time 1
	set gnu_label_index 1
}

proc gnuplot_label { tmpchan label fname } {
	global gnu_first_time gnu_label_index
	if { $gnu_first_time } {
		puts $tmpchan "plot '$fname' title '$label' w points $gnu_label_index 2"
		set gnu_first_time 0
	} else {
		puts $tmpchan "replot '$fname' title '$label' w points $gnu_label_index 2"
	}
	incr gnu_label_index
}

run
