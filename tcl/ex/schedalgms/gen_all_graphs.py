#! /usr/bin/python
import sys
target_ip=sys.argv[1]
from os import system
for linkconfig in ["1mbps", "15mbps", "v3g", "v4g"]:
  for propdelay in [0.02, 0.075]:
    for nsrc in [1, 2, 5, 10]:
      for onbytes in [100000, 10000000]:
        for cc in ["TCP/Linux/cubic", "TCP/Newreno"]:
           offtime=0.5;
           linktrace="link-traces/"+str(nsrc)+"users"+linkconfig+".lt"
           suffix="link."+linktrace.split('/')[-1]+"."+str(propdelay)+"."+"nconn"+str(nsrc)+"."+"bytes"+"."+str(onbytes)+"."+cc.split('/')[-1]+"."+str(offtime);

           genstr="./generate-plots.sh "+str(2*propdelay*1000.0)+" "+suffix+" "+target_ip+" "+str(nsrc);
           print genstr
           system(genstr);
