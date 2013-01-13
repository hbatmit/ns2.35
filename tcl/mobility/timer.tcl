#
# A simple timer class.  You can derive a subclass of Timer
# to provide a simple mechanism for scheduling events:
#
# 	$self sched $delay -- causes "$self timeout" to be called
#				$delay seconds in the future
#	$self cancel	   -- cancels any pending scheduled callback
# 
Class Timer

# sched is the same as resched; the previous setting is cancelled
# and another event is scheduled. No state is kept for the timers.
# This is different than the C++ timer API in timer-handler.cc,h; where a 
# sched aborts if the timer is already set. C++ timers maintain state 
# (e.g. IDLE, PENDING..etc) that is checked before the timer is scheduled.
Timer instproc sched delay {
	global ns
	$self instvar id_
	$self cancel
	set id_ [$ns at [expr [$ns now] + $delay] "$self timeout"]
}

Timer instproc destroy {} {
	$self cancel
}

Timer instproc cancel {} {
	global ns
	$self instvar id_
	if [info exists id_] {
		$ns cancel $id_
		unset id_
	}
}

# resched and expire are added to have a similar API to C++ timers.
Timer instproc resched delay {
	$self sched $delay 
}

# the subclass must provide the timeout function
Timer instproc expire {} {
	$self timeout
}
