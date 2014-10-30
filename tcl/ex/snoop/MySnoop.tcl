puts "sourcing ../../lan/vlan.tcl..."
source ../..//lan/vlan.tcl
source ../../lan/ns-mac.tcl


set opt(tr)   out.tr
set opt(namtr)        "MySnoop.nam"
set opt(seed) 0
set opt(stop) 10
set opt(node) 2

set opt(qsize)        100
set opt(bw)   10Mb
set opt(delay)        1ms
set opt(ll)   LL
set opt(ifq)  Queue/DropTail
set opt(mac)  Mac/802_3
set opt(chan) Channel
set opt(tcp)  TCP/Reno
set opt(sink) TCPSink

set opt(app)  FTP

set loss_prob 10


proc finish {} {
      global ns opt

      $ns flush-trace

      exec nam $opt(namtr) &

      exit 0
}


proc create-trace {} {
      global ns opt

      if [file exists $opt(tr)] {
              catch "exec rm -f $opt(tr) $opt(tr)-bw [glob $opt(tr).*]"
      }

      set trfd [open $opt(tr) w]
      $ns trace-all $trfd
      if {$opt(namtr) != ""} {
              $ns namtrace-all [open $opt(namtr) w]
      }
      return $trfd
}

proc add-error {LossyLink} {

    global loss_prob
    
    # creating the uniform distribution random variable
    set loss_random_variable [new RandomVariable/Uniform] 
    $loss_random_variable set min_ 0    # set the range of the random variable;
    $loss_random_variable set max_ 100
    
    # create the error model;
    set loss_module [new ErrorModel]  
    $loss_module drop-target [new Agent/Null] 
    $loss_module set rate_ $loss_prob  # set error rate to (0.1 = 10 / (100 - 0));
    # error unit: packets (the default);
    $loss_module unit pkt      
    
    # attach random var. to loss module;
    $loss_module ranvar $loss_random_variable 

    # keep a handle to the loss module;
    #set sessionhelper [$ns create-session $n0 $tcp0] 
    $LossyLink errormodule $loss_module



}

proc create-topology {} {
      global ns opt 
      global lan node s d

      set num $opt(node)
      for {set i 1} {$i < $num} {incr i} {
              set node($i) [$ns node]
              lappend nodelist $node($i)
      }

      set lan [$ns make-lan $nodelist $opt(bw) \
                      $opt(delay) $opt(ll) $opt(ifq) $opt(mac) $opt(chan)]
      

      set opt(ll) LL/LLSnoop

      set opt(ifq) Queue/DropTail
      $opt(ifq) set limit_ 100

      # set up snoop agent
      set node(0) [$ns node]
      $lan addNode [list $node(0)]  $opt(bw) $opt(delay) $opt(ll) $opt(ifq) $opt(mac)
      
      # set source and connect to node(0)
      set s [$ns node]
      $ns duplex-link $s $node(0) 5Mb 20ms DropTail
      $ns queue-limit $s $node(0) 100000
      $ns duplex-link-op $s $node(0) orient right

      # set dest and connect to node(1)
      set d [$ns node]
      $ns duplex-link $node(1) $d 5Mb 10ms DropTail
      $ns queue-limit $node(1) $d 100000
      $ns duplex-link-op $d $node(1) orient left



      set LossyLink [$ns link $node(1) $d]  

      add-error $LossyLink
}

## MAIN ##

set ns [new Simulator]

set trfd [create-trace]

create-topology

#set tcp0 [$ns create-connection TCP/Reno $s TCPSink $node(1) 0]
#$tcp0 set window_ 30


#Create a infinite source agent (FTP) tcp and attach it to node n0
set tcp0 [new Agent/TCP/Reno]
$tcp0 set backoff_ 2 
$tcp0 set window_ 30
$ns attach-agent $s $tcp0

set tcp_snk0 [new Agent/TCPSink]
$ns attach-agent $d $tcp_snk0

$ns connect $tcp0 $tcp_snk0


set ftp0 [$tcp0 attach-app FTP]

$ns at 0.0 "$ftp0 start"
$ns at $opt(stop) "finish"
$ns run


