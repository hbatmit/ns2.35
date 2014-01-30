#! /usr/bin/python
import os
from analysis import *

# Set constants
iteration_count = 10
resultfolder = "resultssendersweep/"

# Cross-agility on num senders
for num_senders in range(1, 21, 1):
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/num_senders-100x2senders.plot", "a"),         (num_senders, ),  num_senders, resultfolder + "100x-2senders-num_senders"+str(num_senders), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/num_senders-muxing10.plot", "a"),          (num_senders, ),  num_senders, resultfolder + "multiplexing10-num_senders"+str(num_senders), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/num_senders-100x10senders.plot", "a"),           (num_senders, ),  num_senders, resultfolder + "100x-10senders-tom-final-num_senders"+str(num_senders), "*");
