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
#

# THE CODE INCLUDED IN THIS FILE IS USED FOR BACKWARD COMPATIBILITY ONLY

Class MobileNode/MIPBS -superclass Node/MobileNode/BaseStationNode

MobileNode/MIPBS instproc init {args} {
	$self next $args
	$self instvar regagent_ encap_ decap_ agents_ address_ dmux_ id_

	if { $dmux_ == "" } {
		set dmux_ [new Classifier/Port/Reserve]
		$dmux_ set mask_ 0x7fffffff
		$dmux_ set shift_ 0
		$self add-route $address_ $dmux_
	} 
	set regagent_ [new Agent/MIPBS $self]
	$self attach $regagent_ [Node/MobileNode  set REGAGENT_PORT]
	$self attach-encap 
	$self attach-decap
}

MobileNode/MIPBS instproc attach-encap {} {
	$self instvar encap_ address_ 
	
	set encap_ [new MIPEncapsulator]
	set mask 0x7fffffff
	set shift 0
	set nodeaddr [AddrParams addr2id $address_]
	$encap_ set addr_ [expr ( ~($mask << $shift) & $nodeaddr)]
	$encap_ set port_ 1
	$encap_ target [$self entry]
	$encap_ set node_ $self
}

MobileNode/MIPBS instproc attach-decap {} {
	$self instvar decap_ dmux_ agents_
	set decap_ [new Classifier/Addr/MIPDecapsulator]
	lappend agents_ $decap_
	$dmux_ install [Node/MobileNode set DECAP_PORT] $decap_
}
    
Class MobileNode/MIPMH -superclass Node/MobileNode/BaseStationNode

MobileNode/MIPMH instproc init { args } {
	eval $self next $args
	$self instvar regagent_ dmux_ address_
 
	if { $dmux_ == "" } {
		set dmux_ [new Classifier/Port/Reserve]
		$dmux_ set mask_ 0x7fffffff
		$dmux_ set shift_ 0
		$self add-route $address_ $dmux_
	} 
    
	set regagent_ [new Agent/MIPMH $self]
	$self attach $regagent_ [Node/MobileNode set REGAGENT_PORT]
	$regagent_ node $self
}

Class SRNode/MIPMH -superclass SRNode

SRNode/MIPMH instproc init { args } {
	eval $self next $args
	$self instvar regagent_ dmux_ address_
    
	if { $dmux_ == "" } {
		set dmux_ [new Classifier/Port/Reserve]
		$dmux_ set mask_ 0x7fffffff
		$dmux_ set shift_ 0
		$self add-route $address_ $dmux_
	} 
	eval $self next $args
	set regagent_ [new Agent/MIPMH $self]
	$self attach $regagent_ [Node/MobileNode set REGAGENT_PORT]
	$regagent_ node $self
}
		
		
