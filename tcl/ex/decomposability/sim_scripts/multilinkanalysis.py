#! /usr/bin/python
import os
import numpy
import math
from analysis import *

# constants
iteration_count = 5
resultfolder = "resultsthreelinks/"

# define a logrange function
def logrange(below, above, num_points):
  start = math.log10(below)
  stop  = math.log10(above)
  return list(numpy.around(numpy.logspace(start, stop, num_points), 5));

# IDs
ids = ["0", "1", "2", "3", "4", "5"]

# Fuse results from across multiple senders
def fuse_senders(filename):
  global ids

  # Open the plot file for each sender id
  fh = []
  for sender_id in ids:
    fh.append(open("plots/" + sender_id + filename, "r"))

  # Store the plot file in a vector of vector of strings
  results = []
  for filehandle in fh:
    results.append(filehandle.readlines())

  # Ensure the number of points is the same in all files
  num_points = len(results[0]);
  assert(num_points == len(results[0]));
  assert(num_points == len(results[1]));
  assert(num_points == len(results[2]));
  assert(num_points == len(results[3]));
  assert(num_points == len(results[4]));
  assert(num_points == len(results[5]));

  # aggregate file handle
  aggregate_file = open("plots/"+ filename, "w");

  # for each data point
  for i in range(0, num_points):
    # for each sender of the three senders
    output_str = ""
    for result in results:
      records =  result[i].split()
      output_str += records[3] + " " + records[4] + " " # append throughput followed by delay.
    aggregate_file.write(results[0][i].split(" ")[0] + " " + results[0][i].split(" ")[1] + " " + results[0][i].split(" ")[2] + " " + output_str+"\n");
 
# Make linkspeed topologies
linkspeed_range = logrange(1.0, 1000, 16);

for link1 in linkspeed_range:
  for link2 in linkspeed_range:
   for link3 in linkspeed_range:
    for sender_id  in ids:
      plot_tpt_delay(range(1, iteration_count + 1), open("plots/" + sender_id + "3link1000x.plot", "a"),         (link1, link2, link3), 1, resultfolder + "1000x-link" + str(link1) + "-" + str(link2) + "-" + str(link3), sender_id);
      plot_tpt_delay(range(1, iteration_count + 1), open("plots/" + sender_id + "3linkcubicsfqCoDel.plot", "a"), (link1, link2, link3), 1, resultfolder + "cubicsfqCoDel-link" + str(link1) + "-" + str(link2) + "-" + str(link3), sender_id);
      plot_tpt_delay(range(1, iteration_count + 1), open("plots/" + sender_id + "3linkcubic.plot", "a"),         (link1, link2, link3), 1, resultfolder + "cubic-link" + str(link1) + "-" + str(link2) + "-" + str(link3), sender_id);

# Now, put together the results
fuse_senders("3link1000x.plot");
fuse_senders("3linkcubicsfqCoDel.plot");
fuse_senders("3linkcubic.plot");
