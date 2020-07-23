// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Threading;

namespace System.Fabric.FabricDeployer
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using NetFwTypeLib;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Linq;
    using System.IO;
    using WEX.TestExecution;
    using System.Fabric.Common;
    using Microsoft.Win32;
    using System.Xml;
    using System.Xml.Serialization;

    /// <summary>
    /// This is the class that tests the validation of the files that we deploy.
    /// </summary>
    [TestClass]
    internal class FabricDeployerDeploymentParametersTest
    {
        [TestCleanup]
        public void Cleanup()
        {
            Environment.SetEnvironmentVariable("FabricBinRoot", string.Empty);
            Environment.SetEnvironmentVariable("FabricDataRoot", string.Empty);
            Environment.SetEnvironmentVariable("FabricLogRoot", string.Empty);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDeploymentParameterCreateWithAllEnvironmentSet()
        {
            //SetEnvironment(true, true);
            //string clusterManifestLocation = "ClusterManifest.xml";
            //File.WriteAllText(clusterManifestLocation, "TestManifest");
            //DeploymentParameters parameters = new DeploymentParameters();
            //parameters.Create(new Dictionary<string, dynamic>() { { DeploymentParameters.ClusterManifestString, clusterManifestLocation } }, DeploymentOperations.Create);
            //Validate(
            //    parameters, 
            //    DeploymentOperations.Create,
            //    Path.Combine(RootLocation, "Data"), 
            //    Path.Combine(RootLocation, "Log"), 
            //    clusterManifestLocation, 
            //    false,
            //    System.Fabric.Management.WindowsFabricValidator.FabricValidatorConstants.FabricUpgradeDefaultInstanceId,
            //    System.Fabric.Management.WindowsFabricValidator.FabricValidatorConstants.FabricUpgradeDefaultTargetVersion,
            //    null,
            //    null,
            //    null
            //    );
        }

        [TestProperty("ThreadingModel", "MTA")]
        public void TestDeploymentParameterCreateWithDataAndBinRootEnvironmentSet()
        {
            SetEnvironment(true, false);
            string clusterManifestLocation = "ClusterManifest.xml";
            File.WriteAllText(clusterManifestLocation, "TestManifest");
            DeploymentParameters parameters = new DeploymentParameters();
            parameters.SetParameters(new Dictionary<string, dynamic>() { { DeploymentParameters.ClusterManifestString, clusterManifestLocation } }, DeploymentOperations.Create);
            parameters.Initialize();
            Validate(
                parameters,
                DeploymentOperations.Create,
                Path.Combine(RootLocation, "Data"),
                Path.Combine(RootLocation, "Data", "log"),
                clusterManifestLocation,
                false,
                System.Fabric.FabricValidatorConstants.FabricUpgradeDefaultInstanceId,
                System.Fabric.FabricValidatorConstants.FabricUpgradeDefaultTargetVersion,
                null,
                null,
                null
                );
        }

        [TestProperty("ThreadingModel", "MTA")]
        public void TestDeploymentParameterCreateWithBinRootEnvironmentSet()
        {
            SetEnvironment(true, false);
            string clusterManifestLocation = "ClusterManifest.xml";
            File.WriteAllText(clusterManifestLocation, "TestManifest");
            DeploymentParameters parameters = new DeploymentParameters();
            parameters.SetParameters(new Dictionary<string, dynamic>() { 
            { DeploymentParameters.ClusterManifestString, clusterManifestLocation },
            { DeploymentParameters.FabricDataRootString, Path.Combine(RootLocation, "Data")}
            }, DeploymentOperations.Create);
            parameters.Initialize();
            Validate(
                parameters,
                DeploymentOperations.Create,
                Path.Combine(RootLocation, "Data"),
                Path.Combine(RootLocation, "Data", "log"),
                clusterManifestLocation,
                false,
                System.Fabric.FabricValidatorConstants.FabricUpgradeDefaultInstanceId,
                System.Fabric.FabricValidatorConstants.FabricUpgradeDefaultTargetVersion,
                null,
                null,
                null
                );
        }

        [TestProperty("ThreadingModel", "MTA")]
        public void TestDeploymentParameterCreateWithBinRootEnvironmentAndNodeNameSet()
        {
            SetEnvironment(true, false);
            string clusterManifestLocation = "ClusterManifest.xml";
            File.WriteAllText(clusterManifestLocation, "TestManifest");
            DeploymentParameters parameters = new DeploymentParameters();
            parameters.SetParameters(new Dictionary<string, dynamic>() { 
            { DeploymentParameters.ClusterManifestString, clusterManifestLocation },
            { DeploymentParameters.FabricDataRootString, Path.Combine(RootLocation, "Data")},
            { DeploymentParameters.NodeNameString, "Node1"}
            }, DeploymentOperations.Create);
            parameters.Initialize();
            Validate(
                parameters,
                DeploymentOperations.Create,
                Path.Combine(RootLocation, "Data"),
                Path.Combine(RootLocation, "Data", "log"),
                clusterManifestLocation,
                false,
                System.Fabric.FabricValidatorConstants.FabricUpgradeDefaultInstanceId,
                System.Fabric.FabricValidatorConstants.FabricUpgradeDefaultTargetVersion,
                null,
                Path.Combine(RootLocation, "Data", "Node1", "Fabric.Data", "InfrastructureManifest.xml"),
                "Node1"
                );
        }

        private static void Validate(
            DeploymentParameters parameters, 
            DeploymentOperations expectedOperation,
            string expectedDataRoot, 
            string expectedLogRoot, 
            string expectedClusterManifestLocation, 
            bool expectedDeleteTargetFile, 
            string expectedInstanceId,
            string expectedTargetVersion,
            string expectedCurrentVersion,
            string expectedInfrastructureManifestLocation,
            string expectedNodeName)
        {
            Verify.AreEqual(expectedOperation, parameters.Operation, "DeploymentOperations");
            Verify.AreEqual(Path.Combine(RootLocation, "bin"), parameters.FabricBinRoot, "FabricBinRoot");
            Verify.AreEqual(expectedDataRoot, parameters.FabricDataRoot, "FabricDataRoot");
            Verify.AreEqual(expectedLogRoot, parameters.FabricLogRoot, "FabricLogRoot");
            Verify.AreEqual(expectedClusterManifestLocation, parameters.ClusterManifestLocation, "ClusterManifestLocation");
            Verify.AreEqual(expectedDeleteTargetFile, parameters.DeleteTargetFile, "DeleteTargetFile");
            Verify.AreEqual(expectedInstanceId, parameters.InstanceId, "InstanceId");
            Verify.AreEqual(expectedTargetVersion, parameters.TargetVersion, "TargetVersion");

            if (string.IsNullOrEmpty(expectedCurrentVersion))
            {
                Verify.IsTrue(string.IsNullOrEmpty(parameters.CurrentVersion), "CurrentVersion");
            }
            else
            {
                Verify.AreEqual(expectedCurrentVersion, parameters.CurrentVersion, "CurrentVersion");
            }

            if (string.IsNullOrEmpty(expectedInfrastructureManifestLocation))
            {
                Verify.IsTrue(string.IsNullOrEmpty(parameters.InfrastructureManifestLocation), "InfrastructureManifestLocation");
            }
            else
            {
                Verify.IsTrue(string.IsNullOrEmpty(parameters.InfrastructureManifestLocation), "InfrastructureManifestLocation");
            }

            if (string.IsNullOrEmpty(expectedNodeName))
            {
                Verify.IsTrue(string.IsNullOrEmpty(parameters.NodeName), "NodeName");
            }
            else
            {
                Verify.AreEqual(expectedNodeName, parameters.NodeName, "NodeName");
            }

            Verify.IsTrue((parameters.RunAsPassword != null), "RunAsPassword");
            Verify.IsTrue(string.IsNullOrEmpty(parameters.RunAsUserName), "RunAsUserName");
            Verify.IsTrue(string.IsNullOrEmpty(parameters.RunAsUserType), "RunAsUserType");
            Verify.IsTrue(string.IsNullOrEmpty(parameters.ServiceStartupType), "ServiceStartupType");
        }
        
        private static void SetEnvironment(bool setDataRoot, bool setLogRoot)
        {
            Environment.SetEnvironmentVariable("FabricBinRoot", Path.Combine(RootLocation, "bin"));
            Environment.SetEnvironmentVariable("FabricDataRoot", Path.Combine(RootLocation, "Data"));
            if (setLogRoot)
            {
                Environment.SetEnvironmentVariable("FabricLogRoot", Path.Combine(RootLocation, "Log"));
            }
        }

        internal static string RootLocation = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "log", "DeploymentParametersTest");
    }
}