# Support for the NixVector routing ns
# George F. Riley, Georgia Tech, Spring 2000

Simulator instproc set-nix-routing { } {
    Simulator set nix-routing 1
    #Simulator set node_factory_ Node/NixNode
    Node enable-module "Nix"
}

Simulator instproc get-link-head { n1 n2 } {
    $self instvar link_
    return [$link_($n1:$n2) head]
}

Link instproc set-ipaddr { ipaddr netmask } {
# Fix this later...GFR
#    $self instvar ipaddr_ netmask_
#    set ipaddr_  [[Simulator instance] convert-ipaddr $ipaddr]
#    set netmask_ [[Simulator instance] convert-ipaddr $netmask]
#    # puts "Set ipaddr to [[Simulator instance] format-ipaddr $ipaddr_]"
#    # And set in the nodes ip address list
#    set sn [$self src]
#    $sn set-ipaddr $ipaddr_ $netmask_ $self
}

# NixNode instproc's
#Class Node/NixNode -superclass Node


#Class RtModule/Nix -superclass RtModule

#Nix-vector routing
RtModule/Nix instproc register { node } {
	$self next $node
	# Create classifier
	$self instvar classifier_
	set classifier_ [new Classifier/Nix]
	$classifier_ set-node-id [$node set id_]
	#puts "RtModule/Nix set node id to [$node set id_]"
	$node install-entry $self $classifier_
}

RtModule/Nix instproc route-notify { module } { }


