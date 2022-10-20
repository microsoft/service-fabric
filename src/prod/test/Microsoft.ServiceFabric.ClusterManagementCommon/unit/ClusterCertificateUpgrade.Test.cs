// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using System.Collections.Generic;

    /// <summary>
    /// cluster cert upgrade test
    /// </summary>
    [TestClass]
    public class ClusterCertificateUpgradeTest
    {
        private const string BaselineConfigVersion = "1.0.0";

        private const string TargetConfigVersion = "2.0.0";

        private const int BaselineDataDeletionAgeInDays = 1;

        private const int TargetDataDeletionAgeInDays = 2;

        private const string InterruptedTargetConfigVersion = "3.0.0";

        private const int TargetVMInstanceCount = 2;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void AddCertTest()
        {
            this.InternalAddCertTest("ClusterConfig.X509.DevCluster.AddPrimary.V1.json", "ClusterConfig.X509.DevCluster.AddPrimary.V2.json");

            this.InternalAddCertTest("ClusterConfig.X509.DevCluster.AddSecondary.V1.json", "ClusterConfig.X509.DevCluster.AddSecondary.V2.json");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void AddCertRollbackTest()
        {
            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.AddPrimary.V1.json",
                "ClusterConfig.X509.DevCluster.AddPrimary.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);
            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.AddSecondary.V1.json",
                "ClusterConfig.X509.DevCluster.AddSecondary.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);

            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.AddPrimary.V1.json",
                "ClusterConfig.X509.DevCluster.AddPrimary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);
            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.AddSecondary.V1.json",
                "ClusterConfig.X509.DevCluster.AddSecondary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);

            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.AddPrimary.V1.json",
                "ClusterConfig.X509.DevCluster.AddPrimary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);
            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.AddSecondary.V1.json",
                "ClusterConfig.X509.DevCluster.AddSecondary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RemoveCertTest()
        {
            this.InternalRemoveCertTest("ClusterConfig.X509.DevCluster.RemovePrimary.V1.json", "ClusterConfig.X509.DevCluster.RemovePrimary.V2.json");

            this.InternalRemoveCertTest("ClusterConfig.X509.DevCluster.RemoveSecondary.V1.json", "ClusterConfig.X509.DevCluster.RemoveSecondary.V2.json");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RemoveCertRollbackTest()
        {
            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.RemovePrimary.V1.json",
                "ClusterConfig.X509.DevCluster.RemovePrimary.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);
            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.RemoveSecondary.V1.json",
                "ClusterConfig.X509.DevCluster.RemoveSecondary.V2.json",
                firstIterationSuccess: false,
                rollbackSuccess: false);

            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.RemovePrimary.V1.json",
                "ClusterConfig.X509.DevCluster.RemovePrimary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);
            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.RemoveSecondary.V1.json",
                "ClusterConfig.X509.DevCluster.RemoveSecondary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: true);

            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.RemovePrimary.V1.json",
                "ClusterConfig.X509.DevCluster.RemovePrimary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);
            this.InternalTwoPhaseRollbackTest(
                "ClusterConfig.X509.DevCluster.RemoveSecondary.V1.json",
                "ClusterConfig.X509.DevCluster.RemoveSecondary.V2.json",
                firstIterationSuccess: true,
                rollbackSuccess: false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ReplaceCertTest()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.X509.DevCluster.Replace.V1.json");
            Assert.AreEqual(ClusterCertificateUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateCluster("ClusterConfig.X509.DevCluster.Replace.V2.json", cluster);

            string originalThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            string newThumbprint = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;

            // 1st iteration
            Assert.IsTrue(cluster.RunStateMachine());

            MockupCertificateClusterUpgradeState upgradeState = (MockupCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestReplaceCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalThumbprint,
                newThumbprint,
                upgradeState.CurrentListIndex);

            Utility.RollForwardOneIteration(cluster);

            // 2nd iteration
            Assert.IsFalse(cluster.RunStateMachine());
            upgradeState = (MockupCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestReplaceCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalThumbprint,
                newThumbprint,
                upgradeState.CurrentListIndex);

            Utility.RollForwardOneIteration(cluster);

            //3rd iteration
            Assert.IsFalse(cluster.RunStateMachine());
            upgradeState = (MockupCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestReplaceCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalThumbprint,
                newThumbprint,
                upgradeState.CurrentListIndex);

            Utility.RollForwardOneIteration(cluster);

            // final check
            Assert.IsFalse(cluster.RunStateMachine());

            this.VerifyJsonReplaceCertLists(cluster.Current.CSMConfig, originalThumbprint, newThumbprint);

            Assert.AreEqual(ClusterCertificateUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(ClusterCertificateUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ReplaceCertRollbackTest()
        {
            this.InternalReplaceCertRollbackTest(maxSuccessIteration: -1, rollbackSuccess: false);
            this.InternalReplaceCertRollbackTest(maxSuccessIteration: 0, rollbackSuccess: true);
            this.InternalReplaceCertRollbackTest(maxSuccessIteration: 0, rollbackSuccess: false);
            this.InternalReplaceCertRollbackTest(maxSuccessIteration: 1, rollbackSuccess: true);
            this.InternalReplaceCertRollbackTest(maxSuccessIteration: 1, rollbackSuccess: false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void InterruptWithSimpleClusterUpgradeAtFirstIteration()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.X509.DevCluster.Replace.V1.json");
            Utility.UpdateCluster("ClusterConfig.X509.DevCluster.Replace.V2.json", cluster);

            // 1st iteration
            Assert.IsTrue(cluster.RunStateMachine());
            MockupCertificateClusterUpgradeState upgradeState = (MockupCertificateClusterUpgradeState)cluster.Pending;

            //Interrupt with a simple cluster upgrade
            Utility.UpdateCluster("ClusterConfig.X509.DevCluster.Replace.V2.Interrupt.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(ClusterCertificateUpgradeTest.InterruptedTargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void InterruptWithSimpleClusterUpgradeAtSecondIteration()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.X509.DevCluster.Replace.V1.json");
            Assert.AreEqual(ClusterCertificateUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateCluster("ClusterConfig.X509.DevCluster.Replace.V2.json", cluster);

            string originalThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            string newThumbprint = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;

            // 1st iteration
            Assert.IsTrue(cluster.RunStateMachine());

            MockupCertificateClusterUpgradeState upgradeState = (MockupCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestReplaceCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalThumbprint,
                newThumbprint,
                upgradeState.CurrentListIndex);

            Utility.RollForwardOneIteration(cluster);

            // 2nd iteration
            Assert.IsFalse(cluster.RunStateMachine());
            upgradeState = (MockupCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestReplaceCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalThumbprint,
                newThumbprint,
                upgradeState.CurrentListIndex);

            Utility.UpdateCluster("ClusterConfig.X509.DevCluster.Replace.V2.Interrupt.json", cluster);
            Assert.IsFalse(cluster.RunStateMachine());

            Utility.RollForwardOneIteration(cluster);

            //3rd iteration
            Assert.IsFalse(cluster.RunStateMachine());
            upgradeState = (MockupCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestReplaceCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalThumbprint,
                newThumbprint,
                upgradeState.CurrentListIndex);

            Utility.RollForwardOneIteration(cluster);

            // final check
            Assert.IsFalse(cluster.RunStateMachine());

            this.VerifyJsonReplaceCertLists(cluster.Current.CSMConfig, originalThumbprint, newThumbprint);

            Assert.AreEqual(ClusterCertificateUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(ClusterCertificateUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        internal void InternalReplaceCertRollbackTest(int maxSuccessIteration, bool rollbackSuccess)
        {
            Assert.IsTrue(maxSuccessIteration < 2);

            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.X509.DevCluster.Replace.V1.json");
            Assert.AreEqual(ClusterCertificateUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            ClusterManifestType baselineClusterManifest = cluster.Current.ExternalState.ClusterManifest;
            Utility.UpdateCluster("ClusterConfig.X509.DevCluster.Replace.V2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            if (maxSuccessIteration < 0)
            {
                Utility.RollBackOneIteration(cluster);
                Assert.IsFalse(cluster.RunStateMachine());
                Assert.IsNull(cluster.Pending);
                Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);
            }
            else
            {
                for (int i = 0; i <= maxSuccessIteration; i++)
                {
                    Utility.RollForwardOneIteration(cluster);
                    Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);
                    Assert.IsFalse(cluster.RunStateMachine());
                }

                Utility.RollBackOneIteration(cluster);
                Assert.IsFalse(cluster.RunStateMachine());
                ClusterManifestType expectedManifest = maxSuccessIteration == 0 ? baselineClusterManifest : ((MultiphaseClusterUpgradeState)cluster.Pending).ClusterManifestList[maxSuccessIteration - 1];
                Assert.AreEqual(expectedManifest, cluster.Pending.ExternalState.ClusterManifest);

                if (rollbackSuccess)
                {
                    for (int i = 0; i <= maxSuccessIteration; i++)
                    {
                        Utility.RollForwardOneIteration(cluster);
                        Assert.IsFalse(cluster.RunStateMachine());
                    }

                    Assert.IsNull(cluster.Pending);
                }
                else
                {
                    Utility.RollBackOneIteration(cluster);
                    Assert.IsFalse(cluster.RunStateMachine());
                    Assert.IsNotNull(cluster.Pending);
                }

                Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);
            }
        }

        internal void InternalAddCertTest(string baselineJsonFile, string targetJsonFile)
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade(baselineJsonFile);
            Assert.AreEqual(ClusterCertificateUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateCluster(targetJsonFile, cluster);

            string originalThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            string newPrimaryThumbprint = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            string newSecondarythumbprint = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary;
            string newThumbprint = originalThumbprint == newPrimaryThumbprint ? newSecondarythumbprint : newPrimaryThumbprint;
            bool addPrimarythumbprint = originalThumbprint != newPrimaryThumbprint;

            // 1st iteration
            Assert.IsTrue(cluster.RunStateMachine());

            MockupCertificateClusterUpgradeState upgradeState = (MockupCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestAddCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalThumbprint,
                newThumbprint,
                addPrimarythumbprint,
                upgradeState.CurrentListIndex);

            Assert.AreEqual(ClusterCertificateUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // 2nd iteration
            Assert.IsFalse(cluster.RunStateMachine());
            upgradeState = (MockupCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestAddCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalThumbprint,
                newThumbprint,
                addPrimarythumbprint,
                upgradeState.CurrentListIndex);

            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // final check
            Assert.IsFalse(cluster.RunStateMachine());

            this.VerifyJsonAddCertLists(cluster.Current.CSMConfig, originalThumbprint, newThumbprint, addPrimarythumbprint);

            Assert.AreEqual(ClusterCertificateUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(ClusterCertificateUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        internal void InternalRemoveCertTest(string baselineJsonFile, string targetJsonFile)
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade(baselineJsonFile);
            Assert.AreEqual(ClusterCertificateUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateCluster(targetJsonFile, cluster);

            string originalPrimaryThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            string originalSecondaryThumbprint = cluster.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary;
            string newPrimaryThumbprint = cluster.TargetCsmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint;
            bool removePrimarythumbprint = originalPrimaryThumbprint != newPrimaryThumbprint;

            // 1st iteration
            Assert.IsTrue(cluster.RunStateMachine());

            MockupCertificateClusterUpgradeState upgradeState = (MockupCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestRemoveCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalPrimaryThumbprint,
                originalSecondaryThumbprint,
                removePrimarythumbprint,
                upgradeState.CurrentListIndex);

            Assert.AreEqual(ClusterCertificateUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // 2nd iteration
            Assert.IsFalse(cluster.RunStateMachine());
            upgradeState = (MockupCertificateClusterUpgradeState)cluster.Pending;
            this.VerifyManifestRemoveCertLists(
                cluster.Pending.ExternalState.ClusterManifest,
                originalPrimaryThumbprint,
                originalSecondaryThumbprint,
                removePrimarythumbprint,
                upgradeState.CurrentListIndex);

            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.IsNotNull(cluster.Current);

            Utility.RollForwardOneIteration(cluster);

            // final check
            Assert.IsFalse(cluster.RunStateMachine());

            this.VerifyJsonRemoveCertLists(
                cluster.Current.CSMConfig,
                originalPrimaryThumbprint,
                originalSecondaryThumbprint,
                removePrimarythumbprint);

            Assert.AreEqual(ClusterCertificateUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(ClusterCertificateUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        internal void InternalTwoPhaseRollbackTest(
            string baselineJsonFile,
            string targetJsonFile,
            bool firstIterationSuccess,
            bool rollbackSuccess)
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade(baselineJsonFile);
            Assert.AreEqual(ClusterCertificateUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            ClusterManifestType baselineClusterManifest = cluster.Current.ExternalState.ClusterManifest;
            Utility.UpdateCluster(targetJsonFile, cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            if (!firstIterationSuccess)
            {
                Utility.RollBackOneIteration(cluster);
                Assert.IsFalse(cluster.RunStateMachine());
                Assert.IsNull(cluster.Pending);
                Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);
            }
            else
            {
                Utility.RollForwardOneIteration(cluster);
                Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);

                Assert.IsFalse(cluster.RunStateMachine());
                Utility.RollBackOneIteration(cluster);
                Assert.IsFalse(cluster.RunStateMachine());
                Assert.AreEqual(baselineClusterManifest, cluster.Pending.ExternalState.ClusterManifest);

                if (rollbackSuccess)
                {
                    Utility.RollForwardOneIteration(cluster);
                    Assert.IsFalse(cluster.RunStateMachine());
                    Assert.IsNull(cluster.Pending);
                }
                else
                {
                    Utility.RollBackOneIteration(cluster);
                    Assert.IsFalse(cluster.RunStateMachine());
                    Assert.IsNotNull(cluster.Pending);
                }

                Assert.AreEqual(baselineClusterManifest, cluster.Current.ExternalState.ClusterManifest);
            }
        }

        internal void VerifyManifestAddCertLists(
            ClusterManifestType manifest,
            string originalThumbprint,
            string newThumbprint,
            bool addPrimarythumbprint,
            int currentPhase)
        {
            this.VerifyLoadCertList_Add(manifest, originalThumbprint, newThumbprint, addPrimarythumbprint, currentPhase);
        }

        internal void VerifyManifestRemoveCertLists(
            ClusterManifestType manifest,
            string originalPrimaryThumbprint,
            string originalSecondaryThumbprint,
            bool removePrimarythumbprint,
            int currentPhase)
        {
            this.VerifyLoadCertList_Remove(manifest, originalPrimaryThumbprint, originalSecondaryThumbprint, removePrimarythumbprint);
        }

        internal void VerifyManifestReplaceCertLists(
            ClusterManifestType manifest,
            string originalPrimaryThumbprint,
            string newPrimaryThumbprint,
            int currentPhase)
        {
            this.VerifyLoadCertList_Replace(manifest, originalPrimaryThumbprint, newPrimaryThumbprint, currentPhase);
        }

        internal void VerifyJsonAddCertLists(
            IUserConfig csmConfig,
            string originalThumbprint,
            string newThumbprint,
            bool addPrimarythumbprint)
        {
            string expectedPrimaryThumbprint = addPrimarythumbprint ? newThumbprint : originalThumbprint;
            string expectedSecondaryThumbprint = addPrimarythumbprint ? originalThumbprint : newThumbprint;

            Assert.AreEqual(expectedPrimaryThumbprint, csmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint);
            Assert.AreEqual(expectedSecondaryThumbprint, csmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary);
        }

        internal void VerifyJsonRemoveCertLists(
            IUserConfig csmConfig,
            string originalPrimaryThumbprint,
            string originalSecondaryThumbprint,
            bool removePrimarythumbprint)
        {
            string expectedPrimaryThumbprint = removePrimarythumbprint ? originalSecondaryThumbprint : originalPrimaryThumbprint;
            string expectedSecondaryThumbprint = null;

            Assert.AreEqual(expectedPrimaryThumbprint, csmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint);
            Assert.AreEqual(expectedSecondaryThumbprint, csmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary);
        }

        internal void VerifyJsonReplaceCertLists(
            IUserConfig csmConfig,
            string originalThumbprint,
            string newThumbprint)
        {
            string expectedPrimaryThumbprint = newThumbprint;
            string expectedSecondaryThumbprint = null;

            Assert.AreEqual(expectedPrimaryThumbprint, csmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint);
            Assert.AreEqual(expectedSecondaryThumbprint, csmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary);
        }

        internal void VerifyLoadCertList_Add(
            ClusterManifestType manifest,
            string originalThumbprint,
            string newThumbprint,
            bool addPrimarythumbprint,
            int currentPhase)
        {
            FabricCertificateType certificates = manifest.NodeTypes.First().Certificates.ClusterCertificate;
            string primaryThumbprint = certificates.X509FindValue;
            string secondaryThumbprint = certificates.X509FindValueSecondary;
            string expectedPrimaryThumbprint = null;
            string expectedSecondaryThumbprint = null;

            switch (currentPhase)
            {
                case 0:
                    {
                        expectedPrimaryThumbprint = originalThumbprint;
                        expectedSecondaryThumbprint = null;
                        break;
                    }

                case 1:
                    {
                        expectedPrimaryThumbprint = addPrimarythumbprint ? newThumbprint : originalThumbprint;
                        expectedSecondaryThumbprint = addPrimarythumbprint ? originalThumbprint : newThumbprint;
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(expectedPrimaryThumbprint, primaryThumbprint);
            Assert.AreEqual(expectedSecondaryThumbprint, secondaryThumbprint);
        }

        internal void VerifyLoadCertList_Replace(
            ClusterManifestType manifest,
            string originalThumbprint,
            string newThumbprint,
            int currentPhase)
        {
            FabricCertificateType certificates = manifest.NodeTypes.First().Certificates.ClusterCertificate;
            string primaryThumbprint = certificates.X509FindValue;
            string secondaryThumbprint = certificates.X509FindValueSecondary;
            string expectedPrimaryThumbprint = null;
            string expectedSecondaryThumbprint = null;

            switch (currentPhase)
            {
                case 0:
                    {
                        expectedPrimaryThumbprint = originalThumbprint;
                        expectedSecondaryThumbprint = null;
                        break;
                    }

                case 1:
                case 2:
                    {
                        expectedPrimaryThumbprint = newThumbprint;
                        expectedSecondaryThumbprint = null;
                        break;
                    }

                default:
                    throw new NotSupportedException();
            }

            Assert.AreEqual(expectedPrimaryThumbprint, primaryThumbprint);
            Assert.AreEqual(expectedSecondaryThumbprint, secondaryThumbprint);
        }

        internal void VerifyLoadCertList_Remove(
            ClusterManifestType manifest,
            string originalPrimaryThumbprint,
            string originalSecondaryThumbprint,
            bool removePrimarythumbprint)
        {
            FabricCertificateType certificates = manifest.NodeTypes.First().Certificates.ClusterCertificate;
            string primaryThumbprint = certificates.X509FindValue;
            string secondaryThumbprint = certificates.X509FindValueSecondary;
            string expectedPrimaryThumbprint = removePrimarythumbprint ? originalSecondaryThumbprint : originalPrimaryThumbprint;
            string expectedSecondaryThumbprint = null;

            Assert.AreEqual(expectedPrimaryThumbprint, primaryThumbprint);
            Assert.AreEqual(expectedSecondaryThumbprint, secondaryThumbprint);
        }
    }
}