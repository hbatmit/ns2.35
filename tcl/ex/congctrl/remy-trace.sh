#!/bin/sh 

#time sh -c "export WHISKERS=/Users/hari/wtcp/src/remy/ns-allinone-2.35/ns2.35/tcp/remy/rats/sweeps/rtt150_tp_4.7_to_47.dna && ./remy2.tcl remyconf/generalizability.tcl -bneck 15Mb -nsrc 2 -seed 5 -simtime 100 -tracewhisk 0 -reset false" 2> remycc-whisk-15Mb-150ms-nsrc2-seed5-general-rtt150_tp_4.7_to_47-conn1

#time sh -c "export WHISKERS=/Users/hari/wtcp/src/remy/ns-allinone-2.35/ns2.35/tcp/remy/rats/sweeps/rtt150_tp_4.7_to_47.dna && ./remy2.tcl remyconf/generalizability.tcl -bneck 15Mb -nsrc 2 -seed 5 -simtime 100 -tracewhisk 1 -reset false -tr remycc" 2> remycc-whisk-15Mb-150ms-nsrc2-seed5-general-rtt150_tp_4.7_to_47-conn1

grep maxseq_ remycc.tr|grep "0  0  2" |awk '{print $1, $7}' > remycc0-seq
grep ack_ remycc.tr|grep "0  0  2" |awk '{print $1, $7}' > remycc0-ack
grep maxseq_ remycc.tr|grep "1  0  2" |awk '{print $1, $7}' > remycc1-seq
grep ack_ remycc.tr|grep "1  0  2" |awk '{print $1, $7}' > remycc1-ack
