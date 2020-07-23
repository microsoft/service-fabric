#!/usr/bin/perl -w

if (!@ARGV) {
    print qq{Execute generaterpmmetapayload.pl with filepath parameter\n};
	exit 1;   
}

use File::Basename;

my $filepath = $ARGV[0];
my $filename = basename($filepath);

my $filecontents;
{
    local $/;
    open my $fh, '<', $filepath or die "can't open $filepath: $!";
    $filecontents = <$fh>;
}

my $f = pack("u", $filecontents); # get packed data

$f =~ s/%/%%/g; # RPM expands %s so escape these

print "#FilePayload: $filename\n".
        qq{perl -pe '\$_=unpack("u",\$_)' << '__EOF__' > /tmp/servicefabric.metapayloadout\n}.
        $f."__EOF__\n".
        "#EndFilePayload\n";
