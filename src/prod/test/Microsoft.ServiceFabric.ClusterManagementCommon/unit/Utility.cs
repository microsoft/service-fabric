// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Newtonsoft.Json;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Security.Cryptography.X509Certificates;
    using System.Text.RegularExpressions;
    using System.Xml.Serialization;

    internal class Utility
    {
        internal static MockupJsonModel CreateMockupJsonModel(
            int nodeTypeCount,
            int vmInstancePerNodeType,
            List<SettingsSectionDescription> fabricSettings,
            Security security = null,
            EncryptableDiagnosticStoreInformation diagnosticsStore = null,
            CertificateDescription certificate = null,
            DiagnosticsStorageAccountConfig diagnosticsStorageAccountConfig = null)
        {
            MockupJsonModel jsonModel = new MockupJsonModel();
            jsonModel.Name = "TestJsonConfig";
            jsonModel.ClusterConfigurationVersion = "1.0.0";
            jsonModel.ApiVersion = "2016-09-26";
            jsonModel.Nodes = new List<NodeDescription>();

            jsonModel.Properties = new MockupProperty();
            jsonModel.Properties.DiagnosticsStore = diagnosticsStore;
            jsonModel.Properties.FabricSettings = fabricSettings;
            jsonModel.Properties.Security = security;
            jsonModel.Properties.NodeTypes = new List<NodeTypeDescription>();
            jsonModel.Properties.DiagnosticsStorageAccountConfig = diagnosticsStorageAccountConfig;

            for (int i = 0; i < nodeTypeCount; i++)
            {
                jsonModel.Properties.NodeTypes.Add(CreateNodeType("NodeType" + i, vmInstancePerNodeType, i == 0));
            }

            for (int j = 0; j < vmInstancePerNodeType * nodeTypeCount; j++)
            {
                jsonModel.Nodes.Add(CreateNodeDescription("Node" + j, "NodeType0", "localhost", "fd:/dc1/r0", "UD0"));
            }

            return jsonModel;
        }

        internal static DiagnosticsStorageAccountConfig CreateDiagnosticsStorageAccountConfig()
        {
            DiagnosticsStorageAccountConfig diagnostic = new DiagnosticsStorageAccountConfig
            {
                StorageAccountName = "diagnosticStorageAccountName",
                PrimaryAccessKey = "[PrimaryAccessKey]",
                SecondaryAccessKey = "[SecondaryAccessKey]",
                BlobEndpoint = "https://diagnosticStorageAccountName.blob.core.windows.net/",
                QueueEndpoint = "https://diagnosticStorageAccountName.queue.core.windows.net/",
                TableEndpoint = "https://diagnosticStorageAccountName.table.core.windows.net/"
            };

            return diagnostic;
        }

        internal static Security CreateSecurity()
        {
            Security security = new Security
            {
                ClusterCredentialType = CredentialType.X509,
                ServerCredentialType = CredentialType.X509,
                CertificateInformation = new X509()
            };

            security.CertificateInformation.ClusterCertificate = new CertificateDescription
            {
                Thumbprint = "59EC792004C56225DD6691132C713194D28098F2",
                ThumbprintSecondary = "59EC792004C56225DD6691132C713194D28098F3",
                X509StoreName = StoreName.My
            };

            security.CertificateInformation.ServerCertificate = new CertificateDescription
            {
                Thumbprint = "59EC792004C56225DD6691132C713194D28098F2",
                ThumbprintSecondary = "59EC792004C56225DD6691132C713194D28098F3",
                X509StoreName = StoreName.My
            };

            security.CertificateInformation.ClientCertificateThumbprints = new List<ClientCertificateThumbprint>();

            security.CertificateInformation.ClientCertificateThumbprints.Add(new ClientCertificateThumbprint
            {
                CertificateThumbprint = "59EC792004C56225DD6691132C713194D28098F3",
                IsAdmin = false
            });

            security.CertificateInformation.ClientCertificateCommonNames = new List<ClientCertificateCommonName>();

            security.CertificateInformation.ClientCertificateCommonNames.Add(new ClientCertificateCommonName
            {
                CertificateCommonName = "hiahiaCn",
                CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F3",
                IsAdmin = true
            });

            security.CertificateInformation.ReverseProxyCertificate = new CertificateDescription
            {
                Thumbprint = "59EC792004C56225DD6691132C713194D28098F4",
                ThumbprintSecondary = "59EC792004C56225DD6691132C713194D28098F5",
                X509StoreName = StoreName.My
            };

            return security;
        }

        internal static NodeDescription CreateNodeDescription(string nodeName, string nodeTypeRef, string iPAddress, string faultDomain, string upgradeDomain)
        {
            NodeDescription nodeDescription = new NodeDescription();
            nodeDescription.NodeName = nodeName;
            nodeDescription.NodeTypeRef = nodeTypeRef;
            nodeDescription.IPAddress = iPAddress;
            nodeDescription.FaultDomain = faultDomain;
            nodeDescription.UpgradeDomain = upgradeDomain;

            return nodeDescription;
        }

        internal static NodeTypeDescription CreateNodeType(string nodeTypeName, int vmInstanceCount, bool isPrimary = false, DurabilityLevel durabilityLevel = DurabilityLevel.Bronze)
        {
            NodeTypeDescription nodeType = new NodeTypeDescription();
            nodeType.Name = nodeTypeName;
            nodeType.IsPrimary = isPrimary;
            nodeType.ClientConnectionEndpointPort = 19000;
            nodeType.HttpApplicationGatewayEndpointPort = 19080;
            nodeType.ApplicationPorts = new EndpointRangeDescription() { StartPort = 20000, EndPort = 30000 };
            nodeType.EphemeralPorts = new EndpointRangeDescription() { StartPort = 49100, EndPort = 65500 };
            nodeType.VMInstanceCount = vmInstanceCount;
            nodeType.DurabilityLevel = durabilityLevel;

            return nodeType;
        }

        /** Remove the random guid ClusterId, PrimaryAccountNTLMPasswordSecret and SecondaryAccountNTLMPasswordSecret, then compare two xml strings. */
        internal static string RemoveRandomGuid(string dirtyString)
        {
            string pattern = "[a-zA-Z0-9]{8}-[a-zA-Z0-9]{4}-[a-zA-Z0-9]{4}-[a-zA-Z0-9]{4}-[a-zA-Z0-9]{12}";
            return Regex.Replace(dirtyString, pattern, "");
        }

        /** Inputs a ClusterManifestType object; returns an corresponding string. */
        internal static string GetClusterManifestXMLString(ClusterManifestType clusterManifest)
        {
            XmlSerializer xmlSerializer = new XmlSerializer(typeof(ClusterManifestType));
            StringWriter generatedClusterManifestWriter = new StringWriter();
            xmlSerializer.Serialize(generatedClusterManifestWriter, clusterManifest);
            return generatedClusterManifestWriter.ToString();
        }

        internal static MockupJsonModel GetJsonConfigFromFile(string jsonConfigPath)
        {
            string jsonString = File.ReadAllText(jsonConfigPath);
            return DeserializeJsonConfig<MockupJsonModel>(jsonString);
        }

        internal static MockupCluster PopulateClusterWithBaselineJson(string jsonFilePath)
        {
            MockupJsonModel jsonConfig = Utility.GetJsonConfigFromFile(Path.Combine(Constant.TestDirectory, jsonFilePath));

            var userConfig = jsonConfig.GetUserConfig();
            var clusterTopology = jsonConfig.GetClusterTopology();
            var adminConfig = new MockupAdminConfig();
            var logger = new MockupTraceLogger();

            return new MockupCluster(
                adminConfig,
                userConfig,
                clusterTopology,
                logger);
        }

        /** Update the CSM config of a cluster */
        internal static void UpdateCluster(string targetJsonFilePath, MockupCluster existingCluster)
        {
            MockupJsonModel targetJsonConfig = Utility.GetJsonConfigFromFile(Path.Combine(Constant.TestDirectory, targetJsonFilePath));
            existingCluster.TargetCsmConfig = targetJsonConfig.GetUserConfig();
            existingCluster.Topology = targetJsonConfig.GetClusterTopology();
        }

        /** Update the WRP config of a cluster */
        internal static void UpdateCluster(MockupCluster existingCluster, string msiVersion, string clusterSettingsVersion, bool isUserSet=false)
        {
            existingCluster.TargetWrpConfig = new MockupAdminConfig(isUserSet);
            existingCluster.TargetWrpConfig.Version = new AdminConfigVersion(msiVersion, clusterSettingsVersion);
        }

        internal static MockupCluster DoBaselineUpgrade(string jsonFilePath)
        {
            MockupCluster result = Utility.PopulateClusterWithBaselineJson(jsonFilePath);
            Assert.IsTrue(result.RunStateMachine());
            Utility.RollForwardOneIteration(result);
            Assert.IsFalse(result.RunStateMachine());
            Assert.IsNull(result.Pending);
            Assert.IsNotNull(result.Current);

            return result;
        }

        internal static void RollForwardOneIteration(MockupCluster cluster)
        {
            cluster.ClusterUpgradeCompleted();
        }

        internal static void RollBackOneIteration(ICluster cluster)
        {
            cluster.ClusterUpgradeRolledBackOrFailed(DateTime.UtcNow.ToString(), ClusterUpgradeFailureReason.Unknown);
        }

        private static T DeserializeJsonConfig<T>(string json)
        {
            return JsonConvert.DeserializeObject<T>(
                json,
                new JsonSerializerSettings()
                {
                    DefaultValueHandling = DefaultValueHandling.Populate,
                    PreserveReferencesHandling = PreserveReferencesHandling.None
                });
        }

        public static string FindFabricResourceFile(string filename)
        {
            string filePath = filename;
            string fabricCodePath = null;
            string assemblyCommonPath = null;
            if (File.Exists(filePath))
            {
                return filePath;
            }
            else
            {
                fabricCodePath = GetFabricCodePath();
                if (fabricCodePath != null)
                {
                    filePath = Path.Combine(fabricCodePath, filename);
                    if (File.Exists(filePath))
                    {
                        return filePath;
                    }
                }

                assemblyCommonPath = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                filePath = Path.Combine(assemblyCommonPath, filename);
                if (File.Exists(filePath))
                {
                    return filePath;
                }
            }

            throw new FileNotFoundException("Can't find Fabric resource: wrpSettings.xml");
        }

        internal static string GetFabricCodePath()
        {
            try
            {
                return FabricEnvironment.GetCodePath();
            }
            catch (System.Fabric.FabricException)
            {
                return null;
            }
        }
    }
}