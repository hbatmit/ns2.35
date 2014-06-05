# Analyse a particular results file
def analyze(run, configuration, num_senders, requested_id, def_tpt, def_del):
  tpt_delay = open(configuration + "run" + str( run ) +  ".err", "r");
  tpt = 0.0
  delay = 0.0
  count = 0
  for line in tpt_delay.readlines():
    if line.find("inorder") != -1:
      records = line.split()
      sender_id = records[0].split(":")[0]
      if ((requested_id != "*")) : # haven't asked for all ids
        if (sender_id != requested_id): # ids don't match, continue
          continue
      current_tpt = float( records[1].split('=')[1] )
      current_delay = float( records[3].split('=')[1] )
      if (current_tpt == 0) :
        current_tpt = def_tpt
      if (current_delay == 0):
        current_delay = def_del
      tpt += current_tpt
      delay += current_delay
      count += 1;
  assert(count == num_senders)
  return (tpt, delay, count)

def analyze_helper(acc, y, configuration, num_senders, sender_id, def_tpt, def_del):
  z = analyze(y, configuration, num_senders, sender_id, def_tpt, def_del);
  return (acc[0] + z[0], acc[1] + z[1], acc[2] + z[2]);

# Aggregate results across runs on the same configuration
def reduce_runs(run_range, configuration, num_senders, sender_id, def_tpt, def_del):
  (sum_tpt, sum_delay, total_count) = reduce(lambda acc, y: analyze_helper(acc, y, configuration, num_senders, sender_id, def_tpt, def_del), run_range, (0.0, 0.0, 0));
  assert(total_count == len(run_range) * num_senders);
  return (sum_tpt/total_count, sum_delay/total_count);

# Plot aggregates into a plot file
def plot_tpt_delay(run_range, filehandle, abscissa, num_senders, configuration, sender_id, def_tpt, def_del):
  (tpt, delay) = reduce_runs(run_range, configuration, num_senders, sender_id, def_tpt, def_del);
  output_str = ""
  for x in abscissa:
    output_str += str(x) + " "
  filehandle.write(str(output_str) + " " + str(tpt) + " " + str(delay) + "\n");
