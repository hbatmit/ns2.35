import sys
fh=open(sys.argv[1])                     # filename for pps file
duration=int(sys.argv[2])                # duration to print rates in seconds
BIN_INTERVAL=100                         # i.e number of ms in one bandwidth bin
acc=[0]*(1+duration*(1000/BIN_INTERVAL)) # Create enough bins for 'duration' seconds of data. The 1000 is to convert ms to seconds.

for line in fh.readlines() :
  time=int(float(line.split()[0]));      # time in trace file in ms
  if time > duration*1000 : break        # if it exceeds duration , break
  acc[(time/BIN_INTERVAL)]=acc[(time/BIN_INTERVAL)]+8*1500;

for i in range(0,len(acc)) :
    print "$ns at ",i*BIN_INTERVAL/1000.0,"\" link set bandwidth_ ",(acc[i]*1000.0/BIN_INTERVAL),"\""; # in bits per second
    # time in ms is i*BIN_INTERVAL, divide by 1000 to get seconds
    # bandwidth for that bin is number of bits i.e acc[i] / number of seconds. number of seconds is BIN_INTERVAL/1000.
