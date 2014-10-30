#
# Support for example codes: data collection in TCP-Linux simulations
#
# TCP-Linux module for NS2 
#
# Nov 2007
#
# Author: Xiaoliang (David) Wei  (DavidWei@acm.org)
#
# NetLab, the California Institute of Technology 
# http://netlab.caltech.edu
#
# See a mini-tutorial about TCP-Linux at: http://netlab.caltech.edu/projects/ns2tcplinux/
#
#
# Module: tcl/ex/tcp-linux/sampling.tcl
# This code is not supposed to be run individually. Other codes will call the functions in this code.
# This code provides two functions which samples the TCP connection status and queue status.
# 
# tcp output format in a line: 
#   time 
#   cwnd 
#   maxseq 
#   data_packet_number 
#   ack_packet_number 
#   retransmited_packet_number 
#   number_of_cwnd_cut_event 
#
# queue output format in a line:
#   time
#   queue length in packet number

proc monitor_tcp {ns tcp filename} {
	set nowtime [$ns now]
	set outputfile [open $filename a]
	puts $outputfile "$nowtime [$tcp set cwnd_] [$tcp set maxseq_] [$tcp set ndatapack_] [$tcp set nackpack_] [$tcp set nrexmitpack_] [$tcp set ncwndcuts_]"
	close $outputfile
}

proc monitor_queue {ns qmon filename} {
    set qmonfile [open $filename a]
    puts $qmonfile "[$ns now] [$qmon set pkts_]"
    close $qmonfile
}

