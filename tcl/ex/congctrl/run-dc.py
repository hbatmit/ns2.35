import os
import sys
import subprocess

id = int(sys.argv[1])
#runstr = "./remy2.tcl remyconf/datacenter.tcl -tcp DCTCP -sink TCPSink -gw RED -simtime 3 |grep conn >> dc10G-n32/dc%d" % id
runstr = "./remy2.tcl remyconf/datacenter.tcl -tcp TCP/Rational -sink TCPSink -gw RED -simtime 3 |grep conn >> dc10G-n64/remyred%d" % id

for i in range(id, id+1):
    print runstr
    fnull = open(os.devnull, "w") 
    output = subprocess.call(runstr, stderr=fnull, shell=True)    
