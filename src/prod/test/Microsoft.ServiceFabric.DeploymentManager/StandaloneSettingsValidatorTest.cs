// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Security.Cryptography.X509Certificates;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Model;
    using BPA;

    [TestClass]
    internal class StandaloneSettingsValidatorTest
    {
        [TestInitialize]
        public void Initialize()
        {
#if !DotNetCoreClr
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
#endif

            // in case somehow the certificates are left over from previous run
            this.Cleanup();

            Utility.InstallCertificates(Utility.installedCertificates.Select(p => p.Item2));
        }

        [TestCleanup]
        public void Cleanup()
        {
            Utility.UninstallCertificates(Utility.installedCertificates.Select(p => p.Item1));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateTopologyTest()
        {
            // Node name contains invalid characters.
            List<NodeDescriptionGA> nodelist = new List<NodeDescriptionGA>();
            NodeDescriptionGA node = Utility.CreateNodeDescription("<", "nodetype1", "machine1", "fd:/dc1/r0", "UD0");
            nodelist.Add(node);
            StandAloneInstallerJSONModelAugust2017 v1model = Utility.CreateMockupStandAloneJsonModel("testCluster", "1.0.0", nodes : nodelist);
            StandaloneSettingsValidator settingsValidator = new StandaloneSettingsValidator(v1model);

            Utility.ValidateExpectedValidationException(
                delegate
                {
                    settingsValidator.ValidateTopology();
                },
                ClusterManagementErrorCode.NodeNameContainsInvalidChars);

            // Duplicate node name.
            List<NodeDescriptionGA> nodelist2 = new List<NodeDescriptionGA>();
            NodeDescriptionGA node1 = Utility.CreateNodeDescription("node1", "nodetype1", "machine1", "fd:/dc1/r0", "UD0");
            NodeDescriptionGA node2 = Utility.CreateNodeDescription("node1", "nodetype1", "machine2", "fd:/dc1/r1", "UD1");
            nodelist2.Add(node1);
            nodelist2.Add(node2);
            v1model = Utility.CreateMockupStandAloneJsonModel("testCluster", "1.0.0", nodes: nodelist2);

            Utility.ValidateExpectedValidationException(
                delegate
                {
                    settingsValidator = new StandaloneSettingsValidator(v1model);
                },
                ClusterManagementErrorCode.NodeNameDuplicateDetected);

            // FD count needs to be greater than 2 for multi-box deployment.
            List<NodeDescriptionGA> nodelist4 = new List<NodeDescriptionGA>();
            node1 = Utility.CreateNodeDescription("node1", "nodetype1", "machine1", "fd:/dc1/r0", "UD0");
            node2 = Utility.CreateNodeDescription("node2", "nodetype1", "machine2", "fd:/dc1/r0", "UD1");
            NodeDescriptionGA node3 = Utility.CreateNodeDescription("node3", "nodetype1", "machine3", "fd:/dc1/r0", "UD2");
            nodelist4.Add(node1);
            nodelist4.Add(node2);
            nodelist4.Add(node3);
            v1model = Utility.CreateMockupStandAloneJsonModel("testCluster", "1.0.0", nodes: nodelist4);
            settingsValidator = new StandaloneSettingsValidator(v1model);

            Utility.ValidateExpectedValidationException(
                delegate
                {
                    settingsValidator.ValidateTopology();
                },
                ClusterManagementErrorCode.FDMustBeGreaterThan2);

            // Scale-min should be blocked for multi-box deployment
            List<NodeDescriptionGA> nodelistScaleMin = new List<NodeDescriptionGA>();
            node1 = Utility.CreateNodeDescription("node1", "nodetype1", "machine1", "fd:/dc1/r0", "UD0");
            node2 = Utility.CreateNodeDescription("node2", "nodetype1", "machine1", "fd:/dc1/r1", "UD1");
            node3 = Utility.CreateNodeDescription("node3", "nodetype1", "machine2", "fd:/dc2/r1", "UD2");
            nodelistScaleMin.Add(node1);
            nodelistScaleMin.Add(node2);
            nodelistScaleMin.Add(node3);
            v1model = Utility.CreateMockupStandAloneJsonModel("testCluster", "1.0.0", nodes: nodelistScaleMin);
            settingsValidator = new StandaloneSettingsValidator(v1model);

            Utility.ValidateExpectedValidationException(
                delegate
                {
                    settingsValidator.ValidateTopology();
                },
            ClusterManagementErrorCode.ScaleMinNotAllowed);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateUpgradeTest()
        {
            // Windows/ GMSA security <-> unsecure is not allowed.
            StandAloneInstallerJSONModelAugust2017 v1model = Utility.CreateMockupStandAloneJsonModel("testCluster", "1.0.0", security : Utility.CreateGMSASecurity());
            StandAloneInstallerJSONModelAugust2017 v2model = Utility.CreateMockupStandAloneJsonModel("testCluster", "2.0.0");
            StandaloneSettingsValidator validator = new StandaloneSettingsValidator(v2model);

            Utility.ValidateExpectedValidationException(
                delegate
                {
                    validator.ValidateUpdateFrom(v1model.GetUserConfig(), v1model.GetClusterTopology(), false).GetAwaiter().GetResult();
                },
                ClusterManagementErrorCode.AuthenticationTypeCannotBeChangedFromWindowsToUnsecured);

            // For the same nodename, node IP can't be changed.
            List<NodeDescriptionGA> nodelist = new List<NodeDescriptionGA>();
            NodeDescriptionGA node = Utility.CreateNodeDescription("node1", "NodeType0", "machine1", "fd:/dc1/r0", "UD0");
            List<NodeDescriptionGA> nodelistV2 = new List<NodeDescriptionGA>();
            NodeDescriptionGA nodeV2 = Utility.CreateNodeDescription("node1", "NodeType0", "machine2", "fd:/dc1/r0", "UD0");
            nodelist.Add(node);
            nodelistV2.Add(nodeV2);
            v1model = Utility.CreateMockupStandAloneJsonModel("testCluster", "1.0.0", nodes: nodelist);
            v2model = Utility.CreateMockupStandAloneJsonModel("testCluster", "2.0.0", nodes: nodelistV2);
            validator = new StandaloneSettingsValidator(v2model);

            Utility.ValidateExpectedValidationException(
                delegate
                {
                    validator.ValidateUpdateFrom(v1model.GetUserConfig(), v1model.GetClusterTopology(), false).GetAwaiter().GetResult();
                },
                ClusterManagementErrorCode.NodeIPNodeTypeRefCantBeChanged);

            // Reverse proxy port change is allowed.
            v1model = Utility.CreateMockupStandAloneJsonModel("testCluster", "1.0.0");
            v2model = Utility.CreateMockupStandAloneJsonModel("testCluster", "2.0.0");
            foreach (var currentNodeType in v2model.Properties.NodeTypes)
            {
                currentNodeType.ReverseProxyEndpointPort = 19081;
            }
            validator = new StandaloneSettingsValidator(v2model);
            validator.ValidateUpdateFrom(v1model.GetUserConfig(), v1model.GetClusterTopology(), false).GetAwaiter().GetResult();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void GetUniqueThumbprintsTest()
        {
            this.InternalGetUniqueThumbprintsTest(new CertificateDescription()
            {
                Thumbprint = "lala",
                ThumbprintSecondary = "lolo"
            },
            2);

            this.InternalGetUniqueThumbprintsTest(new CertificateDescription()
            {
                Thumbprint = "lala",
            },
            1);

            this.InternalGetUniqueThumbprintsTest(new CertificateDescription()
            {
                Thumbprint = "lala",
                ThumbprintSecondary = "lala"
            },
            1);

            this.InternalGetUniqueThumbprintsTest(new CertificateDescription()
            {
                Thumbprint = "lala",
                ThumbprintSecondary = "Lala"
            },
            1);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateAllCertsInstalledOnAllNodesTest()
        {
            this.InternalValidateAllCertsInstalledOnAllNodesTest(Utility.installedValidCertificates.Select(p => p.Item1), null);

            this.InternalValidateAllCertsInstalledOnAllNodesTest(Utility.installedCertificates.Select(p => p.Item1), null);

            this.InternalValidateAllCertsInstalledOnAllNodesTest(Utility.uninstalledCertificates, ClusterManagementErrorCode.CertificateNotInstalledOnNode);

            this.InternalValidateAllCertsInstalledOnAllNodesTest(Utility.installedCertificates.Select(p => p.Item1).Union(Utility.uninstalledCertificates), ClusterManagementErrorCode.CertificateNotInstalledOnNode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateEachCertInstalledOnAtLeastOneNodeTest()
        {
            this.InternalValidateEachCertInstalledOnAtLeastOneNodeTest(Utility.installedValidCertificates.Select(p => p.Item1), null);

            this.InternalValidateEachCertInstalledOnAtLeastOneNodeTest(Utility.installedCertificates.Select(p => p.Item1), null);

            this.InternalValidateEachCertInstalledOnAtLeastOneNodeTest(Utility.uninstalledCertificates, ClusterManagementErrorCode.CertificateNotInstalled);

            this.InternalValidateEachCertInstalledOnAtLeastOneNodeTest(Utility.installedCertificates.Select(p => p.Item1).Union(Utility.uninstalledCertificates), ClusterManagementErrorCode.CertificateNotInstalled);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateAtLeastOneCertInstalledOnOneNodeTest()
        {
            this.InternalValidateAtLeastOneCertInstalledOnOneNodeTest(Utility.installedValidCertificates.Select(p => p.Item1), null);

            this.InternalValidateAtLeastOneCertInstalledOnOneNodeTest(Utility.installedCertificates.Select(p => p.Item1), null);

            this.InternalValidateAtLeastOneCertInstalledOnOneNodeTest(Utility.uninstalledCertificates, ClusterManagementErrorCode.CertificateNotInstalled);

            this.InternalValidateAtLeastOneCertInstalledOnOneNodeTest(Utility.installedCertificates.Select(p => p.Item1).Union(Utility.uninstalledCertificates), null);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateClusterCertificateInstallationTest()
        {
            List<string> certTypesToCheck = new List<string>()
            {
                "Cluster", "Server", "ReverseProxy"
            };

            foreach (string certType in certTypesToCheck)
            {
                Console.WriteLine(certType + " thumbprint test");

                this.InternalValidateClusterCertificateInstallationTest(Utility.installedValidCertificates.Select(p => p.Item1).ToArray(), null, certType, null);

                this.InternalValidateClusterCertificateInstallationTest(Utility.installedInvalidCertificates.Select(p => p.Item1).ToArray(), null, certType, ClusterManagementErrorCode.CertificateInvalid);

                this.InternalValidateClusterCertificateInstallationTest(Utility.uninstalledCertificates, null, certType, ClusterManagementErrorCode.CertificateNotInstalledOnNode);

                Console.WriteLine(certType + " cn test");

                this.InternalValidateClusterCertificateInstallationTest(Utility.installedValidCertificates.Select(p => p.Item3).ToArray(), Utility.installedValidCertificates.Select(p => p.Item1).ToArray(), certType, null);

                this.InternalValidateClusterCertificateInstallationTest(Utility.installedInvalidCertificates.Select(p => p.Item3).ToArray(), Utility.installedInvalidCertificates.Select(p => p.Item1).ToArray(), certType, ClusterManagementErrorCode.CertificateInvalid);

                this.InternalValidateClusterCertificateInstallationTest(Utility.uninstalledCertificates, Utility.uninstalledCertificates, certType, ClusterManagementErrorCode.CertificateNotInstalledOnNode);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateClusterThumbprintUpdateTest()
        {
            // positive cases: no cert change
            this.InternalValidateClusterThumbprintUpdateTest("a", null, "a", null);
            this.InternalValidateClusterThumbprintUpdateTest("a", null, "A", null);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "a", "b");
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "A", "B");
            this.InternalValidateClusterThumbprintUpdateTest("a", "a", "A", "A");

            // positive cases: 2->1
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "a", null);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "b", null);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "a", "a");
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "a", "A");
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "b", "B");

            // positive cases: 1->1
            this.InternalValidateClusterThumbprintUpdateTest("a", null, "b", null);
            this.InternalValidateClusterThumbprintUpdateTest("a", null, "b", "b");
            this.InternalValidateClusterThumbprintUpdateTest("a", "A", "b", null);
            this.InternalValidateClusterThumbprintUpdateTest("a", "A", "b", "b");
            this.InternalValidateClusterThumbprintUpdateTest("a", "A", "b", "B");

            // positive cases: 1->2
            this.InternalValidateClusterThumbprintUpdateTest("a", null, "a", "b");
            this.InternalValidateClusterThumbprintUpdateTest("a", "a", "a", "b");
            this.InternalValidateClusterThumbprintUpdateTest("a", "A", "a", "b");
            this.InternalValidateClusterThumbprintUpdateTest("a", null, "b", "a");
            this.InternalValidateClusterThumbprintUpdateTest("a", "a", "b", "a");
            this.InternalValidateClusterThumbprintUpdateTest("a", "A", "b", "a");

            // negative cases: 2->1
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "c", null, ClusterManagementErrorCode.CertificateUpgradeWithNoIntersectionNotAllowed);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "c", "c", ClusterManagementErrorCode.CertificateUpgradeWithNoIntersectionNotAllowed);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "c", "C", ClusterManagementErrorCode.CertificateUpgradeWithNoIntersectionNotAllowed);

            // negative cases: 1->2
            this.InternalValidateClusterThumbprintUpdateTest("a", null, "b", "c", ClusterManagementErrorCode.CertificateUpgradeWithNoIntersectionNotAllowed);
            this.InternalValidateClusterThumbprintUpdateTest("a", "a", "b", "c", ClusterManagementErrorCode.CertificateUpgradeWithNoIntersectionNotAllowed);
            this.InternalValidateClusterThumbprintUpdateTest("a", "A", "b", "c", ClusterManagementErrorCode.CertificateUpgradeWithNoIntersectionNotAllowed);

            // negative cases: swap
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "b", "a", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "B", "a", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "b", "A", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "B", "A", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);

            // negative cases: 2->2
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "c", "d", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "a", "c", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "c", "a", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "c", "b", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);
            this.InternalValidateClusterThumbprintUpdateTest("a", "b", "b", "c", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateClusterCnUpdateTest()
        {
            // positive cases: no cert change
            this.InternalValidateClusterCnUpdateTest("a", "a");

            // positive cases: 2->1
            this.InternalValidateClusterCnUpdateTest("a,b", "a");
            this.InternalValidateClusterCnUpdateTest("a,b", "b");

            // positive cases: 1->1
            this.InternalValidateClusterCnUpdateTest("a", "b");

            // positive cases: 1->2
            this.InternalValidateClusterCnUpdateTest("a", "a,b");
            this.InternalValidateClusterCnUpdateTest("a", "b,a");

            // negative cases: 2->1
            this.InternalValidateClusterCnUpdateTest("a,b", "c", ClusterManagementErrorCode.CertificateUpgradeWithNoIntersectionNotAllowed);
            this.InternalValidateClusterCnUpdateTest("a,b", "A", ClusterManagementErrorCode.CertificateUpgradeWithNoIntersectionNotAllowed);

            // negative cases: 1->2
            this.InternalValidateClusterCnUpdateTest("a", "b,c", ClusterManagementErrorCode.CertificateUpgradeWithNoIntersectionNotAllowed);
            this.InternalValidateClusterCnUpdateTest("a", "A,b", ClusterManagementErrorCode.CertificateUpgradeWithNoIntersectionNotAllowed);

            // negative cases: swap
            this.InternalValidateClusterCnUpdateTest("a,b", "b,a", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);

            // negative cases: 2->2
            this.InternalValidateClusterCnUpdateTest("a,b", "c,d", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);
            this.InternalValidateClusterCnUpdateTest("a,b", "a,c", ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateClusterCertificateIssuerThumbprintUpdateTest()
        {
            // positive: no IT change
            InternalValidateClusterCertificateIssuerThumbprintUpdateTest(
                "cn1;cn2",
                "thumb1,thumb2;thumb3",
                "cn1;cn2",
                "thumb1,thumb2;thumb3");

            // positive: IT change with empty-element intersection
            InternalValidateClusterCertificateIssuerThumbprintUpdateTest(
                "cn1;cn2",
                null,
                "cn1;cn2",
                "thumb1,thumb2;thumb3");

            // positive: IT change and not installed
            InternalValidateClusterCertificateIssuerThumbprintUpdateTest(
                "MyValidCert;MyValidCert2",
                "52C50E3C286B034EEAC8405D9661E211B55A9792;E7AB9AC4F463886A0A905B89AE724DBDE1898021",
                "MyValidCert;MyValidCert2",
                "52C50E3C286B034EEAC8405D9661E211B55A9792;E7AB9AC4F463886A0A905B89AE724DBDE1898021,F7AB9AC4F463886A0A905B89AE724DBDE1898022");

            // negative: IT change without intersection
            InternalValidateClusterCertificateIssuerThumbprintUpdateTest(
                "cn1;cn2",
                "thumb1,thumb2;thumb3",
                "cn1;cn2",
                "thumb1,thumb2;thumb4",
                ClusterManagementErrorCode.IssuerThumbprintUpgradeWithNoIntersection);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateClusterCertificateIssuerStoreUpdateTest()
        {
            // positive: adding new issuer CN with intersection
            var currentClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                };
            var updatedClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "My,           Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer2", X509StoreNames = "My"}
                                                };
            InternalValidateClusterCertificateIssuerStoreUpdateTest(
                currentClusterCertificateIssuerStores,
                updatedClusterCertificateIssuerStores);

            // positive: adding new issuer CN when current is null
            currentClusterCertificateIssuerStores = null;
            updatedClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer2", X509StoreNames = "My"}
                                                };
            InternalValidateClusterCertificateIssuerStoreUpdateTest(
                currentClusterCertificateIssuerStores,
                updatedClusterCertificateIssuerStores);

            // positive: removing issuer CN with intersection
            currentClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                };
            updatedClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "My, Root"}
                                                };
            InternalValidateClusterCertificateIssuerStoreUpdateTest(
                currentClusterCertificateIssuerStores,
                updatedClusterCertificateIssuerStores);

            // positive: adding issuer X509StoreName with intersection
            currentClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "My, Root"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                };
            updatedClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "My, NewStore"},
                                                    new CertificateIssuerStore() { IssuerCommonName = "", X509StoreNames = "My"}
                                                };
            InternalValidateClusterCertificateIssuerStoreUpdateTest(
                currentClusterCertificateIssuerStores,
                updatedClusterCertificateIssuerStores);

            // negative: adding issuer X509StoreName without intersection
            currentClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "My, Root"}
                                                };
            updatedClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "NewStore"}
                                                };
            InternalValidateClusterCertificateIssuerStoreUpdateTest(
                currentClusterCertificateIssuerStores,
                updatedClusterCertificateIssuerStores,
                ClusterManagementErrorCode.IssuerStoreX509StoreNameUpgradeWithNoIntersection);

            // negative: adding issuer CN without intersection
            currentClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer", X509StoreNames = "My, Root"}
                                                };
            updatedClusterCertificateIssuerStores = new List<CertificateIssuerStore>
                                                {
                                                    new CertificateIssuerStore() { IssuerCommonName = "ClusterIssuer2", X509StoreNames = "My, Root"}
                                                };
            InternalValidateClusterCertificateIssuerStoreUpdateTest(
                currentClusterCertificateIssuerStores,
                updatedClusterCertificateIssuerStores,
                ClusterManagementErrorCode.IssuerStoreCNUpgradeWithNoIntersection);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateClusterCertificateUpdateTest()
        {
            List<string> installedValidThumbprints = Utility.installedValidCertificates.Select(p => p.Item1).ToList();
            List<string> installedInvalidThumbprints = Utility.installedInvalidCertificates.Select(p => p.Item1).ToList();
            string[] uninstalledThumbprints = Utility.uninstalledCertificates;

            // positive cases: 1->2, both valid
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                null,
                installedValidThumbprints[0],
                installedValidThumbprints[1]);
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                null,
                installedValidThumbprints[1],
                installedValidThumbprints[0]);

            // positive cases: 2->1, both valid
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                installedValidThumbprints[1],
                installedValidThumbprints[0],
                null);
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                installedValidThumbprints[1],
                installedValidThumbprints[1],
                null);

            // positive cases: 2->1, only 1 valid
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                installedInvalidThumbprints[0],
                installedValidThumbprints[0],
                null);
            this.InternalValidateClusterCertUpdateTest(
                installedInvalidThumbprints[0],
                installedValidThumbprints[0],
                installedValidThumbprints[0],
                null);

            // positive cases: 1->1, both valid
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                null,
                installedValidThumbprints[1],
                null);

            // negative cases: no existing cert is installed
            this.InternalValidateClusterCertUpdateTest(
                uninstalledThumbprints[0],
                null,
                installedValidThumbprints[1],
                null,
                ClusterManagementErrorCode.CertificateNotInstalled);
            this.InternalValidateClusterCertUpdateTest(
                uninstalledThumbprints[0],
                uninstalledThumbprints[1],
                uninstalledThumbprints[0],
                null,
                ClusterManagementErrorCode.CertificateNotInstalled);

            // negative cases: no existing cert is valid
            this.InternalValidateClusterCertUpdateTest(
                installedInvalidThumbprints[0],
                null,
                installedValidThumbprints[0],
                null,
                ClusterManagementErrorCode.CertificateInvalid);
            this.InternalValidateClusterCertUpdateTest(
                installedInvalidThumbprints[0],
                installedInvalidThumbprints[1],
                installedInvalidThumbprints[0],
                null,
                ClusterManagementErrorCode.CertificateInvalid);

            // negative cases: Add. The unchanged cert is not installed
            this.InternalValidateClusterCertUpdateTest(
                uninstalledThumbprints[0],
                null,
                uninstalledThumbprints[0],
                installedValidThumbprints[0],
                ClusterManagementErrorCode.CertificateNotInstalled);
            this.InternalValidateClusterCertUpdateTest(
                uninstalledThumbprints[0],
                null,
                installedValidThumbprints[0],
                uninstalledThumbprints[0],
                ClusterManagementErrorCode.CertificateNotInstalled);

            // negative cases: Remove. The unchanged cert is not installed
            this.InternalValidateClusterCertUpdateTest(
                uninstalledThumbprints[0],
                installedValidThumbprints[0],
                uninstalledThumbprints[0],
                null,
                ClusterManagementErrorCode.CertificateNotInstalled);
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                uninstalledThumbprints[0],
                uninstalledThumbprints[0],
                null,
                ClusterManagementErrorCode.CertificateNotInstalled);

            // negative cases: Add. The unchanged cert is not valid
            this.InternalValidateClusterCertUpdateTest(
                installedInvalidThumbprints[0],
                null,
                installedInvalidThumbprints[0],
                installedValidThumbprints[0],
                ClusterManagementErrorCode.CertificateInvalid);
            this.InternalValidateClusterCertUpdateTest(
                installedInvalidThumbprints[0],
                null,
                installedValidThumbprints[0],
                installedInvalidThumbprints[0],
                ClusterManagementErrorCode.CertificateInvalid);

            // negative cases: Remove. The unchanged cert is not valid
            this.InternalValidateClusterCertUpdateTest(
                installedInvalidThumbprints[0],
                installedValidThumbprints[0],
                installedInvalidThumbprints[0],
                null,
                ClusterManagementErrorCode.CertificateInvalid);
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                installedInvalidThumbprints[0],
                installedInvalidThumbprints[0],
                null,
                ClusterManagementErrorCode.CertificateInvalid);

            // negative cases: The new cert is not installed
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                null,
                uninstalledThumbprints[0],
                null,
                ClusterManagementErrorCode.CertificateNotInstalled);
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                null,
                uninstalledThumbprints[0],
                installedValidThumbprints[0],
                ClusterManagementErrorCode.CertificateNotInstalled);
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                null,
                installedValidThumbprints[0],
                uninstalledThumbprints[0],
                ClusterManagementErrorCode.CertificateNotInstalled);

            // negative cases: The new cert is not valid
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                null,
                installedInvalidThumbprints[0],
                null,
                ClusterManagementErrorCode.CertificateInvalid);
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                null,
                installedInvalidThumbprints[0],
                installedValidThumbprints[0],
                ClusterManagementErrorCode.CertificateInvalid);
            this.InternalValidateClusterCertUpdateTest(
                installedValidThumbprints[0],
                null,
                installedValidThumbprints[0],
                installedInvalidThumbprints[0],
                ClusterManagementErrorCode.CertificateInvalid);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateRepairManagerUpgradeTest()
        {
            IUserConfig[] cfgsWithoutRm = new IUserConfig[]
            {
                new StandAloneUserConfig() { AddonFeatures = null },
                new StandAloneUserConfig(),
                new StandAloneUserConfig() { AddonFeatures = new List<AddonFeature>() { AddonFeature.DnsService } }
            };

            IUserConfig[] cfgsWithRm = new IUserConfig[]
            {
                new StandAloneUserConfig() { AddonFeatures = new List<AddonFeature>() { AddonFeature.RepairManager } },
                new StandAloneUserConfig() { AddonFeatures = new List<AddonFeature>() { AddonFeature.DnsService, AddonFeature.RepairManager } },
            };

            this.InternalValidateRepairManagerUpgradeTest(cfgsWithoutRm, cfgsWithoutRm, shouldPass: true);
            this.InternalValidateRepairManagerUpgradeTest(cfgsWithoutRm, cfgsWithRm, shouldPass: true);
            this.InternalValidateRepairManagerUpgradeTest(cfgsWithRm, cfgsWithoutRm, shouldPass: false);
            this.InternalValidateRepairManagerUpgradeTest(cfgsWithRm, cfgsWithRm, shouldPass: true);
        }

        internal void InternalValidateRepairManagerUpgradeTest(IUserConfig[] currentCfgs, IUserConfig[] newCfgs, bool shouldPass)
        {
            foreach (IUserConfig currentCfg in currentCfgs)
            {
                foreach (IUserConfig newCfg in newCfgs)
                {
                    try
                    {
                        StandaloneSettingsValidator.ValidateRepairManager(currentCfg, newCfg);
                        if (!shouldPass)
                        {
                            throw new Exception("test failed!");
                        }
                    }
                    catch (ValidationException ex)
                    {
                        if (shouldPass)
                        {
                            throw new Exception("test failed!", ex);
                        }
                    }
                }
            }
        }

        internal void InternalValidateClusterCertificateIssuerThumbprintUpdateTest(string originalCns, string originalIssuerThumbprints, string updatedCns, string updatedIssuerThumbprints, ClusterManagementErrorCode? expectedErrorCode = null)
        {
            Utility.ValidateExpectedValidationException(
                delegate
                {
                    ServerCertificateCommonNames original = ConstructServerCns(originalCns, originalIssuerThumbprints);
                    ServerCertificateCommonNames updated = ConstructServerCns(updatedCns, updatedIssuerThumbprints);

                    StandaloneSettingsValidator.ValidateClusterCertificateIssuerThumbprintUpdate(
                        original,
                        updated,
                        new StandAloneTraceLogger("StandAloneDeploymentManager"));
                },
                expectedErrorCode);
        }

        internal void InternalValidateClusterCertificateIssuerStoreUpdateTest(List<CertificateIssuerStore> originalClusterIssuerStore , List<CertificateIssuerStore> updatedClusterIssuerStore, ClusterManagementErrorCode? expectedErrorCode = null)
        {
            Utility.ValidateExpectedValidationException(
                delegate
                {
                    StandaloneSettingsValidator.ValidateClusterIssuerStoreUpdate(
                        originalClusterIssuerStore,
                        updatedClusterIssuerStore,
                        new StandAloneTraceLogger("StandAloneDeploymentManager"));
                },
                expectedErrorCode);
        }

        internal ServerCertificateCommonNames ConstructServerCns(string cns, string issuerThumbprints)
        {
            string[] cnList = cns.Split(';');
            string[] thumbprintList = issuerThumbprints != null ? issuerThumbprints.Split(';') : null;

            ServerCertificateCommonNames result = new ServerCertificateCommonNames()
            {
                CommonNames = new List<CertificateCommonNameBase>()
            };

            for (int i = 0; i < cnList.Length; i++)
            {
                result.CommonNames.Add(new CertificateCommonNameBase()
                {
                    CertificateCommonName = cnList[i],
                    CertificateIssuerThumbprint = thumbprintList != null ? thumbprintList[i] : null,
                });
            }

            return result;
        }

        internal void InternalValidateClusterThumbprintUpdateTest(
            string originalThumbprint,
            string originalSecondaryThumbprint,
            string updatedThumbprint,
            string updatedSecondaryThumbprint,
            ClusterManagementErrorCode? expectedErrorCode = null)
        {
            CertificateDescription currentCert = new CertificateDescription()
            {
                Thumbprint = originalThumbprint,
                ThumbprintSecondary = originalSecondaryThumbprint,
            };

            CertificateDescription updatedCert = new CertificateDescription()
            {
                Thumbprint = updatedThumbprint,
                ThumbprintSecondary = updatedSecondaryThumbprint,
            };

            Utility.ValidateExpectedValidationException(
                delegate
                {
                    StandaloneSettingsValidator.ValidateClusterCertificateThumbprintAndCnUpdate(
                        StandaloneSettingsValidator.GetUniqueThumbprints(currentCert),
                        StandaloneSettingsValidator.GetUniqueThumbprints(updatedCert),
                        true,
                        true);
                },
                expectedErrorCode);
        }

        internal void InternalValidateClusterCnUpdateTest(
            string originalCns,
            string updatedCns,
            ClusterManagementErrorCode? expectedErrorCode = null)
        {
            Utility.ValidateExpectedValidationException(
                delegate
                {
                    X509 originalSecurity = new X509()
                    {
                        ClusterCertificateCommonNames = new ServerCertificateCommonNames()
                        {
                            CommonNames = new List<CertificateCommonNameBase>(originalCns.Split(',').ToList().Select(p => new CertificateCommonNameBase() { CertificateCommonName = p }))
                        }
                    };
                    X509 updatedSecurity = new X509()
                    {
                        ClusterCertificateCommonNames = new ServerCertificateCommonNames()
                        {
                            CommonNames = new List<CertificateCommonNameBase>(updatedCns.Split(',').ToList().Select(p => new CertificateCommonNameBase() { CertificateCommonName = p }))
                        }
                    };

                    List<string> originalThumbprintsOrCns = StandaloneSettingsValidator.GetClusterCertUniqueThumbprintsOrCommonNames(originalSecurity);
                    List<string> updatedThumbprintsOrCns = StandaloneSettingsValidator.GetClusterCertUniqueThumbprintsOrCommonNames(updatedSecurity);

                    StandaloneSettingsValidator.ValidateClusterCertificateThumbprintAndCnUpdate(
                        originalThumbprintsOrCns,
                        updatedThumbprintsOrCns,
                        false,
                        false);
                },
                expectedErrorCode);
        }

        internal void InternalValidateClusterCertUpdateTest(
            string originalThumbprint,
            string originalSecondaryThumbprint,
            string updatedThumbprint,
            string updatedSecondaryThumbprint,
            ClusterManagementErrorCode? expectedErrorCode = null)
        {
            CertificateDescription currentCert = new CertificateDescription()
            {
                Thumbprint = originalThumbprint,
                ThumbprintSecondary = originalSecondaryThumbprint,
            };

            CertificateDescription updatedCert = new CertificateDescription()
            {
                Thumbprint = updatedThumbprint,
                ThumbprintSecondary = updatedSecondaryThumbprint,
            };

            Utility.ValidateExpectedValidationException(
                delegate
                {
                    StandaloneSettingsValidator.ValidateClusterCertificateUpdate(
                        new X509() { ClusterCertificate = currentCert },
                        new X509() { ClusterCertificate = updatedCert },
                        new StandAloneTraceLogger("StandAloneDeploymentManager"));
                },
                expectedErrorCode);
        }

        internal void InternalValidateAllCertsInstalledOnAllNodesTest(IEnumerable<string> thumbprints, ClusterManagementErrorCode? expectedErrorCode)
        {
            Utility.ValidateExpectedValidationException(
                delegate
                {
                    StandaloneSettingsValidator.ValidateAllCertsInstalledOnAllNodes(
                        thumbprints,
                        StoreName.My,
                        X509FindType.FindByThumbprint,
                        new string[] { "localhost" },
                        new StandAloneTraceLogger("StandAloneDeploymentManager"));
                },
                expectedErrorCode);
        }

        internal void InternalValidateEachCertInstalledOnAtLeastOneNodeTest(IEnumerable<string> thumbprints, ClusterManagementErrorCode? expectedErrorCode)
        {
            Utility.ValidateExpectedValidationException(
                delegate
                {
                    StandaloneSettingsValidator.ValidateEachCertInstalledOnCurrentNode(
                        thumbprints,
                        StoreName.My,
                        X509FindType.FindByThumbprint,
                        new StandAloneTraceLogger("StandAloneDeploymentManager"));
                },
                expectedErrorCode);
        }

        internal void InternalValidateAtLeastOneCertInstalledOnOneNodeTest(IEnumerable<string> thumbprints, ClusterManagementErrorCode? expectedErrorCode)
        {
            Utility.ValidateExpectedValidationException(
                delegate
                {
                    StandaloneSettingsValidator.ValidateAtLeastOneCertInstalledOnCurrentNode(
                        thumbprints,
                        StoreName.My,
                        X509FindType.FindByThumbprint,
                        new StandAloneTraceLogger("StandAloneDeploymentManager"));
                },
                expectedErrorCode);
        }

        internal void InternalValidateClusterCertificateInstallationTest(string[] findValues, string[] issuerThumbprints, string certType, ClusterManagementErrorCode? expectedErrorCode)
        {
            if (issuerThumbprints == null)
            {
                IEnumerable<CertificateDescription> certs = findValues.Select(p => new CertificateDescription() { Thumbprint = p });
                foreach (CertificateDescription cert in certs)
                {
                    var x509Cert = new X509();
                    switch (certType)
                    {
                        case "Cluster":
                            x509Cert.ClusterCertificate = cert;
                            break;
                        case "Server":
                            x509Cert.ServerCertificate = cert;
                            break;
                        case "ReverseProxy":
                            x509Cert.ReverseProxyCertificate = cert;
                            break;

                    }

                    Utility.ValidateExpectedValidationException(
                        delegate
                        {
                            StandaloneSettingsValidator.ValidateCertificateInstallation(
                                x509Cert,
                                new string[] { "localhost" },
                                new StandAloneTraceLogger("StandAloneDeploymentManager"));
                        },
                        expectedErrorCode);
                }
            }
            else
            {
                List<ServerCertificateCommonNames> certs = new List<ServerCertificateCommonNames>();
                for (int i = 0; i < findValues.Count(); i++)
                {
                    ServerCertificateCommonNames cert = new ServerCertificateCommonNames() { CommonNames = new List<CertificateCommonNameBase>() };
                    cert.CommonNames.Add(new CertificateCommonNameBase() { CertificateCommonName = findValues[i], CertificateIssuerThumbprint = issuerThumbprints[i] });
                    certs.Add(cert);
                }

                foreach (ServerCertificateCommonNames cert in certs)
                {
                    var x509Cert = new X509();
                    switch (certType)
                    {
                        case "Cluster":
                            x509Cert.ClusterCertificateCommonNames = cert;
                            break;
                        case "Server":
                            x509Cert.ServerCertificateCommonNames = cert;
                            break;
                        case "ReverseProxy":
                            x509Cert.ReverseProxyCertificateCommonNames = cert;
                            break;

                    }

                    Utility.ValidateExpectedValidationException(
                        delegate
                        {
                            StandaloneSettingsValidator.ValidateCertificateInstallation(
                                x509Cert,
                                new string[] { "localhost" },
                                new StandAloneTraceLogger("StandAloneDeploymentManager"));
                        },
                        expectedErrorCode);
                }
            }
        }

        internal void InternalGetUniqueThumbprintsTest(CertificateDescription cert, int expectedThumbprints)
        {
            IEnumerable<string> thumbprints = StandaloneSettingsValidator.GetUniqueThumbprints(cert);

            Assert.AreEqual(expectedThumbprints, thumbprints.Count());
            Assert.IsTrue(thumbprints.Contains(cert.Thumbprint));
            if (expectedThumbprints > 1)
            {
                Assert.IsTrue(thumbprints.Contains(cert.ThumbprintSecondary));
            }
        }
    }
}