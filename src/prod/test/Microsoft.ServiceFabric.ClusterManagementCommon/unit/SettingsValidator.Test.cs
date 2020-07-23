// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using System.Collections.Generic;
    using WEX.TestExecution;
    using System;

    /// <summary>
    /// test 
    /// </summary>
    [TestClass]
    public class SettingsValidatorTest
    {
        static ClusterManifestGeneratorSettings clusterManifestGeneratorSettings;

        [ClassInitialize]
        public static void ClassInitialize()
        {
            clusterManifestGeneratorSettings = new ClusterManifestGeneratorSettings()
            {
                MinClusterSize = 3,
                MaxClusterSize = 10,
                MinAllowedPortNumber = 0,
                MaxAllowedPortNumber = 70000,
                StartReservedPortNumber = 80000,
                EndReservedPortNumber = 90000,
                MinDynamicPortCount = 100,
                AllowUnprotectedDiagnosticsStorageAccountKeys = true
            };
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateNodeTypesTest()
        {
            var noNodeTypesJsonConfig = Utility.CreateMockupJsonModel(0, 1, new List<SettingsSectionDescription>());
            RunValidation(noNodeTypesJsonConfig);

            var correctJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>());
            RunValidation(correctJsonConfig, null, false);

            var duplicateNodeTypesJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>());
            duplicateNodeTypesJsonConfig.Properties.NodeTypes[1].Name = "NodeType0";
            RunValidation(duplicateNodeTypesJsonConfig);

            var clusterSizeNotSupportedJsonConfig = Utility.CreateMockupJsonModel(20, 1, new List<SettingsSectionDescription>());
            RunValidation(clusterSizeNotSupportedJsonConfig);

            var moreThanOnePrimaryJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>());
            moreThanOnePrimaryJsonConfig.Properties.NodeTypes[0].IsPrimary = true;
            moreThanOnePrimaryJsonConfig.Properties.NodeTypes[1].IsPrimary = true;
            RunValidation(moreThanOnePrimaryJsonConfig);

            var portOutRangeJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>());
            portOutRangeJsonConfig.Properties.NodeTypes[0].HttpApplicationGatewayEndpointPort = 85000;
            RunValidation(portOutRangeJsonConfig);

            var portReservedJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>());
            portReservedJsonConfig.Properties.NodeTypes[0].HttpApplicationGatewayEndpointPort = 95000;
            RunValidation(portReservedJsonConfig);

            var dynamicPortsNotSufficientJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>());
            dynamicPortsNotSufficientJsonConfig.Properties.NodeTypes[0].EphemeralPorts.StartPort = 30000;
            dynamicPortsNotSufficientJsonConfig.Properties.NodeTypes[0].EphemeralPorts.EndPort = 30001;
            RunValidation(dynamicPortsNotSufficientJsonConfig);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateFabricSettingsTest()
        {
            var requiredSectionNotFoundJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>());
            var requiredParameters = new Dictionary<string, HashSet<string>>();
            requiredParameters.Add("Federation", new HashSet<string>());
            RunValidation(requiredSectionNotFoundJsonConfig, requiredParameters);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateCertificateTest()
        {
            var securtyJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>(), Utility.CreateSecurity());
            RunValidation(securtyJsonConfig, null, false);

            var clientCertDefinedWithoutServerCertJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>(), Utility.CreateSecurity());
            clientCertDefinedWithoutServerCertJsonConfig.Properties.Security.CertificateInformation.ServerCertificate = null;
            RunValidation(clientCertDefinedWithoutServerCertJsonConfig);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateDiagnosticsStorageAccountConfigTest()
        {
            var diagnosticsStorageAccountJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>(), null, null, null, Utility.CreateDiagnosticsStorageAccountConfig());
            RunValidation(diagnosticsStorageAccountJsonConfig, null, false);

            var invalidDiagnosticsStorageAccountBlobEndpointJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>(), null, null, null, Utility.CreateDiagnosticsStorageAccountConfig());
            invalidDiagnosticsStorageAccountBlobEndpointJsonConfig.Properties.DiagnosticsStorageAccountConfig.BlobEndpoint = "https://diagnosticStorageAccountName";
            RunValidation(invalidDiagnosticsStorageAccountBlobEndpointJsonConfig);

            var invalidDiagnosticsStorageAccountTableEndpointJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>(), null, null, null, Utility.CreateDiagnosticsStorageAccountConfig());
            invalidDiagnosticsStorageAccountTableEndpointJsonConfig.Properties.DiagnosticsStorageAccountConfig.TableEndpoint = "https://diagnosticStorageAccountName";
            RunValidation(invalidDiagnosticsStorageAccountTableEndpointJsonConfig);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateAzureActiveDirectoryTest()
        {
            var azureActiveJsonConfig = Utility.CreateMockupJsonModel(3, 1, new List<SettingsSectionDescription>(), Utility.CreateSecurity());
            azureActiveJsonConfig.Properties.Security.AzureActiveDirectory = new AzureActiveDirectory();
            azureActiveJsonConfig.Properties.Security.CertificateInformation = null;
            RunValidation(azureActiveJsonConfig);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void IsValidThumbprintTest()
        {
            // copied from powershell
            Assert.IsTrue(SettingsValidator.IsValidThumbprint("59EC792004C56225DD6691132C713194D28098F1"));

            // copied from mmc
            Assert.IsTrue(SettingsValidator.IsValidThumbprint("e9 70 f6 20 4a 54 27 a7 96 1d e1 1d 5a c2 14 f5 bb 18 6b d0"));

            Assert.IsFalse(SettingsValidator.IsValidThumbprint("59EC792004C56225DD6691132C713194D28098F1;"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void AreValidIssuerThumbprintsTest()
        {
            Assert.IsTrue(SettingsValidator.AreValidIssuerThumbprints("59EC792004C56225DD6691132C713194D28098F1"));
            Assert.IsTrue(SettingsValidator.AreValidIssuerThumbprints("59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2"));

            Assert.IsFalse(SettingsValidator.AreValidIssuerThumbprints("59EC792004C56225DD6691132C713194D28098F1;59EC792004C56225DD6691132C713194D28098F2"));

        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateCommonNamesTest()
        {
            X509 validCertInfo = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns(new string[] { "hia", "bia" }, new string[] { "59EC792004C56225DD6691132C713194D28098F1", "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" }),
                ServerCertificateCommonNames = ConstructServerCns(new string[] { "hia", "bia" }, new string[] { "59EC792004C56225DD6691132C713194D28098F1", "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" }),
                ReverseProxyCertificateCommonNames = ConstructServerCns(new string[] { "hia", "bia" }, null),
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "lolo", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                    new ClientCertificateCommonName() { CertificateCommonName = "biabia", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                    new ClientCertificateCommonName() { CertificateCommonName = "lala", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateCommonNames(validCertInfo); }, null);

            // IT is supported for certs except reverseProxy cert
            X509 invalidCertInfo;
            invalidCertInfo = new X509()
            {
                ReverseProxyCertificateCommonNames = ConstructServerCns(new string[] { "hia" }, new string[] { "59EC792004C56225DD6691132C713194D28098F1" }),
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "lolo", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                    new ClientCertificateCommonName() { CertificateCommonName = "biabia", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                    new ClientCertificateCommonName() { CertificateCommonName = "lala", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateCommonNames(invalidCertInfo); }, ClusterManagementErrorCode.UnsupportedIssuerThumbprintPair);

            // cns must not be nullOrWhitespace
            invalidCertInfo = new X509()
            {
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "  " },
                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateCommonNames(invalidCertInfo); }, ClusterManagementErrorCode.InvalidCommonName);

            // Issuer thumbprints must be separated by comma
            invalidCertInfo = new X509()
            {
                ServerCertificateCommonNames = ConstructServerCns("hia", "59EC792004C56225DD6691132C713194D28098F1;59EC792004C56225DD6691132C713194D28098F2"),
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateCommonNames(invalidCertInfo); }, ClusterManagementErrorCode.InvalidCertificateIssuerThumbprints);

            // Issuer thumbprints must not dupe under the same cn
            invalidCertInfo = new X509()
            {
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "lolo", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F1" },
                },
                ServerCertificateCommonNames = ConstructServerCns("hia", "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F1"),
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateCommonNames(invalidCertInfo); }, ClusterManagementErrorCode.InvalidCertificateIssuerThumbprints);

            // Up to 2 CNs are supported, except client CN
            invalidCertInfo = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns(new string[] { "hia", "bia", "zaa" }, null),
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateCommonNames(invalidCertInfo); }, ClusterManagementErrorCode.InvalidCommonNameCount);

            // no dupe CN is allowed for the same cert type, except client CN
            invalidCertInfo = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns(new string[] { "hia", "hia"}, null),
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateCommonNames(invalidCertInfo); }, ClusterManagementErrorCode.DupeCommonNameNotAllowedForClusterCert);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateCommonNamesAgainstThumbprintsTest()
        {
            X509 validCertInfo = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns(new string[] { "hia", "bia" }, new string[] { "59EC792004C56225DD6691132C713194D28098F1", "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" }),
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "lolo", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                    new ClientCertificateCommonName() { CertificateCommonName = "biabia", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                    new ClientCertificateCommonName() { CertificateCommonName = "lala", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                },
                ClientCertificateThumbprints = new List<ClientCertificateThumbprint>() { new ClientCertificateThumbprint() { CertificateThumbprint = "59EC792004C56225DD6691132C713194D28098F1" } }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateCommonNamesAgainstThumbprints(validCertInfo); }, null);

            X509 invalidCertInfo = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns(new string[] { "hia", "bia" }, new string[] { "59EC792004C56225DD6691132C713194D28098F1", "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" }),
                ClusterCertificate = new CertificateDescription() { Thumbprint = "59EC792004C56225DD6691132C713194D28098F2" },
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "lolo", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                    new ClientCertificateCommonName() { CertificateCommonName = "biabia", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                    new ClientCertificateCommonName() { CertificateCommonName = "lala", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F2" },
                },
                ClientCertificateThumbprints = new List<ClientCertificateThumbprint>() { new ClientCertificateThumbprint() { CertificateThumbprint = "59EC792004C56225DD6691132C713194D28098F1" } }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateCommonNamesAgainstThumbprints(invalidCertInfo); }, ClusterManagementErrorCode.InvalidCommonNameThumbprintPair);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateThumbprints()
        {
            X509 validCertInfo = new X509()
            {
                ClusterCertificate = new CertificateDescription() {Thumbprint = "e9 70 f6 20 4a 54 27 a7 96 1d e1 1d 5a c2 14 f5 bb 18 6b d0"},
                ServerCertificate = new CertificateDescription() {Thumbprint = "e9 70 f6 20 4a 54 27 a7 96 1d e1 1d 5a c2 14 f5 bb 18 6b d0"},
                ReverseProxyCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ClientCertificateThumbprints = new List<ClientCertificateThumbprint>() { new ClientCertificateThumbprint() { CertificateThumbprint = "59EC792004C56225DD6691132C713194D28098F1" } }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateThumbprints(validCertInfo); }, null);

            X509 invalidCertInfo1 = new X509()
            {
                ClusterCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1*"},
                ServerCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ReverseProxyCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ClientCertificateThumbprints = new List<ClientCertificateThumbprint>() { new ClientCertificateThumbprint() { CertificateThumbprint = "59EC792004C56225DD6691132C713194D28098F1" } }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateThumbprints(invalidCertInfo1); }, ClusterManagementErrorCode.InvalidClusterCertificateThumbprint);

            X509 invalidCertInfo5 = new X509()
            {
                ClusterCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ServerCertificate = null,
                ReverseProxyCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ClientCertificateThumbprints = null
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateThumbprints(invalidCertInfo5); }, null);

            X509 invalidCertInfo2 = new X509()
            {
                ClusterCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ServerCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1*"},
                ReverseProxyCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ClientCertificateThumbprints = new List<ClientCertificateThumbprint>() { new ClientCertificateThumbprint() { CertificateThumbprint = "59EC792004C56225DD6691132C713194D28098F1" } }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateThumbprints(invalidCertInfo2); }, ClusterManagementErrorCode.InvalidServerCertificateThumbprint);

            X509 invalidCertInfo3 = new X509()
            {
                ClusterCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ServerCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ReverseProxyCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1*"},
                ClientCertificateThumbprints = new List<ClientCertificateThumbprint>() { new ClientCertificateThumbprint() { CertificateThumbprint = "59EC792004C56225DD6691132C713194D28098F1" } }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateThumbprints(invalidCertInfo3); }, ClusterManagementErrorCode.InvalidReverseProxyCertificateThumbprint);

            X509 invalidCertInfo4 = new X509()
            {
                ClusterCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ServerCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ReverseProxyCertificate = new CertificateDescription() {Thumbprint = "59EC792004C56225DD6691132C713194D28098F1"},
                ClientCertificateThumbprints = new List<ClientCertificateThumbprint>() { new ClientCertificateThumbprint() { CertificateThumbprint = "59EC792004C56225DD6691132C713194D28098F1*" } }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateThumbprints(invalidCertInfo4); }, ClusterManagementErrorCode.InvalidClientCertificateThumbprint);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateIssuerCertStores()
        {
            X509 validCertInfo1 = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns("clusterCN", ""),
                ServerCertificateCommonNames = ConstructServerCns("serverCN", ""),
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = "" }
                },
                ClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                },
                ServerCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ServerIssuer", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                },
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(validCertInfo1); }, null);

            X509 validCertInfo2 = new X509()
            {
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = ""},
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = ""}
                },
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "Issuer1", X509StoreNames = "My"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "Issuer2", X509StoreNames = "My"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "Issuer3", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(validCertInfo2); }, null);

            X509 invalidCertInfo1 = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns("clusterCN", ""),
                ServerCertificateCommonNames = ConstructServerCns("serverCN", ""),
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = "" }
                },
                ClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer1", X509StoreNames = "My"}
                                                },
                ServerCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ServerIssuer", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                },
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo1); }, ClusterManagementErrorCode.DupIssuerCertificateCN);

            X509 invalidCertInfo2 = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns("clusterCN", ""),
                ServerCertificateCommonNames = ConstructServerCns("serverCN", ""),
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = "" }
                },
                ClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                },
                ServerCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ServerIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "ServerIssuer1", X509StoreNames = "My"}
                                                },
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo2); }, ClusterManagementErrorCode.DupIssuerCertificateCN);

            X509 invalidCertInfo3 = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns("clusterCN", ""),
                ServerCertificateCommonNames = ConstructServerCns("serverCN", ""),
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = "" }
                },
                ClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                },
                ServerCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ServerIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "ServerIssuer2", X509StoreNames = "My"}
                                                },
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClientIssuer1", X509StoreNames = "My"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClientIssuer1", X509StoreNames = "My, Root"},
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo3); }, ClusterManagementErrorCode.DupIssuerCertificateCN);

            X509 invalidCertInfo4 = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns("clusterCN", ""),
                ServerCertificateCommonNames = ConstructServerCns("serverCN", ""),
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = "" }
                },
                ClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer1", X509StoreNames = "My, My"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                },
                ServerCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ServerIssuer1", X509StoreNames = "My, Root, InvalidStore"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                },
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo4); }, ClusterManagementErrorCode.DupIssuerCertificateStore);

            X509 invalidCertInfo5 = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns("clusterCN", ""),
                ClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo5); }, ClusterManagementErrorCode.IssuerCNCannotBeNull);

            X509 invalidCertInfo6 = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns("clusterCN", ""),
                ClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "" },
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo6); }, ClusterManagementErrorCode.IssuerStoreNameCannotBeNull);

            X509 invalidCertInfo7 = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns("clusterCN", ""),
                ClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer2", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo7); }, ClusterManagementErrorCode.IssuerCertStoreCNMoreThan2);

            X509 invalidCertInfo8 = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns("clusterCN", ""),
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = "" }
                },
                ClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                },
                ServerCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ServerIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                },
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo8); }, ClusterManagementErrorCode.IssuerCertStoreSpecifiedWithoutCommonNameCertificate);

            X509 invalidCertInfo9 = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns("clusterCN", ""),
                ServerCertificateCommonNames = ConstructServerCns("serverCN", ""),
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo9); }, ClusterManagementErrorCode.IssuerCertStoreSpecifiedWithoutCommonNameCertificate);

            X509 invalidCertInfo10 = new X509()
            {
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = "" }
                },
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClientIssuer", X509StoreNames = "My"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClientIssuer", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo10); }, ClusterManagementErrorCode.DupIssuerCertificateCN);

            X509 invalidCertInfo11 = new X509()
            {
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = "" }
                },
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "clientIssuer", X509StoreNames = "My"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClientIssuer", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo11); }, ClusterManagementErrorCode.DupIssuerCertificateCN);

            X509 invalidCertInfo12 = new X509()
            {
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = "", IsAdmin = true }
                },
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClientIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClientIssuer2", X509StoreNames = "Root, root"},
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo12); }, ClusterManagementErrorCode.DupIssuerCertificateStore);

            X509 invalidCertInfo13 = new X509()
            {
                ClusterCertificateCommonNames = ConstructServerCns("clusterCN", "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F1"),
                ClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo13); }, ClusterManagementErrorCode.IssuerCertStoreAndIssuerPinningCannotBeTogether);

            X509 invalidCertInfo14 = new X509()
            {
                ServerCertificateCommonNames = ConstructServerCns("serverCN", "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F1"),
                ServerCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ServerIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo14); }, ClusterManagementErrorCode.IssuerCertStoreAndIssuerPinningCannotBeTogether);

            X509 invalidCertInfo15 = new X509()
            {
                ClientCertificateCommonNames = new List<ClientCertificateCommonName>()
                {
                    new ClientCertificateCommonName() { CertificateCommonName = "clientCN", CertificateIssuerThumbprint = "59EC792004C56225DD6691132C713194D28098F1,59EC792004C56225DD6691132C713194D28098F1", IsAdmin = true }
                },
                ClientCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClientIssuer1", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClientIssuer2", X509StoreNames = "root"},
                                                }
            };
            this.RunWrapper(delegate { SettingsValidator.ValidateIssuerCertStore(invalidCertInfo15); }, ClusterManagementErrorCode.IssuerCertStoreAndIssuerPinningCannotBeTogether);
        }

        private ServerCertificateCommonNames ConstructServerCns(string cns, string issuerThumbprints)
        {
            string[] cnsInput = cns != null ? new string[1] { cns } : null;
            string[] issuerThumbprintsInput = issuerThumbprints != null ? new string[1] { issuerThumbprints } : null;
            return ConstructServerCns(cnsInput, issuerThumbprintsInput);
        }

        private ServerCertificateCommonNames ConstructServerCns(string[] cns, string[] issuerThumbprints)
        {
            ServerCertificateCommonNames result = new ServerCertificateCommonNames()
            {
                CommonNames = new List<CertificateCommonNameBase>()
            };

            result.CommonNames = new List<CertificateCommonNameBase>();
            int cnsLength = cns != null ? cns.Length : 0;
            int itLength = issuerThumbprints != null ? issuerThumbprints.Length : 0;
            for (int i = 0; i < Math.Max(cnsLength, itLength); i++)
            {
                CertificateCommonNameBase element = new CertificateCommonNameBase();

                if (i < cnsLength)
                {
                    element.CertificateCommonName = cns[i];
                }

                if (i < itLength)
                {
                    element.CertificateIssuerThumbprint = issuerThumbprints[i];
                }

                result.CommonNames.Add(element);
            }

            return result;
        }

        private void RunWrapper(Action action, ClusterManagementErrorCode? expectedErrorCode)
        {
            try
            {
                action();
                if (expectedErrorCode.HasValue)
                {
                    throw new Exception(expectedErrorCode.Value + " is not thrown");
                }
            }
            catch (ValidationException ex)
            {
                if (!expectedErrorCode.HasValue || ex.Code != expectedErrorCode.Value)
                {
                    throw;
                }
            }
        }

        private static void RunValidation(MockupJsonModel jsonConfig, Dictionary<string, HashSet<string>> requiredParameters = null, bool failureExpected = true)
        {
            IUserConfig userConfig = jsonConfig.GetUserConfig();

            var settingsValidator = new SettingsValidator(
                userConfig,
                new MockupAdminConfig().GetFabricSettingsMetadata(),
                requiredParameters == null ? new Dictionary<string, HashSet<string>>() : requiredParameters,
                clusterManifestGeneratorSettings);

            if (failureExpected)
            {
                Verify.Throws<ValidationException>(() => settingsValidator.Validate(true));
            }
            else
            {
                Verify.NoThrow(() => settingsValidator.Validate(true));
            }
        }
    }
}