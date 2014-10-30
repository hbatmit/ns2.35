 # Copyright 2007, Old Dominion University
 # Copyright 2007, University of North Carolina at Chapel Hill
 #
 # Redistribution and use in source and binary forms, with or without 
 # modification, are permitted provided that the following conditions are met:
 # 
 #    1. Redistributions of source code must retain the above copyright 
 # notice, this list of conditions and the following disclaimer.
 #    2. Redistributions in binary form must reproduce the above copyright 
 # notice, this list of conditions and the following disclaimer in the 
 # documentation and/or other materials provided with the distribution.
 #    3. The name of the author may not be used to endorse or promote 
 # products derived from this software without specific prior written 
 # permission.
 # 
 # THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 # IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 # WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 # DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
 # INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 # (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 # SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 # CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 # STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 # ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 # POSSIBILITY OF SUCH DAMAGE.
 #
 # Contact: Michele Weigle (mweigle@cs.odu.edu)

#
# ONE-WAY TCP FUNCTIONS
#

# one-way TCP agent allocation 
Tmix instproc alloc-agent {tcptype} {
    if {$tcptype != "Tahoe"} {
	return [new Agent/TCP/$tcptype]
    } else {
	# Tahoe is the default in one-way TCP
	return [new Agent/TCP]
    }
}

# one-way TCP sink allocation
Tmix instproc alloc-sink {sinktype} {
    if {$sinktype != "default"} {
	return [new Agent/TCPSink/$sinktype]
    } else {
	return [new Agent/TCPSink]
    }
}

Tmix instproc configure-sink {sink} {
    $sink set packetSize_ 40
}

Tmix instproc configure-source {agent fid wnd mss} {
    # set flow ID, max window, and packet size
    $agent set fid_ $fid
    $agent set window_ $wnd
    $agent set packetSize_ $mss
}

#
# FULL TCP FUNCTIONS
#

# Full TCP agent allocation
Tmix instproc alloc-tcp {tcptype} {
    if {$tcptype != "Reno"} {
	return [new Agent/TCP/FullTcp/$tcptype]
    } else {
	# Reno is the default in Full-TCP
	return [new Agent/TCP/FullTcp]
    }
}

Tmix instproc setup-tcp {tcp fid wnd mss} {
    # set flow ID and max window
    $tcp set fid_ $fid
    $tcp set window_ $wnd
    $tcp set segsize_ $mss

    # register done procedure for when connection is closed
    $tcp proc done {} "$self done $tcp"
}

#
# HELPER FUNCTIONS
#

# This is here so that we can attach the apps to a node
Tmix instproc alloc-app {} {
    return [new Application/Tmix]
}

Tmix instproc done {tcp} {
    # the connection is done, so recycle the agent
    $self recycle $tcp
}

Tmix instproc now {} {
    set ns [Simulator instance]
    return [format "%.6f" [$ns now]]
}
