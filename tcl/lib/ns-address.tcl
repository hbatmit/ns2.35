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

#####################################################################################

# API's for allocating bits to ns addressing structure
# These routines should be used for setting bits for portid (agent), for nodeid 
# (flat or hierarchical).
# DEFADDRSIZE_ and MAXADDRSIZE_ defined in ns-default.tcl


# Name :        set-address-format ()
# Options :     default, hierarchical (default) and hierarchical
#		(with specific allocations) 
# Synopsis :    set-address-format <option> <additional info, if required
# 		by option> 
#
# Option:1      def (default settings)
# Synopsis :    set-address-format def
# Currently 31 bit address space is the default feature in ns , the uppermost
# bit is avoided due to Tcl/C++ integer issues
#               1 bit for multicast
#               30 bits for node id

# Option :2     hierarchical (default)
# Synopsis :    set-address-format hierarchical
# Description:  This allows setting of hierarchical address as follows:
#                   * Sets mcast bit (if specified)
#                   * Sets default hierchical levels with:
#                      * 3 levels of hierarchy and
#                      * (9 11 11) by default 
#                      * (8 11 11) if mcast is enabled.
#
# Option 4:     hierarchical (specified)
# Synopsis:     set-address-format hierarchical <#n hierarchy levels> 
#		<#bits for level1> <#bits for level 2> ....
#		<#bits for nth level>
#               e.g  
#                   set-address-format hierarchical 4 2 2 2 10
# Description:  This allows setting of hierarchical address as specified for 
#		each level.
#                 * Sets mcast bit (if specified)
#                 * Sets hierchical levels with bits specified for each level.

# Errors: 	* if # of bits specified less than 0.
#               * if bit positions clash (contiguous # of requested free bits 
#		  not found)
#               * if total # of bits exceed MAXADDRSIZE_
#               * if # hierarchy levels donot match with #bits specified (for
#                 each level). 

Class AllocAddrBits

Simulator proc address-format {} {
	return [Simulator set AddressFormat_]
}

Simulator proc default-addr? {} {
	if { [Simulator set AddressFormat_] == "DEF" } {
		return 1
	} else { 
		return 0
	}
}

Simulator proc hier-addr? {} {
	if { [Simulator set AddressFormat_] == "HIER" } {
		return 1
	} else {
		return 0
	}
}

Simulator instproc set-address-format {opt args} {
	set len [llength $args]
	if {[string compare $opt "def"] == 0} {
		$self set-address 31
		set mcastshift [AddrParams McastShift]
		Simulator set McastAddr_ [expr 1 << $mcastshift]
		mrtObject expandaddr
		Simulator set AddressFormat_ DEF
	} elseif {[string compare $opt "expanded"] == 0} {
		puts "set-address-format expanded is obsoleted by 31-bit addressing."
	} elseif {[string compare $opt "hierarchical"] == 0 && $len == 0} {
		if [$self multicast?] {
			$self set-hieraddress 3 8 11 11
		} else {
			$self set-hieraddress 3 9 11 11
		}
	} elseif {[string compare $opt "hierarchical"] == 0 && $len > 0} {
		eval $self set-hieraddress [lindex $args 0] \
				[lrange $args 1 [expr $len - 1]]
	} else {
		error "ns-address.tcl:set-address-format: Unknown address format $opt"
	}
}

Simulator instproc set-hieraddress {hlevel args} {
	set a [$self get-AllocAddrBits "new"]
	$a set size_ [AllocAddrBits set MAXADDRSIZE_]
	# By default, setting hierarchical addressing turns on hier rtg
	Simulator set AddressFormat_ HIER
	Node enable-module "Hier"
	if [$self multicast?] {
		$a set-mcastbits 1
	}
	eval $a set-idbits $hlevel $args
}

# Sets address for nodeid and port fields
# The order of setting bits for different fields does matter and should 
# be as follows:
#   mcast
#   idbits
#   portbits
# this is true for both set-address and set-hieraddress
Simulator instproc set-address {node} {
	set a [$self get-AllocAddrBits "new"]
	$a set size_ [AllocAddrBits set DEFADDRSIZE_]
	if {[expr $node] > [$a set size_]} {
		$a set size_ [AllocAddrBits set MAXADDRSIZE_]
	}
		
	# One bit is set aside for mcast as default :: this waste of 1 bit 
	# may be avoided, if mcast option is enabled before the initialization
	# of Simulator.
	$a set-mcastbits 1
	set lastbit [expr $node - [$a set mcastsize_]]
	$a set-idbits 1 $lastbit
}

Simulator instproc get-AllocAddrBits {prog} {
	$self instvar allocAddr_
	if ![info exists allocAddr_] {
		set allocAddr_ [new AllocAddrBits]
	} elseif ![string compare $prog "new"] {
		set allocAddr_ [new AllocAddrBits]
	}
	return $allocAddr_
}

Simulator instproc expand-port-field-bits nbits {
	# The function is obsolete, given that ports are now 31 bits wide
	puts "Warning: Simulator::expand-port-field-bits is obsolete.  Ports are 31 bits wide"
	return
}

Simulator instproc expand-address {} {
	puts "Warning: Simulator::expand-address is obsolete.  The node address is 31 bits wide"
	return
}


AllocAddrBits instproc init {} {
	eval $self next
	$self instvar size_ portsize_ idsize_ mcastsize_
	set size_ 0
	set portsize_ 0
	set idsize_ 0
	set mcastsize_ 0
}

AllocAddrBits instproc get-AllocAddr {} {
	$self instvar addrStruct_
	if ![info exists addrStruct_] {
		set addrStruct_ [new AllocAddr]
	}
	return $addrStruct_
}

AllocAddrBits instproc get-Address {} {
	$self instvar address_
	if ![info exists address_] {
		set address_ [new Address]
	}
	return $address_
}

AllocAddrBits instproc chksize {bit_num prog} {
	$self instvar size_ portsize_ idsize_ mcastsize_  
	if {$bit_num <= 0 } {
		error "$prog : \# bits less than 1"
	}
	set totsize [expr $portsize_ + $idsize_ + $mcastsize_]
	if {$totsize > [AllocAddrBits set MAXADDRSIZE_]} {
		error "$prog : Total \# bits $totsize exceed MAXADDRSIZE"
	}
	if { $size_ < [AllocAddrBits set MAXADDRSIZE_]} {
		if {$totsize > [AllocAddrBits set DEFADDRSIZE_]} {
			set size_ [AllocAddrBits set MAXADDRSIZE_]
			return 1
		} 
	}
	return 0

}

AllocAddrBits instproc set-portbits {bit_num} {
	# The function is obsolete, given that ports are now 31 bits wide
	puts "Warning: AllocAddrBits::set-portbits is obsolete.  Ports are 31 bits wide."
	return
}

AllocAddrBits instproc expand-portbits nbits {
	# The function is obsolete, given that ports are now 31 bits wide
	puts "Warning: AllocAddrBits::expand-portbits is obsolete.  Ports are 31 bits wide."
	return
}

AllocAddrBits instproc set-mcastbits {bit_num} {
	$self instvar size_ mcastsize_
	if {$bit_num != 1} {
		error "setmcast : mcastbit > 1"
	}
	set mcastsize_ $bit_num

	#chk to ensure there;s no change in size
	if [$self chksize mcastsize_ "setmcast"] {
		# assert {$chk == 0} --> assert doesn't seem to work
		error "set-mcastbits: size_ has been changed."
	}
	set a [$self get-AllocAddr] 
	set v [$a setbit $bit_num $size_]
	AddrParams McastMask [lindex $v 0]
	AddrParams McastShift [lindex $v 1]
	### TESTING
	# set mask [lindex $v 0]
	# set shift [lindex $v 1]
	# puts "Mcastshift = $shift \n McastMask = $mask\n"

	set ad [$self get-Address]
	$ad mcastbits-are [AddrParams McastShift] [AddrParams McastMask]
}

AllocAddrBits instproc set-idbits {nlevel args} {
	$self instvar size_ portsize_ idsize_ hlevel_ hbits_
	if {$nlevel != [llength $args]} {
		error "setid: hlevel < 1 OR nlevel and \# args donot match"
	}
	set a [$self get-AllocAddr] 
	set old 0
	set idsize_ 0
	set nodebits 0
	# Set two global variables
	AddrParams hlevel $nlevel
	set hlevel_ $nlevel
	for {set i 0} {$i < $nlevel} {incr i} {
		set bpl($i) [lindex $args $i]
		set idsize_ [expr $idsize_ + $bpl($i)]
		
		# check to ensure there's no change in size
		set chk [$self chksize $bpl($i) "setid"]
		# assert {$chk ==0}
		if {$chk > 0} {
			error "set-idbits: size_ has been changed."
		}
		set v [$a setbit $bpl($i) $size_]
		AddrParams NodeMask [expr $i+1] [lindex $v 0]
		set m([expr $i+1]) [lindex $v 0]
		AddrParams NodeShift [expr $i+1] [lindex $v 1]
		set s([expr $i+1]) [lindex $v 1]
		lappend hbits_ $bpl($i)
		
	}
	AddrParams nodebits $idsize_
	set ad [$self get-Address]
	eval $ad idsbits-are [array get s]
	eval $ad idmbits-are [array get m]
	eval $ad bpl-are $hbits_
	### TESTING
	# set mask [lindex $v 0]
	# set shift [lindex $v 1]
	# puts "Nodeshift = $shift \n NodeMask = $mask\n"
}


# Create an integer address from addr string 
AddrParams proc addr2id addrstr {
	if [Simulator hier-addr?] {
		set addressObj [[[Simulator instance] get-AllocAddrBits ""] \
				get-Address]
		return [$addressObj str2addr $addrstr]
	} else {
		return [expr $addrstr & [AddrParams NodeMask 1] << \
				[AddrParams NodeShift 1]]
	}
}

# Returns address string from an integer address: reverse of set-hieraddr.
AddrParams proc id2addr addr {
	for {set i 1} {$i <= [AddrParams hlevel]} {incr i} {
		set a [expr ($addr >> [AddrParams NodeShift $i]) & \
				[AddrParams NodeMask $i]]
		lappend str $a
	}
	return $str
}

# Splitting up address string
AddrParams proc split-addrstr addrstr {
	return [split $addrstr .]
}

# Returns number of elements at a given hierarchical level, that is visible to 
# a node.
AddrParams proc elements-in-level? {nodeaddr level} {
	AddrParams instvar domain_num_ cluster_num_ nodes_num_ 
	set L [AddrParams split-addrstr $nodeaddr]
	set level [expr $level + 1]
	#
	# if no topology info found for last level, set default values
	#
	
	# For now, assuming only 3 levels of hierarchy 
	if { $level == 1} {
		return $domain_num_
	}
	if { $level == 2} {
		return [lindex $cluster_num_ [lindex $L 0]]
	}
	if { $level == 3} {
		set C 0
		set index 0
		while {$C < [lindex $L 0]} {
			set index [expr $index + [lindex $cluster_num_ $C]]
			incr C
		}
		return [lindex $nodes_num_ [expr $index + [lindex $L 1]]]
	}
}
