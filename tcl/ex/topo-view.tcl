################ XXX temp to view the topology using nam.. !! ###

proc view-topology { infile outfile hier_flag } {
        source $infile
	global ns node
	set f [open $outfile w]
        set ns [new Simulator]
	
        $ns namtrace-all $f

        # create-topology ... [params]..
	if {$hier_flag} {
		create-hier-topology ns node 1.5Mb
	} else {
		create-topology ns node 1.5Mb
	}

        #... dump the file
        $ns dump-topology $f
        delete $ns
        flush $f
        close $f
        # run nam
        puts "running nam with $outfile ... "
        exec nam $outfile &
}
                
Simulator instproc dump-topology { file } {
        $self instvar topology_dumped started_
	set started_ 1

        # set topology_dumped 1

	# make sure this does not affect anything else later on !
	# $self namtrace-all $file
                
        # Color configuration for nam
        $self dump-namcolors
                
        # Node configuration for nam
        $self dump-namnodes
                
        # Lan and Link configurations for nam
        $self dump-namlans
        $self dump-namlinks

        # nam queue configurations
        $self dump-namqueues
 
        # Traced agents for nam
        $self dump-namagents
}

