import os
import sys
from optparse import OptionParser
import subprocess
import matplotlib
if os.uname()[0] == 'Darwin':
    matplotlib.use('macosx')
import matplotlib.pyplot as p
import numpy

def runonce(fullname, proto, workload_dist, gw, traffic_type, simtime, on, off, topo_file, sd_file, outfname):
    if proto.find("XCP") != -1:
        sink = 'TCPSink/XCPSink'
        gw = 'XCP'              # overwrite whatever was given
    elif proto.find("Linux") != -1:
        sink = 'TCPSink/Sack1'
    else:
        sink = 'TCPSink'

    if fullname.find("CoDel") != -1:
        gw = "sfqCoDel"

    if traffic_type == "bytes":
        runstr = 'pls ./decompose.tcl %s %s -tcp %s -sink %s -gw %s -ontype %s -onrand %s -avgbytes %d -offrand %s -offavg %s -simtime %d &'\
                 % (topo_file, sd_file, proto, sink, gw, traffic_type, workload_dist, on, workload_dist, off, simtime)
    elif traffic_type == "time":
        runstr = 'pls ./decompose.tcl %s %s -tcp %s -sink %s -gw %s -ontype %s -onrand %s -onavg %d -offrand %s -offavg %s -simtime %d &'\
                 % (topo_file, sd_file, proto, sink, gw, traffic_type, workload_dist, on, workload_dist, off, simtime)
    else:
        runstr = 'pls ./decompose.tcl %s %s -tcp %s -sink %s -gw %s -ontype %s -offrand %s -offavg %s -simtime %d &'\
                 % (topo_file, sd_file, proto, sink, gw, traffic_type, workload_dist, off, simtime)

    print runstr
    fnull = open(os.devnull, "w") 
    fout = open(outfname, "ab")
    output = subprocess.call(runstr, stdout=fout, stderr=fnull, shell=True)    
    return

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-g", "--graph", type="string", dest="topo",
                      default = "", help = "Topology of underlying graph") 
    parser.add_option("-s", "--sdpairs", type="string", dest="sdpairs",
                      default = "", help = "Source Destination specification")
    parser.add_option("-d", "--dirresults", type="string", dest="resdir", 
                      default = "./tmpres", help = "directory for results")
    parser.add_option("-p", "--proto", type="string", dest="proto",
                      default = "TCP/Newreno", help = "protocol")
    parser.add_option("-t", "--type", type="string", dest="ontype",
                      default = "bytes", help = "by bytes or by seconds")
    (config, args) = parser.parse_args()

    if not os.path.exists(config.resdir):
        os.mkdir(config.resdir)

    topo_file = config.topo
    sd_file   = config.sdpairs

    simtime = 100
    iterations = 16

    protolist = config.proto.split() # which transport protocol(s) are we using?
    onofftimes = [0.5]
    avgbytes = 100000 # from Allman's March 2012 data and 2013 CCR paper
    worktypes = ['Exponential']

    for proto in protolist:
        fullname = proto
        if proto == "Cubic/sfqCoDel":
            proto = "TCP/Linux/cubic"
        for wrk in worktypes:
            for onoff in onofftimes:
                for i in xrange(iterations):
                    if config.ontype == "bytes":
                        outfname = '%s/%s.%s.%son%d.off%d.topo%s.sd%s.simtime%d.iteration%d'\
                                   % (config.resdir, fullname.replace('/','-'), wrk, config.ontype,
                                      avgbytes, onoff, topo_file.replace('/','-'), sd_file.replace('/','-'), simtime, i)
                        runonce(fullname, proto, wrk, 'DropTail', config.ontype, simtime, avgbytes, onoff, topo_file, sd_file, outfname)
                    else:
                        outfname = '%s/%s.%s.%son%d.off%d.topo%s.sd%s.simtime%d.iteration%d'\
                                   % (config.resdir, fullname.replace('/','-'), wrk, config.ontype,
                                      onoff, onoff, topo_file.replace('/','-'), sd_file.replace('/','-'), simtime, i)
                        runonce(fullname, proto, wrk, 'DropTail', config.ontype, simtime, onoff, onoff, topo_file, sd_file, outfname)
                    print outfname
