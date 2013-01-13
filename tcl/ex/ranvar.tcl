##
## simple example demonstrating use of the RandomVariable class from tcl
## (this doesn't perform any simulation).
##

set count 10

set r1 [new RandomVariable/Pareto]
$r1 set avg_ 10.0
$r1 set shape_ 1.2

puts stdout "Testing Pareto Distribution, avg = [$r1 set avg_] shape = [$r1 set shape_]"

$r1 test $count

set r2 [new RandomVariable/Constant]
$r2 set avg_ 5.0

puts stdout "Testing Constant Distribution, avg = [$r2 set avg_]"
$r2 test $count

set r3 [new RandomVariable/Uniform]
$r3 set min_ 0.0
$r3 set max_ 10.0
puts stdout "Testing Uniform Distribution, min = [$r3 set min_] max = [$r3 set max_]"
$r3 test $count

set r4 [new RandomVariable/Exponential]
$r4 set avg_ 10
puts stdout "Testing Exponential Distribution, avg = [$r4 set avg_]"
$r4 test $count

set r5 [new RandomVariable/HyperExponential]
$r5 set avg_ 1.0
$r5 set cov_ 4.0
puts stdout "Testing HyperExponential Distribution, avg = [$r5 set avg_] cov = [$r5 set cov_]"
$r5 test $count

