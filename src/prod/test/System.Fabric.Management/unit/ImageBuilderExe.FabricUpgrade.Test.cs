// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.Test;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.TestExecution;
    using System.Fabric.Management.ImageStore;

    [TestClass]
    public class ImageBuilderExeFabricUpgradeTest
    {
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
        public void TestGetFabricVersion()
        {
            using(FabricLayoutInfo fabricLayoutInfo = new FabricLayoutInfo(this.imageStore))
            {
                string codePath, configPath, infrastructureManifestFilePath;
                fabricLayoutInfo.Create(out codePath, out configPath, out infrastructureManifestFilePath);

                ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

                string codeVersion, configVersion;
                int exitCode = exeWrapper.GetFabricVersion(
                    codePath,
                    configPath,
                    out codeVersion,
                    out configVersion);

                Verify.AreEqual(exitCode, 0);
                Verify.AreEqual(codeVersion, fabricLayoutInfo.PatchVersion);
                Verify.AreEqual(configVersion, fabricLayoutInfo.ClusterManifest.Version);
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

                ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

                int exitCode = exeWrapper.ProvisionFabric(
                    codePath,
                    configPath, 
                    infrastructureManifestFilePath);

                Verify.AreEqual(exitCode, 0);
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

                ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

                int exitCode = exeWrapper.ProvisionFabric(
                    codePath,
                    configPath,
                    infrastructureManifestFilePath);

                Verify.AreEqual(exitCode, 0);
                TestUtility.VerifyWinFabLayout(fabricLayoutInfo);
            }
        }
    }   
}