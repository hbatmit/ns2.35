#!/bin/csh

# get_goodput

set n_run=1
set ns="/users/haldar/ns-2/ns"
set sources=1
set duration=300
set file="urban-GBN-0"

# run ns for N times
set i=0;
while ($i < $n_run) 

	$ns $file $i $sources $duration
	@i = $i + 1
end

# get goodput values for the runs.
exec egrep "STATE 1" case0.tr > states.tr
set i=0
while ($i < $n_run)

         perl ontime.pl $duration $file
         @i = $i +1
end