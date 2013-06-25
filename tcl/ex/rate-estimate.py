import sys
fh=sys.stdin                               # filename for pps file
start_time=int(sys.argv[1])                # start time for printing rates in seconds
end_time=int(sys.argv[2])                  # end time for printing rates in seconds
BIN_INTERVAL=int(sys.argv[3])              # i.e number of ms in one bandwidth bin
acc=[0]*(1+((1000*end_time)/BIN_INTERVAL)) # Create enough bins for 'end_time' seconds of data. The 1000 is to convert ms to seconds.
user_id=int(sys.argv[4])                   # User id to insert into link trace.

for line in fh.readlines() :
  time=int(float(line.split()[0]));        # time in trace file in ms
  if( (time > end_time*1000) or (time < start_time*1000) ) :
    continue                               # if it doesn't fall between start_time and end_time , continue
  else :
    acc[(time/BIN_INTERVAL)]=acc[(time/BIN_INTERVAL)]+8*1500;

for i in range(0,len(acc)) :
    current_time = (i*BIN_INTERVAL/1000.0);
    if ( (current_time >= start_time) and (current_time <= end_time) ) :
      relative_time = current_time - start_time
      print relative_time, user_id, (acc[i]*1000.0/BIN_INTERVAL);
    # rate is in bits per second
    # time in ms is i*BIN_INTERVAL, divide by 1000 to get seconds
    # bandwidth for that bin is number of bits i.e acc[i] / number of seconds. number of seconds is BIN_INTERVAL/1000.
