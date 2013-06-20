#! /bin/bash
if [ $# -lt 5 ] ; then
  echo "Enter parent trace file, bin size in ms, number of users, duration, and name of output file"
  exit 5
fi

parent=$1
bin=$2
num=$3
duration=$4
name=$5

user=0
start_time=0
while [ $user -lt $num ]; do
  end_time=`expr $start_time '+' $duration`
  cat $parent | python ../rate-estimate.py $start_time $end_time $bin $user > /tmp/cellular.$user
  user=`expr $user '+' 1`
  start_time=`expr $start_time '+' $duration`
done

cat /tmp/cellular.* > $name
