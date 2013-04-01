# CBR/UDP server
for { set i 0 } { $i < $opt(num_udp) } { incr i } {
  # Create UDP Agents
  set udp_server($i) [ new Agent/UDP ]
  $ns attach-agent $basestation $udp_server($i)

  # set flow id
  $udp_server($i) set fid_ $counter
  set fid($i) $counter
  set counter [ incr counter ]

  # Generate CBR traffic on UDP Agent
  set cbr_server($i) [ new Application/Traffic/CBR ]
  $cbr_server($i) attach-agent $udp_server($i)

  # Set packetSize_ and related parameters.
  $cbr_server($i) set packetSize_ 1000
  puts "UDP rate $opt(cbr_rate)"
  $cbr_server($i) set rate_   [ bw_parse $opt(cbr_rate) ]
  $cbr_server($i) set random_     0
  $ns at 0.0 "$cbr_server($i) start"
}

# CBR/UDP clients
for { set i 0 } { $i < $opt(num_udp) } { incr i } {
  # Create node corresponding to mobile user
  set udp_client_node($i) [ $ns node ]
  create_link $ns $opt(bottleneck_bw) $opt(bottleneck_latency) $basestation $udp_client_node($i) $opt(bottleneck_qdisc)

  # Get references to cell_link and cell_queue
  set cell_link [ [ $ns link $basestation $udp_client_node($i) ] link ]
  set cell_queue [ [ $ns link $basestation $udp_client_node($i) ] queue ]

  # All the non-standard queue neutering:
  neuter_queue $cell_queue

  # Set user id and other stuff for SFD
  if { $opt(bottleneck_qdisc) == "SFD" } {
    setup_sfd $cell_queue $fid($i) $cell_link $ensemble_scheduler
  }

  # Attach trace_file to queue.
  $ns trace-queue $basestation $udp_client_node($i) $trace_file

  # Attach queue and link to ensemble scheduler
  attach_to_scheduler $ensemble_scheduler $fid($i) $cell_queue $cell_link
  
  # Attach link to rate_generator
  $rate_generator attach-link $cell_link $fid($i)

  # Create Application Sinks
  set udp_client($i) [ new Agent/Null ]
  $ns attach-agent $udp_client_node($i) $udp_client($i)

  # Connect them to their sources
  $ns connect $udp_server($i) $udp_client($i)
}
