#!/usr/bin/perl -w

die qq{Usage: $0 [templatesPath] [copyrightPath] [valueToReplace]\n} if !@ARGV || @ARGV < 3;

my $templatesPath=$ARGV[0];
my $copyrightPath=$ARGV[1];
my $valueToReplace=$ARGV[2];

die "templatesPath $templatesPath does not exist, and I can't go on without it." unless -e $templatesPath;
die "copyrightPath $copyrightPath does not exist, and I can't go on without it." unless -e $copyrightPath;
die "valueToReplace must be a real value." unless $valueToReplace ne "";

my $templatesFileContents;
{
    local $/;
    open my $fh, '<', $templatesPath or die "can't open $templatesPath: $!";
    $templatesFileContents = <$fh>;
}

my $copyrightFileContents;
{
    local $/;
    open my $fh, '<', $copyrightPath or die "can't open $copyrightFileContents: $!";
    $copyrightFileContents = <$fh>;
}

# rewrite placeholder
$templatesFileContents =~ s/$valueToReplace/$copyrightFileContents/g;

print "$templatesFileContents";
