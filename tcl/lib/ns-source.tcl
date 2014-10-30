#
# Copyright (c) 1996 Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-source.tcl,v 1.25 2000/11/01 21:47:59 xuanc Exp $
#

#  NOTE:  Could consider renaming this file to ns-app.tcl and moving the
#  backward compatible code to ns-compat.tcl

#set ns_telnet(interval) 1000ms
#set ns_bursty(interval) 0
#set ns_bursty(burst-size) 2
#set ns_message(packet-size) 40

Class Application/FTP -superclass Application

Application/FTP instproc init {} {
	$self next
}

Application/FTP instproc start {} {
	[$self agent] send -1
}

# Causes TCP to send no more new data.
Application/FTP instproc stop {} {
	[$self agent] advance 0
	[$self agent] close
}

Application/FTP instproc send {nbytes} {
	[$self agent] send $nbytes
}

# For sending packets.  Sends $pktcnt packets.
Application/FTP instproc produce { pktcnt } {
	[$self agent] advance $pktcnt
}

# For sending packets.  Sends $pktcnt more packets.
Application/FTP instproc producemore { pktcnt } {
	[$self agent] advanceby $pktcnt
}

# Backward compatibility
Application/Traffic instproc set args {
	$self instvar packetSize_ rate_
	if { [lindex $args 0] == "packet_size_" } {
		if { [llength $args] == 2 } {
			set packetSize_ [lindex $args 1]
			return
		} elseif { [llength $args] == 1 } {
			return $packetSize_
		}
	}
	eval $self next $args
}
# Helper function to convert rate_ into an interval_
#Hicked to generate pulse UDP traffic (seperate rate_ and interval_).
Application/Traffic/CBR instproc set args {
	$self instvar packetSize_ rate_
	if { [lindex $args 0] == "interval_" } {
		if { [llength $args] == 2 } {
			set ns_ [Simulator instance]
			set interval_ [$ns_ delay_parse [lindex $args 1]]
			$self set rate_ [expr $packetSize_ * 8.0/$interval_]
			return
		} elseif { [llength $args] == 1 } {
			return [expr $packetSize_ * 8.0/$rate_]
		}
	}
	eval $self next $args
}

#
# Below are backward compatible components
#

Class Agent/CBR -superclass Agent/UDP
Class Agent/CBR/UDP -superclass Agent/UDP
Class Agent/CBR/RTP -superclass Agent/RTP
Class Agent/CBR/UDP/SA -superclass Agent/SA

Agent/SA instproc attach-traffic tg {
	$tg attach-agent $self
	eval $self cmd attach-traffic $tg
}

Agent/CBR/UDP instproc attach-traffic tg {
	$self instvar trafgen_
	$tg attach-agent $self
	set trafgen_ $tg
}

Agent/CBR/UDP instproc done {} { }

Agent/CBR/UDP instproc start {} {
	$self instvar trafgen_
	$trafgen_ start
}

Agent/CBR/UDP instproc stop {} {
	$self instvar trafgen_
	$trafgen_ stop
}

Agent/CBR/UDP instproc advance args {
	$self instvar trafgen_
	eval $trafgen_ advance $args
}

Agent/CBR/UDP instproc advanceby args {
	$self instvar trafgen_
	eval $trafgen_ advanceby $args
}

Agent/CBR instproc init {} {
	$self next
	$self instvar trafgen_ interval_ random_ packetSize_ maxpkts_
	# The following used to be in ns-default.tcl
	set packetSize_ 210
	set random_ 0
	set maxpkts_ 268435456	
	set interval_ 0.00375
	set trafgen_ [new Application/Traffic/CBR]
	$trafgen_ attach-agent $self
	# Convert packetSize_ and interval_ to trafgen_ rate_
	$trafgen_ set rate_ [expr $packetSize_ * 8.0/ $interval_]
	$trafgen_ set random_ [$self set random_]
	$trafgen_ set maxpkts_ [$self set maxpkts_]
	$trafgen_ set packetSize_ [$self set packetSize_]
	# The line below is needed for backward compat with v1 test scripts 
        if {[Simulator set nsv1flag] == 0} { 
        
	    puts "using backward compatible Agent/CBR; use Application/Traffic/CBR instead"
	}    
}



Agent/CBR instproc done {} { }

Agent/CBR instproc start {} {
	$self instvar trafgen_
	$trafgen_ start
}

Agent/CBR instproc stop {} {
	$self instvar trafgen_
	$trafgen_ stop
}

Agent/CBR instproc advance args {
	$self instvar trafgen_
	eval $trafgen_ advance $args
}

Agent/CBR instproc advanceby args {
	$self instvar trafgen_
	eval $trafgen_ advanceby $args
}

# Catches parameter settings for overlying traffic generator object
Agent/CBR instproc set args {
	$self instvar interval_ random_ packetSize_ maxpkts_ trafgen_
	if { [info exists trafgen_] } {
		if { [lindex $args 0] == "packetSize_" } {
			if { [llength $args] == 2 } {
				$trafgen_ set packetSize_ [lindex $args 1]
				set packetSize_ [lindex $args 1]
				# Recompute rate 
				$trafgen_ set rate_ [expr $packetSize_ * 8.0/ $interval_]
                        	return 
                	} elseif { [llength $args] == 1 } {
				return $packetSize_
                	}
		} elseif { [lindex $args 0] == "random_" } {
			if { [llength $args] == 2 } {
				$trafgen_ set random_ [lindex $args 1]
				set random_ [lindex $args 1]
				return
                	} elseif { [llength $args] == 1 } {
				return $random_
			}
		} elseif { [lindex $args 0] == "maxpkts_" } {
			if { [llength $args] == 2 } {
				$trafgen_ set maxpkts_ [lindex $args 1]
				set maxpkts_ [lindex $args 1]
				return
                	} elseif { [llength $args] == 1 } {
				return $maxpkts_
			}
		} elseif { [lindex $args 0] == "interval_" } {
			if { [llength $args] == 2 } {
				set ns_ [Simulator instance]
				set interval_ [$ns_ delay_parse [lindex $args 1]]
				# Convert interval_ to rate for trafgen_
				$trafgen_ set rate_ [expr $packetSize_ * 8.0/ $interval_]
				return
                	} elseif { [llength $args] == 1 } {
				return $interval_
			}
		}
	}
	eval $self next $args
}
 
Class Traffic/Expoo -superclass Application/Traffic/Exponential
Class Traffic/Pareto -superclass Application/Traffic/Pareto
Class Traffic/RealAudio -superclass Application/Traffic/RealAudio
Class Traffic/Trace -superclass Application/Traffic/Trace

# These instprocs are needed to map old Traffic/* type variables
Traffic/Expoo instproc set args {
	$self instvar packetSize_ burst_time_ idle_time_ rate_ 
	if { [lindex $args 0] == "packet-size" } {
		if { [llength $args] == 2 } {
			$self set packetSize_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $packetSize_
               	}
	} elseif { [lindex $args 0] == "burst-time" } {
		if { [llength $args] == 2 } {
			$self set burst_time_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $burst_time_
               	}
	} elseif { [lindex $args 0] == "idle-time" } {
		if { [llength $args] == 2 } {
			$self set idle_time_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $idle_time_
               	}
	} elseif { [lindex $args 0] == "rate" } {
		if { [llength $args] == 2 } {
			$self set rate_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $rate_
               	}
	}
	eval $self next $args
}

Traffic/Pareto instproc set args {
	$self instvar packetSize_ burst_time_ idle_time_ rate_ shape_
	if { [lindex $args 0] == "packet-size" } {
		if { [llength $args] == 2 } {
			$self set packetSize_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $packetSize_
               	}
	} elseif { [lindex $args 0] == "burst-time" } {
		if { [llength $args] == 2 } {
			$self set burst_time_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $burst_time_
               	}
	} elseif { [lindex $args 0] == "idle-time" } {
		if { [llength $args] == 2 } {
			$self set idle_time_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $idle_time_
               	}
	} elseif { [lindex $args 0] == "rate" } {
		if { [llength $args] == 2 } {
			$self set rate_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $rate_
               	}
	} elseif { [lindex $args 0] == "shape" } {
		if { [llength $args] == 2 } {
			$self set shape_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $shape_
               	}
	}
	eval $self next $args
}

Traffic/RealAudio instproc set args {
	$self instvar packetSize_ burst_time_ idle_time_ rate_ 
	if { [lindex $args 0] == "packet-size" } {
		if { [llength $args] == 2 } {
			$self set packetSize_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $packetSize_
               	}
	} elseif { [lindex $args 0] == "burst-time" } {
		if { [llength $args] == 2 } {
			$self set burst_time_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $burst_time_
               	}
	} elseif { [lindex $args 0] == "idle-time" } {
		if { [llength $args] == 2 } {
			$self set idle_time_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $idle_time_
               	}
	} elseif { [lindex $args 0] == "rate" } {
		if { [llength $args] == 2 } {
			$self set rate_ [lindex $args 1]
                       	return 
               	} elseif { [llength $args] == 1 } {
			return $rate_
               	}
	}
	eval $self next $args
}

Class Source/FTP -superclass Application
Source/FTP set maxpkts_ 268435456

Source/FTP instproc attach o {
	$self instvar agent_
	set agent_ $o
	$self attach-agent $o
}

Source/FTP instproc init {} {
	$self next
	$self instvar maxpkts_ agent_
	set maxpkts_ 268435456
}

Source/FTP instproc start {} {
	$self instvar agent_ maxpkts_
	$agent_ advance $maxpkts_
}

Source/FTP instproc stop {} {
	$self instvar agent_
	$agent_ advance 0
}

Source/FTP instproc produce { pktcnt } {
	$self instvar agent_ 
	$agent_ advance $pktcnt
}

Source/FTP instproc producemore { pktcnt } {
	$self instvar agent_
	$agent_ advanceby $pktcnt
}


#
# For consistency with other applications
#
Class Source/Telnet -superclass Application/Telnet

Source/Telnet set maxpkts_ 268435456

Source/Telnet instproc attach o {
	$self instvar agent_
	set agent_ $o
	$self attach-agent $o
}

