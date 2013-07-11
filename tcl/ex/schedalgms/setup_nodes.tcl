proc link_setup {origin dst rate_gen ensemble_sched index setup_sfd} {
  global ns opt trace_file

  # Get handles to link and queue from basestation to user
  set cell_link  [ [ $ns link $origin $dst ] link ]
  set cell_queue [ [ $ns link $origin $dst ] queue ]

  # All the non-standard queue neutering:
  neuter_queue $cell_queue

  # Trace sfd events if required
  if { $opt(tracing) == "true" && $setup_sfd } {
    $cell_queue attach $trace_file;
    $cell_queue trace _last_drop_time;
    $cell_queue trace _current_arr_rate;
    $cell_queue trace _arr_rate_at_drop;
  }

  # Set user_id and other stuff for SFD
  if { $setup_sfd } {
    setup_sfd $cell_queue $ensemble_sched
  }

  # Attach queue and link to ensemble_scheduler
  attach_to_scheduler $ensemble_sched $index $cell_queue $cell_link

  # Attach link to rate generator
  $rate_gen attach-link $cell_link $index
}

# Setup nodes
for { set i 0 } { $i < $opt(nsrc) } { incr i } {
  # Create node corresponding to mobile user
  set client_node($i) [ $ns node ]

  # Create forward and reverse links from basestation to mobile user
  create_link $ns $opt(delay) $basestation $client_node($i) $opt(gw) $i $dl_rate_generator $ul_rate_generator $ensemble_dl_sched $ensemble_ul_sched

  ############ Setup things on the downlink ###########
  link_setup $basestation $client_node($i) $dl_rate_generator $ensemble_dl_sched $i [string equal opt(gw) "SFD"]

  ############# Setup things on the uplink ###############
  link_setup $client_node($i) $basestation $ul_rate_generator $ensemble_ul_sched $i 0
}
