#
# Performance counter string resources (WFPerf.rc) are generated in such a way that
# adding new perf counters can easily cause many perf counter resource IDs to
# shift, which results in mis-matching IDs in the comments file (LCI). Adding a new
# perf counter component causes a similar shift. Instead of attempting to enforce
# the generation order of new perf counter components, we instead use this script
# to update perf counter resource comments since they tend to be fairly well-structured
# compared to generic resource strings. Generic string resources do not have the same
# resource shift issue since they are all defined in a single component.
#
# The localization framework has an odd way of processing LCI (comments) files.
# The build process generates the LCG (source strings) file, which is then loaded into
# the LcxAdmin tool to manually add comments and produce a resulting LCI file. Then
# the build again takes any LCI file and applies those comments back to the original
# LCG file. This makes integrating this script into the build awkward, so the current
# process is just to run this script manually as needed to update perf counter comments
# and save it through LcxAdmin before building.
#

use strict;
use warnings;

# These values must match IDS_PERFORMANCE_COUNTER_* defined in "src\prod\src\inc\resourceids.h"
#
my $MinPerfCounterId = 1;
my $MaxPerfCounterId = 9999;

my @LockedKeywords = (
    "Reconfiguration Agent",
    "Atomic Group",
    "AsyncJobQueue",
    "JobQueue",
    "Actor",
    "InBuild",
    "StandBy",
    "Offline",
    "AO",
    "NO",
    "PLB",
    "LFUM",
    "FT",
    "ESE",
);
my %DefinitionsMap = (
    "Counters", "Windows performance counters.",
    "Entities", "System components against which users can report telemetry for monitoring.",
    "Health Report", "Telemetry for monitoring functionality of running processes.",
    "Name", "URI (e.g. \"fabric:/myname\").",
    "Common Queue", "Queue shared by different request types.",
    "Txn", "Database transaction.",
    "IPC", "Inter-Process Communication.",
    "Replicated Store", "Replicated database.",
    "IO", "Input/Output.",
);

my $lcg_in = shift(@ARGV); # e.g. "objd\amd64\FabricResources.dll.lcg"
my $lci_out = "FabricResources.dll.lci";

open (my $lcg_if, "<", $lcg_in) or die "can't read from $lcg_in $!";
open (my $lci_of, ">", $lci_out) or die "can't write to $lci_out $!";

my $isPerfCounterResource = 0;
my $currentResourceString = "";

while (<$lcg_if>)
{
    if (/<Item ItemId="(.*)" ItemType=";STR".*$/)
    {
        if ($1 >= $MinPerfCounterId && $1 <= $MaxPerfCounterId)
        {
            $isPerfCounterResource = 1;
        }
        else
        {
            $isPerfCounterResource = 0;
        }
    }

    if ($isPerfCounterResource)
    {
        if (/<Cmt/ || /<\/Cmt/)
        {
            next;
        }

        print $lci_of $_;

        if (/<Val><!\[CDATA\[(.*)\]\]>/)
        {
            $currentResourceString = $1; 
        }
        elsif (/<Disp Icon=/)
        {
          my $comment = "";
          foreach my $key ( @LockedKeywords )
          {
              if ($currentResourceString =~ /$key/)
              {
                  if ($comment ne "") { $comment = $comment . " "; }
                  $comment = $comment . "{Locked=\"" . $key . "\"}";
              }
          }
          
          foreach my $key ( keys %DefinitionsMap )
          {
              if ($currentResourceString =~ /$key/)
              {
                  if ($comment ne "") { $comment = $comment . " "; }
                  $comment = $comment . "\"" . $key . "\"=\"" . $DefinitionsMap{$key} . "\"";
              }
          }

          if ($comment ne "")
          {
            print $lci_of "      <Cmts>\n";
            print $lci_of "        <Cmt Name=\"LcxAdmin\"><![CDATA[" . $comment . "]]></Cmt>\n";
            print $lci_of "      </Cmts>\n";
          }
        }
    }
    else
    {
        if (/<Cmt Name="Dev"/ || /<Cmt Name="Rccx"/)
        {
            next;
        }

        print $lci_of $_;
    }
}

close ($lci_of);

print STDOUT "DONE";
