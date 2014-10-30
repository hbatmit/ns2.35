#
# Usage: ns rng-test2.tcl [replication number]
#

if {$argc > 1} {
    puts "Usage: ns rng-test2.tcl \[replication number\]"
    exit
}
set run 1
if {$argc == 1} {
    set run [lindex $argv 0]
}
if {$run < 1} {
    set run 1
}

# the default RNG is seeded with 12345

# create the RNGs and set them to the correct substream
set arrivalRNG [new RNG]
set sizeRNG [new RNG]
for {set j 1} {$j < $run} {incr j} {
    $arrivalRNG next-substream
    $sizeRNG next-substream
}

 # print the first 5 arrival times and sizes
for {set j 0} {$j < 5} {incr j} {
    puts [format "%-8.3f  %-4d" [$arrivalRNG lognormal 5 0.1] \
            [expr round([$sizeRNG normal 5000 100])]]
}
