// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.TestExecution;
    using System.Fabric.Interop;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Diagnostics;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Common;
    using System.Globalization;

    [TestClass]
    public class ImageBuilderFabricUpgradeTest
    {
        private TestContext testContextInstance;
        private ImageBuilderTestContext imageBuilderTestContext;
        private ImageBuilder imageBuilder;
        private IImageStore imageStore;

        public TestContext TestContext
        {
            get { return testContextInstance; }
            set { testContextInstance = value; }
        }

        [TestInitialize]
        public void TestSetup()
        {
            this.imageBuilderTestContext = ImageBuilderTestContext.Create(this.TestContext.TestName);
            this.imageBuilder = imageBuilderTestContext.ImageBuilder;
            this.imageStore = imageBuilderTestContext.ImageStore;
            ImageBuilderTest.CreateDropAndSetFabricCodePath();
        }

        [TestCleanup]
        public virtual void TestCleanup()
        {
            this.imageBuilderTestContext.TestCleanup();
        }

        [ClassInitialize]
        static public void ClassSetup()
        {
            Environment.SetEnvironmentVariable("FabricDataRoot", TestUtility.TestDirectory, EnvironmentVariableTarget.Process);
        }

        [ClassCleanup]
        static public void ClassCleanup()
        {
            Environment.SetEnvironmentVariable("FabricDataRoot", null, EnvironmentVariableTarget.Process);
        }        

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestGetFabricVersionInfo()
        {            
            using (FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore))
            {
                string codePath, configPath, infrastructureManifestFilePath;

                fabricLayoutInfo.Create(out codePath, out configPath, out infrastructureManifestFilePath);
                FabricVersion fabricVersion = this.imageBuilder.GetFabricVersionInfo(codePath, configPath, TestUtility.ImageStoreDefaultTimeout);

                Verify.AreEqual(fabricVersion.CodeVersion, fabricLayoutInfo.PatchVersion);
                Verify.AreEqual(fabricVersion.ConfigVersion, fabricLayoutInfo.ClusterManifest.Version);
            }
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestProvisionFabricWithMSI()
        {
            using (FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore))
            {
                string codePath, configPath, infrastructureManifestFilePath;
                fabricLayoutInfo.Create(out codePath, out configPath, out infrastructureManifestFilePath);

                string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                string configurationCsvFilePath = Path.Combine(currentExecutingDirectory, "Configurations.csv");

                this.imageBuilder.ProvisionFabric(
                    codePath,
                    configPath,
                    configurationCsvFilePath,
                    infrastructureManifestFilePath,
                    TimeSpan.MaxValue);

                TestUtility.VerifyWinFabLayout(fabricLayoutInfo);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestProvisionFabricWithCodePackage()
        {
            using (FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore, "ClusterManifest-AllSettings.xml", false))
            {
                string codePath, configPath, infrastructureManifestFilePath;
                fabricLayoutInfo.Create(out codePath, out configPath, out infrastructureManifestFilePath);

                string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                string configurationCsvFilePath = Path.Combine(currentExecutingDirectory, "Configurations.csv");

                this.imageBuilder.ProvisionFabric(
                    codePath,
                    configPath,
                    configurationCsvFilePath,
                    infrastructureManifestFilePath,
                    TimeSpan.MaxValue);

                TestUtility.VerifyWinFabLayout(fabricLayoutInfo);
            }
        }
        
         //TODO - Defect 1670459: System.Fabric.Management.Test and ServerDeployer.Test.dll cluster manifest validation tests fail often
         [TestMethod]
         [TestProperty("ThreadingModel", "MTA")]
        public void TestConfigOnlyProvisionFabric()
        {
            using (FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore))
            {
                fabricLayoutInfo.PatchVersion = null;

                string codePath, configPath, infrastructureManifestFilePath;
                fabricLayoutInfo.Create(out codePath, out configPath, out infrastructureManifestFilePath);

                string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                string configurationCsvFilePath = Path.Combine(currentExecutingDirectory, "Configurations.csv");

                this.imageBuilder.ProvisionFabric(
                    codePath,
                    configPath,
                    configurationCsvFilePath,
                    infrastructureManifestFilePath,
                    TimeSpan.MaxValue);

                TestUtility.VerifyWinFabLayout(fabricLayoutInfo);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestFabricUpgradeWithInvalidPath()
        {
            using (FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore))
            {
                fabricLayoutInfo.PatchVersion = null;

                string codePath, configPath, infrastructureManifestFilePath;
                fabricLayoutInfo.Create(out codePath, out configPath, out infrastructureManifestFilePath);

                string directoryName = Path.GetDirectoryName(configPath);

                Verify.Throws<FabricImageBuilderValidationException>(
                () => this.imageBuilder.GetFabricVersionInfo(codePath, directoryName, TestUtility.ImageStoreDefaultTimeout));
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestConfigOnlyFabricUpgradeWithDiagnostics()
        {
            string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            string configurationCsvFilePath = Path.Combine(currentExecutingDirectory, "Configurations.csv");

            string[] manifests = new string[] { "ClusterManifest-WithoutDiagnostics.xml", "ClusterManifest-WithDiagnostics.xml" };
            FabricVersion[] fabricVersion = new FabricVersion[2];
            for (int i = 0; i < 2; i++)
            {
                using (FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore, manifests[i]))
                {
                    fabricLayoutInfo.PatchVersion = null;

                    string codePath, configPath, infrastructureManifestFilePath;
                    fabricLayoutInfo.Create(out codePath, out configPath, out infrastructureManifestFilePath);
                    fabricVersion[i] = this.imageBuilder.GetFabricVersionInfo(codePath, configPath, TestUtility.ImageStoreDefaultTimeout);

                    this.imageBuilder.ProvisionFabric(
                        codePath,
                        configPath,
                        configurationCsvFilePath,
                        infrastructureManifestFilePath,
                        TimeSpan.MaxValue);

                    TestUtility.VerifyWinFabLayout(fabricLayoutInfo);
                }
            }

            this.imageBuilder.UpgradeFabric(
                fabricVersion[0].ToString(), 
                fabricVersion[1].ToString(), 
                configurationCsvFilePath, 
                TimeSpan.MaxValue);
        }

        [ImageStoreType(ImageStoreEnum.XStore)]
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestConfigOnlyFabricUpgradeWithRunAs()
        {
            string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            string configurationCsvFilePath = Path.Combine(currentExecutingDirectory, "Configurations.csv");

            string[] manifests = new string[] { "ClusterManifest-WithoutDiagnostics.xml", "ClusterManifest-WithRunAs.xml" };
            FabricVersion[] fabricVersion = new FabricVersion[2];
            for (int i = 0; i < 2; i++)
            {
                using (FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore, manifests[i]))
                {
                    fabricLayoutInfo.PatchVersion = null;

                    string codePath, configPath, infrastructureManifestFilePath;
                    fabricLayoutInfo.Create(out codePath, out configPath, out infrastructureManifestFilePath);
                    fabricVersion[i] = this.imageBuilder.GetFabricVersionInfo(codePath, configPath, TestUtility.ImageStoreDefaultTimeout);

                    this.imageBuilder.ProvisionFabric(
                        codePath,
                        configPath,
                        configurationCsvFilePath,
                        infrastructureManifestFilePath,
                        TimeSpan.MaxValue);

                    TestUtility.VerifyWinFabLayout(fabricLayoutInfo);
                }
            }

            this.imageBuilder.UpgradeFabric(
                fabricVersion[0].ToString(),
                fabricVersion[1].ToString(),
                configurationCsvFilePath,
                TimeSpan.MaxValue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestProvisionOfDefaultImageStoreConnectionString()
        {
            using (FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore))
            {
                string defaultImageStoreConnectionString = "_default_";
                fabricLayoutInfo.PatchVersion = null;

                TestUtility.AddToFabricSettings(
                    FabricValidatorConstants.SectionNames.Management,
                    FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                    defaultImageStoreConnectionString,
                    false /*isEncrypted*/,
                    fabricLayoutInfo.ClusterManifest);

                string codePath, configPath, infrastructureManifestFilePath;
                fabricLayoutInfo.Create(out codePath, out configPath, out infrastructureManifestFilePath);

                string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                string configurationCsvFilePath = Path.Combine(currentExecutingDirectory, "Configurations.csv");

                this.imageBuilder.ProvisionFabric(
                    codePath,
                    configPath,
                    configurationCsvFilePath,
                    infrastructureManifestFilePath,
                    TimeSpan.MaxValue);

                string clusterManifestPath = Path.Combine(
                    "WindowsFabricStore", 
                    string.Format("ClusterManifest.{0}.xml", 
                    fabricLayoutInfo.ClusterManifest.Version));
                var provisionedClusterManifest = fabricLayoutInfo.ImageStoreWrapper.GetFromStore<ClusterManifestType>(clusterManifestPath, TestUtility.ImageStoreDefaultTimeout);

                var imageStoreConnectionStringValue = provisionedClusterManifest.FabricSettings.First(
                    section => TestUtility.Equals(section.Name, FabricValidatorConstants.SectionNames.Management)).Parameter.First(
                    parameter => TestUtility.Equals(parameter.Name, FabricValidatorConstants.ParameterNames.ImageStoreConnectionString)).Value;

                Verify.AreNotEqual(imageStoreConnectionStringValue, defaultImageStoreConnectionString);

                Verify.IsTrue(imageStoreConnectionStringValue.StartsWith("file:"));

                TestUtility.VerifyWinFabLayout(fabricLayoutInfo);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidMSIFile()
        {
            string tempInvalidMsiFileName = Path.ChangeExtension(Path.GetRandomFileName(), "msi");
            string tempInvalidMsiFilePath = Path.Combine(TestUtility.TestDirectory, tempInvalidMsiFileName);

            try
            {
                using (var stream = File.CreateText(tempInvalidMsiFilePath))
                {
                    stream.WriteLine("Invalid test msi file");
                }

                this.imageStore.UploadContent(                    
                    tempInvalidMsiFileName,
                    tempInvalidMsiFilePath,
                    TimeSpan.MaxValue,
                    CopyFlag.AtomicCopy,
                    false);

                FabricVersion fabricVersion = this.imageBuilder.GetFabricVersionInfo(tempInvalidMsiFileName, null, TestUtility.ImageStoreDefaultTimeout);
            }
            catch (FabricImageBuilderValidationException exception)
            {
                Verify.IsTrue((int)exception.ErrorCode == unchecked((int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_INVALID_MSI_FILE));
            }
            finally
            {
                if (File.Exists(tempInvalidMsiFilePath))
                {
                    File.Delete(tempInvalidMsiFilePath);
                }

                this.imageStore.DeleteContent(tempInvalidMsiFileName, TimeSpan.MaxValue);
            }
        }

    }
}