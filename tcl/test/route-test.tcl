set r [new RouteLogic]
foreach pair "{1 2} {1 5} {2 3} {3 5} {5 0} {3 4}" {
    set src [lindex $pair 0]
    set dst [lindex $pair 1]
    $r insert $src $dst
    $r insert $dst $src
}
$r compute
set L "0 1 2 3 4 5"
foreach i $L {
	foreach j $L {
		puts "$i -> $j via [$r cmd lookup $i $j]"
	}
}
