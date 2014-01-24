#! /bin/bash
set -x
rm -rf /tmp/withoutnewreno
mkdir /tmp/withoutnewreno

# Newreno alone
run=1;
while [ $run -lt 11 ]; do
  ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Newreno -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/newreno.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 11 ]; do
  ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Newreno -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/newreno.5sec
  run=`expr $run '+' 1`
done

###### Without Newreno #####
run=1;
while [ $run -lt 11 ]; do
  WHISKERS=ratsonlybaselineV4.dna.6 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/baseline.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=ratsonlybaselineV4.dna.6 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/baseline.5sec
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=tcpantagonizeV4.dna.4 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/antagonize.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=tcpantagonizeV4.dna.4 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/withoutnewreno/antagonize.5sec
  run=`expr $run '+' 1`
done

##### With Newreno #####
rm -rf /tmp/withnewreno
mkdir /tmp/withnewreno

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=ratsonlybaselineV4.dna.6 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run -withnewreno 2>&1 | grep mbps >> /tmp/withnewreno/baseline.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=ratsonlybaselineV4.dna.6 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run -withnewreno 2>&1 | grep mbps >> /tmp/withnewreno/baseline.5sec
  run=`expr $run '+' 1`
done
run=1;
while [ $run -lt 11 ]; do
  WHISKERS=tcpantagonizeV4.dna.4 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run -withnewreno 2>&1 | grep mbps >> /tmp/withnewreno/antagonize.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=tcpantagonizeV4.dna.4 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run -withnewreno 2>&1 | grep mbps >> /tmp/withnewreno/antagonize.5sec
  run=`expr $run '+' 1`
done

# Analyze results
cat /tmp/withoutnewreno/baseline.5sec       | python sim_scripts/analyze_compatibility.py "*" "tcp-naive-5sec"
cat /tmp/withoutnewreno/baseline.alwayson   | python sim_scripts/analyze_compatibility.py "*" "tcp-naive-alwayson"

cat /tmp/withoutnewreno/antagonize.5sec     | python sim_scripts/analyze_compatibility.py "*" "tcp-antagonize-5sec"
cat /tmp/withoutnewreno/antagonize.alwayson | python sim_scripts/analyze_compatibility.py "*" "tcp-antagonize-alwayson"

cat /tmp/withoutnewreno/newreno.5sec        | python sim_scripts/analyze_compatibility.py "*" "newreno-5sec"
cat /tmp/withoutnewreno/newreno.alwayson    | python sim_scripts/analyze_compatibility.py "*" "newreno-alwayson"

cat /tmp/withnewreno/baseline.5sec          | python sim_scripts/analyze_compatibility.py 0 "tcp-naive-5sec-withnewreno"
cat /tmp/withnewreno/baseline.5sec          | python sim_scripts/analyze_compatibility.py 1 "newreno-5sec-withtcp-naive"

cat /tmp/withnewreno/baseline.alwayson      | python sim_scripts/analyze_compatibility.py 0 "tcp-naive-alwayson-withnewreno"
cat /tmp/withnewreno/baseline.alwayson      | python sim_scripts/analyze_compatibility.py 1 "newreno-alwayson-withtcp-naive"

cat /tmp/withnewreno/antagonize.5sec        | python sim_scripts/analyze_compatibility.py 0 "tcp-antagonize-5sec-withnewreno"
cat /tmp/withnewreno/antagonize.5sec        | python sim_scripts/analyze_compatibility.py 1 "newreno-5sec-withtcp-antagonize"

cat /tmp/withnewreno/antagonize.alwayson    | python sim_scripts/analyze_compatibility.py 0 "tcp-antagonize-alwayson-withnewreno"
cat /tmp/withnewreno/antagonize.alwayson    | python sim_scripts/analyze_compatibility.py 1 "newreno-alwayson-withtcp-antagonize"
