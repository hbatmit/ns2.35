# source other files
source logging_app.tcl
source stats.tcl
source bulk_sender.tcl

# Setup nodes
source setup_nodes.tcl

# Setup bulk transfers
if { $opt(enable_bulk) == "true" } {
  source setup_bulk_connections.tcl
}

# Setup web connections
if { $opt(enable_web) == "true" } {
  source setup_web_connections.tcl
}

# Setup streaming sources
if { $opt(enable_stream) == "true" } {
  source setup_stream_connections.tcl
}
