#!/usr/bin/perl -w

die qq{Usage: $0 [specFilePath] [payloadFile1] [payloadFile2] ...\n} if @ARGV < 2;

my $specFilePath=$ARGV[0];

use FindBin qw( $RealBin );
my $helpersDirectory=$RealBin;

# validate params
die "specFilePath $specFilePath does not exist, and I can't go on without it." unless -e $specFilePath;
die "Payload file does not exist." unless -e $ARGV[1];
die "$helpersDirectory/generaterpmmetapayload.pl does not exist, and I can't go on without it." unless -e "$helpersDirectory/generaterpmmetapayload.pl";

use strict;
use warnings;

shift @ARGV; # all other params are files to be embedded as meta payloads in the spec file

# generate payloads
my $payloads="";
foreach my $payloadfile (@ARGV) {
	$payloads .= `$helpersDirectory/generaterpmmetapayload.pl $payloadfile\n`;
}

my $specFileContents;
{
    local $/;
    open my $fh, '<', $specFilePath or die "can't open $specFilePath: $!";
    $specFileContents = <$fh>;
}

# write meta payload section
$specFileContents =~ s/#METAPAYLOAD\n/#METAPAYLOAD\n$payloads/g;

print "$specFileContents";

open(my $fh, '>', "$specFilePath") or die "Spec file $specFilePath couldn't be opened for writing: $!.";
print $fh "$specFileContents";
close $fh;
