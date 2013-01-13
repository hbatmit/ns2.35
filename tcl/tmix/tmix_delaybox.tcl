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

Simulator instproc Tmix_DelayBox args {
	$self instvar Node_
	
	set node [new Node/Tmix_DelayBox]

	# set classifier_ in C++ to appropriate object
	$node create-classifier
	
	set Node_([$node id]) $node
	$self add-node $node [$node id]
	$node nodeid [$node id]
	$node set ns_ $self
	$self check-node-num
	return $node
}

Node/Tmix_DelayBox instproc init args {
    eval $self next $args
    $self instvar db_classifier_
   
    set db_classifier_ [new Classifier/Tmix_DelayBox]

    # insert classifier_ as entry point and insert default classifier
    # in slot 0 of classifier_
    $self insert-entry [$self get-module Base] $db_classifier_ 0
}
