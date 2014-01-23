#! /usr/bin/python
import os
import numpy
import math
from analysis import *

# Set constants
iteration_count = 10
resultfolder = "resultsrandom/"

# Make blacklist. I really don't want to fix random errors right now.

for i in range(0, 1000):
  print i
  fh = open("toporandom/toporand" + str(i) + ".txt", "r");
  min_rtt = 2 * float(fh.readlines()[0].split()[3]);
  fh.close();

  fh = open("toporandom/sdrand" + str(i) + ".txt", "r");
  senders = len(fh.readlines());
  fh.close();
  print min_rtt
  print senders
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/random1000x.plot", "a"),    (i,  ), senders, resultfolder + "1000x-rand" + str(i), "*", min_rtt);
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/randomcubicsfqCoDel.plot", "a"), (i,  ), senders, resultfolder + "cubicsfqCoDel-rand" + str(i), "*", min_rtt);
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/randomcubic.plot", "a"),    (i,  ), senders, resultfolder + "cubic-rand" + str(i), "*", min_rtt);
