# source other files

source logging_app.tcl
source stats.tcl
source on_off_sender.tcl

# TCP servers
for { set i 0 } { $i < $opt(nsrc) } { incr i } {
  # Create TCP Agents with congestion control specified in opt(tcp)

  # If opt(tcp) starts with TCP/Linux, then you need to do something different
  if { [string range $opt(tcp) 0 9] == "TCP/Linux/"} {
    set linuxcc [ string range $opt(tcp) 10 [string length $opt(tcp)] ]
    set opt(tcp) "TCP/Linux"
    set tcp_server($i) [ new Agent/$opt(tcp) ]
    $ns at 0.0 "$tcp_server($i) select_ca $linuxcc"
    $ns at 0.0 "$tcp_server($i) set_ca_default_param linux debug_level 2"
  } else {
    set tcp_server($i) [ new Agent/$opt(tcp) ]
  }

  if { $opt(tracing) == "true" } {
      $tcp_server($i) trace cwnd_
      $tcp_server($i) trace rtt_
      $tcp_server($i) trace maxseq_
      $tcp_server($i) trace ack_
      $tcp_server($i) attach $trace_file
  }

  # Attach TCP Agent to basestation Node
  $ns attach-agent $basestation $tcp_server($i)

  # set flow id
  $tcp_server($i) set fid_ $counter
  set fid($i) $counter
  set counter [ incr counter ]

  # Attach source (such as FTP) to TCP Agent
  set on_off_server($i) [ $tcp_server($i) attach-app $opt(app) ]
  $on_off_server($i) setup_and_start $fid($i) $tcp_server($i)
}

# TCP clients
for { set i 0 } { $i < $opt(nsrc) } { incr i } {
  # Create node corresponding to mobile user
  set tcp_client_node($i) [ $ns node ]

  # Create forward and reverse links from basestation to mobile user
  create_link $ns $opt(delay) $basestation $tcp_client_node($i) $opt(gw) $fid($i) $rate_generator

  # Get handles to link and queue from basestation to user
  set cell_link [ [ $ns link $basestation $tcp_client_node($i) ] link ]
  set cell_queue [ [ $ns link $basestation $tcp_client_node($i) ] queue ]

  # All the non-standard queue neutering:
  neuter_queue $cell_queue

  # Set user_id and other stuff for SFD
  if { $opt(gw) == "SFD" } {
    setup_sfd $cell_queue $ensemble_scheduler
  }

  # Attach queue and link to ensemble_scheduler
  attach_to_scheduler $ensemble_scheduler $fid($i) $cell_queue $cell_link

  # Attach link to rate generator
  $rate_generator attach-link $cell_link $fid($i)

  # Create tcp sinks
  set tcp_client($i) [ new Agent/TCPSink/Sack1 ]
  $ns attach-agent $tcp_client_node($i) $tcp_client($i)

  # Create LoggingApp clients for logging received bytes
  set logging_app_client($i) [new LoggingApp $fid($i)]
  $logging_app_client($i) attach-agent $tcp_client($i)
  $ns at 0.0 "$logging_app_client($i) start"

  # Connect them to their sources
  $ns connect $tcp_server($i) $tcp_client($i)
  set tp($i) $tcp_server($i)
}
