import os
import sys
import subprocess
import matplotlib
if os.uname()[0] == 'Darwin':
    matplotlib.use('macosx')
import matplotlib.pyplot as p
import numpy

def runonce(proto, gateway, numconns, simtime, onoff, outfname):
    gw = gateway
    if proto.find("XCP") != -1:
        sink = 'TCPSink/XCPSink'
        gw = 'XCP'              # overwrite whatever was given
    elif proto.find("Linux") != -1:
        sink = 'TCPSink/Sack1/DelAck'
    else:
        sink = 'TCPSink'
        
    runstr = './newremy.tcl -tcp %s -sink %s -gw %s -simtime %d -ontime %s -offtime %s' % (proto, sink, gw, simtime, onoff, onoff)
    print runstr
    fnull = open(os.devnull, "w") 
    fout = open(outfname, "w")
    output = subprocess.call(runstr.split(), stdout=fout, stderr=fnull, shell=True)
#    print output
    return

simtime = 300
simtime = 10
maxconns = 32
maxconns = 1
iterations = 10
iterations = 1
#protolist = ['TCP/Newreno', 'TCP/Linux/cubic', 'TCP/Linux/compound', 'TCP/Vegas', 'TCP/Reno/XCP', 'TCP/Newreno/Rational']
protolist = ['TCP/Newreno']
#gwlist = {}
#gwlist['RED'] = 'RED'
#gwlist['XCP'] = 'XCP'
#gwlist['CoDel'] = 'CoDel'
#gwlist['sfqCoDel'] = 'sfqCoDel'
onofftimes = [1]

for proto in protolist:
    for onoff in onofftimes:
        numconns = 1
        while numconns <= maxconns:
            for i in xrange(iterations):
                outfname = '%s-nconn%d-onoff%d-simtime%d-iter%d' % (proto.replace('/','.'), numconns, onoff, simtime, i)
                print outfname
                runonce(proto, 'DropTail', numconns, simtime, onoff, outfname)
            numconns = 2*numconns
