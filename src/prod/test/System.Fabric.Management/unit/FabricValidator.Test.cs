// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Threading;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    using WEX.TestExecution;
    using Fabric.FabricDeployer;

    [TestClass]
    public class FabricValidatorTest
    {
        static readonly string CertificateThumbprint_Alice = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf";
        static readonly string CertificateThumbprint_Bob = "9d c9 06 b1 69 dc 4f af fd 16 97 ac 78 1e 80 67 90 74 9d 2f";

        private ImageBuilderTestContext imageBuilderTestContext;
        private IImageStore imageStore;

        TestContext testContextInstance;
        public TestContext TestContext
        {
            get { return testContextInstance; }
            set { testContextInstance = value; }
        }

        [TestInitialize]
        public void TestSetup()
        {
            this.imageBuilderTestContext = ImageBuilderTestContext.Create(this.TestContext.TestName);
            this.imageStore = imageBuilderTestContext.ImageStore;
        }

        [TestCleanup]
        public virtual void TestCleanup()
        {
            this.imageBuilderTestContext.TestCleanup();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidTimeSpanUsageInConfigCsv()
        {
            Dictionary<string, Dictionary<string, ConfigDescription>> configurations = new Dictionary<string, Dictionary<string, ConfigDescription>>(StringComparer.OrdinalIgnoreCase);
            try
            {
                foreach (string line in File.ReadAllLines(TestUtility.ConfigurationCsvFilePath))
                {
                    if (line.StartsWith("#", StringComparison.Ordinal))
                    {
                        continue;
                    }

                    string[] configurationInfo = line.Split(',');
                    ConfigDescription configDesc = new ConfigDescription()
                        {
                            SectionName = configurationInfo[0].Trim(),
                            SettingName = configurationInfo[1].Trim(),
                            Type = configurationInfo[2].Trim(),
                            DefaultValue = configurationInfo[3].Trim(),
                            UpgradePolicy = (UpgradePolicy)Enum.Parse(typeof(UpgradePolicy), configurationInfo[4].Trim())
                        };

                    if (!configurations.ContainsKey(configDesc.SectionName))
                    {
                        configurations.Add(configDesc.SectionName, new Dictionary<string, ConfigDescription>(StringComparer.OrdinalIgnoreCase));
                    }

                    if (configurations[configDesc.SectionName].ContainsKey(configDesc.SettingName))
                    {
                        FabricValidator.TraceSource.WriteError(
                            FabricValidatorUtility.TraceTag,
                            "Duplicate setting name '{0}' in section '{1}': {2}",
                            configDesc.SettingName,
                            configDesc.SectionName,
                            TestUtility.ConfigurationCsvFilePath);
                        Verify.Fail("Failed to read config file");
                    }

                    configurations[configDesc.SectionName].Add(
                        configDesc.SettingName,
                        configDesc);
                }
            }
            catch (IOException ie)
            {
                FabricValidator.TraceSource.WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Unable to read the configurations file because {0}",
                    ie);
                Verify.Fail("Failed to read config file");
            }
            
            foreach (KeyValuePair<string, Dictionary<string, ConfigDescription>> element in configurations)
            {
                foreach (KeyValuePair<string, ConfigDescription> parameter in element.Value)
                {
                    if (parameter.Value.Type.Equals("TimeSpan", StringComparison.InvariantCultureIgnoreCase) ||
                        parameter.Value.Type.Equals("Common::TimeSpan", StringComparison.InvariantCultureIgnoreCase))
                    {
                        if (!parameter.Value.DefaultValue.Contains(FabricValidatorConstants.DefaultValues.CommonTimeSpanFromMilliseconds) &&
                            !parameter.Value.DefaultValue.Contains(FabricValidatorConstants.DefaultValues.CommonTimeSpanFromHours) && 
                            !parameter.Value.DefaultValue.Contains(FabricValidatorConstants.DefaultValues.CommonTimeSpanFromMinutes) &&    
                            !parameter.Value.DefaultValue.Contains(FabricValidatorConstants.DefaultValues.CommonTimeSpanFromSeconds) &&    
                            !parameter.Value.DefaultValue.Contains(FabricValidatorConstants.DefaultValues.CommonTimeSpanFromTicks) &&    
                            !parameter.Value.DefaultValue.Contains(FabricValidatorConstants.DefaultValues.TimeSpanFromMilliseconds) &&
                            !parameter.Value.DefaultValue.Contains(FabricValidatorConstants.DefaultValues.TimeSpanFromHours) && 
                            !parameter.Value.DefaultValue.Contains(FabricValidatorConstants.DefaultValues.TimeSpanFromMinutes) &&    
                            !parameter.Value.DefaultValue.Contains(FabricValidatorConstants.DefaultValues.TimeSpanFromSeconds) &&    
                            !parameter.Value.DefaultValue.Contains(FabricValidatorConstants.DefaultValues.TimeSpanFromTicks) &&
                            !parameter.Value.DefaultValue.Equals(FabricValidatorConstants.DefaultValues.TimeSpanZero, StringComparison.OrdinalIgnoreCase) &&
                            !parameter.Value.DefaultValue.Equals(FabricValidatorConstants.DefaultValues.CommonTimeSpanZero, StringComparison.OrdinalIgnoreCase) &&
                            !parameter.Value.DefaultValue.Equals(FabricValidatorConstants.DefaultValues.TimeSpanMinValue, StringComparison.OrdinalIgnoreCase) &&
                            !parameter.Value.DefaultValue.Equals(FabricValidatorConstants.DefaultValues.CommonTimeSpanMinValue, StringComparison.OrdinalIgnoreCase) &&
                            !parameter.Value.DefaultValue.Equals(FabricValidatorConstants.DefaultValues.TimeSpanMaxValue, StringComparison.OrdinalIgnoreCase) &&
                            !parameter.Value.DefaultValue.Equals(FabricValidatorConstants.DefaultValues.CommonTimeSpanMaxValue, StringComparison.OrdinalIgnoreCase))
                        {
                            Verify.Fail("Invalid format of default value for timespan is being used" + parameter.Value.DefaultValue);
                        }
                        var param = parameter.Value;

                        WindowsFabricSettings.EvaluateExpressionTimeSpan(param);
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestCaseInsensitiveValidation()
        {
            ClusterManifestType clusterManifest = (new ClusterManifestHelper(true, false)).ClusterManifest;
            var fmSection = clusterManifest.FabricSettings.FirstOrDefault(section => section.Name == "FailoverManager");
            fmSection.Name = "failovermanager";
            VerifyValidate(clusterManifest, null, "Validate case insensitive parameter and section name match", true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestScaleMinIPAddressIsSameWithMachineValidation()
        {
            var clusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            clusterManifest.Infrastructure = new ClusterManifestTypeInfrastructure();
            ClusterManifestTypeInfrastructureWindowsServer infra = new ClusterManifestTypeInfrastructureWindowsServer();
            infra.IsScaleMin = true;
            clusterManifest.Infrastructure.Item = infra;
            InfrastructureNodeType infrastructureInformationNode = new InfrastructureNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "157.23.109.39",
                IsSeedNode = true
            };
            List<InfrastructureNodeType> infrastructureInformation = new List<InfrastructureNodeType>();
            infrastructureInformation.Add(infrastructureInformationNode);
            VerifyValidate(clusterManifest, infrastructureInformation, "Validate in scale min, if the first node in the infrastructure section has the same IP address as the machine", false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPaaSInfrastructureWithoutMatchingNodeType()
        {
            var clusterManifest = new ClusterManifestHelper(ClusterManifestHelper.InfrastructureType.PaaS).ClusterManifest;
            ClusterManifestTypeInfrastructurePaaS infra = new ClusterManifestTypeInfrastructurePaaS();
            
            InfrastructureNodeType infrastructureInformationNode = new InfrastructureNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "_NodeType2_0",
                NodeTypeRef = "NodeType2",
                IPAddressOrFQDN = "10.0.0.10",
                IsSeedNode = false
            };

            List<InfrastructureNodeType> infrastructureInformation = new List<InfrastructureNodeType>
            {
                infrastructureInformationNode
            };

            VerifyValidate(clusterManifest, infrastructureInformation, "Validate cluster manifest with no matching NodeType in InfrastructureManifest", true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateExpectedClusterSizeWindowsServer()
        {
            //Generate one Cluster Manifest
            ClusterManifestHelper manifestHelper = new ClusterManifestHelper(true, false);
            ClusterManifestType clusterManifest = manifestHelper.ClusterManifest;
            clusterManifest.Infrastructure = new ClusterManifestTypeInfrastructure();
            ClusterManifestTypeInfrastructureWindowsServer newInfra = new ClusterManifestTypeInfrastructureWindowsServer();
            newInfra.NodeList = new FabricNodeType[6];
            newInfra.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            newInfra.NodeList[1] = newInfra.NodeList[0];
            newInfra.NodeList[2] = newInfra.NodeList[0];
            newInfra.NodeList[3] = newInfra.NodeList[0];
            newInfra.NodeList[4] = newInfra.NodeList[0];
            newInfra.NodeList[5] = newInfra.NodeList[0];
            clusterManifest.Infrastructure = new ClusterManifestTypeInfrastructure();
            clusterManifest.Infrastructure.Item = newInfra;
            clusterManifest.NodeTypes = new ClusterManifestTypeNodeType[1];
            clusterManifest.NodeTypes[0] = new ClusterManifestTypeNodeType();
            clusterManifest.NodeTypes[0].Name = "NodeType1";
            clusterManifest.FabricSettings = new SettingsOverridesTypeSection[3];
            List<SettingsOverridesTypeSectionParameter> parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ClusterCredentialType, Value = "None", IsEncrypted = false });
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ServerAuthCredentialType, Value = "None", IsEncrypted = false });
            clusterManifest.FabricSettings[0] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.Security, Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ExpectedClustersize", Value = "100", IsEncrypted = false });          //ExpectedClusterSize
            clusterManifest.FabricSettings[1] = new SettingsOverridesTypeSection() { Name = "Failovermanager", Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ImageStoreConnectionString", Value = "_default_", IsEncrypted = false });
            clusterManifest.FabricSettings[2] = new SettingsOverridesTypeSection() { Name = "Management", Parameter = parameters.ToArray() };
            VerifyValidate(
                clusterManifest,
                null,
                "Validate if ExpectedClusterSize is greater than 0 and smaller or equal to total node count.",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateExpectedClusterSizeAzure()
        {
            //Generate one Cluster Manifest
            ClusterManifestType clusterManifest = new ClusterManifestType();
            clusterManifest.Version = "1.0";
            clusterManifest.Infrastructure = new ClusterManifestTypeInfrastructure();
            ClusterManifestTypeInfrastructureWindowsAzure newInfra = new ClusterManifestTypeInfrastructureWindowsAzure();
            newInfra.Roles = new AzureRoleType[1];
            newInfra.Roles[0] = new AzureRoleType();
            newInfra.Roles[0].RoleName = "WindowsFabricRole";
            newInfra.Roles[0].NodeTypeRef = "NodeType1";
            newInfra.Roles[0].SeedNodeCount = 5;
            clusterManifest.Infrastructure = new ClusterManifestTypeInfrastructure();
            clusterManifest.Infrastructure.Item = newInfra;
            clusterManifest.NodeTypes = new ClusterManifestTypeNodeType[1];
            clusterManifest.NodeTypes[0] = new ClusterManifestTypeNodeType();
            clusterManifest.NodeTypes[0].Name = "NodeType1";
            clusterManifest.FabricSettings = new SettingsOverridesTypeSection[3];
            List<SettingsOverridesTypeSectionParameter> parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ClusterCredentialType, Value = "None", IsEncrypted = false });
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ServerAuthCredentialType, Value = "None", IsEncrypted = false });
            clusterManifest.FabricSettings[0] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.Security, Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ExpectedClustersize", Value = "100", IsEncrypted = false });          //ExpectedClusterSize
            clusterManifest.FabricSettings[1] = new SettingsOverridesTypeSection() { Name = "Failovermanager", Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ImageStoreConnectionString", Value = "_default_", IsEncrypted = false });
            clusterManifest.FabricSettings[2] = new SettingsOverridesTypeSection() { Name = "Management", Parameter = parameters.ToArray() };
            List<InfrastructureNodeType> infrastructureInformation = new List<InfrastructureNodeType>();
            InfrastructureNodeType infrastructureInformationNode1 = new InfrastructureNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "157.23.109.39",
                IsSeedNode = true
            };
            InfrastructureNodeType infrastructureInformationNode2 = new InfrastructureNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node2",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "157.23.109.39",
                IsSeedNode = true
            };
            InfrastructureNodeType infrastructureInformationNode3 = new InfrastructureNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node3",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "157.23.109.39",
                IsSeedNode = true
            };
            infrastructureInformation.Add(infrastructureInformationNode1);
            infrastructureInformation.Add(infrastructureInformationNode2);
            infrastructureInformation.Add(infrastructureInformationNode3);
            VerifyValidate(
                clusterManifest,
                infrastructureInformation,
                "Validate if ExpectedClusterSize is greater than 0 and smaller or equal to total node count.",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateExpectedClusterSizeAzureStaticTopology()
        {
            //Generate one Cluster Manifest
            ClusterManifestType clusterManifest = new ClusterManifestType();
            clusterManifest.Version = "1.0";
            clusterManifest.Infrastructure = new ClusterManifestTypeInfrastructure();
            ClusterManifestTypeInfrastructureWindowsAzureStaticTopology newInfra = new ClusterManifestTypeInfrastructureWindowsAzureStaticTopology();
            newInfra.NodeList = new FabricNodeType[6];
            newInfra.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            newInfra.NodeList[1] = newInfra.NodeList[0];
            newInfra.NodeList[2] = newInfra.NodeList[0];
            newInfra.NodeList[3] = newInfra.NodeList[0];
            newInfra.NodeList[4] = newInfra.NodeList[0];
            newInfra.NodeList[5] = newInfra.NodeList[0];
            clusterManifest.Infrastructure.Item = newInfra;
            clusterManifest.NodeTypes = new ClusterManifestTypeNodeType[1];
            clusterManifest.NodeTypes[0] = new ClusterManifestTypeNodeType();
            clusterManifest.NodeTypes[0].Name = "NodeType1";
            clusterManifest.FabricSettings = new SettingsOverridesTypeSection[3];
            List<SettingsOverridesTypeSectionParameter> parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ClusterCredentialType, Value = "None", IsEncrypted = false });
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ServerAuthCredentialType, Value = "None", IsEncrypted = false });
            clusterManifest.FabricSettings[0] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.Security, Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ExpectedClustersize", Value = "100", IsEncrypted = false });
            clusterManifest.FabricSettings[1] = new SettingsOverridesTypeSection() { Name = "Failovermanager", Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ImageStoreConnectionString", Value = "_default_", IsEncrypted = false });
            clusterManifest.FabricSettings[2] = new SettingsOverridesTypeSection() { Name = "Management", Parameter = parameters.ToArray() };
            VerifyValidate(
                clusterManifest,
                null,
                "Validate if ExpectedClusterSize is greater than 0 and smaller or equal to total node count.",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestKtlLoggerNodeTypeSettings()
        {

            //
            // Test 1: Test with valid ShareLogFilePath and SharedLogFileId and expect success
            //
            {
                var clusterManifest1 = (new ClusterManifestHelper(true, false)).ClusterManifest;
                clusterManifest1.NodeTypes[0].KtlLoggerSettings = new FabricKtlLoggerSettingsType();
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFilePath = new FabricKtlLoggerSettingsTypeSharedLogFilePath();
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileId = new FabricKtlLoggerSettingsTypeSharedLogFileId();

                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFilePath.Value = "f:\\fred.log";
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileId.Value = "{A4562DAB-C898-4a12-AD17-E71268BE6388}";
                VerifyValidate(
                    clusterManifest1,
                    null,
                    "Test with valid ShareLogFilePath and SharedLogFileId and expect success.",
                    true);
            }

            //
            // Test 2: Test with valid SharedLogSizeInMB and expect success
            //
            {
                var clusterManifest1 = (new ClusterManifestHelper(true, false)).ClusterManifest;
                clusterManifest1.NodeTypes[0].KtlLoggerSettings = new FabricKtlLoggerSettingsType();
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileSizeInMB = new FabricKtlLoggerSettingsTypeSharedLogFileSizeInMB();

                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileSizeInMB.Value = 2048;
                VerifyValidate(
                    clusterManifest1,
                    null,
                    "Test with valid SharedLogSizeInMB and expect success.",
                    true);
            }

            //
            // Test 3: Test with invalid SharedLogFilePath and expect failure
            //
            {
                var clusterManifest1 = (new ClusterManifestHelper(true, false)).ClusterManifest;
                clusterManifest1.NodeTypes[0].KtlLoggerSettings = new FabricKtlLoggerSettingsType();
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFilePath = new FabricKtlLoggerSettingsTypeSharedLogFilePath();
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileId = new FabricKtlLoggerSettingsTypeSharedLogFileId();

                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFilePath.Value = "%%$#rrd^^";
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileId.Value = "{A4562DAB-C898-4a12-AD17-E71268BE6388}";
                VerifyValidate(
                    clusterManifest1,
                    null,
                    "Test with invalid SharedLogFilePath and expect failure.",
                    false);
            }

            //
            // Test 4: Test with invalid SharedLogFileId and expect failure
            //
            {
                var clusterManifest1 = (new ClusterManifestHelper(true, false)).ClusterManifest;
                clusterManifest1.NodeTypes[0].KtlLoggerSettings = new FabricKtlLoggerSettingsType();
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFilePath = new FabricKtlLoggerSettingsTypeSharedLogFilePath();
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileId = new FabricKtlLoggerSettingsTypeSharedLogFileId();

                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFilePath.Value = "f:\\fred.log";
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileId.Value = "***AD17-E71268BE6388}";
                VerifyValidate(
                    clusterManifest1,
                    null,
                    "Test with invalid SharedLogFileId and expect failure.",
                    false);
            }

            //
            // Test 4: Test with valid SharedLogFilePath and no SharedLogFileId and expect failure
            //
            {
                var clusterManifest1 = (new ClusterManifestHelper(true, false)).ClusterManifest;
                clusterManifest1.NodeTypes[0].KtlLoggerSettings = new FabricKtlLoggerSettingsType();
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFilePath = new FabricKtlLoggerSettingsTypeSharedLogFilePath();

                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFilePath.Value = "f:\\fred.log";
                VerifyValidate(
                    clusterManifest1,
                    null,
                    "Test with valid SharedLogFilePath and no SharedLogFileId and expect failure.",
                    false);
            }

            //
            // Test 5: Test with valid SharedLogFileId and no SharedLogFilePath and expect failure
            //
            {
                var clusterManifest1 = (new ClusterManifestHelper(true, false)).ClusterManifest;
                clusterManifest1.NodeTypes[0].KtlLoggerSettings = new FabricKtlLoggerSettingsType();
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileId = new FabricKtlLoggerSettingsTypeSharedLogFileId();

                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileId.Value = "{A4562DAB-C898-4a12-AD17-E71268BE6388}";
                VerifyValidate(
                    clusterManifest1,
                    null,
                    "Test with valid SharedLogFileId and no SharedLogFilePath and expect failure.",
                    false);
            }

            //
            // Test 6: Test with invalid SharedLogSizeInMB and expect failure
            //
            {
                var clusterManifest1 = (new ClusterManifestHelper(true, false)).ClusterManifest;
                clusterManifest1.NodeTypes[0].KtlLoggerSettings = new FabricKtlLoggerSettingsType();
                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileSizeInMB = new FabricKtlLoggerSettingsTypeSharedLogFileSizeInMB();

                clusterManifest1.NodeTypes[0].KtlLoggerSettings.SharedLogFileSizeInMB.Value = 256;
                VerifyValidate(
                    clusterManifest1,
                    null,
                    "Test with invalid SharedLogSizeInMB and expect failure.",
                    false);
            }
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPortsValidationWindowsServer()
        {
            //
            //Test 1:Test the validation of StartPort and EndPort Number.
            //
            var clusterManifest1 = (new ClusterManifestHelper(true, false)).ClusterManifest;
            clusterManifest1.NodeTypes[0].Endpoints.ApplicationEndpoints.StartPort = -30000;
            clusterManifest1.NodeTypes[0].Endpoints.ApplicationEndpoints.EndPort = -31000;
            VerifyValidate(
                clusterManifest1,
                null,
                "Validate if StartPort and EndPort are both positive.",
                false);

            //
            //Test 2:Test the validation of port range
            //
            var clusterManifest2 = (new ClusterManifestHelper(true, false)).ClusterManifest;
            clusterManifest2.NodeTypes[0].Endpoints.ApplicationEndpoints.StartPort = 200000;
            clusterManifest2.NodeTypes[0].Endpoints.ApplicationEndpoints.EndPort = 10000;
            VerifyValidate(
                clusterManifest2,
                null,
                "Validate if port range is valid.",
                false);

            //
            //Test 3:Test the validation of clientConnectionEndpoint port
            //
            var clusterManifest3 = (new ClusterManifestHelper(true, false)).ClusterManifest;
            clusterManifest3.NodeTypes[0].Endpoints.ClientConnectionEndpoint.Port = "-200";
            VerifyValidate(
                clusterManifest3,
                null,
                "Validate if clientConnectionEndpoint port is positive.",
                false);

            //
            //Test 4:Test the validation of LeaseDriverEndpoint Port
            //
            var clusterManifest4 = (new ClusterManifestHelper(true, false)).ClusterManifest;
            clusterManifest4.NodeTypes[0].Endpoints.LeaseDriverEndpoint.Port = "test200";
            VerifyValidate(
                clusterManifest4,
                null,
                "Validate if leaseDriverEndpoint port is a sequence of digits.",
                false);

            //
            //Test 5:Test the validation of clusterConnectionEndpoint port
            //
            var clusterManifest5 = (new ClusterManifestHelper(true, false)).ClusterManifest;
            clusterManifest5.NodeTypes[0].Endpoints.ClientConnectionEndpoint.Port = "99999999999999999999999";
            VerifyValidate(
                clusterManifest5,
                null,
                "Validate if leaseDriverEndpoint port is INT32.",
                false);
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPortsValidationAzure()
        {
            ClusterManifestType clusterManifest = new ClusterManifestType();
            clusterManifest.Version = "1.0";
            clusterManifest.Infrastructure = new ClusterManifestTypeInfrastructure();
            ClusterManifestTypeInfrastructureWindowsAzure newInfra = new ClusterManifestTypeInfrastructureWindowsAzure();
            newInfra.Roles = new AzureRoleType[1];
            newInfra.Roles[0] = new AzureRoleType();
            newInfra.Roles[0].RoleName = "WindowsFabricRole";
            newInfra.Roles[0].NodeTypeRef = "NodeType1";
            newInfra.Roles[0].SeedNodeCount = 5;
            clusterManifest.Infrastructure = new ClusterManifestTypeInfrastructure();
            clusterManifest.Infrastructure.Item = newInfra;
            clusterManifest.NodeTypes = new ClusterManifestTypeNodeType[1];
            clusterManifest.NodeTypes[0] = new ClusterManifestTypeNodeType();
            clusterManifest.NodeTypes[0].Name = "NodeType1";
            clusterManifest.FabricSettings = new SettingsOverridesTypeSection[3];
            List<SettingsOverridesTypeSectionParameter> parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ClusterCredentialType, Value = "None", IsEncrypted = false });
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ServerAuthCredentialType, Value = "None", IsEncrypted = false });
            clusterManifest.FabricSettings[0] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.Security, Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ExpectedClustersize", Value = "1", IsEncrypted = false });          //ExpectedClusterSize
            clusterManifest.FabricSettings[1] = new SettingsOverridesTypeSection() { Name = "Failovermanager", Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ImageStoreConnectionString", Value = "_default_", IsEncrypted = false });
            clusterManifest.FabricSettings[2] = new SettingsOverridesTypeSection() { Name = "Management", Parameter = parameters.ToArray() };
            List<InfrastructureNodeType> infrastructureInformation = new List<InfrastructureNodeType>();
            InfrastructureNodeType infrastructureInformationNode1 = new InfrastructureNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "157.23.109.39",
                IsSeedNode = true,
                Endpoints = new FabricEndpointsType()
                {
                    ClientConnectionEndpoint = new InputEndpointType() { Port = "200" },
                    LeaseDriverEndpoint = new InternalEndpointType() { Port = "19001" },
                    ClusterConnectionEndpoint = new InternalEndpointType() { Port = "19002" },
                    ServiceConnectionEndpoint = new InternalEndpointType() { Port = "19003" },
                    HttpGatewayEndpoint = new InputEndpointType() { Port = "19004" },
                    ApplicationEndpoints = new FabricEndpointsTypeApplicationEndpoints() { StartPort = 30001, EndPort = 31000 },
                }
            };
            InfrastructureNodeType infrastructureInformationNode2 = new InfrastructureNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node2",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "157.23.109.39",
                IsSeedNode = true,
                Endpoints = new FabricEndpointsType()
                {
                    ClientConnectionEndpoint = new InputEndpointType() { Port = "200" },
                    LeaseDriverEndpoint = new InternalEndpointType() { Port = "19001" },
                    ClusterConnectionEndpoint = new InternalEndpointType() { Port = "19002" },
                    ServiceConnectionEndpoint = new InternalEndpointType() { Port = "19003" },
                    HttpGatewayEndpoint = new InputEndpointType() { Port = "19004" },
                    ApplicationEndpoints = new FabricEndpointsTypeApplicationEndpoints() { StartPort = 30001, EndPort = 31000 },
                }
            };
            InfrastructureNodeType infrastructureInformationNode3 = new InfrastructureNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node3",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "157.23.109.39",
                IsSeedNode = true,
                Endpoints = new FabricEndpointsType()
                {
                    ClientConnectionEndpoint = new InputEndpointType() { Port = "200" },
                    LeaseDriverEndpoint = new InternalEndpointType() { Port = "19001" },
                    ClusterConnectionEndpoint = new InternalEndpointType() { Port = "19002" },
                    ServiceConnectionEndpoint = new InternalEndpointType() { Port = "-19003" },
                    HttpGatewayEndpoint = new InputEndpointType() { Port = "19004" },
                    ApplicationEndpoints = new FabricEndpointsTypeApplicationEndpoints() { StartPort = 30001, EndPort = 31000 },
                }
            };
            infrastructureInformation.Add(infrastructureInformationNode1);
            infrastructureInformation.Add(infrastructureInformationNode2);
            infrastructureInformation.Add(infrastructureInformationNode3);
            VerifyValidate(
                clusterManifest,
                infrastructureInformation,
                "Validate endpoints in Azure deployment.",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidReplicatorSettings_SecondaryProgressRateDecayFactor()
        {
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SecondaryProgressRateDecayFactor,
                    "0.0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Invalid SecondaryProgressRateDecayFactor Test1",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SecondaryProgressRateDecayFactor,
                    "1.0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Invalid SecondaryProgressRateDecayFactor Test2",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SecondaryProgressRateDecayFactor,
                    "-0.1",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate valid SecondaryProgressRateDecayFactor Test3",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SecondaryProgressRateDecayFactor,
                    "1.1",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate valid SecondaryProgressRateDecayFactor Test4",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SecondaryProgressRateDecayFactor,
                    "0.5",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Invalid SecondaryProgressRateDecayFactor Test5",
                    true);
            }
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidReplicatorSettings_SlowActiveSecondaryRestartAtQueueUsagePercent()
        {
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SlowActiveSecondaryRestartAtQueueUsagePercent,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Invalid SlowActiveSecondaryRestartAtQueueUsagePercent Test1",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SlowActiveSecondaryRestartAtQueueUsagePercent,
                    "101",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Invalid SlowActiveSecondaryRestartAtQueueUsagePercent Test2",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SlowActiveSecondaryRestartAtQueueUsagePercent,
                    "100",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate valid SlowActiveSecondaryRestartAtQueueUsagePercent Test3",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SlowActiveSecondaryRestartAtQueueUsagePercent,
                    "1",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate valid SlowActiveSecondaryRestartAtQueueUsagePercent Test4",
                    true);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidReplicatorSettings_SlowIdleRestartAtQueueUsagePercent()
        {
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SlowIdleRestartAtQueueUsagePercent,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Invalid SlowIdleRestartAtQueueUsagePercent Test1",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SlowIdleRestartAtQueueUsagePercent,
                    "101",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Invalid SlowIdleRestartAtQueueUsagePercent Test2",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SlowIdleRestartAtQueueUsagePercent,
                    "100",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate valid SlowIdleRestartAtQueueUsagePercent Test3",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.SlowIdleRestartAtQueueUsagePercent,
                    "1",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate valid SlowIdleRestartAtQueueUsagePercent Test4",
                    true);
            }
        }
        

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidReplicatorSettings_QueueHealthWarningAtUsagePercent()
        {
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.QueueHealthWarningAtUsagePercent,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Invalid QueueHealthWarningAtUsagePercent Test1",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.QueueHealthWarningAtUsagePercent,
                    "101",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Invalid QueueHealthWarningAtUsagePercent Test2",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.QueueHealthWarningAtUsagePercent,
                    "100",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate valid QueueHealthWarningAtUsagePercent Test3",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Replication.Default,
                    FabricValidatorConstants.ParameterNames.Replicator.QueueHealthWarningAtUsagePercent,
                    "1",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate valid QueueHealthWarningAtUsagePercent Test4",
                    true);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidReplicatorSettings_BatchAckRetryInterval()
        {
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.BatchAckInterval,
                    "10.5",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.RetryInterval,
                    "5.4",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Invalid BatchACk and RetryInterval Test1",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.BatchAckInterval,
                    "5.4",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.RetryInterval,
                    "5.4",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Invalid BatchACk and RetryInterval Test2",
                    false);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_EnableIncrementalBackupsAcrossReplicas_ArgumentException()
        {
            // Note that validation will fail because
            // 1. Native will not have the configuration
            // 2. Managed will not be able to convert it to boolean.
            // As part of adding the native counter part, we will make both the same case.
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.EnableIncrementalBackupsAcrossReplicas,
                "-1",
                "Validate EnableIncrementalBackupsAcrossReplicas is invalid input",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_LessThanZero_ArgumentOutOfRangeException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB,
                "-1",
                "Validate MinLogSizeInMB is less than 0",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_IsZero_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB,
                "0",
                "Validate MinLogSizeInMB is less than 0",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_MuchLessThanCheckpointThresholdInMB_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "1",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB] = "51"
                },
                "Validate should fail pass.",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_LessThanCheckpointThresholdInMB_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "49",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB] = "50"
                },
                "Validate should fail pass.",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_EqualToCheckpointThresholdInMB_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "50",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB] = "50"
                },
                "Validate should fail pass.",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_GreaterThanCheckpointThresholdInMB_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "51",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB] = "50"
                },
                "Success: MinLogSizeInMB is greater than CheckpointThresholdInMB",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_IsLessThanMaxStreamSizeInMB_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "100",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "900"
                },
                "Validate should pass if MinLogSizeInMB is less than MaxStreamSizeInMB",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_IsLessThanMaxStreamSizeInMB_SparseFileOff_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "100",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage] = "false",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "900"
                },
                "Validate should pass if MinLogSizeInMB is less than MaxStreamSizeInMB",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_IsEqualToMaxStreamSizeInMB_SparseFileOff_ArgumentOutOfRangeException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "900",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage] = "false",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "900"
                },
                "Validate should fail if MinLogSizeInMB is equal to MaxStreamSizeInMB",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_IsGreaterThanMaxStreamSizeInMB_SparseFileOff_ArgumentOutOfRangeException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "901",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage] = "false",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "900"
                },
                "Validate should fail if MinLogSizeInMB is greater than MaxStreamSizeInMB",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_IsGreaterThanMaxStreamSizeInMB_SparseFileOn_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "901",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage] = "true",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "900"
                },
                "Validate should pass if MinLogSizeInMB is less than sparse default",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_TruncationThresholdFactor_IsLessThan1_ArgumentOutOfRangeException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
               FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.TruncationThresholdFactor,
               "0",
               "TruncationThresholdFactor > 0",
               false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_TruncationThresholdFactor_IsLessThanMaxStreamSizeInMB_SpareFileOff_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "100",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.TruncationThresholdFactor] = "5",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.ThrottlingThresholdFactor] = "6",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage] = "false",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "900",
                },
                "Should work",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_TruncationThresholdFactor_IsEqualToMaxStreamSizeInMB_ArgumentOutOfRangeException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "450",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.TruncationThresholdFactor] = "2",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage] = "false",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "900",
                },
                "Validate should fail if MinLogSizeInMB * TruncationThresholdFactor is equal to MaxStreamSizeInMB",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_TruncationThresholdFactor_IsGreaterThanMaxStreamSizeInMB_ArgumentOutOfRangeException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "500",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.TruncationThresholdFactor] = "2",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage] = "false",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "900",
                },
                "Validate should fail if MinLogSizeInMB * TruncationThresholdFactor is greater than MaxStreamSizeInMB",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_ThrottlingThresholdFactor_IsLessThan1_ArgumentOutOfRangeException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
               FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.ThrottlingThresholdFactor,
               "0",
               "ThrottlingThresholdFactor > 0",
               false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_ThrottlingThresholdFactor_TruncationHigherThanThrottle_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
               new Dictionary<string, string>
               {
                   [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "50",
                   [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.TruncationThresholdFactor] = "5",
                   [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.ThrottlingThresholdFactor] = "4",
               },
               "Failure",
               false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_ThrottlingThresholdFactor_EqualsTruncationThreshold_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
               new Dictionary<string, string>
               {
                   [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "50",
                   [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.TruncationThresholdFactor] = "4",
                   [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.ThrottlingThresholdFactor] = "4",
               },
               "Failure",
               false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_ThrottlingThresholdFactor_GreaterThanTruncationThresholdInMB_DueToCheckpoint_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB] = "50",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "1",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.TruncationThresholdFactor] = "4",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.ThrottlingThresholdFactor] = "4",
                },
                "Success",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_ThrottlingThresholdFactor_GreaterThanTruncationThresholdInMB_DueToMinLog_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB] = "1",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "50",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.TruncationThresholdFactor] = "4",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.ThrottlingThresholdFactor] = "5",
                },
                "Success",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_ThrottlingThresholdFactor_IsEqualToMaxStreamSizeInMB_SpaseFileOff_ArgumentOutOfRangeException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "200",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.ThrottlingThresholdFactor] = "5",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage] = "false",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "1000",
                },
                "Validate should fail if MinLogSizeInMB * ThrottlingThresholdFactor is equal to MaxStreamSizeInMB",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_ThrottlingThresholdFactor_IsGreaterThanMaxStreamSizeInMB_SparseFileOff_ArgumentOutOfRangeException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "201",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.ThrottlingThresholdFactor] = "5",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage] = "false",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "1000",
                },
                "Validate should fail if MinLogSizeInMB * ThrottlingThresholdFactor is greater than MaxStreamSizeInMB",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_DefaultCase_Success()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Success",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_MinCheckpointSizeInMB_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
               FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB,
               "1",
               "Success",
               true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_CananoicalCase_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB] = "100",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MinLogSizeInMB] = "200",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.TruncationThresholdFactor] = "3",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.ThrottlingThresholdFactor] = "4",
                },
                "Success",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_Upgrade_LargeCheckpointThresholdInMB_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB,
                "1000",
                "Success",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_Upgrade_SmallMaxStreamSize_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB] = "1",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxRecordSizeInKB] = "128",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "7",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxAccumulatedBackupLogSizeInMB] = "1",
                },
                "Fail",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_Upgrade_LargeMaxStreamSize_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB,
                "10000",
                "Success",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_Upgrade_SmallMaxStreamSize_SparseFileOff_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB] = "1",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxRecordSizeInKB] = "128",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage] = "false",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "7",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxAccumulatedBackupLogSizeInMB] = "1",
                },
                "Success",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TRS_MinLogSizeInMB_Upgrade_LargeMaxStreamSize_SpareFileOff_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage] = "false",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB] = "10000"
                },
                "Success",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_SlowApiMonitoringDuration_LessThan0_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.SlowApiMonitoringDuration,
                "-1",
                "SlowApiMonitoringDuration < 0",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_SlowApiMonitoringDuration_EqualTo0_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.SlowApiMonitoringDuration,
                "0",
                "SlowApiMonitoringDuration == 0",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_SlowApiMonitoringDuration_EqualToTimespanMax_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.SlowApiMonitoringDuration,
                "9223372036854775807",
                "SlowApiMonitoringDuration < TimeSpan.MaxValue",
                false);
        }

        /// <summary>
        /// Ensure an exception is thrown if the minimum delay interval is negative
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_Test_LogMinDelayIntervalMilliseconds_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogMinDelayIntervalMilliseconds,
                "-1",
                "Test_LogMinDelayIntervalMilliseconds < 0",
                false);
        }

        /// <summary>
        /// Ensure an exceptoin is thrown if the max delay interval is negative
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_Test_LogMaxDelayIntervalMilliseconds_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogMaxDelayIntervalMilliseconds,
                "-1",
                "Test_LogMaxDelayIntervalMilliseconds < 0",
                false);
        }

        /// <summary>
        /// Ensure exception is thrown if the minimum file log delay value is greater than the maximum
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_Test_LogMinDelay_GreaterThan_LogMaxDelay_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogMaxDelayIntervalMilliseconds] = "10",
                    [FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogMinDelayIntervalMilliseconds] = "20"
                },
                "Test_LogMaxDelayIntervalMilliseconds must be greater than Test_LogMinDelayIntervalMilliseconds",
                false);
        }

        /// <summary>
        /// Ensure an exception is thrown if a negative file log delay value is provided
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_Test_LogDelayRatio_Negative_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogDelayRatio,
                "-0.1",
                "Test_LogDelayRatio must be greater than zero and less than 1",
                false);
        }

        /// <summary>
        /// Ensure an exception is thrown if file log delay ratio is above 1
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_Test_LogDelayRatio_GreaterThan1_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogDelayRatio,
                "1.1",
                "Test_LogDelayRatio must be greater than zero and less than 1",
                false);
        }

        /// <summary>
        /// Ensure an exception is thrown if a negative file log delay value is provided
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_Test_LogDelayProcessExitRatio_Negative_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogDelayProcessExitRatio,
                "-0.1",
                "Test_LogDelayProcessExitRatio must be greater than zero and less than 1",
                false);
        }

        /// <summary>
        /// Ensure an exception is thrown if file log delay ratio is above 1
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_Test_LogDelayProcessExitRatio_GreaterThan1_AORException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LogDelayProcessExitRatio,
                "1.1",
                "Test_LogDelayProcessExitRatio must be greater than zero and less than 1",
                false);
        }

        /// <summary>
        /// Invalid LoggingEngine values should not be accepted
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_Test_LoggingEngine_InvalidValue_ArgException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.Test_LoggingEngine,
                "test",
                "Test_LoggingEngine must be one of ktl/memory/file",
                false);
        }

        /// <summary>
        /// Invalid SerializationVersion values should not be accepted
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("Zhol")]
        public void TRS_Test_SerializationVersion_LessThen0_ArgException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicator2.SerializationVersion,
                "-1",
                "SerializationVersion should only be either 0 or 1.",
                false,
                true);
        }

        /// <summary>
        /// Invalid SerializationVersion values should not be accepted
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("Zhol")]
        public void TRS_Test_SerializationVersion_LargerThen1_ArgException()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicator2.SerializationVersion,
                "2",
                "SerializationVersion should only be either 0 or 1.",
                false,
                true);
        }

        /// <summary>
        /// Valid SerializationVersion values should be accepted
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("Zhol")]
        public void TRS_Test_SerializationVersion_0_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicator2.SerializationVersion,
                "0",
                "SerializationVersion should only be either 0 or 1.",
                true,
                true);
        }

        /// <summary>
        /// Valid SerializationVersion values should be accepted
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("Zhol")]
        public void TRS_Test_SerializationVersion_1_Success()
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                FabricValidatorConstants.ParameterNames.TransactionalReplicator2.SerializationVersion,
                "1",
                "SerializationVersion should only be either 0 or 1.",
                true,
                true);
        }

        /// <summary>
        /// Valid LogTruncationInterval values > 0
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("tyadam")]
        public void TRS_Negative_LogTruncationIntervalSeconds()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.TransactionalReplicator,
                "LogTruncationIntervalSeconds",
                "-1",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "LogTruncationIntervalSeconds must be > 0",
                false);
        }

        private void VerifyValidate_CommonSetting_TransactionalReplicator(
            string parameterName,
            string parameterValue,
            string message,
            bool expectSuccess,
            bool verifyNativeOnly = false)
        {
            this.VerifyValidate_CommonSetting_TransactionalReplicator(
                new Dictionary<string, string>
                {
                    [parameterName] = parameterValue
                }, 
                message,
                expectSuccess,
                verifyNativeOnly);
        }

        // Validate setting in managed and native transactional replicator
        private void VerifyValidate_CommonSetting_TransactionalReplicator(
            Dictionary<string,string> parametersDictionary, 
            string message,
            bool expectSuccess,
            bool verifyNativeOnly = false)
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            if (!verifyNativeOnly)
            {
                // Confirm managed validation
                foreach (var parameter in parametersDictionary)
                {
                    var name = parameter.Key;
                    var value = parameter.Value;

                    TestUtility.AddToFabricSettings(
                        FabricValidatorConstants.SectionNames.TransactionalReplicator,
                        name,
                        value,
                        false /*isEncrypted*/,
                        fabricLayoutInfo.ClusterManifest);
                }

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    message,
                    expectSuccess);
            }

            // Confirm native validation
            var transactionalReplicator2Sections =
                typeof(FabricValidatorConstants.SectionNames.TransactionalReplicator2).GetFields(BindingFlags.Public |
                                                                                                  BindingFlags.Static);
            foreach (var section in transactionalReplicator2Sections)
            {
                fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
                var sectionName = section.GetValue(null).ToString();
                foreach (var parameter in parametersDictionary)
                {
                    var name = parameter.Key;
                    var value = parameter.Value;

                    TestUtility.AddToFabricSettings(
                        sectionName,
                        name,
                        value,
                        false /*isEncrypted*/,
                        fabricLayoutInfo.ClusterManifest);
                }

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    message,
                    expectSuccess);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidReplicatorSettings_ReplicationMessageSize()
        {
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxReplicationQueueMemorySize,
                    "20",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxReplicationMessageSize,
                    "1024",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Replication Message Size Settings",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxPrimaryReplicationQueueMemorySize,
                    "20",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxReplicationMessageSize,
                    "1024",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Replication Message Size Settings",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxSecondaryReplicationQueueMemorySize,
                    "20",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxReplicationMessageSize,
                    "1024",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Replication Message Size Settings",
                    false);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestTransactionalReplicatorSettings_LogSettings()
        {
            //
            // Test 1: Verify that MaxMetadataSizeInKB that is not a multiple of 4 fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxMetadataSizeInKB,
                    "33",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate MaxMetadataSizeInKB that is not multiple of 4 fails",
                    false);
            }

            //
            // Test 2: Verify that MaxMetadataSizeInKB that is a multiple of 4 succeeds
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxMetadataSizeInKB,
                    "64",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate MaxMetadataSizeInKB that is multiple of 4 succeeds",
                    true);
            }

            //
            // Test 3: Verify that MaxRecordSizeInKB that is not a multiple of 4 fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxRecordSizeInKB,
                    "33",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate MaxRecordSizeInKB that is not multiple of 4 fails",
                    false);
            }

            //
            // Test 4: Verify that MaxMetadataSizeInKB that is a multiple of 4 succeeds
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxRecordSizeInKB,
                    "128",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate MaxRecordSizeInKB that is multiple of 4 succeeds",
                    true);
            }

            //
            // Test 5: Verify that a MaxRecordSizeInKB less than 128 fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxRecordSizeInKB,
                    "10",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate MaxRecordSizeInKB that is less than 128 fails",
                    false);
            }

            //
            // Test 6: Verify that a MaxRecordSizeInKB 128 or greater succeeds
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxRecordSizeInKB,
                    "256",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate MaxRecordSizeInKB that is 128 or larger succeeds",
                    true);
            }

            //
            // Test 7: Verify MaxStreamSizeInMB * 1024 is smaller than MaxRecordSizeInKB * 16 fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage,
                    "false",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB,
                    "2",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxRecordSizeInKB,
                    "2048",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxAccumulatedBackupLogSizeInMB,
                    "1",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate MaxStreamSizeInMB * 1024 is less than MaxRecordSizeInKB * 16 fails",
                    false);
            }

            //
            // Test 8: Verify MaxStreamSizeInMB * 1024 is greater or equal to MaxRecordSizeInKB * 16 succeeds
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB,
                    "128",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxRecordSizeInKB,
                    "2048",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxAccumulatedBackupLogSizeInMB,
                    "1",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate MaxStreamSizeInMB * 1024 is greater or equal to MaxRecordSizeInKB * 16 succeeds",
                    true);
            }

            //
            // Test 9: Verify SharedLogId and SharedLogPath are both not set
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogId,
                    "",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogPath,
                    "",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate both SharedLogId and SharedLogPath are not set succeeds",
                    true);
            }

            //
            // Test 10: Verify SharedLogId and SharedLogPath are both set
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogId,
                    "{F67D29E7-DFAB-437e-9F9F-5AC901D3F0BC}",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogPath,
                    "C:\\Temp\\ReplicatorShared.Log",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate both SharedLogId and SharedLogPath are set succeeds",
                    true);
            }

            //
            // Test 11: Verify SharedLogId is set and SharedLogPath not set fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogId,
                    "{F67D29E7-DFAB-437e-9F9F-5AC901D3F0BC}",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogPath,
                    "",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate both SharedLogId and SharedLogPath are set fails",
                    false);
            }

            //
            // Test 12: Verify SharedLogId is not set and SharedLogPath is set fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogId,
                    "",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogPath,
                    "C:\\Temp\\ReplicatorShared.Log",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogId is not set and SharedLogPath is not set fails",
                    false);
            }

            //
            // Test 13: Verify if SharedLogId is not a well-formed guid that it fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogId,
                    "Fred",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogId is not a well-formed guid fails",
                    false);
            }

            //
            // Test 14: Verify SharedLogPath that is not a valid absolute path fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogPath,
                    "fred\\joe\\ReplicatorShared.Log",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogPath is not an absolute path and fails",
                    false);
            }

            //
            // Test 15: Verify SharedLogId is not the default well-known guid
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicator.SharedLogId,
                    "{3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62}",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogId is not the default {3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62} and fails",
                    false);
            }

            //
            // Test 16: Verify MaxStreamSizeInMB is smaller than CheckpointThresholdInMB fails if MaxStreamSizeInMB is > 0 when optimize for low disk usage is disabled
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB,
                    "1025",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);
                
                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.OptimizeLogForLowerDiskUsage,
                    "false",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate MaxStreamSizeInMB is less than CheckpointThresholdInMB fails if OptimizeLogForLowerDiskUsage is disabled",
                    false);
            }

            //
            // Test 17: Verify MaxStreamSizeInMB is smaller than CheckpointThresholdInMB passes if MaxStreamSizeInMB is > 0 when optimize for low disk usage is enabled
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB,
                    "2",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.CheckpointThresholdInMB,
                    "3",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxAccumulatedBackupLogSizeInMB,
                    "1",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate MaxStreamSizeInMB is less than CheckpointThresholdInMB passes if OptimizeLogForLowerDiskUsage is enabled by default",
                    true);
            }

            //
            // Test 18: Fail if MaxStreamSizeInMB == 0
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxAccumulatedBackupLogSizeInMB,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Fail if MaxStreamSizeInMB == 0",
                    false);
            }

            //
            // Test 19: Fail if MaxStreamSizeInMB < MaxAccumulatedBackupLogSizeInMB
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB,
                    "2",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxAccumulatedBackupLogSizeInMB,
                    "3",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Fail if MaxStreamSizeInMB < MaxAccumulatedBackupLogSizeInMB",
                    false);
            }

            //
            // Test 20: Fail if MaxStreamSizeInMB == MaxAccumulatedBackupLogSizeInMB
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB,
                    "2",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxAccumulatedBackupLogSizeInMB,
                    "2",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Fail if MaxStreamSizeInMB == MaxAccumulatedBackupLogSizeInMB",
                    false);
            }

            //
            // Test 21: Pass if MaxStreamSizeInMB > MaxAccumulatedBackupLogSizeInMB
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxStreamSizeInMB,
                    "4",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.TransactionalReplicator,
                    FabricValidatorConstants.ParameterNames.TransactionalReplicatorCommon.MaxAccumulatedBackupLogSizeInMB,
                    "2",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Pass if MaxStreamSizeInMB > MaxAccumulatedBackupLogSizeInMB",
                    true);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestKtlLoggerSettings()
        {
            //
            // Test 1: PeriodicFlushTime must be between 00:00:15 and 00:05:00
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicFlushTime,
                    "10",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicFlushTime that is less than 00:00:15 fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicFlushTime,
                    "310",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicFlushTime that is more than 00:05:00 fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicFlushTime,
                    "61",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicFlushTime that is between 00:00:15 and 00:05:00 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicFlushTime,
                    "15",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicFlushTime that is 00:00:15 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicFlushTime,
                    "300",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicFlushTime that is 00:05:00 succeeds",
                    true);
            }

            //
            // Test 2: PeriodicFlushInterval must be between 00:00:01 and 00:01:00
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicTimerInterval,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicTimerInterval that is less than 00:00:01 fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicTimerInterval,
                    "70",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicTimerInterval that is more than 00:01:00 fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicTimerInterval,
                    "10",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicTimerInterval that is between 00:00:01 and 00:01:00 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicTimerInterval,
                    "1",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicTimerInterval that is 00:00:01 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicTimerInterval,
                    "60",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicTimerInterval that is 00:01:00 succeeds",
                    true);
            }

            //
            // Test 3: Verify that PeriodicTimerInterval is less than PeriodicFlushTime
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicFlushTime,
                    "40",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicTimerInterval,
                    "50",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicTimerInterval that is more than PeriodicFlushTime fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicFlushTime,
                    "60",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PeriodicTimerInterval,
                    "5",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PeriodicTimerInterval that is less than PeriodicFlushTime succeeds",
                    true);
            }

            //
            // Test 4: AllocationTimeout must be between 00:00:00 and 01:00:00
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.AllocationTimeout,
                    "3670",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate AllocationTimeout that is more than 01:00:00 fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.AllocationTimeout,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate AllocationTimeout that is 00:00:00 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.AllocationTimeout,
                    "1",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate AllocationTimeout that is  00:00:01 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.AllocationTimeout,
                    "3600",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate AllocationTimeout that is  01:00:00 succeeds",
                    true);
            }

            //
            // Test 5: PinnedMemoryLimitInKB must be zero
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PinnedMemoryLimitInKB,
                    "22",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PinnedMemoryLimitInKB that is not zero fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.PinnedMemoryLimitInKB,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate PinnedMemoryLimitInKB that is zero succeeds",
                    true);
            }

            //
            // Test 6: WriteBufferMemoryPoolMinimumInKB is zero or greater or equal to 16MB (16384)
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMinimumInKB,
                    "1000",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolMinimumInKB that is less than 16383 fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMinimumInKB,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolMinimumInKB that is zero succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMinimumInKB,
                    "17384",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolMinimumInKB that greater than 16384 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMinimumInKB,
                    "16384",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolMinimumInKB that is 16384 succeeds",
                    true);
            }

            //
            // Test 7: WriteBufferMemoryPoolPerStreamInKB must be -1, 0 or greater than 1024
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolPerStreamInKB,
                    "22",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolPerStreamInKB that is less than 1024 fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolPerStreamInKB,
                    "-1",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolPerStreamInKB that is -1 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolPerStreamInKB,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolPerStreamInKB that is zero succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolPerStreamInKB,
                    "1024",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolPerStreamInKB that is 1024 succeeds",
                    true);
            }

            //
            // Test 8: If WriteBufferMemoryPoolMinimumInKB is 0 then WriteBufferMemoryPoolMaximumInKB must be 0
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMinimumInKB,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMaximumInKB,
                    "32787",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolMinimumInKB is 0 and WriteBufferMemoryPoolMaximumInKB is not 0 fails",
                    false);
            }
                        
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMinimumInKB,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMaximumInKB,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolMaximumInKB is 0 and WriteBufferMemoryPoolMinimumInKB is 0 succeeds",
                    true);
            }
            
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMinimumInKB,
                    "32768",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMaximumInKB,
                    "32768",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolMaximumInKB is 32768 and WriteBufferMemoryPoolMinimumInKB is 32768 succeeds",
                    true);
            }
            
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMinimumInKB,
                    "32768",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMaximumInKB,
                    "65536",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolMaximumInKB is 65536 and WriteBufferMemoryPoolMinimumInKB is 32768 succeeds",
                    true);
            }
            
            //
            // Test 9: WriteBufferMemoryPoolMaximumInKB must be less than WriteBufferMemoryPoolMinimumInKB if both are not 0
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMinimumInKB,
                    "65536",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMaximumInKB,
                    "32768",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate WriteBufferMemoryPoolMinimumInKB is 65536 and WriteBufferMemoryPoolMaximumInKB is 32768 fails",
                    false);
            }

            //
            // Test 10: Verify SharedLogId and SharedLogPath are both not set
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogId,
                    "",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogPath,
                    "",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate both SharedLogId and SharedLogPath are not set succeeds",
                    true);
            }

            //
            // Test 11: Verify SharedLogId and SharedLogPath are both set
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogId,
                    "{F67D29E7-DFAB-437e-9F9F-5AC901D3F0BC}",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogPath,
                    "C:\\Temp\\ReplicatorShared.Log",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate both SharedLogId and SharedLogPath are set succeeds",
                    true);
            }

            //
            // Test 12: Verify SharedLogId is set and SharedLogPath not set fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogId,
                    "{F67D29E7-DFAB-437e-9F9F-5AC901D3F0BC}",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogPath,
                    "",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate both SharedLogId and SharedLogPath are set fails",
                    false);
            }

            //
            // Test 13: Verify SharedLogId is not set and SharedLogPath is set fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogId,
                    "",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogPath,
                    "C:\\Temp\\ReplicatorShared.Log",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogId is not set and SharedLogPath is not set fails",
                    false);
            }

            //
            // Test 14: Verify if SharedLogId is not a well-formed guid that it fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogId,
                    "Fred",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogId is not a well-formed guid fails",
                    false);
            }

            //
            // Test 15: Verify SharedLogPath that is not a valid absolute path fails
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogPath,
                    "fred\\joe\\ReplicatorShared.Log",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogPath is not an absolute path and fails",
                    false);
            }

            //
            // Test 16: Verify SharedLogId is not the default well-known guid
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogId,
                    "{3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62}",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogId is not the default {3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62} and fails",
                    false);
            }

            //
            // Test 17: Test SharedLogCreateFlags is zero
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogCreateFlags,
                    "22",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogCreateFlags that is not zero fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogCreateFlags,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogCreateFlags that is zero succeeds",
                    true);
            }

            //
            // Test 18: SharedLogNumberStreams must be between 3*512 and 3*8192
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogNumberStreams,
                    "32768",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogNumberStreams that is more than 3*8192 fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogNumberStreams,
                    "256",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogNumberStreams that is less than 3*512 fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogNumberStreams,
                    "24576",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogNumberStreams that is 3*8192 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogNumberStreams,
                    "1536",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogNumberStreams that is 3*512 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogNumberStreams,
                    "4096",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogNumberStreams that is between 3*512 and 3*8192 succeeds",
                    true);
            }

            //
            // Test 19: Test SharedLogMaximumRecordSizeInKB is zero
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogMaximumRecordSizeInKB,
                    "22",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogMaximumRecordSizeInKB that is not zero fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogMaximumRecordSizeInKB,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogMaximumRecordSizeInKB that is zero succeeds",
                    true);
            }

            //
            // Test 20: SharedLogSizeInMB must be greater than 512
            //
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogSizeInMB,
                    "256",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogSizeInMB that is less than 512 fails",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogSizeInMB,
                    "512",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogNumberStreams that is 512 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogSizeInMB,
                    "8192",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogSizeInMB that is 8192 succeeds",
                    true);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.KtlLogger,
                    FabricValidatorConstants.ParameterNames.KtlLogger.SharedLogSizeInMB,
                    "65536",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate SharedLogSizeInMB that is larger than 512 succeeds",
                    true);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Ignore]
        [Owner("AlanWar")]
        public void KTLLogger_WriteBufferMemoryPoolMinimumInKBIsNoneZero_WriteBufferMemoryPoolMaximumInKBIsZero_ShouldFail()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.KtlLogger,
                FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMinimumInKB,
                "32768",
                false,
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.KtlLogger,
                FabricValidatorConstants.ParameterNames.KtlLogger.WriteBufferMemoryPoolMaximumInKB,
                "0",
                false,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate WriteBufferMemoryPoolMaximumInKB is 0 and WriteBufferMemoryPoolMinimumInKB is not 0 fails",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TStore_EnableStrict2PL_Default_False()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate EnableStrict2PL is default (false)",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TStore_EnableStrict2PL_True_True()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ReliableStateProvider,
                FabricValidatorConstants.ParameterNames.ReliableStateProvider.EnableStrict2PL,
                "true",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate EnableStrict2PL is true",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TStore_EnableStrict2PL_False_False()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ReliableStateProvider,
                FabricValidatorConstants.ParameterNames.ReliableStateProvider.EnableStrict2PL,
                "false",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate EnableStrict2PL is false",
                expectSuccess: true);
        }

        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TStore_SweepThreshold_LessThanNegativeOne_ArgumentOutOfRangeException()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ReliableStateProvider,
                FabricValidatorConstants.ParameterNames.ReliableStateProvider.SweepThreshold,
                "-2",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate SweepThreshold is less than -1",
                expectSuccess: false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TStore_SweepThreshold_NegativeOne_DisablesSweep()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ReliableStateProvider,
                FabricValidatorConstants.ParameterNames.ReliableStateProvider.SweepThreshold,
                "-1",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate SweepThreshold is -1 : disabled",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TStore_SweepThreshold_Zero_UsesTStoreDefinedDefault()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ReliableStateProvider,
                FabricValidatorConstants.ParameterNames.ReliableStateProvider.SweepThreshold,
                "0",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate SweepThreshold is 0 : default",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TStore_SweepThreshold_SmallPositive_Success()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ReliableStateProvider,
                FabricValidatorConstants.ParameterNames.ReliableStateProvider.SweepThreshold,
                "50",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate SweepThreshold is positive in range.",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TStore_SweepThreshold_OneLessThanMax_Success()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ReliableStateProvider,
                FabricValidatorConstants.ParameterNames.ReliableStateProvider.SweepThreshold,
                "4999999",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate SweepThreshold is 4999999 : Max -1",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TStore_SweepThreshold_AtMax_Success()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ReliableStateProvider,
                FabricValidatorConstants.ParameterNames.ReliableStateProvider.SweepThreshold,
                "5000000",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate SweepThreshold is 5000000 : Max",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("MCoskun")]
        public void TStore_SweepThreshold_LargerThanMax_ArgumentOutOfRangeException()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ReliableStateProvider,
                FabricValidatorConstants.ParameterNames.ReliableStateProvider.SweepThreshold,
                "5000001",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate SweepThreshold is 5000001",
                expectSuccess: false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TStore_SweepThreshold_Default()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate SweepThreshold default",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestNodeCertificateThumbprints()
        {
            {
                string validThumprint = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf";
                {
                    FabricLayoutInfo fabricLayoutInfo = SetClusterCertificateThumbprint(validThumprint, this.imageStore);
                    VerifyValidate(
                       fabricLayoutInfo.ClusterManifest,
                       null,
                       "Validate cluster certificate thumbprint",
                       true);
                }

                string validThumprint2 = "7812205a39d22376daa037f05aede3601a7e64bf";
                string validThumprint3 = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf";
                {
                    FabricLayoutInfo fabricLayoutInfo = SetClientServerCertificatesThumbprint(validThumprint2, validThumprint3, this.imageStore);

                    VerifyValidate(
                        fabricLayoutInfo.ClusterManifest,
                        null,
                        "Validate client and server certificate thumbprint",
                        true);
                }

                string validThumprint4 = "7812205a39d22376daa037f05aede3601a7e64bf";
                string validThumprint5 = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf";
                {
                    FabricLayoutInfo fabricLayoutInfo = SetClientServerCertificatesThumbprint(validThumprint4, validThumprint5, this.imageStore);
                    VerifyValidate(
                        fabricLayoutInfo.ClusterManifest,
                        null,
                        "Validate client and server certificate thumbprint",
                        true);
                }

                // The following is copied from Certificates.msc(mmc.exe), the beginning is non-displayable character 200E
                string badThumbprintWithNonDisplayableCharacter = "‎2b d6 3d 28 d7 bc d0 e2 51 19 5a eb 51 92 43 c1 31 42 eb c3";
                {
                    FabricLayoutInfo fabricLayoutInfo = SetClusterCertificateThumbprint(badThumbprintWithNonDisplayableCharacter, this.imageStore);
                    VerifyValidate(
                        fabricLayoutInfo.ClusterManifest,
                        null,
                        "Test invalid cluster certificate thumbprint",
                        false);
                }

                {
                    FabricLayoutInfo fabricLayoutInfo = SetClientServerCertificatesThumbprint(
                        validThumprint,
                        badThumbprintWithNonDisplayableCharacter, 
                        this.imageStore);
                    VerifyValidate(
                        fabricLayoutInfo.ClusterManifest,
                        null,
                        "Test invalid server certificate thumbprint",
                        false);
                }

                {
                    FabricLayoutInfo fabricLayoutInfo = SetClientServerCertificatesThumbprint(
                        badThumbprintWithNonDisplayableCharacter,
                        validThumprint, 
                        this.imageStore);
                    VerifyValidate(
                        fabricLayoutInfo.ClusterManifest,
                        null,
                        "Test invalid client certificate thumbprint",
                        false);
                }
            }
        }

        private static FabricLayoutInfo SetClusterCertificateThumbprint(string certThumbprint, IImageStore imageStore)
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Security,
                FabricValidatorConstants.ParameterNames.ClusterCredentialType,
                "X509",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            if (fabricLayoutInfo.ClusterManifest.NodeTypes == null)
            {
                fabricLayoutInfo.ClusterManifest.NodeTypes = new ClusterManifestTypeNodeType[1];
            }

            if (fabricLayoutInfo.ClusterManifest.NodeTypes[0] == null)
            {
                fabricLayoutInfo.ClusterManifest.NodeTypes[0] = new ClusterManifestTypeNodeType();
            }

            if (fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates == null)
            {
                fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates = new CertificatesType();
            }

            fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates.ClusterCertificate = new FabricCertificateType();
            fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates.ClusterCertificate.X509FindType = FabricCertificateTypeX509FindType.FindByThumbprint;
            fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates.ClusterCertificate.X509FindValue = certThumbprint;

            return fabricLayoutInfo;
        }

        private static FabricLayoutInfo SetClientServerCertificatesThumbprint(string clientCertThumbprint, string serverCertThumbprint, IImageStore imageStore)
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Security,
                FabricValidatorConstants.ParameterNames.ServerAuthCredentialType,
                "X509",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            if (fabricLayoutInfo.ClusterManifest.NodeTypes == null)
            {
                fabricLayoutInfo.ClusterManifest.NodeTypes = new ClusterManifestTypeNodeType[1];
            }

            if (fabricLayoutInfo.ClusterManifest.NodeTypes[0] == null)
            {
                fabricLayoutInfo.ClusterManifest.NodeTypes[0] = new ClusterManifestTypeNodeType();
            }

            if (fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates == null)
            {
                fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates = new CertificatesType();
            }

            fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates.ClientCertificate = new FabricCertificateType();
            fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates.ClientCertificate.X509FindType = FabricCertificateTypeX509FindType.FindByThumbprint;
            fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates.ClientCertificate.X509FindValue = clientCertThumbprint;

            fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates.ServerCertificate = new FabricCertificateType();
            fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates.ServerCertificate.X509FindType = FabricCertificateTypeX509FindType.FindByThumbprint;
            fabricLayoutInfo.ClusterManifest.NodeTypes[0].Certificates.ServerCertificate.X509FindValue = serverCertThumbprint;

            return fabricLayoutInfo;
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidReplicatorSettings_CopyQueueSize()
        {
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxCopyQueueSize,
                    "1024",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialCopyQueueSize,
                    "2048",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Copy Queue Settings Test1",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialCopyQueueSize,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Copy Queue Settings Test2",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialCopyQueueSize,
                    "22",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Copy Queue Settings Test3",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxCopyQueueSize,
                    "4567",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Copy Queue Settings Test4",
                    false);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidReplicatorSettings_ReplicationQueueSize()
        {
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxReplicationQueueMemorySize,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxReplicationQueueSize,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Replication Queue Settings Test1",
                    false);
            }
            
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxReplicationQueueSize,
                    "1024",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialReplicationQueueSize,
                    "2048",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Replication Queue Settings Test2",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialReplicationQueueSize,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Replication Queue Settings Test3",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialReplicationQueueSize,
                    "12",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Replication Queue Settings Test4",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxReplicationQueueSize,
                    "4567",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Replication Queue Settings Test5",
                    false);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidReplicatorSettings_PrimaryReplicationQueueSize()
        {
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxPrimaryReplicationQueueMemorySize,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxPrimaryReplicationQueueSize,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Primary Replication Queue Settings Test1",
                    false);
            }
            
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxPrimaryReplicationQueueSize,
                    "1024",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialPrimaryReplicationQueueSize,
                    "2048",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Primary Replication Queue Settings Test2",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialPrimaryReplicationQueueSize,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Primary Replication Queue Settings Test3",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialPrimaryReplicationQueueSize,
                    "12",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Primary Replication Queue Settings Test4",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxPrimaryReplicationQueueSize,
                    "4567",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Primary Replication Queue Settings Test5",
                    false);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidReplicatorSettings_SecondaryReplicationQueueSize()
        {
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxSecondaryReplicationQueueMemorySize,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxSecondaryReplicationQueueSize,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Secondary Replication Queue Settings Test1",
                    false);
            }
            
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxSecondaryReplicationQueueSize,
                    "1024",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialSecondaryReplicationQueueSize,
                    "2048",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Secondary Replication Queue Settings Test2",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialSecondaryReplicationQueueSize,
                    "0",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Secondary Replication Queue Settings Test3",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.InitialSecondaryReplicationQueueSize,
                    "12",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Secondary Replication Queue Settings Test4",
                    false);
            }

            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

                TestUtility.AddToFabricSettings(
                    GetRandomReplicatorSectionName(),
                    FabricValidatorConstants.ParameterNames.Replicator.MaxSecondaryReplicationQueueSize,
                    "4567",
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate Secondary Replication Queue Settings Test5",
                    false);
            }
        }

        private string GetRandomReplicatorSectionName()
        {
            Random rand = new Random(DateTime.Now.Hour);
            int num = rand.Next() % 5;

            switch(num)
            {
                case 0:
                    return FabricValidatorConstants.SectionNames.Replication.Default;
                case 1:
                    return FabricValidatorConstants.SectionNames.Replication.ClusterManager;
                case 2:
                    return FabricValidatorConstants.SectionNames.Replication.Failover;
                case 3:
                    return FabricValidatorConstants.SectionNames.Replication.FileStoreService;
                case 4:
                    return FabricValidatorConstants.SectionNames.Replication.Naming;
                default:
                    Assert.Fail("Invalid random case");
                    return null;
            }
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidImageStoreServiceSettings1()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                "fabric:ImageStore",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            // ClusterManifest TargetReplicaSetSize explicitly set to 1 in ClusterManifest file
            // ImageStoreService TargetReplicaSetSize is default ie. 7
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidImageStoreServiceSettings2()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                "fabric:ImageStore",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            // ClusterManifest TargetReplicaSetSize explicitly set to 1 in ClusterManifest file
            // ImageStoreService TargetReplicaSetSize is set to 7
            int issTargetReplicaSetSize = 7;
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ImageStoreService, 
                FabricValidatorConstants.ParameterNames.TargetReplicaSetSize, 
                issTargetReplicaSetSize.ToString(), 
                false /*isEncrypted*/, 
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
               fabricLayoutInfo.ClusterManifest,
               null,
               "Validate",
               false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestImageStoreConnectionStringUpgrade()
        {
            ClusterManifestType clusterManifest = (new ClusterManifestHelper(true, false)).ClusterManifest;
            clusterManifest.FabricSettings = new SettingsOverridesTypeSection[4];
            List<SettingsOverridesTypeSectionParameter> parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ClusterCredentialType, Value = "None", IsEncrypted = false });
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ServerAuthCredentialType, Value = "None", IsEncrypted = false });
            clusterManifest.FabricSettings[0] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.Security, Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ExpectedClustersize", Value = "1", IsEncrypted = false });
            clusterManifest.FabricSettings[1] = new SettingsOverridesTypeSection() { Name = "Failovermanager", Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "ImageStoreConnectionString", Value = "fabric:ImageStore", IsEncrypted = false });
            clusterManifest.FabricSettings[2] = new SettingsOverridesTypeSection() { Name = "Management", Parameter = parameters.ToArray() };
            parameters = new List<SettingsOverridesTypeSectionParameter>();
            parameters.Add(new SettingsOverridesTypeSectionParameter() { Name = "RunAsPolicyEnabled", Value = "true", IsEncrypted = false });
            clusterManifest.FabricSettings[3] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.Hosting, Parameter = parameters.ToArray() };

            ClusterManifestType targetClusterManifest = (new ClusterManifestHelper(true, false)).ClusterManifest;
            targetClusterManifest.FabricSettings = new SettingsOverridesTypeSection[4];
            List<SettingsOverridesTypeSectionParameter> parameters2 = new List<SettingsOverridesTypeSectionParameter>();
            parameters2.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ClusterCredentialType, Value = "None", IsEncrypted = false });
            parameters2.Add(new SettingsOverridesTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ServerAuthCredentialType, Value = "None", IsEncrypted = false });
            targetClusterManifest.FabricSettings[0] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.Security, Parameter = parameters2.ToArray() };
            parameters2 = new List<SettingsOverridesTypeSectionParameter>();
            parameters2.Add(new SettingsOverridesTypeSectionParameter() { Name = "ExpectedClustersize", Value = "1", IsEncrypted = false });
            targetClusterManifest.FabricSettings[1] = new SettingsOverridesTypeSection() { Name = "Failovermanager", Parameter = parameters2.ToArray() };
            parameters2 = new List<SettingsOverridesTypeSectionParameter>();
            parameters2.Add(new SettingsOverridesTypeSectionParameter() { Name = "ImageStoreConnectionString", Value = "xstore: ImageStore", IsEncrypted = false });
            targetClusterManifest.FabricSettings[2] = new SettingsOverridesTypeSection() { Name = "Management", Parameter = parameters2.ToArray() };
            parameters2 = new List<SettingsOverridesTypeSectionParameter>();
            parameters2.Add(new SettingsOverridesTypeSectionParameter() { Name = "RunAsPolicyEnabled", Value = "true", IsEncrypted = false });
            targetClusterManifest.FabricSettings[3] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.Hosting, Parameter = parameters.ToArray() };

            Verify.Throws<ArgumentException>(
                () => VerifyCompareAndAnalyze(
                     clusterManifest,
                     targetClusterManifest,
                     null,
                     0));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidImageStoreServiceSettings()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                "fabric:ImageStore",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Hosting,
                FabricValidatorConstants.ParameterNames.RunAsPolicyEnabled,
                "true",
                false,
                fabricLayoutInfo.ClusterManifest);

            // ClusterManifest TargetReplicaSetSize explicitly set to 1 in ClusterManifest file
            // ImageStoreService TargetReplicaSetSize is set to 1
            int issTargetReplicaSetSize = 1;
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ImageStoreService,
                FabricValidatorConstants.ParameterNames.TargetReplicaSetSize,
                issTargetReplicaSetSize.ToString(),
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            int issMinReplicaSetSize = 1;
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ImageStoreService,
                FabricValidatorConstants.ParameterNames.MinReplicaSetSize,
                issMinReplicaSetSize.ToString(),
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestImageStoreServiceSettingsWithAffinityDisabled()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                "fabric:ImageStore",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(
                       FabricValidatorConstants.SectionNames.Hosting,
                       FabricValidatorConstants.ParameterNames.RunAsPolicyEnabled,
                       "true",
                       false,
                       fabricLayoutInfo.ClusterManifest);

            bool enableAffinity = false;
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.ImageStoreService,
                FabricValidatorConstants.ParameterNames.EnableClusterManagerAffinity,
                enableAffinity.ToString(),
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Hosting,
                FabricValidatorConstants.ParameterNames.RunAsPolicyEnabled,
                "true",
                false,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidClusterHealthPolicyAppTypeHealthPolicyMap()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.HealthManager_ClusterHealthPolicy,
                FabricValidatorConstants.ParameterNames.ApplicationTypeMaxPercentUnhealthyApplicationsPrefix + "AppType1",
                "10",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.HealthManager_ClusterHealthPolicy,
                FabricValidatorConstants.ParameterNames.ApplicationTypeMaxPercentUnhealthyApplicationsPrefix + "AppType2",
                "20",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate ClusterHealthPolicy",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidClusterHealthPolicyAppTypeHealthPolicyWithEmptyAppTypeName()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Add empty app type name
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.HealthManager_ClusterHealthPolicy,
                FabricValidatorConstants.ParameterNames.ApplicationTypeMaxPercentUnhealthyApplicationsPrefix,
                "10",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate ApplicationTypeMaxPercentUnhealthyApplications with empty app type name",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidClusterHealthPolicyAppTypeHealthPolicyWithInvalidPercentage()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.HealthManager_ClusterHealthPolicy,
                FabricValidatorConstants.ParameterNames.ApplicationTypeMaxPercentUnhealthyApplicationsPrefix + "AppType",
                "10943",
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate ApplicationTypeMaxPercentUnhealthyApplications with invalid percentage",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidFMReplicaSetSize()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            int fmTargetReplicaSetSize = 1;
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FailoverManager,
                FabricValidatorConstants.ParameterNames.TargetReplicaSetSize,
                fmTargetReplicaSetSize.ToString(),
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            int fmMinReplicaSetSize = 2;
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FailoverManager,
                FabricValidatorConstants.ParameterNames.MinReplicaSetSize,
                fmMinReplicaSetSize.ToString(),
                false /*isEncrypted*/,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }    

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEncryptedDefaultImageStoreConnectionString()
        {
            string imageStoreConnectionString = @"_default_";
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            UpdateWithEncryptedImageStoreConnectionString(fabricLayoutInfo.ClusterManifest, imageStoreConnectionString, CertificateThumbprint_Alice, X509Credentials.StoreNameDefault);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPLBPropertyGroupSectionsAddingDuringUpgrade()
        {
            FabricLayoutInfo oldFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            FabricLayoutInfo newFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            newFabricLayoutInfo.ClusterManifest.Version = "2.0";
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricActivityThresholds, "PrimaryCount", "100", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricBalancingThresholds, "PrimaryCount", "1.0", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.GlobalMetricWeights, "PrimaryCount", "1.0", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationMetrics, "PrimaryCount", "true", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, "PrimaryCount", "0.1", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeDistributionPolicy, "PrimaryCount", "1", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor, "PrimaryCount", "1.4", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementHeuristicEmptySpacePercent, "PrimaryCount", "0.5", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.NodeBufferPercentage, "PrimaryCount", "0.2", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationScopedAlgorithmEnabled, "PrimaryCount", "true", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight, "PrimaryCount", "0.5", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricEmptyNodeThresholds, "PrimaryCount", "700", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNode, "NodeType1", "1", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeBalancingThrottle, "NodeType1", "2", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeConstraintCheckThrottle, "NodeType1", "3", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodePlacementThrottle, "NodeType1", "4", false, newFabricLayoutInfo.ClusterManifest);

            VerifyCompareAndAnalyze(
                    oldFabricLayoutInfo.ClusterManifest,
                    newFabricLayoutInfo.ClusterManifest,
                    null /*infrastructure*/,
                    0 /*expectedModifedStaticSettingsCount*/);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPLBPropertyGroupSectionsRemoveDuringUpgrade()
        {
            FabricLayoutInfo oldFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricActivityThresholds, "PrimaryCount", "100", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricBalancingThresholds, "PrimaryCount", "1.0", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.GlobalMetricWeights, "PrimaryCount", "1.0", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationMetrics, "PrimaryCount", "true", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, "PrimaryCount", "0.1", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeDistributionPolicy, "PrimaryCount", "1", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor, "PrimaryCount", "1.4", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementHeuristicEmptySpacePercent, "PrimaryCount", "0.5", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.NodeBufferPercentage, "PrimaryCount", "0.2", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationScopedAlgorithmEnabled, "PrimaryCount", "true", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight, "PrimaryCount", "0.5", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricEmptyNodeThresholds, "PrimaryCount", "700", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNode, "NodeType1", "1", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeBalancingThrottle, "NodeType1", "2", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeConstraintCheckThrottle, "NodeType1", "3", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodePlacementThrottle, "NodeType1", "4", false, oldFabricLayoutInfo.ClusterManifest);

            FabricLayoutInfo newFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            newFabricLayoutInfo.ClusterManifest.Version = "2.0";

            VerifyCompareAndAnalyze(
                    oldFabricLayoutInfo.ClusterManifest,
                    newFabricLayoutInfo.ClusterManifest,
                    null /*infrastructure*/,
                    0 /*expectedModifedStaticSettingsCount*/);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPLBPropertyGroupSectionsChangeDuringUpgrade()
        {
            FabricLayoutInfo oldFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricActivityThresholds, "PrimaryCount", "100", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricBalancingThresholds, "PrimaryCount", "1.0", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.GlobalMetricWeights, "PrimaryCount", "1.0", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationMetrics, "PrimaryCount", "true", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, "PrimaryCount", "0.1", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeDistributionPolicy, "PrimaryCount", "1", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor, "PrimaryCount", "1.4", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementHeuristicEmptySpacePercent, "PrimaryCount", "0.5", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.NodeBufferPercentage, "PrimaryCount", "0.2", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationScopedAlgorithmEnabled, "PrimaryCount", "true", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight, "PrimaryCount", "0.5", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricEmptyNodeThresholds, "PrimaryCount", "700", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNode, "NodeType1", "1", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeBalancingThrottle, "NodeType1", "2", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeConstraintCheckThrottle, "NodeType1", "3", false, oldFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodePlacementThrottle, "NodeType1", "4", false, oldFabricLayoutInfo.ClusterManifest);

            FabricLayoutInfo newFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            newFabricLayoutInfo.ClusterManifest.Version = "2.0";
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricActivityThresholds, "PrimaryCount", "105", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricBalancingThresholds, "PrimaryCount", "1.1", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.GlobalMetricWeights, "PrimaryCount", "1.5", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationMetrics, "PrimaryCount", "false", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold, "PrimaryCount", "4.0", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeDistributionPolicy, "PrimaryCount", "0", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor, "PrimaryCount", "1.3", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementHeuristicEmptySpacePercent, "PrimaryCount", "0.2", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.NodeBufferPercentage, "PrimaryCount", "0.5", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationScopedAlgorithmEnabled, "PrimaryCount", "false", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight, "PrimaryCount", "0.6", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MetricEmptyNodeThresholds, "PrimaryCount", "100", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNode, "NodeType1", "2", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeBalancingThrottle, "NodeType1", "3", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeConstraintCheckThrottle, "NodeType1", "4", false, newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodePlacementThrottle, "NodeType1", "5", false, newFabricLayoutInfo.ClusterManifest);

            VerifyCompareAndAnalyze(
                    oldFabricLayoutInfo.ClusterManifest,
                    newFabricLayoutInfo.ClusterManifest,
                    null /*infrastructure*/,
                    0 /*expectedModifedStaticSettingsCount*/);
        }


        private void VerifyPLBSetting(string settingName, string value, bool expectSuccess)
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing,
                settingName,
                value,
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                expectSuccess);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingCorrectParameters()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing, "YieldDurationPer10ms", "5", false, fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing, "SwapPrimaryProbability", "0", false, fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing, "LoadDecayFactor", "0", false, fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing, "AffinityMoveParentReplicaProbability", ".5", false, fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing, "CapacityConstraintPriority", "-1", false, fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing, "AffinityConstraintPriority", "0", false, fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing, "FaultDomainConstraintPriority", "1", false, fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing, "PLBRefreshInterval", "0.5", false, fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingYieldDuration()
        {
            VerifyPLBSetting("YieldDurationPer10ms", "15", false);
            VerifyPLBSetting("YieldDurationPer10ms", "-1", false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingSwapPrimaryProbability()
        {
            VerifyPLBSetting("SwapPrimaryProbability", "-1", false);
            VerifyPLBSetting("SwapPrimaryProbability", "0.5", true);
            VerifyPLBSetting("SwapPrimaryProbability", "1", true);
            VerifyPLBSetting("SwapPrimaryProbability", "2", false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingLoadDecayFactor()
        {
            VerifyPLBSetting("LoadDecayFactor", "-1", false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingAffinityMoveParentReplicaProbability()
        {
            VerifyPLBSetting("AffinityMoveParentReplicaProbability", "1", true);
            VerifyPLBSetting("AffinityMoveParentReplicaProbability", "2", false);
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingUpgradeDomainConstraintPriority()
        {
            VerifyPLBSetting("UpgradeDomainConstraintPriority", ".5", false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingThrottlingConstraintPriority()
        {
            VerifyPLBSetting("ThrottlingConstraintPriority", ".5", false);
            VerifyPLBSetting("ThrottlingConstraintPriority", "-2", false);
            VerifyPLBSetting("ThrottlingConstraintPriority", "error", false);
            VerifyPLBSetting("ThrottlingConstraintPriority", "-1", true);
            VerifyPLBSetting("ThrottlingConstraintPriority", "0", true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingLocalBalancingThreshold()
        {
            VerifyPLBSetting("LocalBalancingThreshold", "0.0", true);
            VerifyPLBSetting("LocalBalancingThreshold", "1.0", true);
            VerifyPLBSetting("LocalBalancingThreshold", "2.0", true);
            VerifyPLBSetting("LocalBalancingThreshold", "0.5", false);
            VerifyPLBSetting("LocalBalancingThreshold", "-1", false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingTimerConfigurations()
        {
            // In LoadBalancing section, we can either define PLBRefreshInterval or new timers (MinPlacementInterval and MinConstraintCheckInterval).
            // This test verifies if validator will correctly catch invalid configuarions in the manifest.
            // Test: only refresh timer
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing, 
                FabricValidatorConstants.ParameterNames.PLB.PLBRefreshInterval, 
                "1", 
                false, 
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Test: only refresh timer
            fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing,
                FabricValidatorConstants.ParameterNames.PLB.MinPlacementInterval,
                "1",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Test: refreah and placement timers (invalid)
            fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing,
                FabricValidatorConstants.ParameterNames.PLB.PLBRefreshInterval,
                "1",
                false,
                fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing,
                FabricValidatorConstants.ParameterNames.PLB.MinPlacementInterval,
                "1",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);

            // Test: refresh and constraint check (invalid)
            fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing,
                FabricValidatorConstants.ParameterNames.PLB.PLBRefreshInterval,
                "1",
                false,
                fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing,
                FabricValidatorConstants.ParameterNames.PLB.MinPlacementInterval,
                "1",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);

            // Test: all timers (invalid)
            fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing,
                FabricValidatorConstants.ParameterNames.PLB.PLBRefreshInterval,
                "1",
                false,
                fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing,
                FabricValidatorConstants.ParameterNames.PLB.MinPlacementInterval,
                "1",
                false,
                fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementAndLoadBalancing,
                FabricValidatorConstants.ParameterNames.PLB.MinConstraintCheckInterval,
                "1",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingLocalDomainWeight()
        {
            // LocalDomainWeight is an internal parameter that can have value between 0.0 and 1.0 (inclusive)
            VerifyPLBSetting(FabricValidatorConstants.ParameterNames.PLB.LocalDomainWeight, "0.0", true);
            VerifyPLBSetting(FabricValidatorConstants.ParameterNames.PLB.LocalDomainWeight, "1.0", true);
            VerifyPLBSetting(FabricValidatorConstants.ParameterNames.PLB.LocalDomainWeight, "0.12345", true);
            // Invalid values
            VerifyPLBSetting(FabricValidatorConstants.ParameterNames.PLB.LocalDomainWeight, "-0.001", false);
            VerifyPLBSetting(FabricValidatorConstants.ParameterNames.PLB.LocalDomainWeight, "1.001", false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold()
        {
            // DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold is a public parameter that can have value greater than or equal to 0.0
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                "DefragMetric1",
                "0.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                "DefragMetric1",
                "0.758",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                "DefragMetric1",
                "17.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                "DefragMetric1",
                "-0.1",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                "DefragMetric1",
                "-25.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold,
                "DefragMetric1",
                "RedStarForever",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDefragmentationEmptyNodeDistributionPolicy()
        {
            // DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold is a public parameter that can have value greater than or equal to 0.0
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeDistributionPolicy,
                "DefragMetric1",
                "0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeDistributionPolicy,
                "DefragMetric1",
                "1",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeDistributionPolicy,
                "DefragMetric1",
                "2",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeDistributionPolicy,
                "DefragMetric1",
                "error",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementHeuristicFactorPerMetric()
        {
            // PlacementHeuristicFactorPerMetric is a internal parameter that can have value greater than or equal to 0.0
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor,
                "DefragMetric1",
                "0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor,
                "DefragMetric1",
                "0.512",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor,
                "DefragMetric1",
                "1.5",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor,
                "DefragMetric1",
                "-0.1",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor,
                "DefragMetric1",
                "-25.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementHeuristicIncomingLoadFactor,
                "DefragMetric1",
                "MetricStringValue",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementHeuristicEmptyLoadFactorPerMetric()
        {
            // PlacementHeuristicEmptyLoadFactorPerMetric is a internal parameter that can have value in range [0.0, 1.0]
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementHeuristicEmptySpacePercent,
                "DefragMetric1",
                "0.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementHeuristicEmptySpacePercent,
                "DefragMetric1",
                "1.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementHeuristicEmptySpacePercent,
                "DefragMetric1",
                "2",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementHeuristicEmptySpacePercent,
                "DefragMetric1",
                "error",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.PlacementHeuristicEmptySpacePercent,
                "DefragMetric1",
                "-3.2",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingDefragmentationScopedAlgorithmEnabled()
        {
            // DefragmentationScopedAlgorithmEnabled is a internal bool parameter
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationScopedAlgorithmEnabled,
                "DefragMetric1",
                "true",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationScopedAlgorithmEnabled,
                "DefragMetric1",
                "false",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationScopedAlgorithmEnabled,
                "DefragMetric1",
                "123",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationScopedAlgorithmEnabled,
                "DefragMetric1",
                "error",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDefragmentationEmptyNodeWeight()
        {
            // DefragmentationEmptyNodeWeight is a internal parameter that can have value in range [0.0, 1.0]
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight,
                "DefragMetric1",
                "0.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight,
                "DefragMetric1",
                "1.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight,
                "DefragMetric1",
                "0.532",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight,
                "DefragMetric1",
                "123",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight,
                "DefragMetric1",
                "-1",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationEmptyNodeWeight,
                "DefragMetric1",
                "error",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestMetricEmptyNodeThresholds()
        {
            // MetricEmptyNodeThresholds is a internal parameter that can have value >= 0
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricEmptyNodeThresholds,
                "DefragMetric1",
                "0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricEmptyNodeThresholds,
                "DefragMetric1",
                "700",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricEmptyNodeThresholds,
                "DefragMetric1",
                "-5",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricEmptyNodeThresholds,
                "DefragMetric1",
                "error",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestMetricActivityThresholds()
        {
            // MetricActivityThresholds is a public parameter that can have value >= 0
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricActivityThresholds,
                "Metric1",
                "0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricActivityThresholds,
                "Metric1",
                "700",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricActivityThresholds,
                "Metric1",
                "-5",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricActivityThresholds,
                "Metric1",
                "RedStar",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestMetricBalancingThresholds()
        {
            // MetricBalancingThresholds is a public parameter that can have value >= 1.0
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricBalancingThresholds,
                "Metric1",
                "1.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricBalancingThresholds,
                "Metric1",
                "7.5",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricBalancingThresholds,
                "Metric1",
                "0.9999",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.MetricBalancingThresholds,
                "Metric1",
                "SomeValue",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestGlobalMetricWeights()
        {
            // GlobalMetricWeights is a public parameter that can have value >= 0.0
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.GlobalMetricWeights,
                "Metric1",
                "0.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.GlobalMetricWeights,
                "Metric1",
                "7.5",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.GlobalMetricWeights,
                "Metric1",
                "-0.5",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.GlobalMetricWeights,
                "Metric1",
                "error",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        private void PerNodeThrottlesHelper(string sectionName)
        {
            // Different types of per node throttles are public parameter that need to have value > 0
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                sectionName,
                "NodeType1",
                "1",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                sectionName,
                "NodeType1",
                "7",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                sectionName,
                "NodeType1",
                "0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                sectionName,
                "NodeType1",
                "-3",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                sectionName,
                "NodeType1",
                "error",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPerNodeThrottleLimits()
        {
            PerNodeThrottlesHelper(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNode);
            PerNodeThrottlesHelper(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeBalancingThrottle);
            PerNodeThrottlesHelper(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodeConstraintCheckThrottle);
            PerNodeThrottlesHelper(FabricValidatorConstants.SectionNames.MaximumInBuildReplicasPerNodePlacementThrottle);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestNodeBufferPercentage()
        {
            // NodeBufferPercentage is a public parameter that can have value in range [0.0, 1.0]
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.NodeBufferPercentage,
                "Metric1",
                "0.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.NodeBufferPercentage,
                "Metric1",
                "1.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.NodeBufferPercentage,
                "Metric1",
                "-0.5",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.NodeBufferPercentage,
                "Metric1",
                "error",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.NodeBufferPercentage,
                "Metric1",
                "2.0",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDefragmentationMetrics()
        {
            // DefragmentationScopedAlgorithmEnabled is a internal bool parameter
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            // Valid edge values and valid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationMetrics,
                "DefragMetric1",
                "true",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationMetrics,
                "DefragMetric1",
                "false",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);

            // Invalid values
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationMetrics,
                "DefragMetric1",
                "123",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DefragmentationMetrics,
                "DefragMetric1",
                "error",
                false,
                fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingStatisticsTracingIntervalSetting()
        {
            // Valid edge values and valid values
            VerifyPLBSetting(
                FabricValidatorConstants.ParameterNames.PLB.StatisticsTracingInterval,
                "0.000001",
                true);
            VerifyPLBSetting(
                FabricValidatorConstants.ParameterNames.PLB.StatisticsTracingInterval,
                "314",
                true);
            VerifyPLBSetting(
                FabricValidatorConstants.ParameterNames.PLB.StatisticsTracingInterval,
                "0.0",
                true);

            // Invalid values
            VerifyPLBSetting(
                FabricValidatorConstants.ParameterNames.PLB.StatisticsTracingInterval,
                "-0.00001",
                false);
            VerifyPLBSetting(
                FabricValidatorConstants.ParameterNames.PLB.StatisticsTracingInterval,
                "-314",
                false);
            VerifyPLBSetting(
                FabricValidatorConstants.ParameterNames.PLB.StatisticsTracingInterval,
                "NotAStringValue",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndLoadBalancingQuorumBasedLogicAutoSwitchSetting()
        {
            // Valid edge values and valid values
            VerifyPLBSetting(
                FabricValidatorConstants.ParameterNames.PLB.QuorumBasedLogicAutoSwitch,
                "true",
                true);
            VerifyPLBSetting(
                FabricValidatorConstants.ParameterNames.PLB.QuorumBasedLogicAutoSwitch,
                "false",
                true);

            // Invalid values
            VerifyPLBSetting(
                FabricValidatorConstants.ParameterNames.PLB.QuorumBasedLogicAutoSwitch,
                "123",
                false);
            VerifyPLBSetting(
                FabricValidatorConstants.ParameterNames.PLB.QuorumBasedLogicAutoSwitch,
                "error",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementConstraintConfigurationSettings1()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.NamingService, "PlacementConstraints", "美国==美国", false, fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.ClusterManager, "PlacementConstraints", "美国==美国", false, fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.ImageStoreService, "PlacementConstraints", "美国==美国", false, fabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.RepairManager, "PlacementConstraints", "美国==美国", false, fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementConstraintConfigurationSettings2()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.NamingService, "PlacementConstraints", "!@#$^*()_++?>:{}| dr3美国", false, fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementConstraintConfigurationSettings3()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.ClusterManager, "PlacementConstraints", "!@#$^*()_++?>:{}| dr3美国", false, fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementConstraintConfigurationSettings4()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.ImageStoreService, "PlacementConstraints", "!@#$^*()_++?>:{}| dr3美国", false, fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementConstraintConfigurationSettings5()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(FabricValidatorConstants.SectionNames.RepairManager, "PlacementConstraints", "!@#$^*()_++?>:{}| dr3美国", false, fabricLayoutInfo.ClusterManifest);
            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidXStoreConnectionStringChange()
        {
            string oldImageStoreConnectionString = @"xstore:DefaultEndpointsProtocol=https;AccountName=TestAccountName;AccountKey=TestAccountKey;Container=TestContainer";
            string newImageStoreConnectionString = @"xstore:DefaultEndpointsProtocol=https;AccountName=TestAccountName;AccountKey=ModifiedTestAccountKey;Container=TestContainer";

            FabricLayoutInfo oldFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                oldImageStoreConnectionString,
                false /*isEncrypted*/,
                oldFabricLayoutInfo.ClusterManifest);

            FabricLayoutInfo newFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            newFabricLayoutInfo.ClusterManifest.Version = "2.0";
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                newImageStoreConnectionString,
                false /*isEncrypted*/,
                newFabricLayoutInfo.ClusterManifest);

            VerifyCompareAndAnalyze(
                    oldFabricLayoutInfo.ClusterManifest,
                    newFabricLayoutInfo.ClusterManifest,
                    null /*infrastructure*/,
                    1 /*expectedModifedStaticSettingsCount*/);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidXStoreConnectionStringWithCustomEndpointChange()
        {
            string oldImageStoreConnectionString = @"xstore:BlobEndpoint=myBlobEndpoint;AccountName=TestAccountName;AccountKey=TestAccountKey;Container=TestContainer";
            string newImageStoreConnectionString = @"xstore:BlobEndpoint=myBlobEndpoint1;AccountName=TestAccountName;AccountKey=ModifiedTestAccountKey;Container=TestContainer";

            FabricLayoutInfo oldFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                oldImageStoreConnectionString,
                false /*isEncrypted*/,
                oldFabricLayoutInfo.ClusterManifest);

            FabricLayoutInfo newFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            newFabricLayoutInfo.ClusterManifest.Version = "2.0";
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                newImageStoreConnectionString,
                false /*isEncrypted*/,
                newFabricLayoutInfo.ClusterManifest);

            VerifyCompareAndAnalyze(
                    oldFabricLayoutInfo.ClusterManifest,
                    newFabricLayoutInfo.ClusterManifest,
                    null /*infrastructure*/,
                    1 /*expectedModifedStaticSettingsCount*/);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidConnectionStringChange()
        {
            string oldImageStoreConnectionString = @"file:\\MyShare\ImageStore";
            string newImageStoreConnectionString = @"fabric:ImageStore";

            FabricLayoutInfo oldFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                oldImageStoreConnectionString,
                false /*isEncrypted*/,
                oldFabricLayoutInfo.ClusterManifest);

            FabricLayoutInfo newFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            newFabricLayoutInfo.ClusterManifest.Version = "2.0";
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                newImageStoreConnectionString,
                false /*isEncrypted*/,
                newFabricLayoutInfo.ClusterManifest);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.AllowImageStoreConnectionStringChange,
                "true",
                false /*isEncrypted*/,
                newFabricLayoutInfo.ClusterManifest);

            VerifyCompareAndAnalyze(
                oldFabricLayoutInfo.ClusterManifest,
                newFabricLayoutInfo.ClusterManifest,
                null,
                1);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidXStoreConnectionStringChange()
        {
            string oldImageStoreConnectionString = @"xstore:DefaultEndpointsProtocol=https;AccountName=TestAccountName;AccountKey=TestAccountKey;Container=TestContainer";
            string newImageStoreConnectionString = @"xstore:DefaultEndpointsProtocol=https;AccountName=TestAccountName;AccountKey=TestAccountKey;Container=ModifiedTestContainer";

            FabricLayoutInfo oldFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                oldImageStoreConnectionString,
                false /*isEncrypted*/,
                oldFabricLayoutInfo.ClusterManifest);

            FabricLayoutInfo newFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            newFabricLayoutInfo.ClusterManifest.Version = "2.0";
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                newImageStoreConnectionString,
                false /*isEncrypted*/,
                newFabricLayoutInfo.ClusterManifest);

            Verify.Throws<ArgumentException>(
                () => VerifyCompareAndAnalyze(
                     oldFabricLayoutInfo.ClusterManifest,
                     newFabricLayoutInfo.ClusterManifest,
                     null,
                     0));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidXStoreConnectionStringWithCustomEndpointChange()
        {
            string oldImageStoreConnectionString = @"xstore:BlobEndpoint=myBlobEndpoint;AccountName=TestAccountName;AccountKey=TestAccountKey;Container=TestContainer";
            string newImageStoreConnectionString = @"xstore:BlobEndpoint=myBlobEndpoint1;AccountName=TestAccountName;AccountKey=TestAccountKey;Container=ModifiedTestContainer";

            FabricLayoutInfo oldFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                oldImageStoreConnectionString,
                false /*isEncrypted*/,
                oldFabricLayoutInfo.ClusterManifest);

            FabricLayoutInfo newFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            newFabricLayoutInfo.ClusterManifest.Version = "2.0";
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                newImageStoreConnectionString,
                false /*isEncrypted*/,
                newFabricLayoutInfo.ClusterManifest);

            Verify.Throws<ArgumentException>(
                () => VerifyCompareAndAnalyze(
                     oldFabricLayoutInfo.ClusterManifest,
                     newFabricLayoutInfo.ClusterManifest,
                     null,
                     0));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestModifiedEncryptedConnectionStringWithSameValueFile()
        {
            string imageStoreConnectionString = @"file:\\fakeshare\ImageStore";
            FabricLayoutInfo oldFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            UpdateWithEncryptedImageStoreConnectionString(oldFabricLayoutInfo.ClusterManifest, imageStoreConnectionString, CertificateThumbprint_Alice, X509Credentials.StoreNameDefault);

            FabricLayoutInfo newFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            newFabricLayoutInfo.ClusterManifest.Version = "2.0";
            UpdateWithEncryptedImageStoreConnectionString(newFabricLayoutInfo.ClusterManifest, imageStoreConnectionString, CertificateThumbprint_Bob, X509Credentials.StoreNameDefault);

            VerifyCompareAndAnalyze(
                    oldFabricLayoutInfo.ClusterManifest,
                    newFabricLayoutInfo.ClusterManifest,
                    null /*infrastructure*/,
                    0 /*expectedModifedStaticSettingsCount*/);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestModifiedEncryptedConnectionStringWithSameValueAzure()
        {

            string imageStoreConnectionString = @"xstore:DefaultEndpointsProtocol=https;AccountName=TestAccountName;AccountKey=TestAccountKey;Container=TestContainer";
            FabricLayoutInfo oldFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            UpdateWithEncryptedImageStoreConnectionString(oldFabricLayoutInfo.ClusterManifest, imageStoreConnectionString, CertificateThumbprint_Alice, X509Credentials.StoreNameDefault);

            FabricLayoutInfo newFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            newFabricLayoutInfo.ClusterManifest.Version = "2.0";
            UpdateWithEncryptedImageStoreConnectionString(newFabricLayoutInfo.ClusterManifest, imageStoreConnectionString, CertificateThumbprint_Bob, X509Credentials.StoreNameDefault);

            VerifyCompareAndAnalyze(
                    oldFabricLayoutInfo.ClusterManifest,
                    newFabricLayoutInfo.ClusterManifest,
                    null /*infrastructure*/,
                    0 /*expectedModifedStaticSettingsCount*/);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestModifiedEncryptedConnectionStringWithSameValueFabric()
        {
            string imageStoreConnectionString = @"fabric:ImageStore";
            FabricLayoutInfo oldFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            UpdateWithEncryptedImageStoreConnectionString(oldFabricLayoutInfo.ClusterManifest, imageStoreConnectionString, CertificateThumbprint_Alice, X509Credentials.StoreNameDefault);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Hosting,
                FabricValidatorConstants.ParameterNames.RunAsPolicyEnabled,
                "true",
                false,
                oldFabricLayoutInfo.ClusterManifest);
            
            FabricLayoutInfo newFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            newFabricLayoutInfo.ClusterManifest.Version = "2.0";
            UpdateWithEncryptedImageStoreConnectionString(newFabricLayoutInfo.ClusterManifest, imageStoreConnectionString, CertificateThumbprint_Bob, X509Credentials.StoreNameDefault);
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Hosting,
                FabricValidatorConstants.ParameterNames.RunAsPolicyEnabled,
                "true",
                false,
                newFabricLayoutInfo.ClusterManifest);

            VerifyCompareAndAnalyze(
                    oldFabricLayoutInfo.ClusterManifest,
                    newFabricLayoutInfo.ClusterManifest,
                    null /*infrastructure*/,
                    0 /*expectedModifedStaticSettingsCount*/);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestFileStoreService_PrimaryAccountValidation()
        {
            string imageStoreConnectionString = @"fabric:ImageStore";
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Hosting,
                FabricValidatorConstants.ParameterNames.RunAsPolicyEnabled,
                "true",
                false,
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management, 
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString, 
                imageStoreConnectionString, 
                false, 
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FileStoreService,
                FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType,
                SecurityPrincipalsTypeUserAccountType.LocalUser.ToString(),
                false,
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FileStoreService,
                FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                SecurityPrincipalsTypeUserAccountType.LocalUser.ToString(),
                false,
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FileStoreService,
                FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountNTLMPasswordSecret,
                "PasswordSecret_Secondary",
                false,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate empty NTLMPasswordSecret for LocalUser",
                false);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FileStoreService,
                FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountNTLMPasswordSecret,
                "PasswordSecret_Primary",
                true,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate valid NTLMPasswordSecret for LocalUser",
                true);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FileStoreService,
                FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountUserPassword,
                "AccountPasswordProvidedForLocalUser",
                false,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate AccountUserPassword specified for LocalUser",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestFileStoreService_SecondaryAccountValidation()
        {
            string imageStoreConnectionString = @"fabric:ImageStore";
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Hosting,
                FabricValidatorConstants.ParameterNames.RunAsPolicyEnabled,
                "true",
                false,
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.Management,
                FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                imageStoreConnectionString,
                false,
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FileStoreService,
                FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType,
                SecurityPrincipalsTypeUserAccountType.LocalUser.ToString(),
                false,
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FileStoreService,
                FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountNTLMPasswordSecret,
                "PasswordSecret_Primary",
                false,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate secondary AccountInformation not provided for LocalUser",
                false);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FileStoreService,
                FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                SecurityPrincipalsTypeUserAccountType.DomainUser.ToString(),
                false,
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FileStoreService,
                FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountUserName,
                @"remond\FooUser",
                false,
                fabricLayoutInfo.ClusterManifest);     
       
            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FileStoreService,
                FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountUserPassword,
                "FakePassword$1",
                true,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate secondary account has domain user.",
                true);            

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.FileStoreService,
                FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountNTLMPasswordSecret,
                "NTLMSecretForDomainUser",
                false,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate NTLMPassword secret provided for domain user",
                false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPlacementAndCapacityChange()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            var clusterManifest = fabricLayoutInfo.ClusterManifest;
            clusterManifest.NodeTypes[0].Capacities = null;
            clusterManifest.NodeTypes[0].PlacementProperties = new KeyValuePairType[] 
            { 
                new KeyValuePairType() { Name = "NewPlacmentA", Value = "PlacementValueA" },
                new KeyValuePairType() { Name = "NewPlacmentB", Value = "PlacementValueB" },
            };

            FabricLayoutInfo updatedFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            var updatedClusterManfiest = updatedFabricLayoutInfo.ClusterManifest;
            updatedClusterManfiest.NodeTypes[0].Capacities = new KeyValuePairType[] { new KeyValuePairType() { Name = "NewCapacityName", Value = "10" } };
            updatedClusterManfiest.NodeTypes[0].PlacementProperties = new KeyValuePairType[] 
            { 
                new KeyValuePairType() { Name = "NewPlacmentA", Value = "PlacementValueA1" },
                new KeyValuePairType() { Name = "NewPlacmentC", Value = "PlacementValueC" },
            };

            VerifyCompareAndAnalyze(
                    clusterManifest,
                    updatedClusterManfiest,
                    null /*infrastructure*/,
                    4 /*expectedModifedStaticSettingsCount*/,
                    clusterManifest.NodeTypes[0].Name);

            FabricLayoutInfo updatedFabricLayoutInfo1 = new FabricLayoutInfo(this.imageStore);
            var updatedClusterManfiest1 = updatedFabricLayoutInfo1.ClusterManifest;
            updatedClusterManfiest1.NodeTypes[0].Capacities = null;
            updatedClusterManfiest1.NodeTypes[0].PlacementProperties = null;

            VerifyCompareAndAnalyze(
                    updatedClusterManfiest,
                    updatedClusterManfiest1,
                    null /*infrastructure*/,
                    3 /*expectedModifedStaticSettingsCount*/,
                    clusterManifest.NodeTypes[0].Name);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidSfssRgPolicySettings()
        {
            // Valid Rg policy settings
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            var clusterManifest = fabricLayoutInfo.ClusterManifest;
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessCpusetCpus", Value = "0,1,2,62,63" },
                new KeyValuePairType() { Name = "DCAProcessCpusetCpus", Value = "1" },
                new KeyValuePairType() { Name = "FabricProcessCpuShares", Value = "0" },
                new KeyValuePairType() { Name = "DCAProcessCpuShares", Value = "10000" },
                new KeyValuePairType() { Name = "FabricProcessMemoryInMB", Value = "2000" },
                new KeyValuePairType() { Name = "DCAProcessMemoryInMB", Value = "0" },
                new KeyValuePairType() { Name = "FabricProcessMemorySwapInMB", Value = "3000" },
                new KeyValuePairType() { Name = "DCAProcessMemorySwapInMB", Value = "100000" },
            };
            VerifyValidate(
                clusterManifest,
                null,
                "Validate valid SF system service resource governance settings",
                true);

            FabricLayoutInfo updatedFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            var updatedClusterManfiest = updatedFabricLayoutInfo.ClusterManifest;
            updatedClusterManfiest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessCpusetCpus", Value = "0,1,2" },
                new KeyValuePairType() { Name = "DCAProcessCpusetCpus", Value = "62,63" },
                new KeyValuePairType() { Name = "FabricProcessMemoryInMB", Value = "0" },
                new KeyValuePairType() { Name = "DCAProcessMemoryInMB", Value = "2000" },
            };
            VerifyValidate(
                updatedClusterManfiest,
                null,
                "Validate valid SF system service resource governance settings",
                true);

            VerifyCompareAndAnalyze(
                    clusterManifest,
                    updatedClusterManfiest,
                    null /*infrastructure*/,
                    0 /*expectedModifedStaticSettingsCount*/,
                    clusterManifest.NodeTypes[0].Name);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidSfssRgPolicySettings()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            var clusterManifest = fabricLayoutInfo.ClusterManifest;

            // CpusetCpus: Not-numeric value
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessCpusetCpus", Value = "1,core2" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // CpusetCpus: Negative value
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessCpusetCpus", Value = "-1,2" },
            };
            VerifyValidate(clusterManifest, null,"Invalid SFSS resource governance settings",false);

            // CpusetCpus: Not whole number
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessCpusetCpus", Value = "1,2.5" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // CpusetCpus: Value out of allowed range
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessCpusetCpus", Value = "62,64" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // CpuShares: Not-numeric value
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessCpuShares", Value = "shares2000" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // CpuShares: Negative value
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessCpuShares", Value = "-5000" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // CpuShares: Not whole number
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessCpuShares", Value = "5000.56" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // CpuShares: Value out of allowed range
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessCpuShares", Value = "20000" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // MemoryInMB: Not-numeric value
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessMemoryInMB", Value = "mem1000" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // MemoryInMB: Negative value
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessMemoryInMB", Value = "-1000" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // MemoryInMB: Not whole number
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessMemoryInMB", Value = "1000.56" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // MemoryInMB: Value out of allowed range (max is 100GB)
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessMemoryInMB", Value = "103000" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // MemorySwapInMB: Not-numeric value
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessMemorySwapInMB", Value = "mem1000" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // MemorySwapInMB: Negative value
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessMemorySwapInMB", Value = "-1000" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // MemorySwapInMB: Not whole number
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessMemorySwapInMB", Value = "1000.56" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);

            // MemorySwapInMB: Value out of allowed range (max is 1TB)
            clusterManifest.NodeTypes[0].SfssRgPolicies = new KeyValuePairType[]
            {
                new KeyValuePairType() { Name = "FabricProcessMemorySwapInMB", Value = "1050000" },
            };
            VerifyValidate(clusterManifest, null, "Invalid SFSS resource governance settings", false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestCertificatesChange()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            var clusterManifest = fabricLayoutInfo.ClusterManifest;
            clusterManifest.NodeTypes[0].Certificates = new CertificatesType();
            clusterManifest.NodeTypes[0].Certificates.ClientCertificate = new FabricCertificateType()
            {
                Name = "MyClientCert",
                X509FindType = FabricCertificateTypeX509FindType.FindByThumbprint,
                X509FindValue = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf"
            };
            clusterManifest.NodeTypes[0].Certificates.ClusterCertificate = new FabricCertificateType()
            {
                Name = "MyClusterCert",
                X509StoreName = X509Credentials.StoreNameDefault,
                X509FindType = FabricCertificateTypeX509FindType.FindByThumbprint,
                X509FindValue = "79 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf"
            };
            

            FabricLayoutInfo updatedFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            var updatedClusterManfiest = updatedFabricLayoutInfo.ClusterManifest;
            updatedClusterManfiest.NodeTypes[0].Certificates = new CertificatesType();
            updatedClusterManfiest.NodeTypes[0].Certificates.ClientCertificate = new FabricCertificateType()
            {
                Name = "MyCert",
                X509FindType = FabricCertificateTypeX509FindType.FindBySubjectName,
                X509FindValue = "SubjectName"
            };
            updatedClusterManfiest.NodeTypes[0].Certificates.ServerCertificate = new FabricCertificateType()
            {
                Name = "MyServerCert",
                X509StoreName = X509Credentials.StoreNameDefault,
                X509FindType = FabricCertificateTypeX509FindType.FindByThumbprint,
                X509FindValue = "79 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf"
            };

            VerifyCompareAndAnalyze(
                    clusterManifest,
                    updatedClusterManfiest,
                    null /*infrastructure*/,
                    0 /*expectedModifedStaticSettingsCount*/,
                    clusterManifest.NodeTypes[0].Name);

            FabricLayoutInfo updatedFabricLayoutInfo1 = new FabricLayoutInfo(this.imageStore);
            var updatedClusterManfiest1 = updatedFabricLayoutInfo1.ClusterManifest;
            updatedClusterManfiest1.NodeTypes[0].Certificates = null;

            VerifyCompareAndAnalyze(
                    updatedClusterManfiest,
                    updatedClusterManfiest1,
                    null /*infrastructure*/,
                    0 /*expectedModifedStaticSettingsCount*/,
                    clusterManifest.NodeTypes[0].Name);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEndpointsChange()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            var clusterManifest = fabricLayoutInfo.ClusterManifest;            
            clusterManifest.NodeTypes[0].Endpoints.ServiceConnectionEndpoint = new InternalEndpointType() { Port = "50000" };
            clusterManifest.NodeTypes[0].Endpoints.HttpGatewayEndpoint = null;
            clusterManifest.NodeTypes[0].Endpoints.ImageStoreServiceReplicatorEndpoint = new InternalEndpointType() { Port = "60000" };
            clusterManifest.NodeTypes[0].Endpoints.ApplicationEndpoints = new FabricEndpointsTypeApplicationEndpoints()
            {
                StartPort = 29000,
                EndPort = 30000
            };

            FabricLayoutInfo updatedFabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            var updatedClusterManifest = updatedFabricLayoutInfo.ClusterManifest;
            updatedClusterManifest.NodeTypes[0].Endpoints.ClientConnectionEndpoint.Port = "50000";
            updatedClusterManifest.NodeTypes[0].Endpoints.LeaseDriverEndpoint.Port = "50001";
            updatedClusterManifest.NodeTypes[0].Endpoints.ClusterConnectionEndpoint.Port = "50002";
            updatedClusterManifest.NodeTypes[0].Endpoints.ServiceConnectionEndpoint = new InternalEndpointType() { Port = "50003" };
            updatedClusterManifest.NodeTypes[0].Endpoints.HttpGatewayEndpoint = new InputEndpointType() { Port = "50004" };
            updatedClusterManifest.NodeTypes[0].Endpoints.ImageStoreServiceReplicatorEndpoint = null;
            updatedClusterManifest.NodeTypes[0].Endpoints.ApplicationEndpoints = new FabricEndpointsTypeApplicationEndpoints()
            {
                StartPort = 29500,
                EndPort = 30000
            };

            VerifyCompareAndAnalyze(
                    clusterManifest,
                    updatedClusterManifest,
                    null /*infrastructure*/,
                    7 /*expectedModifedStaticSettingsCount*/,
                    clusterManifest.NodeTypes[0].Name);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestNullApplicationEndpointsWithEndpointProviderEnabled()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
            fabricLayoutInfo.ClusterManifest.NodeTypes[0].Endpoints.ApplicationEndpoints = null;

            TestUtility.AddToFabricSettings(
               FabricValidatorConstants.SectionNames.Hosting,
               FabricValidatorConstants.ParameterNames.EndpointProviderEnabled,
               @"true",
               false,
               fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate null ApplicationEndpoints when EndpointProviderEnabled is true",
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidUpgradeServiceSection()
        {
            // Validate against ContinuousFailureWarningThreshold > ContinuousFailureErrorThreshold
            {
                FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);
                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.UpgradeService,
                    FabricValidatorConstants.ParameterNames.UpgradeService.ContinuousFailureWarningThreshold,
                    "5",
                    false,
                    fabricLayoutInfo.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.UpgradeService,
                    FabricValidatorConstants.ParameterNames.UpgradeService.ContinuousFailureErrorThreshold,
                    "3",
                    false,
                    fabricLayoutInfo.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo.ClusterManifest,
                    null,
                    "Validate against UpgradeSerivce.ContinuousFailureWarningThreshold greater than UpgradeService.ContinuousFailureErrorThreshold",
                    expectSuccess: false);
            }

            // Validate against ContinuousFailureErrorThreshold > ContinuousFailureFaultThreshold
            {
                FabricLayoutInfo fabricLayoutInfo1 = new FabricLayoutInfo(this.imageStore);
                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.UpgradeService,
                    FabricValidatorConstants.ParameterNames.UpgradeService.ContinuousFailureErrorThreshold,
                    "10",
                    false,
                    fabricLayoutInfo1.ClusterManifest);

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.UpgradeService,
                    FabricValidatorConstants.ParameterNames.UpgradeService.ContinuousFailureFaultThreshold,
                    "5",
                    false,
                    fabricLayoutInfo1.ClusterManifest);

                VerifyValidate(
                    fabricLayoutInfo1.ClusterManifest,
                    null,
                    "Validate against UpgradeSerivce.ContinuousFailureErrorThreshold greater than UpgradeService.ContinuousFailureFaultThreshold",
                    expectSuccess: false);
            }

        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("Shalabhs")]
        public void Verify_NativeRunConfiguration_When_EnableNativeReliableStateManager_Default_Expect_False()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate EnableNativeReliableStateManager is default (false)",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("Shalabhs")]
        public void Verify_NativeRunConfiguration_When_EnableNativeReliableStateManager_True_Expect_True()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.NativeRunConfiguration,
                FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager,
                "true",
                false,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate EnableNativeReliableStateManager is true",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("Shalabhs")]
        public void Verify_NativeRunConfiguration_When_EnableNativeReliableStateManager_False_Expect_False()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.NativeRunConfiguration,
                FabricValidatorConstants.ParameterNames.NativeRunConfiguration.EnableNativeReliableStateManager,
                "false",
                false,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate EnableNativeReliableStateManager is false",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("Shalabhs")]
        public void Verify_NativeRunConfiguration_When_LoadEnableNativeReliableStateManager_Default_Expect_False()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate Test_LoadEnableNativeReliableStateManager is default (false)",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("Shalabhs")]
        public void Verify_NativeRunConfiguration_When_LoadEnableNativeReliableStateManager_True_Expect_True()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.NativeRunConfiguration,
                FabricValidatorConstants.ParameterNames.NativeRunConfiguration.Test_LoadEnableNativeReliableStateManager,
                "true",
                false,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate Test_LoadEnableNativeReliableStateManager is true",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("Shalabhs")]
        public void Verify_NativeRunConfiguration_When_LoadEnableNativeReliableStateManager_False_Expect_False()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.NativeRunConfiguration,
                FabricValidatorConstants.ParameterNames.NativeRunConfiguration.Test_LoadEnableNativeReliableStateManager,
                "false",
                false,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate Test_LoadEnableNativeReliableStateManager is false",
                expectSuccess: true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vivekagg,dariopb")]
        public void Verify_DnServicePartitionQuerySettings()
        {
            string prefixStr = null;
            string suffixStr = null;
            bool exceptionExpected = false;

            // Test data format : prefixString:SuffixString:ExceptedExceptionValue
            string[] TestData = {
                "--:--:false",
                "-:-:false",
                "prefi:suffi:false",
                "test::false",
                "::true",
                "a.b:--:true",
                "--:b.a:true",
                "prefix:suffix:true",
                ":test:true",
                "^:--:true",
            };

            for (uint i = 0; i < TestData.Length; i++)
            {
                string[] content = TestData[i].Split(':');

                prefixStr = content[0];
                suffixStr = content[1];
                exceptionExpected = Convert.ToBoolean(content[2]);
                bool exceptionHappened = false;

                try
                {
                    DnsServiceConfigurationValidator.ValidatePrefixAndSuffix(prefixStr, suffixStr);
                }
                catch (Exception)
                {
                    exceptionHappened = true;
                }

                Verify.AreEqual(exceptionExpected, exceptionHappened, 
                    "Exception value mistmatch for TestData " + TestData[i] + ", Expected: " +
                    exceptionExpected.ToString() + " , Actual: " + exceptionHappened.ToString() );
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vivekagg,dariopb")]
        public void Verify_DnsServiceConfigurationValidatorRunsAsPartofFabricValidator()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DnsService,
                FabricValidatorConstants.ParameterNames.IsEnabled, 
                "true",
                false,
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DnsService,
                FabricValidatorConstants.ParameterNames.DNSService.PartitionPrefix,
                "",
                false,
                fabricLayoutInfo.ClusterManifest);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DnsService,
                FabricValidatorConstants.ParameterNames.DNSService.PartitionSuffix,
                "suff",
                false,
                fabricLayoutInfo.ClusterManifest);

            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate DnsService configuration validator runs as part of FabricValidator",
                expectSuccess: false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vivekagg,dariopb")]
        public void Verify_DefaultDnsServiceConfigurationSettings()
        {
            FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore);

            TestUtility.AddToFabricSettings(
                FabricValidatorConstants.SectionNames.DnsService,
                FabricValidatorConstants.ParameterNames.IsEnabled,
                "true",
                false,
                fabricLayoutInfo.ClusterManifest);


            VerifyValidate(
                fabricLayoutInfo.ClusterManifest,
                null,
                "Validate default DnsService configuration settings",
                expectSuccess: true);
        }

        private static void UpdateWithEncryptedImageStoreConnectionString(
            ClusterManifestType clusterManifest, 
            string imageStoreConnectionString, 
            string certificateThumbprint, 
            string certificateStoreName)
        {
            var connectionStringParameter = clusterManifest.FabricSettings.First(section => section.Name.Equals("Management")).Parameter.First(parameter => parameter.Name.Equals("ImageStoreConnectionString"));

            string encryptedConnectionString = NativeConfigStore.EncryptText(
                imageStoreConnectionString,
                certificateThumbprint,
                certificateStoreName);

            connectionStringParameter.Value = encryptedConnectionString;
            connectionStringParameter.IsEncrypted = true;

            AppTrace.TraceSource.WriteInfo(encryptedConnectionString);

            clusterManifest.Certificates = new ClusterManifestTypeCertificates();
            clusterManifest.Certificates.SecretsCertificate = new FabricCertificateType();
            clusterManifest.Certificates.SecretsCertificate.X509FindType = FabricCertificateTypeX509FindType.FindByThumbprint;
            clusterManifest.Certificates.SecretsCertificate.X509FindValue = certificateThumbprint;
            clusterManifest.Certificates.SecretsCertificate.X509StoreName = certificateStoreName;
        }

        private static void VerifyValidate(
            ClusterManifestType clusterManifest,
            List<InfrastructureNodeType> infrastructure,
            string message,
            bool expectSuccess)
        {
            Exception exception = null;

            try
            {
                FabricValidator validator = FabricValidator.Create(clusterManifest, infrastructure, new WindowsFabricSettings(clusterManifest), DeploymentOperations.Update);
                validator.Validate();
            }
            catch (Exception e)
            {
                exception = e;
            }

            Assert.AreEqual(expectSuccess, exception == null, message + " " + exception);
        }

        private static void VerifyCompareAndAnalyze(
            ClusterManifestType currentClusterManifest,
            ClusterManifestType targetClusterManifest,
            List<InfrastructureNodeType> infrastructure,
            int expectedModifedStaticSettingsCount,
            string nodeTypeName = null)
        {
            FabricValidator validator = FabricValidator.Create(currentClusterManifest, infrastructure, new WindowsFabricSettings(currentClusterManifest), DeploymentOperations.Update);
            validator.Validate();
            
            IEnumerable<KeyValuePair<string, string>> modifiedStaticSettings = validator.CompareAndAnalyze(targetClusterManifest, nodeTypeName);
            
            Verify.AreEqual(expectedModifedStaticSettingsCount, modifiedStaticSettings.Count());
        }
    }
}