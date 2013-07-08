# Setup nodes
for { set i 0 } { $i < $opt(nsrc) } { incr i } {
  # Create node corresponding to mobile user
  set client_node($i) [ $ns node ]

  # Create forward and reverse links from basestation to mobile user
  create_link $ns $opt(delay) $basestation $client_node($i) $opt(gw) $i $dl_rate_generator $ensemble_scheduler

  # Get handles to link and queue from basestation to user
  set cell_link [ [ $ns link $basestation $client_node($i) ] link ]
  set cell_queue [ [ $ns link $basestation $client_node($i) ] queue ]

  # All the non-standard queue neutering:
  neuter_queue $cell_queue

  # Trace sfd events if required
  if { $opt(tracing) == "true" && $opt(gw) == "SFD" } {
    $cell_queue attach $trace_file;
    $cell_queue trace _last_drop_time;
    $cell_queue trace _current_arr_rate;
    $cell_queue trace _arr_rate_at_drop;
  }

  # Set user_id and other stuff for SFD
  if { $opt(gw) == "SFD" } {
    setup_sfd $cell_queue $ensemble_scheduler
  }

  # Attach queue and link to ensemble_scheduler
  attach_to_scheduler $ensemble_scheduler $i $cell_queue $cell_link

  # Attach link to rate generator
  $dl_rate_generator attach-link $cell_link $i
}
