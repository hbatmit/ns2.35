
sub usage {
    die("usage: $0 <mainfile> <toolname>");
}

if ($#ARGV != 1) {
    &usage;
}

$mainfile = $ARGV[0];
$toolname = $ARGV[1];
open(OF, ">$toolname");
print "expanding $mainfile to $toolname ... \n";

# print header
open(H, "head.tcl");
while (<H>) {
    s/TOOLNAME/$toolname/;
    print OF;
}
close(H);

$tclsh = $ENV{'TCLSH'};
print "using tclsh=$tclsh\n";
open(S, "$tclsh tcl-expand.tcl $mainfile |");

while (<S>) {
    if (!(/^\s*#/ || /^\s*$/)) {
        print OF;
        $n++;
        ($n % 200 == 0) && print("*");
    }
}
print "\ntotal $n lines\n";
close(S);
close(OF);

