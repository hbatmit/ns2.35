#! /bin/bash

cd ..

# Regression tests where each user has a dynamic link.
# Furthermore the rate variation is uncorrelated across users.

# sfqCoDel on 100ms coherence time link, 10 long running TCP flows
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc sfqCoDel -num_tcp 10  -num_udp 0 -duration 100 -ensemble_scheduler pf -cdma_ewma_slots 100 -congestion_control cubic  -headroom 0.0 -link_trace regression/v4g.100msec10users -enable_on_off false > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/sfqCoDel10users-v4g.100ms-longrunning
echo "Result of test is $?"

# sfqCoDel on 100ms coherence time link, 10 on/off TCP flows
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc sfqCoDel -num_tcp 10  -num_udp 0 -duration 100 -ensemble_scheduler pf -cdma_ewma_slots 100 -congestion_control cubic  -headroom 0.0 -link_trace regression/v4g.100msec10users -enable_on_off true > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/sfqCoDel10users-v4g.100ms-onoff
echo "Result of test is $?"

# SFD+PF on 100ms coherence time link, 10 long running TCP flows
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc SFD -num_tcp 10  -num_udp 0 -duration 100 -ensemble_scheduler pf -congestion_control cubic  -headroom 0.0 -link_trace regression/v4g.100msec10users -enable_on_off false > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/SFDPF-10users-v4g.100ms-longrunning
echo "Result of test is $?"

# SFD+PF on 100ms coherence time link, 10 on/off TCP flows.
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc SFD -num_tcp 10  -num_udp 0 -duration 100 -ensemble_scheduler pf -congestion_control cubic  -headroom 0.0 -link_trace regression/v4g.100msec10users -enable_on_off true > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/SFDPF-10users-v4g.100ms-onoff
echo "Result of test is $?"

# SFD+FCFS on 100ms coherence time link, 10 long running TCP flows
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc SFD -num_tcp 10  -num_udp 0 -duration 100 -ensemble_scheduler fcfs -congestion_control cubic  -headroom 0.0 -link_trace regression/v4g.100msec10users -enable_on_off false > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/SFDFCFS-10users-v4g.100ms-longrunning
echo "Result of test is $?"

# SFD+FCFS on 100ms coherence time link, 10 on/off TCP flows
set -v
set -x
../../ns cell-link.tcl -bottleneck_qdisc SFD -num_tcp 10  -num_udp 0 -duration 100 -ensemble_scheduler fcfs -congestion_control cubic  -headroom 0.0 -link_trace regression/v4g.100msec10users -enable_on_off true > /dev/null 2> /dev/null
set +v
set +x
python ../../utils/parse-trace.py cell-link.tr 100 0.95 > /dev/null 2> test.output
diff test.output regression/SFDFCFS-10users-v4g.100ms-onoff
echo "Result of test is $?"

cd -
