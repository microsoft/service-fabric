// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Diagnostics;

namespace System.Fabric.FabricDeployer
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using NetFwTypeLib;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.IO;
    using System.Text;
    using WEX.TestExecution;

    /// <summary>
    /// This is the class that tests the validation of the files that we deploy.
    /// </summary>
    [TestClass]
    internal class FabricDeployerValidatorTests
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateProductionClusterManifests()
        {
            //InitializeTracing("ValidateProductionClusterManifests");
            //string[] productionConfigurationFilesToBeValidated = { "Development-Nine-Node-Server.xml" };
            //string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            //string schemaFile = Path.Combine(fabunittest, "ServiceFabricServiceModel.xsd");
            //foreach (string fileName in productionConfigurationFilesToBeValidated)
            //{
            //    string data = File.ReadAllText(Path.Combine(fabunittest, fileName));
            //    data = data.Replace("IsScaleMin=\"false\"", "IsScaleMin=\"true\"");
            //    data = AddDiagnosticStoreInformation(data);
            //    string clusterManifestFilePath = "Test-" + fileName;
            //    File.WriteAllText(clusterManifestFilePath, data);
            //    ValidateClusterManifest(clusterManifestFilePath, false);
            //}
        }

        private void ValidateClusterManifest(string clusterManifestFilePath, bool exceptionExcpected)
        {
            bool exceptionFound = false;
            try
            {
                DeploymentParameters parameters = new DeploymentParameters();
                parameters.SetParameters(new Dictionary<string, dynamic>()
                    {
                        { DeploymentParameters.ClusterManifestString, clusterManifestFilePath },
                        { DeploymentParameters.FabricDataRootString, "TestFabricDataRoot"}
                    },
                    DeploymentOperations.ValidateClusterManifest);
                DeploymentOperation.ExecuteOperation(parameters);
            }
            catch (Exception e)
            {
                DeployerTrace.WriteInfo("Test", "ValidateClusterManifest failed with exception {0}", e);
                exceptionFound = true;
            }
            Verify.AreEqual(exceptionExcpected, exceptionFound);
        }

        private string AddDiagnosticStoreInformation(string manifest)
        {
            const string storePlaceHolder = "Name=\"StoreConnectionString\" Value=\"\"";
            const string fileStore = "Name=\"StoreConnectionString\" Value=\"file://myshare/mydata\"";
            const string azureStore = "Name=\"StoreConnectionString\" Value=\"xstore:DefaultEndpointsProtocol=https;AccountName=myaccount;AccountKey=mykey\"";

            StringBuilder manifestBuilder = new StringBuilder(manifest);
            // The Azure store is specified at the end
            int azureStoreIndex = manifest.LastIndexOf(storePlaceHolder);
            if (azureStoreIndex != -1)
            {
                // First specify the Azure store
                manifestBuilder = manifestBuilder.Replace(
                                    storePlaceHolder,
                                    azureStore,
                                    azureStoreIndex,
                                    (manifest.Length - azureStoreIndex));

                // Specify the file stores
                manifestBuilder = manifestBuilder.Replace(
                                    storePlaceHolder,
                                    fileStore,
                                    0,
                                    azureStoreIndex);
            }
            return manifestBuilder.ToString();
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateInvalidSectionsNotAllowed()
        {
            InitializeTracing("ValidateInvalidSectionsNotAllowed");
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            string schemaFile = Path.Combine(fabunittest, "ServiceFabricServiceModel.xsd");
            ValidateClusterManifest(Path.Combine(fabunittest, "ClusterManifest-FabricNodeSection.xml"), true);
            ValidateClusterManifest(Path.Combine(fabunittest, "ClusterManifest-NodeCapacitiesSection.xml"), true);
            ValidateClusterManifest(Path.Combine(fabunittest, "ClusterManifest-NodeDomainIdsSection.xml"), true);
            ValidateClusterManifest(Path.Combine(fabunittest, "ClusterManifest-NodePropertiesSection.xml"), true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateDependenciesAreChecked()
        {
            InitializeTracing("ValidateDependenciesAreChecked");
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            ValidateClusterManifest(Path.Combine(fabunittest, "ClusterManifest-ClusterCredentialTypeDependencyCheck.xml"), true);
            ValidateClusterManifest(Path.Combine(fabunittest, "ClusterManifest-NTLMEnabledDependencyCheck.xml"), true);
            ValidateClusterManifest(Path.Combine(fabunittest, "ClusterManifest-ServerAuthCredentialTypeDependencyCheck.xml"), true);
        }

        private static void InitializeTracing(string testName)
        {
            string log = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "log");
            string fileName = Path.Combine(log, string.Format(System.Globalization.CultureInfo.InvariantCulture, "{0}.trace", testName));
            DeployerTrace.UpdateFileLocation(fileName);
            DeployerTrace.WriteInfo("Test", "Starting test {0}", testName);
        }
    }
}