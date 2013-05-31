#! /bin/bash
rm onramp50ms.nconn10.bytesflowcdf.off0.5.simtime100
rm onramp100ms.nconn10.bytesflowcdf.off0.5.simtime100
rm onramp200ms.nconn10.bytesflowcdf.off0.5.simtime100
rm RED.nconn10.bytesflowcdf.off0.5.simtime100
rm sfqCoDel.nconn10.bytesflowcdf.off0.5.simtime100
tcp_cc=$1
i=1
set -x

outdir=15mb-flowcdf

while [ $i -lt 11 ]; do
  echo $i
#  ../../../ns onramp.tcl -bottleneck_qdisc SFD -num_tcp 10 -simtime 100 -ensemble_scheduler fcfs -link_trace link-traces/10users15mbps.lt -bottleneck_latency 75ms -ontype flowcdf -iter $i -offavg 0.5 -est_time_constant 0.05 -tcp $tcp_cc 2> /dev/null | grep sndrttMs > $outdir/onramp50ms.nconn10.bytesflowcdf.off0.5.simtime100-iter$i &

#  ../../../ns onramp.tcl -bottleneck_qdisc SFD -num_tcp 10 -simtime 100 -ensemble_scheduler fcfs -link_trace link-traces/10users15mbps.lt -bottleneck_latency 75ms -ontype flowcdf -iter $i -offavg 0.5 -est_time_constant 0.1 -tcp $tcp_cc 2> /dev/null | grep sndrttMs > $outdir/onramp100ms.nconn10.bytesflowcdf.off0.5.simtime100-iter$i &

#  ../../../ns onramp.tcl -bottleneck_qdisc SFD -num_tcp 10 -simtime 100 -ensemble_scheduler fcfs -link_trace link-traces/10users15mbps.lt -bottleneck_latency 75ms -ontype flowcdf -iter $i -offavg 0.5 -est_time_constant 0.2 -tcp $tcp_cc 2> /dev/null | grep sndrttMs > $outdir/onramp200ms.nconn10.bytesflowcdf.off0.5.simtime100-iter$i &

#  ../../../ns onramp.tcl -bottleneck_qdisc sfqCoDel -num_tcp 10 -simtime 100 -ensemble_scheduler pf -link_trace link-traces/10users15mbps.lt -bottleneck_latency 75ms -ontype flowcdf -iter $i -offavg 0.5 -tcp $tcp_cc -cdma_slot_duration 0.012 2> /dev/null | grep sndrttMs > $outdir/sfqCoDel.nconn10.bytesflowcdf.off0.5.simtime100-iter$i &

#  ../../../ns onramp.tcl -bottleneck_qdisc RED -num_tcp 10 -simtime 100 -ensemble_scheduler pf -link_trace link-traces/10users15mbps.lt -bottleneck_latency 75ms -ontype flowcdf -iter $i -offavg 0.5 -tcp $tcp_cc -cdma_slot_duration 0.012 2> /dev/null | grep sndrttMs > $outdir/RED.nconn10.bytesflowcdf.off0.5.simtime100-iter$i &

#  ../../../ns onramp.tcl -bottleneck_qdisc DropTail -num_tcp 10 -simtime 100 -ensemble_scheduler fcfs -link_trace link-traces/10users15mbps.lt -bottleneck_latency 75ms -ontype flowcdf -iter $i -offavg 0.5 -tcp $tcp_cc -cdma_slot_duration 0.012 2> /dev/null | grep sndrttMs > $outdir/DropTailFCFS.nconn10.bytesflowcdf.off0.5.simtime100-iter$i &

  ../../../ns onramp.tcl -bottleneck_qdisc sfqCoDel -num_tcp 10 -simtime 100 -ensemble_scheduler fcfs -link_trace link-traces/10users15mbps.lt -bottleneck_latency 75ms -ontype flowcdf -iter $i -offavg 0.5 -tcp $tcp_cc -cdma_slot_duration 0.012 2> /dev/null | grep sndrttMs > $outdir/CoDelFCFS.nconn10.bytesflowcdf.off0.5.simtime100-iter$i &

  i=`expr $i '+' 1`
done

