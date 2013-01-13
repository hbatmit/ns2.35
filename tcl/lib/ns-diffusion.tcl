# All diffusion related procedures go here

Simulator instproc attach-diffapp { node diffapp } {
    $diffapp dr [$node get-dr]
}

Node instproc get-dr {} {
    $self instvar diffAppAgent_
    if [info exists diffAppAgent_] {
	return $diffAppAgent_
    } else {
	puts "Error: No DiffusionApp agent created for this node!\n" 
	exit 1
    }
}


Node instproc create-diffusionApp-agent { diffFilters } {
    $self instvar gradient_ diffAppAgent_
    
    # first we create diffusion agent
    # if it doesnot exist already
    # do we really need this check, I mean for a given node
    # is create-diffusionApp-agent{} called more than once??
    
    if [info exists diffAppAgent_] {
	puts "diffAppAgent_ exists: $diffAppAgent_"
	return $diffAppAgent_
    }

    # create diffApp agent for this node
    $self set diffAppAgent_ [new Agent/DiffusionApp]
    set da $diffAppAgent_
    set port [get-da-port $da $self]
    $da agent-id $port
    $da node $self
    
    # now setup filters for this node
    if {$diffFilters == ""} {
	puts "Error: No filter defined for diffusion!\n"
	exit 1
    }
    #XXXX FOR NOW
    set n 0
    foreach filtertype [split $diffFilters "/"] {
	if {$filtertype == "GeoRoutingFilter" } continue
	set filter($n) [new Application/DiffApp/$filtertype $da]
	#$self set filtertype_($n) $filter($n)
	$filter($n) start         ;# starts filter
	incr n
    }
    
    #return $da
}


proc get-da-port {da node} {

    # diffusion assumes diffusion-application agent
    # to be attached to non-zero port numbers
    # thus for assigning port 254 to diffAppAgent
    set port [Node set DIFFUSION_APP_PORT]
    $node attach $da $port
    return $port
}
