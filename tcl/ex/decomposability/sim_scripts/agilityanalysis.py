#! /usr/bin/python
import os
import numpy
import math
from analysis import *

# Set constants
iteration_count = 10
resultfolder = "resultslogarithmic/"

# Cross-agility on delay
for delay in range(5, 151, 5):
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/rtt10x.plot", "a"),          (delay, ), 2, resultfolder + "rtt10x"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/rtt20x.plot", "a"),          (delay, ), 2, resultfolder + "rtt20x"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/rtt30x.plot", "a"),          (delay, ), 2, resultfolder + "rtt30x"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/rttandlinkspeed.plot", "a"),          (delay, ), 2, resultfolder + "rttandlinkspeed"+str(delay), "*");
