import sys
fh=open(sys.argv[1])
duration=int(sys.argv[2])
BIN_INTERVAL=10                      # i.e number of ms in one bandwidth bin
acc=[0]*duration*(1000/BIN_INTERVAL) # Create enough bins for 'duration' seconds of data

for line in fh.readlines() :
  time=int(float(line.split()[0]));
  acc[(time/BIN_INTERVAL)]=acc[(time/BIN_INTERVAL)]+8*1500;

for i in range(0,len(acc)) :
    print "$ns at ",i*BIN_INTERVAL/1000.0,"\" link set bandwidth_ ",(acc[i]*1000.0/BIN_INTERVAL),"\""; # in bits per second
