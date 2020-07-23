#!/usr/bin/perl -w

if (!@ARGV) {
    print qq{Execute generaterpmscript.pl with filepath parameter\n};
	exit 1;   
}

my $filepath = $ARGV[0];
my $filecontents;
{
    local $/;
    open my $fh, '<', $filepath or die "can't open $filepath: $!";
    $filecontents = <$fh>;
}

my $f = pack("u", $filecontents); # get packed script
$f =~ s/%/%%/g; # RPM expands %s so escape these

print "#!/bin/sh\n".
       "set -e\n".
	   "mkdir /tmp/servicefabric.\$\$\n".
	   qq{perl -pe '\$_=unpack("u",\$_)' << '__EOF__' > /tmp/servicefabric.\$\$/script\n}.
	   $f."__EOF__\n".
	   "chmod 755 /tmp/servicefabric.\$\$/script\n".
	   "/tmp/servicefabric.\$\$/script \"\$@\"\n".
	   "rm -f /tmp/servicefabric.\$\$/script\n".
	   "rmdir /tmp/servicefabric.\$\$\n";
