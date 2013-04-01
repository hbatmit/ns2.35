# TCP servers
for { set i 0 } { $i < $opt(num_tcp) } { incr i } {
  # Create TCP Agents
  set tcp_server($i) [ new Agent/TCP/Linux ]
  $tcp_server($i) select_ca $opt(congestion_control)
  $ns attach-agent $basestation $tcp_server($i)

  # set flow id
  $tcp_server($i) set fid_ $counter
  set fid($i) $counter
  set counter [ incr counter ]

  # Generate FTP traffic on TCP Agent
  set ftp_server($i) [ new Application/FTP ]
  $ftp_server($i) attach-agent $tcp_server($i)
  $ns at 0.0 "$ftp_server($i) start"
}

# TCP clients
for { set i 0 } { $i < $opt(num_tcp) } { incr i } {
  # Create node corresponding to mobile user
  set tcp_client_node($i) [ $ns node ]
  create_link $ns $opt(bottleneck_bw) $opt(bottleneck_latency) $basestation $tcp_client_node($i) $opt(bottleneck_qdisc)

  # Get handles to link and queue
  set cell_link [ [ $ns link $basestation $tcp_client_node($i) ] link ]
  set cell_queue [ [ $ns link $basestation $tcp_client_node($i) ] queue ]

  # All the non-standard queue neutering:
  neuter_queue $cell_queue

  # Set user_id and other stuff for SFD
  if { $opt(bottleneck_qdisc) == "SFD" } {
    setup_sfd $cell_queue $fid($i) $cell_link $ensemble_scheduler
  }

  # Attach trace_file to queue.
  $ns trace-queue $basestation $tcp_client_node($i) $trace_file

  # Attach queue and link to ensemble_scheduler
  attach_to_scheduler $ensemble_scheduler $fid($i) $cell_queue $cell_link

  # Attach link to rate generator
  $rate_generator attach-link $cell_link $fid($i)

  # Create tcp sinks
  set tcp_client($i) [ new Agent/TCPSink/Sack1 ]
  $ns attach-agent $tcp_client_node($i) $tcp_client($i)

  # Connect them to their sources
  $ns connect $tcp_server($i) $tcp_client($i)
}
