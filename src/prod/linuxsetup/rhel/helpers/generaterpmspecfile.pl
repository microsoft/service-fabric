#!/usr/bin/perl -w

die qq{Usage: $0 [specTemplatePath] [newVersion] [scriptsDirectory]\n} if @ARGV < 3;

my $specTemplatePath=$ARGV[0];
my $newVersion=$ARGV[1];
my $scriptsDirectory=$ARGV[2];

use FindBin qw( $RealBin );
my $helpersDirectory=$RealBin;

# validate params
die "specTemplatePath $specTemplatePath does not exist, and I can't go on without it." unless -e $specTemplatePath;
die "newVersion $newVersion does not match expected format." unless $newVersion =~ /\d+\.\d+(\.\d+){1,2}/;
die "scriptsDirectory $scriptsDirectory does not exist, and I can't go on without it." unless -d $scriptsDirectory;
die "$helpersDirectory/generaterpmscript.pl does not exist, and I can't go on without it." unless -e "$helpersDirectory/generaterpmscript.pl";

# generate rpm scripts
my $pre=`$helpersDirectory/generaterpmscript.pl $scriptsDirectory/preinst`;
my $post=`$helpersDirectory/generaterpmscript.pl $scriptsDirectory/postinst`;
my $preun=`$helpersDirectory/generaterpmscript.pl $scriptsDirectory/prerm`;
my $postun=`$helpersDirectory/generaterpmscript.pl $scriptsDirectory/postrm`;

my $specFileContents;
{
    local $/;
    open my $fh, '<', $specTemplatePath or die "can't open $specTemplatePath: $!";
    $specFileContents = <$fh>;
}

# rewrite version
$specFileContents =~ s/(%define\s*version\s*(\d+\.\d+\.\d+\.\d+))/%define\t\tversion\t\t$newVersion/g;

# rewrite script sections
$specFileContents =~ s/%pre\n/%pre\n$pre/g;
$specFileContents =~ s/%post\n/%post\n$post/g;
$specFileContents =~ s/%preun\n/%preun\n$preun/g;
$specFileContents =~ s/%postun\n/%postun\n$postun/g;

print "$specFileContents";
