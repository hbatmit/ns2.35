# Copyright 2002, Statistics Research, Bell Labs, Lucent Technologies and
# The University of North Carolina at Chapel Hill
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

# Reference
#     Stochastic Models for Generating Synthetic HTTP Source Traffic 
#     J. Cao, W.S. Cleveland, Y. Gao, K. Jeffay, F.D. Smith, and M.C. Weigle 
#     IEEE INFOCOM 2004.
#
# Documentation available at http://dirt.cs.unc.edu/packmime/
# 
# Contacts: Michele Weigle (mcweigle@cs.unc.edu),
#           Kevin Jeffay (jeffay@cs.unc.edu)

PackMimeHTTP instproc alloc-tcp {tcptype} {
    if {$tcptype != "Reno"} {
	set tcp [new Agent/TCP/FullTcp/$tcptype]
    } else {
	set tcp [new Agent/TCP/FullTcp]
    }
    return $tcp
}

PackMimeHTTP instproc setup-tcp {tcp fid} {
    # set flow ID
    $tcp set fid_ $fid

    # register done procedure for when connection is closed
    $tcp proc done {} "$self done $tcp"
}

# These need to be here so that we can attach the
# apps to a node (done in Tcl code)
PackMimeHTTP instproc alloc-client-app {} {
    return [new Application/PackMimeHTTP/Client]
}

PackMimeHTTP instproc alloc-server-app {} {
    return [new Application/PackMimeHTTP/Server]
}

PackMimeHTTP instproc done {tcp} {
    # the connection is done, so recycle the agent
    $self recycle $tcp
}

PackMimeHTTP instproc now {} {
    set ns [Simulator instance]
    return [format "%.6f" [$ns now]]
}
