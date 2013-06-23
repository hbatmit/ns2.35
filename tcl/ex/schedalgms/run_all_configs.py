#! /usr/bin/python
from os import system
for linkconfig in ["1mbps", "15mbps", "v3g", "v4g"]:
  for propdelay in [0.02, 0.075]:
    for nsrc in [1, 2, 5, 10]:
      for onbytes in [100000, 10000000]:
        for cc in ["TCP/Linux/cubic", "TCP/Newreno"]:
           offtime=0.5;
           linktrace="link-traces/"+str(nsrc)+"users"+linkconfig+".lt"
           runstr="./run-tcps.sh "+linktrace+" "+str(propdelay)+" "+str(nsrc)+" "+"bytes"+" "+str(onbytes)+" "+cc+" "+str(offtime);
           print runstr
           system(runstr);
           suffix="link."+linktrace.split('/')[-1]+"."+str(propdelay)+"."+"nconn"+str(nsrc)+"."+"bytes"+"."+str(onbytes)+"."+cc.split('/')[-1]+"."+str(offtime);

           while (system("ps -e | grep \" ns\" ") == 0) :
             system("sleep 1")
             print "Waiting"
