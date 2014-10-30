#
# nodes: 30, send rate: 0.167
# seed: 0.0
#
#
# node 7 is ready to send data type 0 at time 0.036182026954452516
#
set src_(0) [new Agent/Diff_Sink]
$ns_ attach-agent $node_(7) $src_(0)
$ns_ put-in-list $src_(0)
$src_(0) set packetSize_ 64
$src_(0) set interval_ 0.167
$src_(0) data-type 0
$src_(0) always-max-rate
$src_(0) $opt(duplicate)
$node_(7) color red
$ns_ at 0.036182026954452516 "$src_(0) ready"

#
# node 3 expresses interest type 0 at time 0.036182026954452516
#
set sk_(0) [new Agent/Diff_Sink]
$ns_ attach-agent $node_(3) $sk_(0)
$ns_ put-in-list $sk_(0)
$sk_(0) set packetSize_ 64
$sk_(0) set interval_ 0.167
$sk_(0) data-type 0
$sk_(0) always-max-rate
$sk_(0) $opt(duplicate)
$node_(3) color blue
$ns_ at 0.036182026954452516 "$sk_(0) announce"
