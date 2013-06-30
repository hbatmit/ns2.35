source on_off_sender.tcl

# TCP web servers
for { set i 0 } { $i < $opt(nsrc) } { incr i } {
  # Create TCP Agents with congestion control specified in opt(tcp)

  # If opt(tcp) starts with TCP/Linux, then you need to do something different
  if { [string range $opt(tcp) 0 9] == "TCP/Linux/"} {
    set linuxcc [ string range $opt(tcp) 10 [string length $opt(tcp)] ]
    set cc_family "TCP/Linux"
    set web_server($i) [ new Agent/$cc_family ]
    $ns at 0.0 "$web_server($i) select_ca $linuxcc"
    $ns at 0.0 "$web_server($i) set_ca_default_param linux debug_level 2"
  } else {
    set web_server($i) [ new Agent/$opt(tcp) ]
  }

  if { $opt(tracing) == "true" } {
    $web_server($i) trace cwnd_
    $web_server($i) trace rtt_
    $web_server($i) trace maxseq_
    $web_server($i) trace ack_
    $web_server($i) attach $trace_file
  }

  # Attach TCP Agent to basestation Node
  $ns attach-agent $basestation $web_server($i)

  # set flow id
  $web_server($i) set fid_ $i

  # Attach source (such as FTP) to TCP Agent
  set on_off_server($i) [ $web_server($i) attach-app $opt(app) ]
  $on_off_server($i) setup_and_start $i $web_server($i)
}

# TCP web clients
for { set i 0 } { $i < $opt(nsrc) } { incr i } {
  # Create tcp sinks
  set web_client($i) [ new Agent/$opt(sink) ]
  $ns attach-agent $client_node($i) $web_client($i)

  # Create LoggingApp clients for logging received bytes
  set web_logging_client($i) [new LoggingApp $i $web_server($i)];
  $web_logging_client($i) attach-agent $web_client($i)
  $ns at 0.0 "$web_logging_client($i) start"

  # Connect them to their sources
  $ns connect $web_server($i) $web_client($i)
}
