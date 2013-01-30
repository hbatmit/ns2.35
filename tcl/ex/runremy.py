import os
import sys
import subprocess
import matplotlib
if os.uname()[0] == 'Darwin':
    matplotlib.use('macosx')
import matplotlib.pyplot as p
import numpy

def runonce(proto, w, gateway, numconns, simtime, onoff, outfname):
    gw = gateway
    if proto.find("XCP") != -1:
        sink = 'TCPSink/XCPSink'
        gw = 'XCP'              # overwrite whatever was given
    elif proto.find("Linux") != -1:
        sink = 'TCPSink/Sack1/DelAck'
    else:
        sink = 'TCPSink'
        
    runstr = './newremy.tcl -tcp %s -sink %s -gw %s -onrand %s -offrand %s -ontime %s -offtime %s -nsrc %d -simtime %d' % (proto, sink, gw, w, w, onoff, onoff,  numconns, simtime)
    print runstr
    fnull = open(os.devnull, "w") 
    fout = open(outfname, "ab")
    output = subprocess.call(runstr, stdout=fout, stderr=fnull, shell=True)    
    return

if not os.path.exists('./remy-results'):
    os.mkdir('./remy-results')
simtime = 300
maxconns = 32
iterations = 5
#protolist = ['TCP/Newreno', 'TCP/Linux/cubic', 'TCP/Linux/compound', 'TCP/Vegas', 'TCP/Reno/XCP', 'TCP/Newreno/Rational']
#gwlist = {}
#gwlist['RED'] = 'RED'
#gwlist['XCP'] = 'XCP'
#gwlist['CoDel'] = 'CoDel'
#gwlist['sfqCoDel'] = 'sfqCoDel'

protolist = [ sys.argv[1] ]                 # which transport protocol(s) are we using?
onofftimes = [1, 5, 10]
worktypes = ['Exponential', 'Pareto']

for proto in protolist:
    for w in worktypes:
        for onoff in onofftimes:
            numconns = 1
            while numconns <= maxconns:
                for i in xrange(iterations):
                    outfname = 'remy-results/%s.%s.nconn%d.onoff%d.simtime%d' % (proto.replace('/','-'), w, numconns, onoff, simtime)
                    print outfname
                    runonce(proto, w, 'DropTail', numconns, simtime, onoff, outfname)
                numconns = 2*numconns
