proc getopt {argc argv} {
        global opt
	lappend optlist tr 

        for {set i 0} {$i < $argc} {incr i} {
                set arg [lindex $argv $i]
                if {[string range $arg 0 0] != "-"} continue

	                set name [string range $arg 1 end]
	                set opt($name) [lindex $argv [expr $i+1]]
        }
}
		
getopt $argc $argv

set f [open $opt(file) r]
while {[gets $f line] >= 0} {

 set xitem [lindex $line 0]
 set yitem [lindex $line 2]

 if { [expr $xitem] != 0} {
 	puts "[expr log10($xitem)] $yitem"
 }
}
