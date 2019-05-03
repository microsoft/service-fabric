#!perl -w
use strict;
use utf8;

use Exporter;
use vars qw(@ISA @EXPORT);

@ISA = qw(Exporter);
our @EXPORT = qw(ReadUntil ReadAllText ReadAllLines WriteAllText);

sub ReadUntil
{
    my $lines = shift;
    my $text = shift;
    my $start = shift;

    for (my $i = $start; $i <= $#$lines; ++$i)
    {
        my $line = $lines->[$i];
        chomp($line);
        $line =~ s/^\s+//;
        $line =~ s/\s+$//;
        if ($line eq $text)
        {
            return $i;
        }
    }

    die "ERROR: Text line '$text' not matched.\n";
}

sub ReadAllText
{
    my $file = shift;
    my $text = "";
    open(FILE, "<$file") || die "ERROR: Could not open '$file' for reading.\n";
    while (<FILE>)
    {
        $text .= $_;
    }

    close(FILE);

    return $text;
}

sub ReadAllLines
{
    my $file = shift;
    my @lines = ();
    open(FILE, "<$file") || die "ERROR: Could not open '$file' for reading.\n";
    while (<FILE>)
    {
        push(@lines, $_);
    }

    close(FILE);

    return \@lines;
}

sub WriteAllText
{
    my $file = shift;
    my $text = shift;

    open(FILE, ">$file") || die "ERROR: Could not open '$file' for writing.\n";
    print FILE $text;
    close(FILE);
}