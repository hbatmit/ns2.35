source bulk_sender.tcl

# TCP bulk transfer(bt) servers
for { set i 0 } { $i < $opt(nsrc) } { incr i } {
  # Create TCP Agents with congestion control specified in opt(tcp)

  # If opt(tcp) starts with TCP/Linux, then you need to do something different
  if { [string range $opt(tcp) 0 9] == "TCP/Linux/"} {
    set linuxcc [ string range $opt(tcp) 10 [string length $opt(tcp)] ]
    set opt(tcp) "TCP/Linux"
    set bt_server($i) [ new Agent/$opt(tcp) ]
    $ns at 0.0 "$bt_server($i) select_ca $linuxcc"
    $ns at 0.0 "$bt_server($i) set_ca_default_param linux debug_level 2"
  } else {
    set bt_server($i) [ new Agent/$opt(tcp) ]
  }

  if { $opt(tracing) == "true" } {
    $bt_server($i) trace cwnd_
    $bt_server($i) trace rtt_
    $bt_server($i) trace maxseq_
    $bt_server($i) trace ack_
    $bt_server($i) attach $trace_file
  }

  # Attach TCP Agent to basestation Node
  $ns attach-agent $basestation $bt_server($i)

  # set flow id
  $bt_server($i) set fid_ $i

  # Attach source (such as FTP) to TCP Agent
  set download_server($i) [ $bt_server($i) attach-app $opt(btapp) ]
  $download_server($i) setup_and_start $i $bt_server($i)
}

# TCP bulk transfer clients
for { set i 0 } { $i < $opt(nsrc) } { incr i } {
  # Create tcp sinks
  set bt_client($i) [ new Agent/$opt(sink) ]
  $ns attach-agent $client_node($i) $bt_client($i)

  # Create LoggingApp clients for logging received bytes
  set bt_logging_client($i) [new LoggingApp $i $bt_server($i)]
  $bt_logging_client($i) attach-agent $bt_client($i)
  $ns at 0.0 "$bt_logging_client($i) start"

  # Connect them to their sources
  $ns connect $bt_server($i) $bt_client($i)
}
