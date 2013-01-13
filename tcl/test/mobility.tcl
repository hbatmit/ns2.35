#
# nodes: 5, pause: 400.00, max speed: 20.00  max x = 670.00, max y: 670.00
#
set god_ [God instance]
$node_(0) set X_ 143.437509218780
$node_(0) set Y_ 94.217567273200
$node_(0) set Z_ 0.000000000000
$node_(1) set X_ 304.653244246784
$node_(1) set Y_ 167.076325969835
$node_(1) set Z_ 0.000000000000
$node_(2) set X_ 81.810723227699
$node_(2) set Y_ 152.825360505610
$node_(2) set Z_ 0.000000000000
$node_(3) set X_ 325.834153339995
$node_(3) set Y_ 354.615563074325
$node_(3) set Z_ 0.000000000000
$node_(4) set X_ 23.768638624379
$node_(4) set Y_ 159.509381009931
$node_(4) set Z_ 0.000000000000
$god_ set-dist 0 1 1
$god_ set-dist 0 2 1
$god_ set-dist 0 3 2
$god_ set-dist 0 4 1
$god_ set-dist 1 2 1
$god_ set-dist 1 3 1
$god_ set-dist 1 4 2
$god_ set-dist 2 3 2
$god_ set-dist 2 4 1
$god_ set-dist 3 4 3
$ns_ at 400.000000000000 "$node_(0) setdest 204.166775406815 360.994443446432 11.451084311191"
$ns_ at 400.000000000000 "$node_(1) setdest 615.529949830732 411.867351157209 14.464813867968"
$ns_ at 400.000000000000 "$node_(2) setdest 339.244174366253 646.838874543792 0.028806906205"
$ns_ at 400.000000000000 "$node_(3) setdest 139.282032746586 603.124495402331 8.459545135266"
$ns_ at 400.000000000000 "$node_(4) setdest 655.765713025551 624.339402170651 12.009935402613"
$ns_ at 402.190313067921 "$god_ set-dist 1 2 2"
$ns_ at 402.190313067921 "$god_ set-dist 2 3 3"
$ns_ at 406.245213537287 "$god_ set-dist 0 3 1"
$ns_ at 406.245213537287 "$god_ set-dist 2 3 2"
$ns_ at 406.245213537287 "$god_ set-dist 3 4 2"
$ns_ at 409.445211616863 "$god_ set-dist 0 1 2"
$ns_ at 409.445211616863 "$god_ set-dist 1 2 3"
$ns_ at 409.445211616863 "$god_ set-dist 1 4 3"
$ns_ at 412.724403589490 "$god_ set-dist 1 4 2"
$ns_ at 412.724403589490 "$god_ set-dist 3 4 1"
$ns_ at 421.330331512029 "$god_ set-dist 0 1 16777215"
$ns_ at 421.330331512029 "$god_ set-dist 1 2 16777215"
$ns_ at 421.330331512029 "$god_ set-dist 1 3 16777215"
$ns_ at 421.330331512029 "$god_ set-dist 1 4 16777215"
$ns_ at 423.893091118903 "$node_(0) setdest 204.166775406815 360.994443446433 0.000000000000"
$ns_ at 424.170572475068 "$god_ set-dist 2 4 2"
$ns_ at 427.355031976147 "$node_(1) setdest 615.529949830732 411.867351157209 0.000000000000"
$ns_ at 435.324757998609 "$god_ set-dist 0 1 2"
$ns_ at 435.324757998609 "$god_ set-dist 1 2 3"
$ns_ at 435.324757998609 "$god_ set-dist 1 3 2"
$ns_ at 435.324757998609 "$god_ set-dist 1 4 1"
$ns_ at 442.338397622170 "$god_ set-dist 0 1 3"
$ns_ at 442.338397622170 "$god_ set-dist 0 4 2"
$ns_ at 442.338397622170 "$god_ set-dist 1 2 4"
$ns_ at 442.338397622170 "$god_ set-dist 2 4 3"
$ns_ at 446.824570750405 "$god_ set-dist 0 1 16777215"
$ns_ at 446.824570750405 "$god_ set-dist 0 4 16777215"
$ns_ at 446.824570750405 "$god_ set-dist 1 2 16777215"
$ns_ at 446.824570750405 "$god_ set-dist 1 3 16777215"
$ns_ at 446.824570750405 "$god_ set-dist 2 4 16777215"
$ns_ at 446.824570750405 "$god_ set-dist 3 4 16777215"
$ns_ at 465.323411734640 "$node_(4) setdest 655.765713025551 624.339402170651 0.000000000000"
$ns_ at 473.072413525966 "$god_ set-dist 0 3 16777215"
$ns_ at 473.072413525966 "$god_ set-dist 2 3 16777215"
$ns_ at 473.153941050381 "$node_(3) setdest 139.282032746586 603.124495402331 0.000000000000"
#
# Destination Unreachables: 12
#
# Route Changes: 31
#
# Link Changes: 10
#
# Node | Route Changes | Link Changes
#    0 |             9 |            4
#    1 |            18 |            4
#    2 |            12 |            2
#    3 |            11 |            5
#    4 |            12 |            5
#
