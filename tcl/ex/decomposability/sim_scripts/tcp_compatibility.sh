#! /bin/bash
set -x
rm -rf /tmp/withoutnewreno
mkdir /tmp/withoutnewreno

# Newreno alone
run=1;
while [ $run -lt 51 ]; do
  ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Newreno -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/newreno.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 51 ]; do
  ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Newreno -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/newreno.5sec
  run=`expr $run '+' 1`
done

###### Without Newreno #####
run=1;
while [ $run -lt 51 ]; do
  WHISKERS=ratsonlybaselineV5-delta0.1.dna.2 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/baseline.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 51 ]; do
  WHISKERS=ratsonlybaselineV5-delta0.1.dna.2 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/baseline.5sec
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 51 ]; do
  WHISKERS=tcpcompatibleV5-delta0.1.dna.3 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/compatible.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 51 ]; do
  WHISKERS=tcpcompatibleV5-delta0.1.dna.3 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/compatible.5sec
  run=`expr $run '+' 1`
done

##### With Newreno #####
rm -rf /tmp/withnewreno
mkdir /tmp/withnewreno

run=1;
while [ $run -lt 51 ]; do
  WHISKERS=ratsonlybaselineV5-delta0.1.dna.2 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run -withnewreno 2>&1 | grep mbps >> /tmp/withnewreno/baseline.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 51 ]; do
  WHISKERS=ratsonlybaselineV5-delta0.1.dna.2 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run -withnewreno 2>&1 | grep mbps >> /tmp/withnewreno/baseline.5sec
  run=`expr $run '+' 1`
done
run=1;
while [ $run -lt 51 ]; do
  WHISKERS=tcpcompatibleV5-delta0.1.dna.3 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run -withnewreno 2>&1 | grep mbps >> /tmp/withnewreno/compatible.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 51 ]; do
  WHISKERS=tcpcompatibleV5-delta0.1.dna.3 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run -withnewreno 2>&1 | grep mbps >> /tmp/withnewreno/compatible.5sec
  run=`expr $run '+' 1`
done
set +x

# Analyze results
cat /tmp/withoutnewreno/baseline.5sec       | python sim_scripts/analyze_compatibility.py "*" "tcp-naive-5sec"
cat /tmp/withoutnewreno/baseline.alwayson   | python sim_scripts/analyze_compatibility.py "*" "tcp-naive-alwayson"

cat /tmp/withoutnewreno/compatible.5sec     | python sim_scripts/analyze_compatibility.py "*" "tcp-compatible-5sec"
cat /tmp/withoutnewreno/compatible.alwayson | python sim_scripts/analyze_compatibility.py "*" "tcp-compatible-alwayson"

cat /tmp/withoutnewreno/newreno.5sec        | python sim_scripts/analyze_compatibility.py "*" "newreno-5sec"
cat /tmp/withoutnewreno/newreno.alwayson    | python sim_scripts/analyze_compatibility.py "*" "newreno-alwayson"

cat /tmp/withnewreno/baseline.5sec          | python sim_scripts/analyze_compatibility.py 0 "tcp-naive-5sec-withnewreno"
cat /tmp/withnewreno/baseline.5sec          | python sim_scripts/analyze_compatibility.py 1 "newreno-5sec-withtcp-naive"

cat /tmp/withnewreno/baseline.alwayson      | python sim_scripts/analyze_compatibility.py 0 "tcp-naive-alwayson-withnewreno"
cat /tmp/withnewreno/baseline.alwayson      | python sim_scripts/analyze_compatibility.py 1 "newreno-alwayson-withtcp-naive"

cat /tmp/withnewreno/compatible.5sec        | python sim_scripts/analyze_compatibility.py 0 "tcp-compatible-5sec-withnewreno"
cat /tmp/withnewreno/compatible.5sec        | python sim_scripts/analyze_compatibility.py 1 "newreno-5sec-withtcp-compatible"

cat /tmp/withnewreno/compatible.alwayson    | python sim_scripts/analyze_compatibility.py 0 "tcp-compatible-alwayson-withnewreno"
cat /tmp/withnewreno/compatible.alwayson    | python sim_scripts/analyze_compatibility.py 1 "newreno-alwayson-withtcp-compatible"
