use strict;
use warnings;

#
# *Config.h files are parsed for *_CONFIG_ENTRY macros to generate a CSV file
# containing all valid fabric settings (see src\prod\tools\ConfigurationValidator\GenerateConfigurationsCSV.pl).
# The settings CSV file is used both for validating cluster upgrades (unrecognized configs
# in a cluster manifest will fail upgrade validation) and to generate C# wrapper classes 
# used by our automation test and ScriptTest frameworks to override and generate cluster
# manifests for tests (WinFabricConfigSections.cs).
#
# This script creates a *Config.h file containing the necessary *_CONFIG_ENTRY macros expanded
# from another macro (see SL_CONFIG_PROPERTIES in src\prod\src\ServiceModel\data\txnreplicator\SLConfigMacros.h).
# Each component that includes a macro requiring expansion must run this script as part of its build 
# (see src\prod\src\Naming\StoreService\StoreService.vcxproj).
#
# The alternative would be to have special-case handling of such "nested" config sections in GenerateConfigurationsCSV.pl,
# which is a bit more messy to maintain.
#

my $clExePath = shift( @ARGV );
my $srcFileName = shift( @ARGV );
my $dstFileName = shift( @ARGV );
my $configSubsectionName = shift( @ARGV );

# preprocess to expand the macro
#
my $tmpFileName = $dstFileName . ".tmp";
my @command = ( $clExePath . "\\cl.exe", "/C", "/P", $srcFileName, "/Fi$tmpFileName" );
system( @command ) == 0 or die "system( @command ) failed: $?";

# Keep only the config entries in the relevant config sub-section
#
open (my $tmpFile, "<", $tmpFileName) or die "can't read from $tmpFileName $!";
my @fileContents;
while (<$tmpFile>)
{
    if (/$configSubsectionName/)
    {
        my @lines = split(';', $_);
        foreach ( @lines )
        {
            if (/CONFIG_ENTRY/)
            {
                my $line = $_;
                $line =~ s/^\s+//;
                $line =~ s/\s+$//;
                push (@fileContents, $line);
            }
        }
    }
    else
    {
        # Remove all other lines
    }
}

# Emit resulting config entries to a *Config.h file
#
close $tmpFile;
unlink $tmpFileName;
open (my $dstFile, ">", $dstFileName) or die "can't write to $dstFileName $!";

print $dstFile "// ------------------------------------------------------------\n";
print $dstFile "// Copyright (c) Microsoft Corporation.  All rights reserved.\n";
print $dstFile "// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.\n";
print $dstFile "// ------------------------------------------------------------\n";
print $dstFile "\n";
print $dstFile "// ************************************************************************\n";
print $dstFile "// This is a generated file.\n";
print $dstFile "// See src\\prod\\src\\scripts\\preprocess_config_macros.pl\n";
print $dstFile "//\n";
print $dstFile "// Do not modify it manually, but include it in your commit if it's updated\n";
print $dstFile "// by the build.\n";
print $dstFile "// ************************************************************************\n";
print $dstFile "\n";

while ((scalar @fileContents) > 0)
{
    my $line = pop (@fileContents);
    print $dstFile $line . "\n";
}

close $dstFile;
