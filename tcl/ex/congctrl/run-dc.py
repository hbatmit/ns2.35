import os
import sys
import subprocess

id = int(sys.argv[1])*4
runstr = "./remy2.tcl remyconf/datacenter.tcl -tcp DCTCP -sink TCPSink -gw RED -simtime 2 |grep conn >> dc/t%d" % id

for i in range(id, id+4):
    print runstr
    fnull = open(os.devnull, "w") 
    output = subprocess.call(runstr, stderr=fnull, shell=True)    
