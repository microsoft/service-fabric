// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using System;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Collections.Generic;
    using System.Linq;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;

    /// <summary>
    /// cluster cert upgrade test
    /// </summary>
    [TestClass]
    public class ClusterCertUpgradeTest
    {
        private const string BaselineConfigVersion = "1.0.0";

        private const string TargetConfigVersion = "2.0.0";

        private const int BaselineDataDeletionAgeInDays = 1;

        private const int TargetDataDeletionAgeInDays = 2;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneAddCertTest()
        {
            this.InternalStandAloneAddThumbprintCertTest("myClusterConfig.X509.DevCluster.AddPrimary.V1.json", "myClusterConfig.X509.DevCluster.AddPrimary.V2.json");

            this.InternalStandAloneAddThumbprintCertTest("myClusterConfig.X509.DevCluster.AddSecondary.V1.json", "myClusterConfig.X509.DevCluster.AddSecondary.V2.json");

            this.InternalStandAloneAddCnCertTest("myClusterConfig.X509.DevCluster.AddCn1.V1.json", "myClusterConfig.X509.DevCluster.AddCn1.V2.json");

            this.InternalStandAloneAddCnCertTest("myClusterConfig.X509.DevCluster.AddCn2.V1.json", "myClusterConfig.X509.DevCluster.AddCn2.V2.json");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneAddCertRollbackTest()
        {
            // thumbprint
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddPrimary.V1.json",
                "myClusterConfig.X509.DevCluster.AddPrimary.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddSecondary.V1.json",
                "myClusterConfig.X509.DevCluster.AddSecondary.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);

            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddPrimary.V1.json",
                "myClusterConfig.X509.DevCluster.AddPrimary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddSecondary.V1.json",
                "myClusterConfig.X509.DevCluster.AddSecondary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);

            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddPrimary.V1.json",
                "myClusterConfig.X509.DevCluster.AddPrimary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddSecondary.V1.json",
                "myClusterConfig.X509.DevCluster.AddSecondary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);

            // cn
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddCn1.V1.json",
                "myClusterConfig.X509.DevCluster.AddCn1.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddCn2.V1.json",
                "myClusterConfig.X509.DevCluster.AddCn2.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);

            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddCn1.V1.json",
                "myClusterConfig.X509.DevCluster.AddCn1.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddCn2.V1.json",
                "myClusterConfig.X509.DevCluster.AddCn2.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);

            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddCn1.V1.json",
                "myClusterConfig.X509.DevCluster.AddCn1.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.AddCn2.V1.json",
                "myClusterConfig.X509.DevCluster.AddCn2.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneRemoveCertTest()
        {
            this.InternalStandAloneRemoveThumbprintCertTest("myClusterConfig.X509.DevCluster.RemovePrimary.V1.json", "myClusterConfig.X509.DevCluster.RemovePrimary.V2.json");

            this.InternalStandAloneRemoveThumbprintCertTest("myClusterConfig.X509.DevCluster.RemoveSecondary.V1.json", "myClusterConfig.X509.DevCluster.RemoveSecondary.V2.json");

            this.InternalStandAloneRemoveCnCertTest("myClusterConfig.X509.DevCluster.RemoveCn1.V1.json", "myClusterConfig.X509.DevCluster.RemoveCn1.V2.json");

            this.InternalStandAloneRemoveCnCertTest("myClusterConfig.X509.DevCluster.RemoveCn2.V1.json", "myClusterConfig.X509.DevCluster.RemoveCn2.V2.json");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneRemoveCertRollbackTest()
        {
            // thumbprint
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemovePrimary.V1.json",
                "myClusterConfig.X509.DevCluster.RemovePrimary.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemoveSecondary.V1.json",
                "myClusterConfig.X509.DevCluster.RemoveSecondary.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);

            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemovePrimary.V1.json",
                "myClusterConfig.X509.DevCluster.RemovePrimary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemoveSecondary.V1.json",
                "myClusterConfig.X509.DevCluster.RemoveSecondary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);

            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemovePrimary.V1.json",
                "myClusterConfig.X509.DevCluster.RemovePrimary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemoveSecondary.V1.json",
                "myClusterConfig.X509.DevCluster.RemoveSecondary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);

            // cn
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemoveCn1.V1.json",
                "myClusterConfig.X509.DevCluster.RemoveCn1.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemoveCn2.V1.json",
                "myClusterConfig.X509.DevCluster.RemoveCn2.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);

            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemoveCn1.V1.json",
                "myClusterConfig.X509.DevCluster.RemoveCn1.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemoveCn2.V1.json",
                "myClusterConfig.X509.DevCluster.RemoveCn2.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);

            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemoveCn1.V1.json",
                "myClusterConfig.X509.DevCluster.RemoveCn1.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);
            this.InternalStandAloneTwoPhaseRollbackTest(
                "myClusterConfig.X509.DevCluster.RemoveCn2.V1.json",
                "myClusterConfig.X509.DevCluster.RemoveCn2.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneReplaceCertTest()
        {
            this.InternalStandAloneReplaceCertTest("myClusterConfig.X509.DevCluster.Replace.V1.json", "myClusterConfig.X509.DevCluster.Replace.V2.json", FabricCertificateTypeX509FindType.FindByThumbprint);

            this.InternalStandAloneReplaceCertTest("myClusterConfig.X509.DevCluster.ReplaceCn.V1.json", "myClusterConfig.X509.DevCluster.ReplaceCn.V2.json", FabricCertificateTypeX509FindType.FindBySubjectName);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneReplaceCertRollbackTest()
        {
            string baselineFile = "myClusterConfig.X509.DevCluster.Replace.V1.json";
            string targetFile= "myClusterConfig.X509.DevCluster.Replace.V2.json";

            this.InternalStandAloneReplaceCertRollbackTest(baselineFile, targetFile, maxSuccessIteration: -1, rollbackSuccess: false);
            this.InternalStandAloneReplaceCertRollbackTest(baselineFile, targetFile, maxSuccessIteration: 0, rollbackSuccess: true);
            this.InternalStandAloneReplaceCertRollbackTest(baselineFile, targetFile, maxSuccessIteration: 0, rollbackSuccess: false);
            this.InternalStandAloneReplaceCertRollbackTest(baselineFile, targetFile, maxSuccessIteration: 1, rollbackSuccess: true);
            this.InternalStandAloneReplaceCertRollbackTest(baselineFile, targetFile, maxSuccessIteration: 1, rollbackSuccess: false);

            baselineFile = "myClusterConfig.X509.DevCluster.ReplaceCn.V1.json";
            targetFile = "myClusterConfig.X509.DevCluster.ReplaceCn.V2.json";
            this.InternalStandAloneReplaceCertRollbackTest(baselineFile, targetFile, maxSuccessIteration: -1, rollbackSuccess: false);
            this.InternalStandAloneReplaceCertRollbackTest(baselineFile, targetFile, maxSuccessIteration: 0, rollbackSuccess: true);
            this.InternalStandAloneReplaceCertRollbackTest(baselineFile, targetFile, maxSuccessIteration: 0, rollbackSuccess: false);
            this.InternalStandAloneReplaceCertRollbackTest(baselineFile, targetFile, maxSuccessIteration: 1, rollbackSuccess: true);
            this.InternalStandAloneReplaceCertRollbackTest(baselineFile, targetFile, maxSuccessIteration: 1, rollbackSuccess: false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneChangeCertTypeTest()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.ChangeCertType.V1.json");
            Assert.AreEqual(ClusterCertUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneCluster("myClusterConfig.X509.DevCluster.ChangeCertType.V2.json", cluster);

            string originalValue1 = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            string originalValue2 = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary;
            string newValue1 = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName;
            string newValue2 = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[1].CertificateCommonName;

            // 1st iteration
            Assert.IsTrue(Utility.RunStateMachine(cluster));

            StandAloneCertificateClusterUpgradeState upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestChangeCertTypeLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalValue1,
                originalValue2,
                newValue1,
                newValue2,
                upgradeState.CurrentListIndex);

            Utility.RollForwardOneIteration(cluster);

            // 2nd iteration
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestChangeCertTypeLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalValue1,
                originalValue2,
                newValue1,
                newValue2,
                upgradeState.CurrentListIndex);

            Utility.RollForwardOneIteration(cluster);

            //3rd iteration
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestChangeCertTypeLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalValue1,
                originalValue2,
                newValue1,
                newValue2,
                upgradeState.CurrentListIndex);

            Utility.RollForwardOneIteration(cluster);

            // final check
            Assert.IsFalse(Utility.RunStateMachine(cluster));

            this.VerifyJsonChangeCertTypeLists(cluster.Current.CSMConfig, newValue1, newValue2);

            Assert.AreEqual(ClusterCertUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(ClusterCertUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneIssuerThumbprintUpdateTest()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.UpdateIssuerThumbprint.V1.json");
            Assert.AreEqual(ClusterCertUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);
            string originalIssuerThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint;
            Assert.IsFalse(string.IsNullOrWhiteSpace(originalIssuerThumbprint));

            string originalServerCertIssuerThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ServerCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint;
            Assert.IsFalse(string.IsNullOrWhiteSpace(originalServerCertIssuerThumbprint));
            Assert.AreEqual(1, cluster.Current.CSMConfig.Security.CertificateInformation.ServerCertificateCommonNames.CommonNames.Count);

            Utility.UpdateStandAloneCluster("myClusterConfig.X509.DevCluster.UpdateIssuerThumbprint.V2.json", cluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));
            
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);

            string newIssuerThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint;
            Assert.IsFalse(string.IsNullOrWhiteSpace(newIssuerThumbprint));
            Assert.AreNotEqual(originalIssuerThumbprint, newIssuerThumbprint);

            string newServerCertIssuerThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ServerCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint;
            Assert.IsFalse(string.IsNullOrWhiteSpace(newServerCertIssuerThumbprint));
            Assert.AreNotEqual(originalServerCertIssuerThumbprint, newServerCertIssuerThumbprint);
            Assert.AreEqual(2, cluster.Current.CSMConfig.Security.CertificateInformation.ServerCertificateCommonNames.CommonNames.Count);

            Assert.AreEqual(ClusterCertUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneIssuerStoreUpdateTest()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.UpdateIssuerStore.V1.json");
            Assert.AreEqual(ClusterCertUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);
            var clusterIssuerStores = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateIssuerStores;
            Assert.IsTrue(clusterIssuerStores.Count() == 2, "clusterIssuerStores count should be 2");
            Assert.AreEqual(clusterIssuerStores[0].IssuerCommonName, "ClusterIssuer1", "clusterIssuerStores[0].IssuerCommonName should be ClusterIssuer1");
            Assert.AreEqual(clusterIssuerStores[0].X509StoreNames, "My, Root", "clusterIssuerStores[0].X509StoreNames should be My, Root");
            Assert.AreEqual(clusterIssuerStores[1].IssuerCommonName, "", "clusterIssuerStores[1].IssuerCommonName should be empty");
            Assert.AreEqual(clusterIssuerStores[1].X509StoreNames, "My", "clusterIssuerStores[0].X509StoreNames should be My");

            var serverIssuerStores = cluster.Current.CSMConfig.Security.CertificateInformation.ServerCertificateIssuerStores;
            Assert.IsTrue(serverIssuerStores.Count() == 1, "serverIssuerStores count should be 1");
            Assert.AreEqual(serverIssuerStores[0].IssuerCommonName, "ServerIssuer1", "serverIssuerStores[0].IssuerCommonName should be ServerIssuer1");
            Assert.AreEqual(serverIssuerStores[0].X509StoreNames, "My", "serverIssuerStores[0].X509StoreNames should be My");

            var clientIssuerStores = cluster.Current.CSMConfig.Security.CertificateInformation.ClientCertificateIssuerStores;
            Assert.IsTrue(clientIssuerStores.Count() == 1, "clientIssuerStores count should be 2");
            Assert.AreEqual(clientIssuerStores[0].IssuerCommonName, "ClientIssuer1", "(clientIssuerStores[0].IssuerCommonNameshould be ClientIssuer1");
            Assert.AreEqual(clientIssuerStores[0].X509StoreNames, "My", "clientIssuerStores[0].X509StoreNames should be My");

            Utility.UpdateStandAloneCluster("myClusterConfig.X509.DevCluster.UpdateIssuerStore.V2.json", cluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            var cm = upgradeState.ExternalState.ClusterManifest;
            var clusterCertificateIssuerStore = cm.FabricSettings.ToList().Where(section => section.Name == StringConstants.SectionName.SecurityClusterCertificateIssuerStores).ToList();
            Assert.IsTrue(clusterCertificateIssuerStore.Count() == 1, "clusterCertificateIssuerStore count should be 1");
            Assert.IsTrue(clusterCertificateIssuerStore[0].Parameter.ToList().Count == 2, "clusterCertificateIssuerStore parameter count should be 2");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[0].Name, "ClusterIssuer1");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[0].Value, "My, Root, NewStore");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[1].Name, "ClusterIssuer2");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[1].Value, "My");

            var serverCertificateIssuerStore = cm.FabricSettings.ToList().Where(section => section.Name == StringConstants.SectionName.SecurityServerCertificateIssuerStores).ToList();
            Assert.IsTrue(serverCertificateIssuerStore.Count() == 1, "serverCertificateIssuerStore count should be 1");
            Assert.IsTrue(serverCertificateIssuerStore[0].Parameter.ToList().Count == 1, "serverCertificateIssuerStore parameter count should be 1");
            Assert.AreEqual(serverCertificateIssuerStore[0].Parameter[0].Name, "ServerIssuer2");
            Assert.AreEqual(serverCertificateIssuerStore[0].Parameter[0].Value, "My");

            var clientCertificateIssuerStore = cm.FabricSettings.ToList().Where(section => section.Name == StringConstants.SectionName.SecurityClientCertificateIssuerStores).ToList();
            Assert.IsTrue(clientCertificateIssuerStore.Count() == 1, "clientCertificateIssuerStore count should be 1");
            Assert.IsTrue(clientCertificateIssuerStore[0].Parameter.ToList().Count == 1, "clientCertificateIssuerStore parameter count should be 1");
            Assert.AreEqual(clientCertificateIssuerStore[0].Parameter[0].Name, "ClientIssuer2");
            Assert.AreEqual(clientCertificateIssuerStore[0].Parameter[0].Value, "My");

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);

            var newClusterIssuerStores = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateIssuerStores;
            Assert.IsTrue(newClusterIssuerStores.Count() == 2, "newClusterIssuerStores count should be 2");
            Assert.AreEqual(newClusterIssuerStores[0].IssuerCommonName, "ClusterIssuer1", "newClusterIssuerStores[0].IssuerCommonName should be ClusterIssuer1");
            Assert.AreEqual(newClusterIssuerStores[0].X509StoreNames, "My, Root, NewStore", "newClusterIssuerStores[0].X509StoreNames should be My, Root");
            Assert.AreEqual(newClusterIssuerStores[1].IssuerCommonName, "ClusterIssuer2", "newClusterIssuerStores[1].IssuerCommonName should be empty");
            Assert.AreEqual(newClusterIssuerStores[1].X509StoreNames, "My", "newClusterIssuerStores[0].X509StoreNames should be My");

            var newServerIssuerStores = cluster.Current.CSMConfig.Security.CertificateInformation.ServerCertificateIssuerStores;
            Assert.IsTrue(newServerIssuerStores.Count() == 1, "newServerIssuerStores count should be 1");
            Assert.AreEqual(newServerIssuerStores[0].IssuerCommonName, "ServerIssuer2", "newServerIssuerStores[0].IssuerCommonName should be ServerIssuer1");
            Assert.AreEqual(newServerIssuerStores[0].X509StoreNames, "My", "newServerIssuerStores[0].X509StoreNames should be My");

            var newClientIssuerStores = cluster.Current.CSMConfig.Security.CertificateInformation.ClientCertificateIssuerStores;
            Assert.IsTrue(newClientIssuerStores.Count() == 1, "clientIssuerStores count should be 1");
            Assert.AreEqual(newClientIssuerStores[0].IssuerCommonName, "ClientIssuer2", "(newClientIssuerStores[0].IssuerCommonNameshould be ClientIssuer1");
            Assert.AreEqual(newClientIssuerStores[0].X509StoreNames, "My", "newClientIssuerStores[0].X509StoreNames should be My");

            Assert.AreEqual(ClusterCertUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneIssuerStoreAndCnUpdateTest()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.UpdateIssuerStore.V1.json");
            Assert.AreEqual(ClusterCertUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneCluster("myClusterConfig.X509.DevCluster.UpdateIssuerStoreAndCn.V2.json", cluster);

            Tuple<string, string> originalCn1 = new Tuple<string, string>(cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName, cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint);
            Tuple<string, string> newCn1 = new Tuple<string, string>(cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName, cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint);
            Tuple<string, string> newCn2 = new Tuple<string, string>(cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[1].CertificateCommonName, cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[1].CertificateIssuerThumbprint);
            Tuple<string, string> newCn = originalCn1.Item1 == newCn1.Item1 ? newCn2 : newCn1;
            bool addCn1 = originalCn1.Item1 != newCn1.Item1;

            // 1st iteration - update whitelist for CN and issuers
            Assert.IsTrue(Utility.RunStateMachine(cluster));

            StandAloneCertificateClusterUpgradeState upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestAddCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalCn1,
                newCn,
                addCn1,
                upgradeState.CurrentListIndex,
                FabricCertificateTypeX509FindType.FindBySubjectName);

            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            var cm = upgradeState.ExternalState.ClusterManifest;
            var clusterCertificateIssuerStore = cm.FabricSettings.ToList().Where(section => section.Name == StringConstants.SectionName.SecurityClusterCertificateIssuerStores).ToList();
            Assert.IsTrue(clusterCertificateIssuerStore.Count() == 1, "clusterCertificateIssuerStore count should be 1");
            Assert.IsTrue(clusterCertificateIssuerStore[0].Parameter.ToList().Count == 2, "clusterCertificateIssuerStore parameter count should be 2");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[0].Name, "ClusterIssuer1");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[0].Value, "My, Root, NewStore");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[1].Name, "ClusterIssuer2");
            Assert.AreEqual(clusterCertificateIssuerStore[0].Parameter[1].Value, "My");

            var serverCertificateIssuerStore = cm.FabricSettings.ToList().Where(section => section.Name == StringConstants.SectionName.SecurityServerCertificateIssuerStores).ToList();
            Assert.IsTrue(serverCertificateIssuerStore.Count() == 1, "serverCertificateIssuerStore count should be 1");
            Assert.IsTrue(serverCertificateIssuerStore[0].Parameter.ToList().Count == 1, "serverCertificateIssuerStore parameter count should be 1");
            Assert.AreEqual(serverCertificateIssuerStore[0].Parameter[0].Name, "ServerIssuer2");
            Assert.AreEqual(serverCertificateIssuerStore[0].Parameter[0].Value, "My");

            var clientCertificateIssuerStore = cm.FabricSettings.ToList().Where(section => section.Name == StringConstants.SectionName.SecurityClientCertificateIssuerStores).ToList();
            Assert.IsTrue(clientCertificateIssuerStore.Count() == 1, "clientCertificateIssuerStore count should be 1");
            Assert.IsTrue(clientCertificateIssuerStore[0].Parameter.ToList().Count == 1, "clientCertificateIssuerStore parameter count should be 1");
            Assert.AreEqual(clientCertificateIssuerStore[0].Parameter[0].Name, "ClientIssuer2");
            Assert.AreEqual(clientCertificateIssuerStore[0].Parameter[0].Value, "My");

            Utility.RollForwardOneIteration(cluster);

            // 2nd iteration - update load list for CN
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestAddCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalCn1,
                newCn,
                addCn1,
                upgradeState.CurrentListIndex,
                FabricCertificateTypeX509FindType.FindBySubjectName);

            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // final check
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);

            var newClusterIssuerStores = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateIssuerStores;
            Assert.IsTrue(newClusterIssuerStores.Count() == 2, "newClusterIssuerStores count should be 2");
            Assert.AreEqual(newClusterIssuerStores[0].IssuerCommonName, "ClusterIssuer1", "newClusterIssuerStores[0].IssuerCommonName should be ClusterIssuer1");
            Assert.AreEqual(newClusterIssuerStores[0].X509StoreNames, "My, Root, NewStore", "newClusterIssuerStores[0].X509StoreNames should be My, Root");
            Assert.AreEqual(newClusterIssuerStores[1].IssuerCommonName, "ClusterIssuer2", "newClusterIssuerStores[1].IssuerCommonName should be empty");
            Assert.AreEqual(newClusterIssuerStores[1].X509StoreNames, "My", "newClusterIssuerStores[0].X509StoreNames should be My");
     
            Assert.AreEqual(ClusterCertUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        internal void InternalStandAloneAddThumbprintCertTest(string baselineJsonFile, string targetJsonFile)
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade(baselineJsonFile);
            Assert.AreEqual(ClusterCertUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneCluster(targetJsonFile, cluster);

            string originalThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            string newPrimaryThumbprint = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            string newSecondarythumbprint = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary;
            string newThumbprint = originalThumbprint == newPrimaryThumbprint ? newSecondarythumbprint : newPrimaryThumbprint;
            bool addPrimarythumbprint = originalThumbprint != newPrimaryThumbprint;

            // 1st iteration
            Assert.IsTrue(Utility.RunStateMachine(cluster));

            StandAloneCertificateClusterUpgradeState upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestAddCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                new Tuple<string, string>(originalThumbprint, null),
                new Tuple<string, string>(newThumbprint, null),
                addPrimarythumbprint,
                upgradeState.CurrentListIndex,
                FabricCertificateTypeX509FindType.FindByThumbprint);

            Assert.AreEqual(ClusterCertUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // 2nd iteration
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestAddCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                new Tuple<string, string>(originalThumbprint, null),
                new Tuple<string, string>(newThumbprint, null),
                addPrimarythumbprint,
                upgradeState.CurrentListIndex,
                FabricCertificateTypeX509FindType.FindByThumbprint);

            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // final check
            Assert.IsFalse(Utility.RunStateMachine(cluster));

            this.VerifyJsonAddCertLists(cluster.Current.CSMConfig, new Tuple<string, string>(originalThumbprint, null), new Tuple<string, string>(newThumbprint, null), addPrimarythumbprint, FabricCertificateTypeX509FindType.FindByThumbprint);

            Assert.AreEqual(ClusterCertUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(ClusterCertUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        internal void InternalStandAloneAddCnCertTest(string baselineJsonFile, string targetJsonFile)
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade(baselineJsonFile);
            Assert.AreEqual(ClusterCertUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(1, cluster.Current.CSMConfig.Security.CertificateInformation.ServerCertificateCommonNames.CommonNames.Count);
            Assert.AreEqual(1, cluster.Current.CSMConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames.CommonNames.Count);

            Utility.UpdateStandAloneCluster(targetJsonFile, cluster);

            Tuple<string, string> originalCn1 = new Tuple<string, string>(cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName, cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint);
            Tuple <string, string> newCn1 = new Tuple<string, string>(cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName, cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint);
            Tuple<string, string> newCn2 = new Tuple<string, string>(cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[1].CertificateCommonName, cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[1].CertificateIssuerThumbprint);
            Tuple<string, string> newCn = originalCn1.Item1 == newCn1.Item1 ? newCn2 : newCn1;
            bool addCn1 = originalCn1.Item1 != newCn1.Item1;

            // 1st iteration
            Assert.IsTrue(Utility.RunStateMachine(cluster));

            StandAloneCertificateClusterUpgradeState upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestAddCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalCn1,
                newCn,
                addCn1,
                upgradeState.CurrentListIndex,
                FabricCertificateTypeX509FindType.FindBySubjectName);

            Assert.AreEqual(ClusterCertUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // 2nd iteration
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestAddCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalCn1,
                newCn,
                addCn1,
                upgradeState.CurrentListIndex,
                FabricCertificateTypeX509FindType.FindBySubjectName);

            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // final check
            Assert.IsFalse(Utility.RunStateMachine(cluster));

            this.VerifyJsonAddCertLists(cluster.Current.CSMConfig, originalCn1, newCn, addCn1, FabricCertificateTypeX509FindType.FindBySubjectName);

            Assert.AreEqual(ClusterCertUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(ClusterCertUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);

            List<CertificateCommonNameBase> serverCns = cluster.Current.CSMConfig.Security.CertificateInformation.ServerCertificateCommonNames.CommonNames;
            Assert.AreEqual(2, serverCns.Count);
            Assert.AreNotEqual(serverCns[0].CertificateCommonName, serverCns[1].CertificateCommonName);
            List<CertificateCommonNameBase> reverseProxyCns = cluster.Current.CSMConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames.CommonNames;
            Assert.AreEqual(2, reverseProxyCns.Count);
            Assert.AreNotEqual(reverseProxyCns[0].CertificateCommonName, reverseProxyCns[1].CertificateCommonName);
        }

        internal void InternalStandAloneRemoveThumbprintCertTest(string baselineJsonFile, string targetJsonFile)
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade(baselineJsonFile);
            Assert.AreEqual(ClusterCertUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneCluster(targetJsonFile, cluster);

            string originalPrimaryThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            string originalSecondaryThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary;
            string newPrimaryThumbprint = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            bool removePrimarythumbprint = originalPrimaryThumbprint != newPrimaryThumbprint;

            // 1st iteration
            Assert.IsTrue(Utility.RunStateMachine(cluster));

            StandAloneCertificateClusterUpgradeState upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestRemoveCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                new Tuple<string, string>(originalPrimaryThumbprint, null),
                new Tuple<string, string>(originalSecondaryThumbprint, null),
                removePrimarythumbprint,
                upgradeState.CurrentListIndex,
                FabricCertificateTypeX509FindType.FindByThumbprint);

            Assert.AreEqual(ClusterCertUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // 2nd iteration
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestRemoveCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                new Tuple<string, string>(originalPrimaryThumbprint, null),
                new Tuple<string, string>(originalSecondaryThumbprint, null),
                removePrimarythumbprint,
                upgradeState.CurrentListIndex,
                FabricCertificateTypeX509FindType.FindByThumbprint);

            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // final check
            Assert.IsFalse(Utility.RunStateMachine(cluster));

            this.VerifyJsonRemoveCertLists(
                cluster.Current.CSMConfig,
                new Tuple<string, string>(originalPrimaryThumbprint, null),
                new Tuple<string, string>(originalSecondaryThumbprint, null),
                removePrimarythumbprint,
                FabricCertificateTypeX509FindType.FindByThumbprint);

            Assert.AreEqual(ClusterCertUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(ClusterCertUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        internal void InternalStandAloneRemoveCnCertTest(string baselineJsonFile, string targetJsonFile)
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade(baselineJsonFile);
            Assert.AreEqual(ClusterCertUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(2, cluster.Current.CSMConfig.Security.CertificateInformation.ServerCertificateCommonNames.CommonNames.Count);
            Assert.AreEqual(2, cluster.Current.CSMConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames.CommonNames.Count);

            Utility.UpdateStandAloneCluster(targetJsonFile, cluster);

            Tuple<string, string> originalCn1 = new Tuple<string, string>(cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName, cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint);
            Tuple<string, string> originalCn2 = new Tuple<string, string>(cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[1].CertificateCommonName, cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[1].CertificateIssuerThumbprint);
            Tuple<string, string> newCn1 = new Tuple<string, string>(cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName, cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint);
            bool removeCn1 = originalCn1.Item1 != newCn1.Item1;

            // 1st iteration
            Assert.IsTrue(Utility.RunStateMachine(cluster));

            StandAloneCertificateClusterUpgradeState upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestRemoveCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalCn1,
                originalCn2,
                removeCn1,
                upgradeState.CurrentListIndex,
                FabricCertificateTypeX509FindType.FindBySubjectName);

            Assert.AreEqual(ClusterCertUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // 2nd iteration
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestRemoveCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalCn1,
                originalCn2,
                removeCn1,
                upgradeState.CurrentListIndex,
                FabricCertificateTypeX509FindType.FindBySubjectName);

            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // final check
            Assert.IsFalse(Utility.RunStateMachine(cluster));

            this.VerifyJsonRemoveCertLists(
                cluster.Current.CSMConfig,
                originalCn1,
                originalCn2,
                removeCn1,
                FabricCertificateTypeX509FindType.FindBySubjectName);

            Assert.AreEqual(ClusterCertUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(ClusterCertUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);

            List<CertificateCommonNameBase> serverCns = cluster.Current.CSMConfig.Security.CertificateInformation.ServerCertificateCommonNames.CommonNames;
            Assert.AreEqual(1, serverCns.Count);
            List<CertificateCommonNameBase> reverseProxyCns = cluster.Current.CSMConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames.CommonNames;
            Assert.AreEqual(1, reverseProxyCns.Count);
        }

        internal void InternalStandAloneReplaceCertTest(string baselineJsonFile, string targetJsonFile, FabricCertificateTypeX509FindType findType)
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade(baselineJsonFile);
            Assert.AreEqual(ClusterCertUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneCluster(targetJsonFile, cluster);

            string originalValue = null;
            string newValue = null;

            if (findType == FabricCertificateTypeX509FindType.FindByThumbprint)
            {
                originalValue = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
                newValue = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            }
            else
            {
                originalValue = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName;
                newValue = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName;
            }

            // 1st iteration
            Assert.IsTrue(Utility.RunStateMachine(cluster));

            StandAloneCertificateClusterUpgradeState upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestReplaceCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalValue,
                newValue,
                upgradeState.CurrentListIndex,
                findType);

            Utility.RollForwardOneIteration(cluster);

            // 2nd iteration
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestReplaceCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalValue,
                newValue,
                upgradeState.CurrentListIndex,
                findType);

            Utility.RollForwardOneIteration(cluster);

            //3rd iteration
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            upgradeState = (StandAloneCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestReplaceCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalValue,
                newValue,
                upgradeState.CurrentListIndex,
                findType);

            Utility.RollForwardOneIteration(cluster);

            // final check
            Assert.IsFalse(Utility.RunStateMachine(cluster));

            this.VerifyJsonReplaceCertLists(cluster.Current.CSMConfig, newValue, findType);

            Assert.AreEqual(ClusterCertUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(ClusterCertUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        internal void InternalStandAloneTwoPhaseRollbackTest(
            string baselineJsonFile,
            string targetJsonFile,
            bool firstIterationSuccess,
            bool rollbackSuccess)
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade(baselineJsonFile);
            Assert.AreEqual(ClusterCertUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            ClusterManifestType baselineClusterManifest = cluster.Current.ExternalState.ClusterManifest;
            Utility.UpdateStandAloneCluster(targetJsonFile, cluster);
            Assert.IsTrue(Utility.RunStateMachine(cluster));

            if (!firstIterationSuccess)
            {
                Utility.RollBackOneIteration(cluster);
                Assert.IsFalse(Utility.RunStateMachine(cluster));
                Assert.IsNull(cluster.Pending);
                Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);
            }
            else
            {
                Utility.RollForwardOneIteration(cluster);
                Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);

                Assert.IsFalse(Utility.RunStateMachine(cluster));
                Utility.RollBackOneIteration(cluster);
                Assert.IsFalse(Utility.RunStateMachine(cluster));
                Assert.AreEqual(baselineClusterManifest, cluster.Pending.ExternalState.ClusterManifest);

                if (rollbackSuccess)
                {
                    Utility.RollForwardOneIteration(cluster);
                    Assert.IsFalse(Utility.RunStateMachine(cluster));
                    Assert.IsNull(cluster.Pending);
                }
                else
                {
                    Utility.RollBackOneIteration(cluster);
                    Assert.IsFalse(Utility.RunStateMachine(cluster));
                    Assert.IsNotNull(cluster.Pending);
                }

                Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);
            }
        }

        internal void InternalStandAloneReplaceCertRollbackTest(
            string baselineJsonFile,
            string targetJsonFile,
            int maxSuccessIteration,
            bool rollbackSuccess)
        {
            Assert.IsTrue(maxSuccessIteration < 2);

            StandAloneCluster cluster = Utility.DoBaselineUpgrade(baselineJsonFile);
            Assert.AreEqual(ClusterCertUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            ClusterManifestType baselineClusterManifest = cluster.Current.ExternalState.ClusterManifest;
            Utility.UpdateStandAloneCluster(targetJsonFile, cluster);
            Assert.IsTrue(Utility.RunStateMachine(cluster));

            if (maxSuccessIteration < 0)
            {
                Utility.RollBackOneIteration(cluster);
                Assert.IsFalse(Utility.RunStateMachine(cluster));
                Assert.IsNull(cluster.Pending);
                Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);
            }
            else
            {
                for (int i = 0; i <= maxSuccessIteration; i++)
                {
                    Utility.RollForwardOneIteration(cluster);
                    Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);
                    Assert.IsFalse(Utility.RunStateMachine(cluster));
                }

                Utility.RollBackOneIteration(cluster);
                Assert.IsFalse(Utility.RunStateMachine(cluster));
                ClusterManifestType expectedManifest = maxSuccessIteration == 0 ? baselineClusterManifest : ((MultiphaseClusterUpgradeState)cluster.Pending).ClusterManifestList[maxSuccessIteration - 1];
                Assert.AreEqual(expectedManifest, cluster.Pending.ExternalState.ClusterManifest);

                if (rollbackSuccess)
                {
                    for (int i = 0; i <= maxSuccessIteration; i++)
                    {
                        Utility.RollForwardOneIteration(cluster);
                        Assert.IsFalse(Utility.RunStateMachine(cluster));
                    }

                    Assert.IsNull(cluster.Pending);
                }
                else
                {
                    Utility.RollBackOneIteration(cluster);
                    Assert.IsFalse(Utility.RunStateMachine(cluster));
                    Assert.IsNotNull(cluster.Pending);
                }

                Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);
            }
        }

        internal void VerifyManifestAddCertLists(
            ClusterManifestType manifest,
            Tuple<string, string> originalValue,
            Tuple<string, string> newValue,
            bool addValue1,
            int currentPhase,
            FabricCertificateTypeX509FindType findType)
        {
            this.VerifyLoadCertList_Add(manifest, originalValue.Item1, newValue.Item1, addValue1, currentPhase, findType);
            this.VerifyWhiteCertList_Add(manifest, originalValue, newValue, findType);
            this.VerifyFileStoreSvcCertList_Add(manifest, originalValue.Item1, newValue.Item1, addValue1, currentPhase, findType);
        }

        internal void VerifyManifestRemoveCertLists(
            ClusterManifestType manifest,
            Tuple<string, string> originalValue1,
            Tuple<string, string> originalValue2,
            bool removeValue1,
            int currentPhase,
            FabricCertificateTypeX509FindType findType)
        {
            this.VerifyLoadCertList_Remove(manifest, originalValue1.Item1, originalValue2.Item1, removeValue1, findType);
            this.VerifyWhiteCertList_Remove(manifest, originalValue1, originalValue2, removeValue1, currentPhase, findType);
            this.VerifyFileStoreSvcCertList_Remove(manifest, originalValue1.Item1, originalValue2.Item1, removeValue1, currentPhase, findType);
        }

        internal void VerifyManifestReplaceCertLists(
            ClusterManifestType manifest,
            string originalValue,
            string newValue,
            int currentPhase,
            FabricCertificateTypeX509FindType findType)
        {
            this.VerifyLoadCertList_Replace(manifest, originalValue, newValue, currentPhase, findType);
            this.VerifyWhiteCertList_Replace(manifest, originalValue, newValue, currentPhase, findType);
            this.VerifyFileStoreSvcCertList_Replace(manifest, originalValue, newValue, currentPhase, findType);
        }

        internal void VerifyManifestChangeCertTypeLists(
            ClusterManifestType manifest,
            string originalValue1,
            string originalValue2,
            string newValue1,
            string newValue2,
            int currentPhase)
        {
            this.VerifyLoadCertList_ChangeType(manifest, originalValue1, originalValue2, newValue1, newValue2, currentPhase);
            this.VerifyWhiteCertList_ChangeType(manifest, originalValue1, originalValue2, newValue1, newValue2, currentPhase);
            this.VerifyFileStoreSvcCertList_ChangeType(manifest, originalValue1, originalValue2, newValue1, newValue2, currentPhase);
        }

        internal void VerifyJsonAddCertLists(
            IUserConfig csmConfig,
            Tuple<string, string> originalValue,
            Tuple<string, string> newValue,
            bool addValue1,
            FabricCertificateTypeX509FindType findType)
        {
            Tuple<string, string> expectedValue1 = addValue1 ? newValue : originalValue;
            Tuple<string, string> expectedValue2 = addValue1 ? originalValue : newValue;
            Tuple<string, string> actualValue1 = null;
            Tuple<string, string> actualValue2 = null;

            if (findType == FabricCertificateTypeX509FindType.FindByThumbprint)
            {
                actualValue1 = new Tuple<string, string>(csmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint, null);
                actualValue2 = new Tuple<string, string>(csmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary, null);
            }
            else
            {
                actualValue1 = new Tuple<string, string>(csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName, csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint);
                actualValue2 = new Tuple<string, string>(csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[1].CertificateCommonName, csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[1].CertificateIssuerThumbprint);
            }

            Assert.AreEqual(expectedValue1, actualValue1);
            Assert.AreEqual(expectedValue2, actualValue2);
        }

        internal void VerifyJsonRemoveCertLists(
            IUserConfig csmConfig,
            Tuple<string, string> originalValue1,
            Tuple<string, string> originalValue2,
            bool removeValue1,
            FabricCertificateTypeX509FindType findType)
        {
            Tuple<string, string> expectedValue1 = removeValue1 ? originalValue2 : originalValue1;
            Tuple<string, string> expectedValue2 = new Tuple<string, string>(null, null);
            Tuple<string, string> actualValue1 = null;
            Tuple<string, string> actualValue2 = null;

            if (findType == FabricCertificateTypeX509FindType.FindByThumbprint)
            {
                actualValue1 = new Tuple<string, string>(csmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint, null);
                actualValue2 = new Tuple<string, string>(csmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary, null);
            }
            else
            {
                actualValue1 = new Tuple<string, string>(csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName, csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateIssuerThumbprint);
                Assert.AreEqual(1, csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames.Count);
                actualValue2 = new Tuple<string, string>(null, null);
            }

            Assert.AreEqual(expectedValue1, actualValue1);
            Assert.AreEqual(expectedValue2, actualValue2);
        }

        internal void VerifyJsonReplaceCertLists(
            IUserConfig csmConfig,
            string newValue,
            FabricCertificateTypeX509FindType fineType)
        {
            string expectedValue1 = newValue;
            string expectedValue2 = null;
            string actualValue1 = null;
            string actualValue2 = null;

            if (fineType == FabricCertificateTypeX509FindType.FindByThumbprint)
            {
                actualValue1 = csmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
                actualValue2 = csmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary;
            }
            else
            {
                actualValue1 = csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName;
                Assert.AreEqual(1, csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames.Count);
            }

            Assert.AreEqual(expectedValue1, actualValue1);
            Assert.AreEqual(expectedValue2, actualValue2);
        }

        internal void VerifyJsonChangeCertTypeLists(
            IUserConfig csmConfig,
            string newValue1,
            string newValue2)
        {
            string expectedValue1 = newValue1;
            string expectedValue2 = newValue2;
            string actualValue1 = csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[0].CertificateCommonName;
            string actualValue2 = csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames[1].CertificateCommonName;

            Assert.IsNull(csmConfig.Security.CertificateInformation.ClusterCertificate);
            Assert.IsNull(csmConfig.Security.CertificateInformation.ServerCertificate);

            Assert.IsNotNull(actualValue1);
            Assert.IsNotNull(actualValue2);

            Assert.AreEqual(expectedValue1, actualValue1);
            Assert.AreEqual(expectedValue2, actualValue2);
        }

        internal void VerifyLoadCertList_Add(
            ClusterManifestType manifest,
            string originalValue,
            string newValue,
            bool addValue1,
            int currentPhase,
            FabricCertificateTypeX509FindType expectedFindType)
        {
            FabricCertificateType certificates = manifest.NodeTypes.First().Certificates.ClusterCertificate;
            string actualValue1 = certificates.X509FindValue;
            string actualValue2 = certificates.X509FindValueSecondary;
            string expectedValue1 = null;
            string expectedValue2 = null;

            Assert.AreEqual(expectedFindType, certificates.X509FindType);

            switch (currentPhase)
            {
                case 0:
                    {
                        expectedValue1 = originalValue;
                        expectedValue2 = null;
                        break;
                    }

                case 1:
                    {
                        expectedValue1 = addValue1 ? newValue : originalValue;
                        expectedValue2 = addValue1 ? originalValue : newValue;
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(expectedValue1, actualValue1);
            Assert.AreEqual(expectedValue2, actualValue2);
        }

        internal void VerifyLoadCertList_Replace(
            ClusterManifestType manifest,
            string originalValue,
            string newValue,
            int currentPhase,
            FabricCertificateTypeX509FindType expectedFindType)
        {
            FabricCertificateType certificates = manifest.NodeTypes.First().Certificates.ClusterCertificate;
            string actualValue1 = certificates.X509FindValue;
            string actualValue2 = certificates.X509FindValueSecondary;
            string expectedValue1 = null;
            string expectedValue2 = null;

            Assert.AreEqual(expectedFindType, certificates.X509FindType);

            switch (currentPhase)
            {
                case 0:
                    {
                        expectedValue1 = originalValue;
                        expectedValue2 = null;
                        break;
                    }

                case 1:
                case 2:
                    {
                        expectedValue1 = newValue;
                        expectedValue2 = null;
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(expectedValue1, actualValue1);
            Assert.AreEqual(expectedValue2, actualValue2);
        }

        internal void VerifyLoadCertList_ChangeType(
            ClusterManifestType manifest,
            string originalValue1,
            string origilanValue2,
            string newValue1,
            string newValue2,
            int currentPhase)
        {
            FabricCertificateType certificates = manifest.NodeTypes.First().Certificates.ClusterCertificate;
            string actualValue1 = certificates.X509FindValue;
            string actualValue2 = certificates.X509FindValueSecondary;
            string expectedValue1 = null;
            string expectedValue2 = null;

            switch (currentPhase)
            {
                case 0:
                    {
                        expectedValue1 = originalValue1;
                        expectedValue2 = origilanValue2;
                        Assert.AreEqual(FabricCertificateTypeX509FindType.FindByThumbprint, certificates.X509FindType);
                        break;
                    }

                case 1:
                case 2:
                    {
                        expectedValue1 = newValue1;
                        expectedValue2 = newValue2;
                        Assert.AreEqual(FabricCertificateTypeX509FindType.FindBySubjectName, certificates.X509FindType);
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(expectedValue1, actualValue1);
            Assert.AreEqual(expectedValue2, actualValue2);
        }

        internal void VerifyLoadCertList_Remove(
            ClusterManifestType manifest,
            string originalValue1,
            string originalValue2,
            bool removeValue1,
            FabricCertificateTypeX509FindType expectedFindType)
        {
            FabricCertificateType certificates = manifest.NodeTypes.First().Certificates.ClusterCertificate;
            string actualValue1 = certificates.X509FindValue;
            string actualValue2 = certificates.X509FindValueSecondary;
            string expectedValue1 = removeValue1 ? originalValue2 : originalValue1;
            string expectedValue2 = null;

            Assert.AreEqual(expectedFindType, certificates.X509FindType);
            Assert.AreEqual(expectedValue1, actualValue1);
            Assert.AreEqual(expectedValue2, actualValue2);
        }

        internal void VerifyWhiteCertList_Add(
            ClusterManifestType manifest,
            Tuple<string, string> originalValue,
            Tuple<string, string> newValue,
            FabricCertificateTypeX509FindType findType)
        {
            string adminClientCertValues = null;
            string nonAdminClientCertValues = null;
            string clusterCertValues = null;
            string issuerThumbprintValues = null;

            if (findType == FabricCertificateTypeX509FindType.FindByThumbprint)
            {
                SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.Security);
                adminClientCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.AdminClientCertThumbprints).Value;
                nonAdminClientCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.ClientCertThumbprints).Value;
                clusterCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.ClusterCertThumbprints).Value;
            }
            else
            {
                SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityAdminClientX509Names);
                adminClientCertValues = string.Join(",", section.Parameter.Select(p => p.Name));
                section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityClientX509Names);
                nonAdminClientCertValues = string.Join(",", section.Parameter.Select(p => p.Name));
                section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityClusterX509Names);
                clusterCertValues = string.Join(",", section.Parameter.Select(p => p.Name));
                issuerThumbprintValues = string.Join(",", section.Parameter.Select(p => p.Value));
                Assert.IsNotNull(issuerThumbprintValues);
            }

            Assert.AreEqual(adminClientCertValues, nonAdminClientCertValues);
            Assert.AreEqual(nonAdminClientCertValues, clusterCertValues);
            Assert.IsTrue(clusterCertValues.Contains(originalValue.Item1) && clusterCertValues.Contains(newValue.Item1));
            if (issuerThumbprintValues != null)
            {
                Assert.IsTrue(issuerThumbprintValues.Contains(originalValue.Item2) && issuerThumbprintValues.Contains(newValue.Item2));
            }
        }

        internal void VerifyWhiteCertList_Remove(
            ClusterManifestType manifest,
            Tuple<string, string> originalValue1,
            Tuple<string, string> originalValue2,
            bool removeValue1,
            int currentPhase,
            FabricCertificateTypeX509FindType findType)
        {
            string adminClientCertValues = null;
            string nonAdminClientCertValues = null;
            string clusterCertValues = null;
            string expectedCertValues = null;
            string issuerThumbprintValues = null;
            string expectedIssuerThumbprintValues = null;

            if (findType == FabricCertificateTypeX509FindType.FindByThumbprint)
            {
                SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.Security);
                adminClientCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.AdminClientCertThumbprints).Value;
                nonAdminClientCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.ClientCertThumbprints).Value;
                clusterCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.ClusterCertThumbprints).Value;
            }
            else
            {
                SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityAdminClientX509Names);
                adminClientCertValues = string.Join(",", section.Parameter.Select(p => p.Name));
                section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityClientX509Names);
                nonAdminClientCertValues = string.Join(",", section.Parameter.Select(p => p.Name));
                section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityClusterX509Names);
                clusterCertValues = string.Join(",", section.Parameter.Select(p => p.Name));
                issuerThumbprintValues = string.Join(",", section.Parameter.Select(p => p.Value));
                Assert.IsFalse(string.IsNullOrWhiteSpace(issuerThumbprintValues));
            }

            switch (currentPhase)
            {
                case 0:
                    {
                        expectedCertValues = string.Format("{0},{1}", originalValue1.Item1, originalValue2.Item1);
                        if (issuerThumbprintValues != null)
                        {
                            expectedIssuerThumbprintValues = string.Format("{0},{1}", originalValue1.Item2, originalValue2.Item2);
                        }
                        break;
                    }

                case 1:
                    {
                        expectedCertValues = removeValue1 ? originalValue2.Item1 : originalValue1.Item1;
                        if (issuerThumbprintValues != null)
                        {
                            expectedIssuerThumbprintValues = removeValue1 ? originalValue2.Item2 : originalValue1.Item2;
                        }
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(adminClientCertValues, nonAdminClientCertValues);
            Assert.AreEqual(nonAdminClientCertValues, clusterCertValues);
            Assert.AreEqual(expectedCertValues, clusterCertValues);
            Assert.AreEqual(expectedIssuerThumbprintValues, issuerThumbprintValues);
        }

        internal void VerifyWhiteCertList_Replace(
            ClusterManifestType manifest,
            string originalValue,
            string newValue,
            int currentPhase,
            FabricCertificateTypeX509FindType findType)
        {
            string adminClientCertValues = null;
            string nonAdminClientCertValues = null;
            string clusterCertValues = null;
            string expectedValues = null;

            if (findType == FabricCertificateTypeX509FindType.FindByThumbprint)
            {
                SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.Security);
                adminClientCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.AdminClientCertThumbprints).Value;
                nonAdminClientCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.ClientCertThumbprints).Value;
                clusterCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.ClusterCertThumbprints).Value;
            }
            else
            {
                SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityAdminClientX509Names);
                adminClientCertValues = string.Join(",", section.Parameter.Select(p => p.Name));
                section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityClientX509Names);
                nonAdminClientCertValues = string.Join(",", section.Parameter.Select(p => p.Name));
                section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityClusterX509Names);
                clusterCertValues = string.Join(",", section.Parameter.Select(p => p.Name));
            }

            switch (currentPhase)
            {
                case 0:
                case 1:
                    {
                        expectedValues = string.Format("{0},{1}", originalValue, newValue);
                        break;
                    }

                case 2:
                    {
                        expectedValues = newValue;
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(adminClientCertValues, nonAdminClientCertValues);
            Assert.AreEqual(nonAdminClientCertValues, clusterCertValues);
            Assert.AreEqual(expectedValues, clusterCertValues);
        }

        internal void VerifyWhiteCertList_ChangeType(
            ClusterManifestType manifest,
            string originalValue1,
            string originalValue2,
            string newValue1,
            string newValue2,
            int currentPhase)
        {
            string adminClientCertValues = null;
            string nonAdminClientCertValues = null;
            string clusterCertValues = null;
            string expectedValues = null;

            switch (currentPhase)
            {
                case 0:
                case 1:
                    {
                        SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.Security);
                        adminClientCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.AdminClientCertThumbprints).Value;
                        nonAdminClientCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.ClientCertThumbprints).Value;
                        clusterCertValues = section.Parameter.First(p => p.Name == StringConstants.ParameterName.ClusterCertThumbprints).Value;

                        section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityAdminClientX509Names);
                        adminClientCertValues += ("," + string.Join(",", section.Parameter.Select(p => p.Name)));
                        section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityClientX509Names);
                        nonAdminClientCertValues += ("," + string.Join(",", section.Parameter.Select(p => p.Name)));
                        section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityClusterX509Names);
                        clusterCertValues += ("," + string.Join(",", section.Parameter.Select(p => p.Name)));

                        expectedValues = string.Format("{0},{1},{2},{3}", originalValue1, originalValue2, newValue1, newValue2);
                        break;
                    }

                case 2:
                    {
                        SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityAdminClientX509Names);
                        adminClientCertValues = string.Join(",", section.Parameter.Select(p => p.Name));
                        section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityClientX509Names);
                        nonAdminClientCertValues = string.Join(",", section.Parameter.Select(p => p.Name));
                        section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.SecurityClusterX509Names);
                        clusterCertValues = string.Join(",", section.Parameter.Select(p => p.Name));

                        expectedValues = string.Format("{0},{1}", newValue1, newValue2);
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(adminClientCertValues, nonAdminClientCertValues);
            Assert.AreEqual(nonAdminClientCertValues, clusterCertValues);
            Assert.AreEqual(expectedValues, clusterCertValues);
        }

        internal void VerifyFileStoreSvcCertList_Add(
            ClusterManifestType manifest,
            string originalValue,
            string newValue,
            bool addValue1,
            int currentPhase,
            FabricCertificateTypeX509FindType findType)
        {
            SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.FileStoreService);
            string actualValue1 = null;
            string actualValue2 = null;
            string expectedValue1 = null;
            string expectedValue2 = null;

            if (findType == FabricCertificateTypeX509FindType.FindByThumbprint)
            {
                actualValue1 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.PrimaryAccountNtlmx509Thumbprint).Value;
                actualValue2 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.SecondaryAccountNtlmx509Thumbprint).Value;
            }
            else
            {
                actualValue1 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.CommonName1Ntlmx509CommonName).Value;
                actualValue2 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.CommonName2Ntlmx509CommonName).Value;
            }

            switch (currentPhase)
            {
                case 0:
                    {
                        expectedValue1 = originalValue;
                        expectedValue2 = originalValue;
                        break;
                    }

                case 1:
                    {
                        expectedValue1 = addValue1 ? newValue : originalValue;
                        expectedValue2 = addValue1 ? originalValue : newValue;
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(expectedValue1, actualValue1);
            Assert.AreEqual(expectedValue2, actualValue2);
        }

        internal void VerifyFileStoreSvcCertList_Remove(
            ClusterManifestType manifest,
            string originalValue1,
            string originalValue2,
            bool removeValue1,
            int currentPhase,
            FabricCertificateTypeX509FindType findType)
        {
            SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.FileStoreService);
            string actualValue1 = null;
            string actualValue2 = null;
            string expectedValue1 = null;
            string expectedValue2 = null;

            if (findType == FabricCertificateTypeX509FindType.FindByThumbprint)
            {
                actualValue1 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.PrimaryAccountNtlmx509Thumbprint).Value;
                actualValue2 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.SecondaryAccountNtlmx509Thumbprint).Value;
            }
            else
            {
                actualValue1 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.CommonName1Ntlmx509CommonName).Value;
                actualValue2 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.CommonName2Ntlmx509CommonName).Value;
            }

            switch (currentPhase)
            {
                case 0:
                    {
                        expectedValue1 = originalValue1;
                        expectedValue2 = originalValue2;
                        break;
                    }

                case 1:
                    {
                        expectedValue1 = removeValue1 ? originalValue2 : originalValue1;
                        expectedValue2 = expectedValue1;
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(expectedValue1, actualValue1);
            Assert.AreEqual(expectedValue2, actualValue2);
        }

        internal void VerifyFileStoreSvcCertList_Replace(
            ClusterManifestType manifest,
            string originalValue,
            string newValue,
            int currentPhase,
            FabricCertificateTypeX509FindType findType)
        {
            SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.FileStoreService);
            string actualValue1 = null;
            string actualValue2 = null;
            string expectedValue1 = null;
            string expectedValue2 = null;

            if (findType == FabricCertificateTypeX509FindType.FindByThumbprint)
            {
                actualValue1 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.PrimaryAccountNtlmx509Thumbprint).Value;
                actualValue2 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.SecondaryAccountNtlmx509Thumbprint).Value;
            }
            else
            {
                actualValue1 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.CommonName1Ntlmx509CommonName).Value;
                actualValue2 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.CommonName2Ntlmx509CommonName).Value;
            }

            switch (currentPhase)
            {
                case 0:
                    {
                        expectedValue1 = originalValue;
                        expectedValue2 = originalValue;
                        break;
                    }

                case 1:
                    {
                        expectedValue1 = originalValue;
                        expectedValue2 = newValue;
                        break;
                    }

                case 2:
                    {
                        expectedValue1 = newValue;
                        expectedValue2 = newValue;
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(expectedValue1, actualValue1);
            Assert.AreEqual(expectedValue2, actualValue2);
        }

        internal void VerifyFileStoreSvcCertList_ChangeType(
            ClusterManifestType manifest,
            string originalValue1,
            string originalValue2,
            string newValue1,
            string newValue2,
            int currentPhase)
        {
            SettingsOverridesTypeSection section = manifest.FabricSettings.First(p => p.Name == StringConstants.SectionName.FileStoreService);
            string actualThumbprintValue1 = null;
            string actualThumbprintValue2 = null;
            string actualCnValue1 = null;
            string actualCnValue2 = null;
            string expectedThumbprintValue1 = null;
            string expectedThumbprintValue2 = null;
            string expectedCnValue1 = null;
            string expectedCnValue2 = null;

            if (section.Parameter.Any(p => p.Name == StringConstants.ParameterName.PrimaryAccountNtlmx509Thumbprint))
            {
                actualThumbprintValue1 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.PrimaryAccountNtlmx509Thumbprint).Value;
                actualThumbprintValue2 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.SecondaryAccountNtlmx509Thumbprint).Value;
            }

            if (section.Parameter.Any(p => p.Name == StringConstants.ParameterName.CommonName1Ntlmx509CommonName))
            {
                actualCnValue1 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.CommonName1Ntlmx509CommonName).Value;
                actualCnValue2 = section.Parameter.First(p => p.Name == StringConstants.ParameterName.CommonName2Ntlmx509CommonName).Value;
            }

            switch (currentPhase)
            {
                case 0:
                    {
                        expectedThumbprintValue1 = originalValue1;
                        expectedThumbprintValue2 = originalValue2;
                        expectedCnValue1 = null;
                        expectedCnValue2 = null;
                        break;
                    }

                case 1:
                    {
                        expectedThumbprintValue1 = originalValue1;
                        expectedThumbprintValue2 = originalValue2;
                        expectedCnValue1 = newValue1;
                        expectedCnValue2 = newValue2;
                        break;
                    }

                case 2:
                    {
                        expectedThumbprintValue1 = null;
                        expectedThumbprintValue2 = null;
                        expectedCnValue1 = newValue1;
                        expectedCnValue2 = newValue2;
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(expectedThumbprintValue1, actualThumbprintValue1);
            Assert.AreEqual(expectedThumbprintValue2, actualThumbprintValue2);
            Assert.AreEqual(expectedCnValue1, actualCnValue1);
            Assert.AreEqual(expectedCnValue2, actualCnValue2);
        }
    }
}