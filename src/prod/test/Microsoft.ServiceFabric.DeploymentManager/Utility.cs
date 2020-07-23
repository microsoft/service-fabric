// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using KeyVault.Wrapper;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Newtonsoft.Json;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Security.Cryptography.X509Certificates;

    using CertInfo = System.Tuple<string, string, string>;

    internal static class Utility
    {
        internal static StandAloneInstallerJSONModelAugust2017 CreateMockupStandAloneJsonModel(
            string name,
            string clusterConfigurationVersion,
            List<SettingsSectionDescription> fabricSettings = null,
            SecurityMay2017 security = null,
            EncryptableDiagnosticStoreInformation diagnosticsStore = null,
            List<NodeDescriptionGA> nodes = null)
        {
            StandAloneInstallerJSONModelAugust2017 jsonModel = new StandAloneInstallerJSONModelAugust2017();
            jsonModel.Name = name;
            jsonModel.ClusterConfigurationVersion = clusterConfigurationVersion;
            jsonModel.ApiVersion = "08-2017";
            jsonModel.Nodes = new List<NodeDescriptionGA>();

            jsonModel.Properties = new PropertyAugust2017();
            jsonModel.Properties.DiagnosticsStore = diagnosticsStore;
            jsonModel.Properties.FabricSettings = fabricSettings;
            jsonModel.Properties.Security = security;
            jsonModel.Properties.NodeTypes = new List<NodeTypeDescriptionAugust2017>();

            for (int i = 0; i < 5; i++)
            {
                jsonModel.Properties.NodeTypes.Add(CreateNodeType("NodeType" + i, i == 0));

                if (nodes == null)
                {
                    jsonModel.Nodes.Add(CreateNodeDescription("Node" + i.ToString(), "NodeType" + i, "machine" + i, "fd:/dc1/r" + i, "UD" + i));
                }
            }

            if (nodes != null)
            {
                jsonModel.Nodes = nodes;
            }

            if (fabricSettings == null)
            {
                fabricSettings = new List<SettingsSectionDescription>();
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

        internal static SecurityMay2017 CreateWindowsSecurity()
        {
            List <ClientIdentity> clientIdentities =  new List<ClientIdentity>();
            ClientIdentity clientIdentity = new ClientIdentity();
            clientIdentity.Identity = "domain\\username";
            clientIdentity.IsAdmin = true;
            clientIdentities.Add(clientIdentity);

            return new SecurityMay2017
            {
                ClusterCredentialType = CredentialType.Windows,
                ServerCredentialType = CredentialType.Windows,
                WindowsIdentities = new WindowsJanuary2017() {
                    ClientIdentities = clientIdentities,
                    ClusterIdentity = "domain\\machinegroup"
                }
            };
        }

        internal static SecurityMay2017 CreateGMSASecurity()
        {
            List<ClientIdentity> clientIdentities = new List<ClientIdentity>();
            ClientIdentity clientIdentity = new ClientIdentity();
            clientIdentity.Identity = "domain\\username";
            clientIdentity.IsAdmin = true;
            clientIdentities.Add(clientIdentity);

            return new SecurityMay2017
            {
                WindowsIdentities = new WindowsJanuary2017()
                {
                    ClustergMSAIdentity = "accountname@fqdn",
                    ClusterSPN = "fqdn",
                    ClientIdentities = clientIdentities
                }
            };
        }

        internal static NodeDescriptionGA CreateNodeDescription(string nodeName, string nodeTypeRef, string iPAddress, string faultDomain, string upgradeDomain)
        {
            NodeDescriptionGA nodeDescription = new NodeDescriptionGA();
            nodeDescription.NodeName = nodeName;
            nodeDescription.NodeTypeRef = nodeTypeRef;
            nodeDescription.IPAddress = iPAddress;
            nodeDescription.FaultDomain = faultDomain;
            nodeDescription.UpgradeDomain = upgradeDomain;

            return nodeDescription;
        }

        internal static NodeTypeDescriptionAugust2017 CreateNodeType(string nodeTypeName, bool isPrimary)
        {
            NodeTypeDescriptionAugust2017 nodeType = new NodeTypeDescriptionAugust2017();
            nodeType.Name = nodeTypeName;
            nodeType.IsPrimary = isPrimary;
            nodeType.ClientConnectionEndpointPort = 19000;
            nodeType.HttpGatewayEndpointPort = 19003;
            nodeType.ApplicationPorts = new EndpointRangeDescription() { StartPort = 20000, EndPort = 30000 };
            nodeType.EphemeralPorts = new EndpointRangeDescription() { StartPort = 49100, EndPort = 65500 };

            return nodeType;
        }

        // thumbprint, keyvault name, CN
        internal static readonly CertInfo[] installedValidCertificates = new CertInfo[]
        {
            new CertInfo("52C50E3C286B034EEAC8405D9661E211B55A9792", "Certificate-SaValidationMyValidCert1", "MyValidCert"),
            new CertInfo("E7AB9AC4F463886A0A905B89AE724DBDE1898021", "Certificate-SaValidationMyValidCert2", "MyValidCert2"),
            new CertInfo("958742894A00579698B612D829E138780D2A0245", "Certificate-SaValidationMyValidCert3", "MyValidCert3"),
            new CertInfo("F7CF27BE28BB76EFEAB956BEF431A08810B67E68", "Certificate-SaValidationMyValidCert4", "MyValidCert4"),
        };

        internal static readonly CertInfo[] installedInvalidCertificates = new CertInfo[]
        {
            new CertInfo("973E86EA5A375C382B22C4AF2B9AA5CAEBCB7228", "Certificate-SaValidationMyExpiredCert1", "MyExpiredCert"),
            new CertInfo("C199B6D1EB005747CCD137870921AB613365C84C", "Certificate-SaValidationMyExpiredCert2", "MyExpiredCert2"),
        };

        internal static readonly string[] uninstalledCertificates = new string[2]
        {
            "72C50E3C286B034EEAC8405D9661E211B55A9798",
            "82C50E3C286B034EEAC8405D9661E211B55A9798"
        };

        internal static readonly CertInfo[] installedCertificates = installedValidCertificates
            .Union(installedInvalidCertificates).ToArray();

        internal static string TestDirectory = new FileInfo(Assembly.GetExecutingAssembly().Location).DirectoryName;

        static Utility()
        {
            Directory.SetCurrentDirectory(Utility.TestDirectory);
        }

        internal static void RollForwardOneIteration(ICluster cluster)
        {
            cluster.ClusterUpgradeCompleted();
        }

        internal static void RollBackOneIteration(ICluster cluster)
        {
            cluster.ClusterUpgradeRolledBackOrFailed(DateTime.UtcNow.ToString(), ClusterUpgradeFailureReason.Unknown);
        }

        internal static bool RunStateMachine(ICluster cluster)
        {
            Utility.VerifyEndpointV2Enabled(cluster);
            Utility.VerifyUseSecondaryIfNewerEnabled(cluster);
            bool result = cluster.RunStateMachine();
            Utility.VerifyEndpointV2Enabled(cluster);
            Utility.VerifyUseSecondaryIfNewerEnabled(cluster);

            return result;
        }

        internal static StandAloneCluster PopulateStandAloneClusterWithBaselineJson(string jsonFilePath)
        {
            StandAloneInstallerJsonModelBase jsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(Path.Combine(Utility.TestDirectory, jsonFilePath));

            var userConfig = jsonConfig.GetUserConfig();
            var clusterTopology = jsonConfig.GetClusterTopology();
            var adminConfig = new StandaloneAdminConfig();
            var logger = new StandAloneTraceLogger("StandAloneDeploymentManager");

            string customizedClusterId = jsonConfig.GetClusterRegistrationId();
            return new StandAloneCluster(
                adminConfig,
                userConfig,
                clusterTopology,
                customizedClusterId ?? "acf4dc93-9b64-4504-8b6f-0b7fd052e096",
                logger);
        }

        internal static void UpdateStandAloneCluster(string targetJsonFilePath, StandAloneCluster existingCluster, bool isUserSet = false)
        {
            StandAloneInstallerJsonModelBase targetJsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(Path.Combine(Utility.TestDirectory, targetJsonFilePath));
            StandaloneSettingsValidator validator = new StandaloneSettingsValidator(targetJsonConfig);
            existingCluster.TargetCsmConfig = validator.ClusterProperties;
            existingCluster.Topology = validator.Topology;

            if (!string.IsNullOrEmpty(validator.ClusterProperties.CodeVersion))
            {
                var adminConfigVersion = new StandaloneAdminConfig(null, isUserSet);
                adminConfigVersion.Version.MsiVersion = validator.ClusterProperties.CodeVersion;
                existingCluster.TargetWrpConfig = adminConfigVersion;
            }
        }

        internal static void UpdateStandAloneClusterWRPSettings(string clusterSettingsFilePath, string version, StandAloneCluster existingCluster)
        {
            string fabricSettingFilePath = StandaloneUtility.FindFabricResourceFile("Configurations.csv");
            FabricSettingsMetadata fabricSettingsMetadata = FabricSettingsMetadata.Create(fabricSettingFilePath);
            AdminConfigVersion adminConfigVersion = new AdminConfigVersion("Baseline", version);

            StandAloneClusterManifestSettings standAloneClusterManifestSettings = JsonConvert.DeserializeObject<StandAloneClusterManifestSettings>(
                        File.ReadAllText(clusterSettingsFilePath));

            existingCluster.TargetWrpConfig = new StandaloneAdminConfig(fabricSettingsMetadata, standAloneClusterManifestSettings, adminConfigVersion, false);
        }

        internal static void UpdateStandAloneClusterAutoWrpUpgrade(StandAloneCluster existingCluster, string msiCodeVersion)
        {
            var adminConfigVersion = new StandaloneAdminConfig(null, false);
            adminConfigVersion.Version.MsiVersion = msiCodeVersion;
            existingCluster.TargetWrpConfig = adminConfigVersion;
        }

        internal static void UpdateStandAloneClusterForDisableNode(string targetJsonFilePath, StandAloneCluster existingCluster)
        {
            StandAloneInstallerJsonModelBase targetJsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(Path.Combine(Utility.TestDirectory, targetJsonFilePath));
            StandaloneSettingsValidator validator = new StandaloneSettingsValidator(targetJsonConfig);
            existingCluster.TargetCsmConfig = validator.ClusterProperties;

            List<NodeStatus> nodeStatus = new List<NodeStatus>();

            foreach (var node in existingCluster.Current.NodeConfig.NodesStatus)
            {
                NodeStatus ns = new NodeStatus();
                ns.NodeName = node.NodeName;
                ns.NodeState = targetJsonConfig.Nodes.Select(n => n.NodeName).Contains(node.NodeName) ? NodeState.Enabled : NodeState.Disabling;
                ns.NodeDeactivationIntent = targetJsonConfig.Nodes.Select(n => n.NodeName).Contains(node.NodeName) ? WrpNodeDeactivationIntent.Invalid : WrpNodeDeactivationIntent.RemoveNode;
                ns.NodeType = node.NodeType;
                ns.InstanceId = 0;
                nodeStatus.Add(ns);
            }

            existingCluster.TargetNodeConfig = new ClusterNodeConfig
            {
                NodesStatus = nodeStatus,
                Version = 2,
                IsUserSet = true
            };
        }

        internal static void UpdateStandAloneClusterForAddNode(string targetJsonFilePath, StandAloneCluster existingCluster)
        {
            StandAloneInstallerJsonModelBase targetJsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(Path.Combine(Utility.TestDirectory, targetJsonFilePath));
            StandaloneSettingsValidator validator = new StandaloneSettingsValidator(targetJsonConfig);
            existingCluster.TargetCsmConfig = validator.ClusterProperties;
            existingCluster.Topology = validator.Topology;
            List<NodeStatus> nodeStatus = new List<NodeStatus>();

            foreach (var node in targetJsonConfig.Nodes)
            {
                NodeStatus ns = new NodeStatus();
                ns.NodeName = node.NodeName;
                ns.NodeState = NodeState.Enabled;
                ns.NodeDeactivationIntent = WrpNodeDeactivationIntent.Invalid;
                ns.NodeType = node.NodeTypeRef;
                ns.InstanceId = 0;
                nodeStatus.Add(ns);
            }

            existingCluster.TargetNodeConfig = new ClusterNodeConfig
            {
                NodesStatus = nodeStatus,
                Version = 2
            };
        }

        internal static void UpdateStandAloneClusterForRemoveNode(string targetJsonFilePath, StandAloneCluster existingCluster)
        {
            StandAloneInstallerJsonModelBase targetJsonConfig = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(Path.Combine(Utility.TestDirectory, targetJsonFilePath));
            StandaloneSettingsValidator validator = new StandaloneSettingsValidator(targetJsonConfig);
            existingCluster.TargetCsmConfig = validator.ClusterProperties;
            existingCluster.Topology = validator.Topology;
            List<NodeStatus> nodeStatus = new List<NodeStatus>();

            foreach (var node in existingCluster.Current.NodeConfig.NodesStatus)
            {
                NodeStatus ns = new NodeStatus();
                ns.NodeName = node.NodeName;
                ns.NodeState = targetJsonConfig.Nodes.Select(n => n.NodeName).Contains(node.NodeName) ? NodeState.Enabled : NodeState.Removed;
                ns.NodeDeactivationIntent = targetJsonConfig.Nodes.Select(n => n.NodeName).Contains(node.NodeName) ? WrpNodeDeactivationIntent.Invalid : WrpNodeDeactivationIntent.RemoveNode;
                ns.NodeType = node.NodeType;
                ns.InstanceId = 0;
                nodeStatus.Add(ns);
            }

            existingCluster.TargetNodeConfig = new ClusterNodeConfig
            {
                NodesStatus = nodeStatus,
                Version = 3
            };
        }

        internal static StandAloneCluster DoBaselineUpgrade(string jsonFilePath)
        {
            StandAloneCluster result = Utility.PopulateStandAloneClusterWithBaselineJson(jsonFilePath);
            Assert.IsTrue(result.RunStateMachine());
            Utility.RollForwardOneIteration(result);
            Assert.IsFalse(result.RunStateMachine());
            Assert.IsNull(result.Pending);
            Assert.IsNotNull(result.Current);

            return result;
        }

        public static void InstallCertificates(IEnumerable<string> keyVaultSecretNames)
        {
            IEnumerable<X509Certificate2> certs = keyVaultSecretNames.Select(secretName => LoadKeyVaultCertificate(secretName));

            ProcessCertificates(new Action<X509Store, X509Certificate2>(
                (store, cert) =>
                {
                    store.Add(cert);
                }),
                certs);
        }

        public static void UninstallCertificates(IEnumerable<string> thumbprints)
        {
            IEnumerable<X509Certificate2> installedCerts;
            X509CertificateUtility.TryFindCertificate("localhost", StoreName.My, thumbprints, X509FindType.FindByThumbprint, out installedCerts);

            ProcessCertificates(new Action<X509Store, X509Certificate2>(
                (store, cert) =>
                {
                    store.Remove(cert);
                }),
                installedCerts);
        }

        public static void VerifyEndpointV2Enabled(ICluster cluster)
        {
            List<ClusterManifestType> manifests = new List<ClusterManifestType>();
            if (cluster.Pending != null)
            {
                manifests.Add(cluster.Pending.ExternalState.ClusterManifest);
            }

            if (cluster.Current != null)
            {
                manifests.Add(cluster.Current.ExternalState.ClusterManifest);
            }

            foreach (ClusterManifestType manifest in manifests)
            {
                Assert.IsTrue(manifest.FabricSettings.Any(
                section => section.Name == StringConstants.SectionName.Common
                && section.Parameter.Any(
                parameter => parameter.Name == StringConstants.ParameterName.EnableEndpointV2 && parameter.Value == true.ToString())));
            }
        }

        public static void VerifyUseSecondaryIfNewerEnabled(ICluster cluster)
        {
            List<ClusterManifestType> manifests = new List<ClusterManifestType>();
            if (cluster.Pending != null)
            {
                manifests.Add(cluster.Pending.ExternalState.ClusterManifest);
            }

            if (cluster.Current != null)
            {
                manifests.Add(cluster.Current.ExternalState.ClusterManifest);
            }

            if (!manifests.Any())
            {
                return;
            }
            
            SettingsOverridesTypeSection secureSection = manifests[0].FabricSettings.First(section => section.Name == StringConstants.SectionName.Security);
            if (!secureSection.Parameter.Any(p => p.Name == StringConstants.ParameterName.ClusterCredentialType
                && p.Value == CredentialType.X509.ToString()))
            {
                return;
            }

            foreach (ClusterManifestType manifest in manifests)
            {
                Assert.IsTrue(manifest.FabricSettings.Any(
                section => section.Name == StringConstants.SectionName.Security
                && section.Parameter.Any(
                parameter => parameter.Name == StringConstants.ParameterName.UseSecondaryIfNewer && parameter.Value == true.ToString())));
            }
        }

        internal static void ProcessCertificates(Action<X509Store, X509Certificate2> action, IEnumerable<X509Certificate2> certs)
        {
            X509Store store = null;

            try
            {
                store = new X509Store(StoreName.My, StoreLocation.LocalMachine);
                store.Open(OpenFlags.ReadWrite);

                foreach (X509Certificate2 cert in certs)
                {
                    action(store, cert);
                }
            }
            finally
            {
                if (store != null)
                {
                    store.Close();
                }
            }
        }

        internal static void ValidateExpectedValidationException(Action action, ClusterManagementErrorCode? expectedErrorCode)
        {
            try
            {
                action();
                if (expectedErrorCode.HasValue)
                {
                    Assert.Fail(String.Format("ValidationException '{0}' was expected, but not thrown.", expectedErrorCode));
                }
            }
            catch (ValidationException ex)
            {
                if (!expectedErrorCode.HasValue)
                {
                    throw;
                }
                else if (ex.Code != expectedErrorCode)
                {
                    throw;
                }
            }
        }

        internal static X509Certificate2 LoadKeyVaultCertificate(string secretName)
        {
            KeyVaultWrapper kvw = KeyVaultWrapper.GetTestInfraKeyVaultWrapper();
            string keyVaultUri = KeyVaultWrapper.GetTestInfraKeyVaultUri();
            X509Certificate2 cert = kvw.GetCertificateFromSecret(keyVaultUri, secretName);
            var certBytes = cert.Export(X509ContentType.Pfx);
            return new X509Certificate2(certBytes);
        }

        internal static void SetTargetReplicaSetSize(ClusterUpgradeStateBase upgradeState)
        {
            var targetReplicaSetSize = new Dictionary<string, ReplicaSetSize>() { { "", new ReplicaSetSize() } };
            StandAloneAutoScaleClusterUpgradeState autoScaleUpgradeState = (StandAloneAutoScaleClusterUpgradeState)upgradeState;
            autoScaleUpgradeState.TargetSystemServicesSize = new Dictionary<string, ReplicaSetSize>();
            autoScaleUpgradeState.GetSystemServiceReplicaSetSize(targetReplicaSetSize);
        }

        internal static StandAloneCluster DeserealizeUsingDefaults(string normalizedString)
        {
            JsonSerializerSettings settings = new JsonSerializerSettings
            {
                ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                NullValueHandling = NullValueHandling.Ignore,
                TypeNameHandling = TypeNameHandling.Objects
            };

            return JsonConvert.DeserializeObject<StandAloneCluster>(normalizedString, settings);
        }

        internal static StandAloneCluster DeserealizeUsingCustomDeserealizer(string normalizedString)
        {
            JsonSerializerSettings settings = new JsonSerializerSettings
            {
                ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                NullValueHandling = NullValueHandling.Ignore,
                TypeNameHandling = TypeNameHandling.None
            };
            settings.Converters.Add(new StandAloneClusterJsonDeserializer());
            return JsonConvert.DeserializeObject<StandAloneCluster>(normalizedString, settings);
        }

        internal static string SerealizeUsingDefaults(StandAloneCluster cluster)
        {
            return JsonConvert.SerializeObject(
                    cluster,
                    new JsonSerializerSettings
                    {
                        Formatting = Newtonsoft.Json.Formatting.Indented,
                        ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                        NullValueHandling = NullValueHandling.Ignore,
                        TypeNameHandling = TypeNameHandling.Objects,
                        PreserveReferencesHandling = PreserveReferencesHandling.None,
                        TypeNameAssemblyFormat = System.Runtime.Serialization.Formatters.FormatterAssemblyStyle.Full
                    });
        }

        internal static string SerealizeUsingNewSerializerSettings(StandAloneCluster cluster)
        {
            JsonSerializerSettings settings = new JsonSerializerSettings
            {
                Formatting = Newtonsoft.Json.Formatting.Indented,
                ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                NullValueHandling = NullValueHandling.Ignore,
                TypeNameHandling = TypeNameHandling.Auto,
                PreserveReferencesHandling = PreserveReferencesHandling.None
            };

            return JsonConvert.SerializeObject(cluster, settings);
        }
    }
}