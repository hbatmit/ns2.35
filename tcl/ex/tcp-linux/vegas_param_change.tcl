#
# Example code for parameter changing in Vegas congestion control algorithm
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
# Module: tcl/ex/tcp-linux/vegas_param_change.tcl
#    This is an example on how to change parameters in congestion control algorithms
# with TCP-Linux. This code uses TCP-Vegas as an example and demonstrates how to change
# default parameter and individual parameter for a connection.
#
# The code simulates the scenario in which three vegas flows running together through the same bottleneck queue.
# The default parameters alpha and beta are set to 40 (40/2=20 packets) at time 60 second.
# (hence the queue length is expected to be 40/2*3 = 60 packets afterwards.)
# The alpha beta  parameters of individual flow 3 is changed to 20 (20/2=10 packets) at time 120 second.
# (hence the queue length is expected to be 40/2*2+20/2*1 = 50 packets afterwards, and the throughput
# of flow 3 will be smaller than flow 1 and 2 afterwards)
#
#    The code outputs:
# queue.trace: the queue length over time in the format of <time> <queue len>
# p1.trace, p2.trace, p3.trace: the connection variables over time in format specified in sampling.tcl
#
#
# After the simulation, run
#    gnuplot gnuplot_vegas_param.script 
# to generate figures on cwnd trajectory and queue trajectory.
#

#Create a simulator object
set ns [new Simulator]

#Create a bottleneck link.
set router_snd [$ns node]
set router_rcv [$ns node]
$ns duplex-link $router_snd $router_rcv 10Mb 10ms DropTail
$ns queue-limit $router_snd $router_rcv 10000
# Create two flows sharing the bottleneck link
for {set i 1} {$i<=3} {incr i 1} {
  #Create the sending nodes,the receiving nodes.
  set bs($i) [$ns node]
  $ns duplex-link $bs($i) $router_snd 100Mb 1ms DropTail
  set br($i) [$ns node]
  $ns duplex-link $router_rcv $br($i) 100Mb 1ms DropTail
  #setup sender side
  set tcp($i) [new Agent/TCP/Linux]
  $tcp($i) set timestamps_ true
  $tcp($i) set window_ 100000
  $ns at 0 "$tcp($i) select_ca vegas"
  $ns attach-agent $bs($i) $tcp($i)

  #set up receiver side
  set sink($i) [new Agent/TCPSink/Sack1]
  $sink($i) set ts_echo_rfc1323_ true
  $ns attach-agent $br($i) $sink($i)

  #logical connection
  $ns connect $tcp($i) $sink($i)

  #Setup a FTP over TCP connection
  set ftp($i) [new Application/FTP]
  $ftp($i) attach-agent $tcp($i)
  $ftp($i) set type_ FTP

  #Schedule the life of the FTP
  $ns at 0 "$ftp($i) start"
  $ns at 200 "$ftp($i) stop"
}

#change default parameters, all TCP/Linux will see the changes!
$ns at 60 "$tcp(1) set_ca_default_param vegas alpha 40"
$ns at 60 "$tcp(1) set_ca_default_param vegas beta 40"
# confirm the changes by printing the parameter values (optional)
$ns at 60 "$tcp(2) get_ca_default_param vegas alpha"
$ns at 60 "$tcp(2) get_ca_default_param vegas beta"


# change local parameters, only tcp(3) is affected. (optional)
$ns at 120 "$tcp(3) set_ca_param vegas alpha 20"
$ns at 120 "$tcp(3) set_ca_param vegas beta 20"
# confirm the changes by printing the parameter values (optional)
$ns at 120 "$tcp(3) get_ca_param vegas alpha"
$ns at 120 "$tcp(3) get_ca_param vegas beta"

#Schedule the stop of the simulation
$ns at 201 "exit 0"


set MonitorInterval 0.1
set qmonfile [open "queue.trace" "w"]
close $qmonfile
set qmon [$ns monitor-queue $router_snd $router_rcv "" $MonitorInterval]
source "sampling.tcl"
proc monitor {interval} {
    global ns tcp qmon
    set nowtime [$ns now]
    set id 0
    for {set i 1} {$i<=3} {incr i 1} {
       set id [expr $id+1]
       monitor_tcp $ns $tcp($id) p$i.trace
    }
    monitor_queue $ns $qmon queue.trace
    $ns after $interval "monitor $interval"
}
$ns at 0 "monitor $MonitorInterval"

#Start the simulation
$ns run
