#! /usr/bin/python
import os
import numpy
import math
from analysis import *

# constants
iteration_count = 10
resultfolder = "resultsmultilink/"

# define a logrange function
def logrange(below, above, num_points):
  start = math.log10(below)
  stop  = math.log10(above)
  return list(numpy.around(numpy.logspace(start, stop, num_points), 5));

# IDs
ids = ["0", "1", "2"]

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

  # aggregate file handle
  aggregate_file = open("plots/"+ filename, "w");

  # for each data point
  for i in range(0, num_points):
    # for each sender of the three senders
    output_str = ""
    for result in results:
      records =  result[i].split()
      output_str += records[2] + " " + records[3] + " " 
    aggregate_file.write(results[0][i].split(" ")[0] + " " + results[0][i].split(" ")[1] + " " + output_str+"\n");
 
# Make linkspeed topologies
linkspeed_range = logrange(1.0, 1000, 32);

for link1 in linkspeed_range:
  for link2 in linkspeed_range:
    for sender_id  in ids:
      plot_tpt_delay(range(1, iteration_count + 1), open("plots/" + sender_id + "multilink1000x.plot", "a"),         (link1, link2), 1, resultfolder + "1000x-link" + str(link1) + "-" + str(link2), sender_id);
      plot_tpt_delay(range(1, iteration_count + 1), open("plots/" + sender_id + "multilink100x.plot", "a"),          (link1, link2), 1, resultfolder + "100x-link" + str(link1) + "-" + str(link2), sender_id);
      plot_tpt_delay(range(1, iteration_count + 1), open("plots/" + sender_id + "multilink10x.plot", "a"),           (link1, link2), 1, resultfolder + "10x-link" + str(link1) + "-" + str(link2), sender_id);
      plot_tpt_delay(range(1, iteration_count + 1), open("plots/" + sender_id + "multilinkcubicsfqCoDel.plot", "a"), (link1, link2), 1, resultfolder + "cubicsfqCoDel-link" + str(link1) + "-" + str(link2), sender_id);
      plot_tpt_delay(range(1, iteration_count + 1), open("plots/" + sender_id + "multilinkcubic.plot", "a"),         (link1, link2), 1, resultfolder + "cubic-link" + str(link1) + "-" + str(link2), sender_id);

# Now, put together the results
fuse_senders("multilink1000x.plot");
fuse_senders("multilink100x.plot");
fuse_senders("multilink10x.plot");
fuse_senders("multilinkcubicsfqCoDel.plot");
fuse_senders("multilinkcubic.plot");
