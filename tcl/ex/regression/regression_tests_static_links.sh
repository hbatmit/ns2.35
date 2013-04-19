#! /bin/bash

cd ..

# SFD+FCFS test on 100 TCP users on fixed link rates from 1 through 100 mbps
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc SFD -num_tcp 100  -num_udp 0 -duration 100 -ensemble_scheduler fcfs -congestion_control cubic  -headroom 0.0 -link_trace regression/1to100mbps.trace > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/sfd-fcfs-tcp100-longrunning-1to100mbps.output
echo "Result of test is $?"

# SFD+PF test on 100 TCP users on fixed link rates from 1 through 100 mbps
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc SFD -num_tcp 100  -num_udp 0 -duration 100 -ensemble_scheduler pf -congestion_control cubic  -headroom 0.0 -link_trace regression/1to100mbps.trace > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/sfd-pf-tcp100-longrunning-1to100mbps.output
echo "Result of test is $?"

# CoDel test on 100 TCP users on fixed link rates from 1 through 100 mbps
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc CoDel -num_tcp 100  -num_udp 0 -duration 100 -ensemble_scheduler pf -cdma_ewma_slots 100 -congestion_control cubic -headroom 0.0 -link_trace regression/1to100mbps.trace > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/codel-tcp100-longrunning-1to100mbps.output
echo "Result of test is $?"

# SFD+FCFS test on 100 TCP users on fixed link rates from 1 through 100 mbps; TCP ON/OFF Pattern
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc SFD -num_tcp 100  -num_udp 0 -duration 100 -ensemble_scheduler fcfs -congestion_control cubic  -headroom 0.0 -link_trace regression/1to100mbps.trace -enable_on_off true  > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/sfd-fcfs-tcp100-onoff-1to100mbps.output
echo "Result of test is $?"

# SFD+PF test on 100 TCP users on fixed link rates from 1 through 100 mbps; TCP ON/OFF Pattern
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc SFD -num_tcp 100  -num_udp 0 -duration 100 -ensemble_scheduler pf -congestion_control cubic  -headroom 0.0 -link_trace regression/1to100mbps.trace -enable_on_off true  > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/sfd-pf-tcp100-onoff-1to100mbps.output
echo "Result of test is $?"

# CoDel test on 100 TCP users on fixed link rates from 1 through 100 mbps; TCP ON/OFF Pattern
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc CoDel -num_tcp 100  -num_udp 0 -duration 100 -ensemble_scheduler pf -cdma_ewma_slots 100 -congestion_control cubic  -headroom 0.0 -link_trace regression/1to100mbps.trace -enable_on_off true > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/codel-tcp100-onoff-1to100mbps.output
echo "Result of test is $?"

cd -
