#!perl -w
use strict;
use utf8;
use WFFileIO;

my $FabricRoot = shift;
my $ProductConfigurationFile = shift;
my $DestFile = shift;

my $TestGenRoot = "$FabricRoot/test/generated";

my $ConfigTemplateFile = "$TestGenRoot/WinFabricConfigSectionsTemplate.txt";
my $ConfigClassTemplate =
    "\n" .
    "    [Serializable]\n" .
    "    [GeneratedCode(\"GenerateConfigSections.pl\", \"1.0\")]\n" .
    "    public class __CLASS_NAME__Section : ConfigSection\n" .
    "    {\n" .
    "__PROPERTY_DEFAULTS__\n" .
    "        public __CLASS_NAME__Section()\n" .
    "            : base(\"__SECTION_NAME__\")\n" .
    "        {\n" .
    "__CTOR__" .
    "        }\n\n" .
    "        public __CLASS_NAME__Section(__CLASS_NAME__Section other)\n" .
    "            : this()\n" .
    "        {\n" .
    "            if (other == null)\n" .
    "            {\n" .
    "                throw new ArgumentNullException(\"other\");\n" .
    "            }\n\n" .
    "__COPYCTOR__" .
    "        }\n\n" .
    "__PROPERTY_ACCESSORS__" .
    "    }\n";

# Config sections to exclude entirely from generation
my %ExcludedSections =
(
    "Trace/Console" => 1,
    "Trace/File" => 1,
    "FabricRandomTest" => 1,
    "FabricTest" => 1,
    "FabricTestQueryWeights" => 1,
    "FederationTest" => 1,
    "FederationRandomTest" => 1,
    "LBSimulator" => 1,
    "UnreliableTransport" => 1,
    "SeedNodeClientConnectionAddresses" => 1
);

# Config sections which are node-specific that should be excluded from cluster-wide config
my %ClusterConfigExcludedSections =
(
    "FabricNode" => 1,
    "NodeDomainIds" => 1,
    "NodeProperties" => 1
);

my $ClusterConfigCtor = "";
my $ClusterConfigCopyCtor = "";
my $ClusterConfigProperties = "";

sub IsTypeNullable
{
    my $propertyType = shift;

    return
        $propertyType eq "int" ||
        $propertyType eq "Int64" ||
        $propertyType eq "uint" ||
        $propertyType eq "bool" ||
        $propertyType eq "double" ||
        $propertyType eq "TimeSpanInSeconds";
}

sub IsTypeCLSCompliant
{
    my $propertyType = shift;

    return
        $propertyType ne "uint";
}

sub FixupPropertyType
{
    my $rawPropertyType = shift;
    my $rawPropertyName = shift;

    my $propertyType = $rawPropertyType;
    if (index($rawPropertyName, "AllowedCommonNames") > 0 ||
        ($rawPropertyName eq "AdminClientCommonNames") ||
        ($rawPropertyName eq "ClusterIdentities") ||
        ($rawPropertyName eq "ClientIdentities") ||
        ($rawPropertyName eq "AdminClientIdentities") ||
        ($rawPropertyName eq "ProducerInstances") ||
        ($rawPropertyName eq "ConsumerInstances"))
    {
        $propertyType = "IList<string>";
    }
    elsif ($propertyType eq "VoteConfig" && $rawPropertyName eq "PropertyGroup")
    {
        $propertyType = "IDictionary<string, string>";
    }
    elsif ($propertyType eq "KeyDoubleValueMap")
    {
        $propertyType = "IDictionary<string, double>";
    }
    elsif ($propertyType eq "Common::SecurityConfig::X509NameMap")
    {
        $propertyType = "IDictionary<string, string>";
    }
    elsif ($propertyType eq "X509NameMap")
    {
        $propertyType = "IDictionary<string, string>";
    }
    elsif ($propertyType eq "Common::SecurityConfig::IssuerStoreKeyValueMap")
    {
        $propertyType = "IDictionary<string, string>";
    }
    elsif ($propertyType eq "IssuerStoreKeyValueMap")
    {
        $propertyType = "IDictionary<string, string>";
    }
    elsif ($propertyType eq "KeyBoolValueMap")
    {
        $propertyType = "IDictionary<string, bool>";
    }
    elsif ($propertyType eq "KeyIntegerValueMap")
    {
        $propertyType = "IDictionary<string, int>";
    }
    elsif ($propertyType eq "NodeFaultDomainIdCollection")
    {
        $propertyType = "IDictionary<string, string>";
    }
    elsif ($propertyType eq "NodeCapacityCollectionMap")
    {
        $propertyType = "IDictionary<string, double>";
    }
    elsif ($propertyType eq "NodePropertyCollectionMap")
    {
        $propertyType = "IDictionary<string, string>";
    }
    elsif ($propertyType eq "NodeSfssRgPolicyMap")
    {
        $propertyType = "IDictionary<string, string>";
    }
    elsif ($propertyType eq "wstring")
    {
        $propertyType = "string";
    }
    elsif ($propertyType eq "SecureString")
    {
        $propertyType = "string";
    }
	elsif ($propertyType eq "FabricVersionInstance")
    {
        $propertyType = "string";
    }
	elsif ($propertyType eq "KeyStringValueCollectionMap")
    {
        $propertyType = "IDictionary<string, string>";
    }
    return $propertyType;
}

sub FixupPropertyValue
{
    my $rawPropertyDefaultValue = shift;

    my $propertyDefaultValue = $rawPropertyDefaultValue;
    if ($propertyDefaultValue eq "")
    {
        $propertyDefaultValue = "string.Empty";
    }
    elsif ($propertyDefaultValue eq "L\"\"")
    {
        $propertyDefaultValue = "string.Empty";
    }
    elsif (substr($propertyDefaultValue, 0, 25) eq "Common::SecureString(L\"\")")
    {
        $propertyDefaultValue = "string.Empty";
    }
    elsif (substr($propertyDefaultValue, 0, 20) eq "Common::SecureString")
    {
        $propertyDefaultValue = substr($propertyDefaultValue, 22, index($propertyDefaultValue, ')') - 22);
    }
    elsif (substr($propertyDefaultValue, 0, 1) eq "L")
    {
        $propertyDefaultValue = substr($propertyDefaultValue, 1);
    }
    elsif (substr($propertyDefaultValue, 0, 4) eq "None")
    {
        $propertyDefaultValue = "null";
    }
    elsif (substr($propertyDefaultValue, 0, 21) eq "TimeSpan::FromSeconds")
    {
        $propertyDefaultValue = substr($propertyDefaultValue, 22, index($propertyDefaultValue, ')') - 22);
    }
    elsif (substr($propertyDefaultValue, 0, 29) eq "Common::TimeSpan::FromSeconds")
    {
        $propertyDefaultValue = substr($propertyDefaultValue, 30, index($propertyDefaultValue, ')') - 30);
    }
    elsif (substr($propertyDefaultValue, 0, 29) eq "Common::TimeSpan::FromMinutes")
    {
        $propertyDefaultValue = substr($propertyDefaultValue, 30, index($propertyDefaultValue, ')') - 30);
        $propertyDefaultValue *= 60;
    }
    elsif (substr($propertyDefaultValue, 0, 34) eq "Common::TimeSpan::FromMilliseconds")
    {
        $propertyDefaultValue = substr($propertyDefaultValue, 35, index($propertyDefaultValue, ')') - 35);
        if ($propertyDefaultValue != 0)
        {
            $propertyDefaultValue = 1.0 / (1000 / $propertyDefaultValue);
        }
    }
    elsif (substr($propertyDefaultValue, 0, 22) eq "Common::TimeSpan::Zero")
    {
        $propertyDefaultValue = "0";
    }
    elsif (substr($propertyDefaultValue, 0, 26) eq "Common::TimeSpan::MaxValue")
    {
        $propertyDefaultValue = "TimeSpan.MaxValue";
    }
	elsif (substr($propertyDefaultValue, 0, 30) eq "FabricVersionInstance::Default")
    {
        $propertyDefaultValue = "string.Empty";
    }
    return $propertyDefaultValue;
}

sub FixupParameterAttribute
{
    my $propertyType = shift;
    my $propertyName = shift;

    my $testParameterAttributeText = "[TestParameter]";

    if ($propertyType eq "TimeSpanInSeconds")
    {
        $testParameterAttributeText = "[TestParameter(Parser = typeof(TimeSpanInSecondsParser))]";
    }

    return $testParameterAttributeText;
}

sub BuildPropertyDefault
{
    my $propertyName = shift;
    my $propertyType = shift;
    my $propertyDefaultValue = shift;
    my $propertyUpgradePolicy = shift;

    $propertyType = FixupPropertyType($propertyType, "");
    $propertyDefaultValue = FixupPropertyValue($propertyDefaultValue);

    my $modifier = "static readonly";

    if ($propertyType eq "TimeSpan")
    {
        if ($propertyDefaultValue ne "TimeSpan.MaxValue")
        {
            $propertyDefaultValue = "TimeSpan.FromSeconds($propertyDefaultValue)"
        }
    }
    elsif ($propertyType eq "int64")
    {
        $propertyType = "Int64";
    }

    my $mainAccess = IsTypeCLSCompliant($propertyType) ? "        public" : "#if !DotNetCoreClr\n        [System.CLSCompliant(false)]\n #endif\n        public ";
    return
        "$mainAccess $modifier ConfigPropertyWrapper<$propertyType> Default$propertyName = new ConfigPropertyWrapper<$propertyType>($propertyDefaultValue, UpgradePolicy.$propertyUpgradePolicy);\n";
}

sub BuildPropertyAccessor
{
    my $propertyName = shift;
    my $propertyType = shift;

    my $setterText = "set";
    my $ctorText = "";
    my $copyCtorText = "            this.$propertyName = other.$propertyName;\n";
    $propertyType = FixupPropertyType($propertyType, $propertyName);

    if ($propertyType eq "TimeSpan")
    {
        $propertyType = "TimeSpanInSeconds";
    }
    elsif ($propertyType eq "int64")
    {
        $propertyType = "Int64";
    }
    elsif ($propertyType =~ /(IList)<([^>]+)>/)
    {
        $setterText = "private set";
        my $type = substr($1, 1);
        $ctorText = "            this.$propertyName = new $type<$2>();\n";
        $copyCtorText =
            "            foreach ($2 data in other.$propertyName)\n" .
            "            {\n" .
            "                this.$propertyName.Add(data);\n" .
            "            }\n\n";
    }
    elsif ($propertyType =~ /(IDictionary)<([^>]+)>/)
    {
        $setterText = "private set";
        my $type = substr($1, 1);
        $ctorText = "            this.$propertyName = new $type<$2>();\n";
        $copyCtorText =
            "            foreach (KeyValuePair<$2> data in other.$propertyName)\n" .
            "            {\n" .
            "                this.$propertyName.Add(data.Key, data.Value);\n" .
            "            }\n\n";
    }
    elsif ($propertyType =~ /Section/)
    {
        $setterText = "private set";
        $ctorText = "            this.$propertyName = new $propertyType();\n";
        $copyCtorText = "            this.$propertyName = new $propertyType(other.$propertyName);\n";
    }

    my $testParameterAttributeText = FixupParameterAttribute($propertyType, $propertyName);

    my $nullable = IsTypeNullable($propertyType) ? "?" : "";
    my $clsCompliantAttributeText = IsTypeCLSCompliant($propertyType) ? "" : "#if !DotNetCoreClr\n        [System.CLSCompliant(false)]\n#endif\n";
    my $accessorText =
          "$clsCompliantAttributeText" .
          "        $testParameterAttributeText\n" .
          "        public $propertyType$nullable $propertyName\n" .
          "        {\n" .
          "            get;\n" .
          "            $setterText;\n" .
          "        }\n\n";

    my @accessor = ($accessorText, $ctorText, $copyCtorText);
    return \@accessor;
}

sub BuildClass
{
    my $sectionName = shift;
    my $sectionPropertiesRef = shift;
    my @sectionProperties = @{$sectionPropertiesRef};

    # Do not generate classes for various sections...
    if (exists $ExcludedSections{$sectionName})
    {
        return "";
    }

    # Sub-Sections are not easily supported.
    # Just create an entirely new section (and class) for these right now...
    my $className = $sectionName;
    if (index($sectionName, '/') > 0)
    {
        # g implies replace all occurances. Multiple Sub-Sections will be supported
        $className =~ s/\///g;
    }

    my $defaultText = "";
    my $ctorText = "";
    my $copyCtorText = "";
    my $accessorText = "";

    # Generate the text for each of the class properties...
    for (@sectionProperties)
    {
        my @propertyDefinition = @{$_};

        # Some config properties are not picking up a type correctly...
        next if ($propertyDefinition[1] eq "");

        $defaultText .= BuildPropertyDefault($propertyDefinition[0], $propertyDefinition[1], $propertyDefinition[2], $propertyDefinition[3]);

        my @accessor = @{BuildPropertyAccessor($propertyDefinition[0], $propertyDefinition[1])};
        $accessorText .= $accessor[0];
        $ctorText .= $accessor[1];
        $copyCtorText .= $accessor[2];
    }

    # Finally build up the actual class definition by replacing all the placeholders...
    my $classText = $ConfigClassTemplate;
    $classText =~ s/__CLASS_NAME__/$className/g;
    $classText =~ s/__SECTION_NAME__/$sectionName/;
    $classText =~ s/__PROPERTY_DEFAULTS__/$defaultText/;
    $classText =~ s/__CTOR__/$ctorText/;
    $classText =~ s/__COPYCTOR__/$copyCtorText/;
    $classText =~ s/__PROPERTY_ACCESSORS__/$accessorText/;

    # Generate code for the top-level ClusterConfig class too...
    if (!exists $ClusterConfigExcludedSections{$sectionName})
    {
        my @accessor = @{BuildPropertyAccessor($className, $className . "Section")};
        $accessorText = $accessor[0];
        $ctorText = $accessor[1];
        $copyCtorText = $accessor[2];

        $ClusterConfigProperties .= $accessorText;
        $ClusterConfigCtor .= $ctorText;
        $ClusterConfigCopyCtor .= $copyCtorText;
    }

    return $classText;
}

sub ProcessData
{
    my $configDataRef = shift;
    my %configData = %{$configDataRef};

    my $generatedClasses = "";
    foreach my $key (sort keys %configData)
    {
        my @sectionProperties = @{$configData{$key}};

        $generatedClasses .= BuildClass($key, \@sectionProperties);
    }

    return $generatedClasses;
}

sub ProcessFile
{
    my $file = shift;
    my %csvData = ();

    my @csvLines = @{ReadAllLines($file)};
    for (@csvLines)
    {
        # [SectionName], [Setting Name], [Type], [DefaultValue]
        my @columns = split(/,/, $_);

        my $sectionName = $columns[0];
        if (substr($sectionName, 0, 1) ne "#")
        {
            my @propertyDefinition = ($columns[1], $columns[2], $columns[3], $columns[4]);
            push(@{$csvData{$sectionName}}, \@propertyDefinition);
        }
    }

    return %csvData;
}

sub BuildConfigSections
{
    # Generate a table where each entry is [SectionName, Array of Property definitions]...
    my %configData = ProcessFile($ProductConfigurationFile);

    # Build up all the generated classes...
    my $allConfigText = ProcessData(\%configData);

    # Put the generated classes within the context of the wrapper file...
    my $templateFile = ReadAllText($ConfigTemplateFile);
    $templateFile =~ s/__CONFIG_CONTENT__/$allConfigText/;
    $templateFile =~ s/__CLUSTER_CONFIG_CTOR__/$ClusterConfigCtor/;
    $templateFile =~ s/__CLUSTER_CONFIG_COPYCTOR__/$ClusterConfigCopyCtor/;
    $templateFile =~ s/__CLUSTER_CONFIG_PROPERTIES__/$ClusterConfigProperties/;

    WriteAllText($DestFile, $templateFile);

    return 1;
}

sub Main
{
    my $exitCode = 0;

    my $success = BuildConfigSections();
    if (!$success)
    {
        $exitCode = 1;
    }

    return $exitCode;
}

my $exitCode = Main();
exit $exitCode;
