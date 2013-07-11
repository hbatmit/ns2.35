# TCP Stream servers
for { set i 0 } { $i < $opt(nsrc) } { incr i } {
  # Create TCP Agents with congestion control specified in opt(tcp)

  # If opt(tcp) starts with TCP/Linux, then you need to do something different
  if { [string range $opt(tcp) 0 9] == "TCP/Linux/"} {
    set linuxcc [ string range $opt(tcp) 10 [string length $opt(tcp)] ]
    set cc_family "TCP/Linux"
    set stream_server($i) [ new Agent/$cc_family ]
    $ns at 0.0 "$stream_server($i) select_ca $linuxcc"
    $ns at 0.0 "$stream_server($i) set_ca_default_param linux debug_level 2"
  } else {
    set stream_server($i) [ new Agent/$opt(tcp) ]
  }

  if { $opt(tracing) == "true" } {
    $stream_server($i) trace cwnd_
    $stream_server($i) trace rtt_
    $stream_server($i) trace maxseq_
    $stream_server($i) trace ack_
    $stream_server($i) attach $trace_file
  }

  # Attach TCP Agent to basestation Node
  $ns attach-agent $basestation $stream_server($i)

  # set flow id
  $stream_server($i) set fid_ 2

  # Attach source (such as FTP) to TCP Agent
  set video_server($i) [ $stream_server($i) attach-app $opt(btapp) ]
  $video_server($i) setup_and_start $i $stream_server($i)
}

# TCP bulk transfer clients
for { set i 0 } { $i < $opt(nsrc) } { incr i } {
  # Create tcp sinks
  set stream_client($i) [ new Agent/$opt(sink) ]
  $ns attach-agent $client_node($i) $stream_client($i)

  # Create LoggingApp clients for logging received bytes
  set stream_logging_client($i) [new LoggingApp $i $stream_server($i)]
  $stream_logging_client($i) attach-agent $stream_client($i)
  $ns at 0.0 "$stream_logging_client($i) start"

  # Connect them to their sources
  $ns connect $stream_server($i) $stream_client($i)
}
