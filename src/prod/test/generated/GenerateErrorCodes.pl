#!perl -w
use strict;
use utf8;
use WFFileIO;

my $FabricRoot = shift;
my $DestFile = shift;

my $SdxRoot = $ENV{SDXROOT};
my $TestGenRoot = "$FabricRoot\\test\\generated";

my $ErrorCodeTemplateFile = "$TestGenRoot\\WinFabricErrorCodeInteropTemplate.txt";

my $IdlPublicErrorCodeFile = "$FabricRoot\\src\\idl\\public\\FabricTypes.idl";
my $IdlPublicErrorCodeLineFirst = "// Windows Error Codes reserved for Windows Fabric are [7100, 7499]";
my $IdlPublicErrorCodeLineLast = "} FABRIC_ERROR_CODE;";

my $CppInternalErrorCodeFile = "$FabricRoot\\src\\Common\\ErrorCodeValue.h";
my $CppInternalErrorCodeLineFirst = "// Public ErrorCodes";
my $CppInternalErrorCodeLineLast = "};";

sub TransformPublicErrorCode
{
    my $s = shift;
    $s =~ s/= 0x([A-Fa-f0-9]+)/= unchecked((int)0x$1)/;
    return $s;
}

sub TransformInternalErrorCode
{
    my $s = shift;
    $s =~ s/^    //;
    $s =~ s/\(int\)FABRIC_E/FABRIC_E/;
    if ($s !~ /ERROR_CODE/)
    {
        $s =~ s/ = / = WinFabricErrorCodeInterop./;
    }

    return $s;
}

sub ProcessFile
{
    my $nativeFile = shift;
    my $nativeLineFirst = shift;
    my $nativeLineLast = shift;
    my $transform = shift;

    my @nativeLines = @{ReadAllLines($nativeFile)};
    my $nativeStart = ReadUntil(\@nativeLines, $nativeLineFirst, 0);
    my $nativeEnd = ReadUntil(\@nativeLines, $nativeLineLast, $nativeStart + 1);
    @nativeLines = @nativeLines[$nativeStart .. ($nativeEnd - 1)];

    my $expectedManagedText = '';
    for (@nativeLines)
    {
        my $nativeLine = &$transform($_);
        $expectedManagedText .= $nativeLine;
    }

    return $expectedManagedText;
}

sub BuildErrorCodes
{
    my $nativePublicText = ProcessFile(
        $IdlPublicErrorCodeFile,
        $IdlPublicErrorCodeLineFirst,
        $IdlPublicErrorCodeLineLast,
        \&TransformPublicErrorCode);
    my $nativeInternalText = ProcessFile(
        $CppInternalErrorCodeFile,
        $CppInternalErrorCodeLineFirst,
        $CppInternalErrorCodeLineLast,
        \&TransformInternalErrorCode);

    my $templateFile = ReadAllText($ErrorCodeTemplateFile);

    $templateFile =~ s/__PUBLIC_ERROR_CONTENT__/$nativePublicText/;
    $templateFile =~ s/__INTERNAL_ERROR_CONTENT__/$nativeInternalText/;

    WriteAllText($DestFile, $templateFile);

    return 1;
}

sub Main
{
    my $exitCode = 0;

    my $success = BuildErrorCodes();
    if (!$success)
    {
        $exitCode = 1;
    }

    return $exitCode;
}

my $exitCode = Main();
exit $exitCode;
