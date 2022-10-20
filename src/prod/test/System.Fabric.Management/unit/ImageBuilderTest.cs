// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.FabricDeployer;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Management.Automation.Runspaces;
    using WEX.TestExecution;

    [TestClass]
    public class ImageBuilderTest
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
            CreateDropAndSetFabricCodePath();
        }

        [TestCleanup]
        public virtual void TestCleanup()
        {
            this.imageBuilderTestContext.TestCleanup();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationPackageThroughPowershell()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            string applicationLocalPath = Path.Combine(TestUtility.TestDirectory, Guid.NewGuid().ToString());
            try
            {
                buildLayoutInfo.ImageStore.DownloadContent(applicationTypeName, applicationLocalPath, TimeSpan.MaxValue, CopyFlag.AtomicCopy);

                var command = new Command("Test-ServiceFabricApplicationPackage");
                command.Parameters.Add("ApplicationPackagePath", applicationLocalPath);

                using (Pipeline pipeline = TestUtility.PowershellRunspace.CreatePipeline())
                {
                    pipeline.Commands.Add(command);
                    var result = pipeline.Invoke();

                    Verify.IsTrue(result.Count == 1 && (bool)result[0].ImmediateBaseObject == true, "Test-ServiceFabricApplicationPackage");
                }
            }
            finally
            {
                if(Directory.Exists(applicationLocalPath))
                {
                    Directory.Delete(applicationLocalPath, true);
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationFromExternalSfpkg()
        {
            AppTrace.TraceSource.WriteInfo("ImageBuilderTest", "Start TestBuildApplicationFromExternalSfpkg");

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.CreateBuildLayout();

            string sfPkgDestinationFolder = GetTempDirectoryPath("SfpkgFolder");
            
            string blobName = null;
            try
            {
                // Archive the build layout into a .sfpkg file in the source folder.
                AppTrace.TraceSource.WriteInfo("ImageBuilderTest", "TestBuildApplicationFromExternalSfpkg: GenerateSfpkg at {0}", sfPkgDestinationFolder);
                string sfpkgFilePath = buildLayoutInfo.GenerateSfpkg(sfPkgDestinationFolder, false /*applyCompression*/);
                
                //
                // -------------------- Upload the sfpkg to XStore, to a unique blob name.
                // If multiple tests are running in parallel, each test will create new blob.
                //
                blobName = string.Format("{0}{1}", Guid.NewGuid(), buildLayoutInfo.GetSfpkgFileName());
                string uri = TestUtility.UploadFileToXStoreTempTestResources(sfpkgFilePath, blobName);
                AppTrace.TraceSource.WriteInfo("ImageBuilderTest", "Uploaded {0} to xstore at {1}", blobName, uri);

                //
                // ---------------- Download failure cases
                // 
                string nonExistingUri = Path.ChangeExtension(uri, "sfpkg1");
                Verify.Throws<FabricImageBuilderValidationException>(
                    () => this.imageBuilder.DownloadAndBuildApplicationType(
                        nonExistingUri,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                        sfPkgDestinationFolder,
                        TimeSpan.MaxValue));

                string wrongProtocolUri = "x" + uri;
                Verify.Throws<FabricImageBuilderValidationException>(
                    () => this.imageBuilder.DownloadAndBuildApplicationType(
                        wrongProtocolUri,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                        sfPkgDestinationFolder,
                        TimeSpan.FromSeconds(100)));

                string noProtocol = uri.Remove(0, uri.IndexOf(':'));
                Verify.Throws<FabricImageBuilderValidationException>(
                    () => this.imageBuilder.DownloadAndBuildApplicationType(
                        noProtocol,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                        sfPkgDestinationFolder,
                        TimeSpan.FromSeconds(100)));

                noProtocol = uri.Remove(0, uri.IndexOf(':') + 3);
                Verify.Throws<FabricImageBuilderValidationException>(
                    () => this.imageBuilder.DownloadAndBuildApplicationType(
                        noProtocol,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                        sfPkgDestinationFolder,
                        TimeSpan.FromSeconds(100)));

                // Download with small timeout, expect timeout exception
                Verify.Throws<TimeoutException>(
                    () => this.imageBuilder.DownloadAndBuildApplicationType(
                        uri,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                        sfPkgDestinationFolder,
                        TimeSpan.FromMilliseconds(1)));

                //
                // ---------------- Successful download, incorrect app type info, fails
                //
                Verify.Throws<FabricImageBuilderValidationException>(
                    () => this.imageBuilder.DownloadAndBuildApplicationType(
                        uri,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                        "InvalidAppTypeVersion",
                        sfPkgDestinationFolder,
                        TimeSpan.MaxValue));

                Verify.Throws<FabricImageBuilderValidationException>(
                    () => this.imageBuilder.DownloadAndBuildApplicationType(
                        uri,
                        "InvalidAppTypeName",
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                        sfPkgDestinationFolder,
                        TimeSpan.MaxValue));

                //
                // ---------------- Successful download, successful build
                //
                this.imageBuilder.DownloadAndBuildApplicationType(
                    uri,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    sfPkgDestinationFolder,
                    TimeSpan.FromMinutes(10));
                Verify.IsTrue(Directory.Exists(imageBuilderTestContext.FabricWorkingDir));
                TestUtility.VerifyStoreLayout(buildLayoutInfo);
            }
            finally
            {
                if (!string.IsNullOrEmpty(blobName))
                {
                    //
                    // Delete the .sfpkg from XStore
                    //
                    TestUtility.DeleteXStoreTempTestResourcesBlob(blobName);
                }

                if (Directory.Exists(sfPkgDestinationFolder))
                {
                    Directory.Delete(sfPkgDestinationFolder, true);
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestGetApplicationTypeInfo()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            ApplicationTypeInformation applicationTypeInfo = this.imageBuilder.GetApplicationTypeInfo(applicationTypeName, TestUtility.ImageStoreDefaultTimeout);

            Verify.AreEqual("MasterApplicationType", applicationTypeInfo.Name);
            Verify.AreEqual("1.0", applicationTypeInfo.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            Verify.IsTrue(Directory.Exists(imageBuilderTestContext.FabricWorkingDir));

            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationTypeWithSpecialCharacters()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2012 {EOW}";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType_Diff_ChangedApplicationManifestOnly()
        {
            // Provision an application with version 1.0 (full package)
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string version1 = "1.0";
            buildLayoutInfo.UpdateApplicationVersion(version1);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);
                        
            // Provision updated version 2.0
            // - same service manifest ref, do not include in build layout
            string version2 = "2.0";
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            updatedBuildLayoutInfo.UpdateApplicationVersion(version2);
            // Create build layout with just entries at new version - no service manifest included
            applicationTypeName = updatedBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version2 });
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Check that both versions have been provisioned to store
            TestUtility.VerifyStoreLayout(buildLayoutInfo);
            TestUtility.VerifyStoreLayout(updatedBuildLayoutInfo);

            // Provision updated version 3.0
            // - same service manifest ref, include it in build layout
            string lastVersion = "3.0";
            BuildLayoutInfo lastBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            lastBuildLayoutInfo.UpdateApplicationVersion(lastVersion);
            // Create build layout with service manifest included
            applicationTypeName = lastBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version1, lastVersion });
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);
            TestUtility.VerifyStoreLayout(lastBuildLayoutInfo);            
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType_Diff_ChangedCodePackageOnly()
        {
            // Provision an application with version 1.0 (full package)
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string version1 = "1.0";
            buildLayoutInfo.UpdateApplicationVersion(version1);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);
            
            // Provision updated version 2.0
            // - changed service manifest ref (version 2.0), pointing to new code package (version 2.0)
            // Only new code package included in the diff layout
            string version2 = "2.0";
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            updatedBuildLayoutInfo.UpdateApplicationVersion(version2);

            Verify.IsTrue(updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport.Length > 0);
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = version2;

            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes.Count > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].Version = version2;

            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage.Length > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Version = version2;
            
            // Create build layout with just entries at new version
            applicationTypeName = updatedBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version2 });

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Check that both versions have been provisioned to store
            TestUtility.VerifyStoreLayout(buildLayoutInfo);
            TestUtility.VerifyStoreLayout(updatedBuildLayoutInfo);         
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType_Diff_ChangedConfigPackageOnly()
        {
            // Provision an application with version 1.0 (full package)
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);
            Verify.IsTrue(Directory.Exists(imageBuilderTestContext.FabricWorkingDir));
            TestUtility.VerifyStoreLayout(buildLayoutInfo);

            // Provision updated version 2.0
            // - changed service manifest ref (version 2.0), pointing to new config package (version 2.0)
            // Only new config package included in the diff layout
            string version2 = "2.0";
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            updatedBuildLayoutInfo.UpdateApplicationVersion(version2);

            Verify.IsTrue(updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport.Length > 0);
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = version2;

            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes.Count > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].Version = version2;

            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes[0].ConfigPackage.Length > 0); 
            updatedBuildLayoutInfo.ServiceManifestTypes[0].ConfigPackage[0].Version = version2;

            // Create build layout with just entries at new version
            applicationTypeName = updatedBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version2 });

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Check that both versions have been provisioned to store
            TestUtility.VerifyStoreLayout(buildLayoutInfo);
            TestUtility.VerifyStoreLayout(updatedBuildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType_Diff_ChangedDataPackageOnly()
        {
            // Provision an application with version 1.0 (full package)
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);
            Verify.IsTrue(Directory.Exists(imageBuilderTestContext.FabricWorkingDir));
            TestUtility.VerifyStoreLayout(buildLayoutInfo);

            // Provision updated version 2.0
            // - changed service manifest ref (version 2.0), pointing to new data package (version 2.0)
            // Only new data package included in the diff layout
            string version2 = "2.0";
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            updatedBuildLayoutInfo.UpdateApplicationVersion(version2);

            Verify.IsTrue(updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport.Length > 0);
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = version2;

            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes.Count > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].Version = version2;

            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage.Length > 0); 
            updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage[0].Version = version2;

            // Create build layout with just entries at new version
            applicationTypeName = updatedBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version2 });

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Check that both versions have been provisioned to store
            TestUtility.VerifyStoreLayout(buildLayoutInfo);
            TestUtility.VerifyStoreLayout(updatedBuildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType_Diff_RemoveDataPackage()
        {
            // Provision an application with version 1.0 (full package)
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);
            Verify.IsTrue(Directory.Exists(imageBuilderTestContext.FabricWorkingDir));
            TestUtility.VerifyStoreLayout(buildLayoutInfo);

            // Provision updated version 2.0
            // - changed service manifest ref (version 2.0), to have one less data package (version 2.0)
            // Only service manifest included in layout, no inner folders
            string version2 = "2.0";
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            updatedBuildLayoutInfo.UpdateApplicationVersion(version2);

            Verify.IsTrue(updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport.Length > 0);
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = version2;

            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes.Count > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].Version = version2;
            
            // Remove the last data package
            var dataPackages = updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage;
            List<DataPackageType> newDataPackages = new List<DataPackageType>();
            for (int i = 0; i < dataPackages.Length - 1; i++)
            {
                newDataPackages.Add(dataPackages[i]);
            }

            updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage = newDataPackages.ToArray();

            // Create build layout with just entries at new version
            applicationTypeName = updatedBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version2 });

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Check that both versions have been provisioned to store
            TestUtility.VerifyStoreLayout(buildLayoutInfo);
            TestUtility.VerifyStoreLayout(updatedBuildLayoutInfo);
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType_Diff_MultipleVersion()
        {
            // Provision an application with version 1.0 (full package)
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string version1 = "1.0";
            buildLayoutInfo.UpdateApplicationVersion(version1);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Provision updated version 2.0
            // - changed service manifest ref (version 2.0), pointing to new code package (version 2.0)
            string version2 = "2.0";
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            updatedBuildLayoutInfo.UpdateApplicationVersion(version2);
            Verify.IsTrue(updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport.Length > 0);
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = version2;
            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes.Count > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].Version = version2;
            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage.Length > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Version = version2;
            applicationTypeName = updatedBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version2 });
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Provision updated version 3.0
            // - changed service manifest ref (version 3.0), pointing to 
            //          one code package in version 2.0 (already in store)
            //          one config package in version 3.0 (new, must be provided)
            //          the rest in version 1.0 (already in store)
            string version3 = "3.0";
            BuildLayoutInfo updatedBuildLayoutInfo2 = new BuildLayoutInfo(this.imageStore);
            updatedBuildLayoutInfo2.UpdateApplicationVersion(version3);

            Verify.IsTrue(updatedBuildLayoutInfo2.ApplicationManifestType.ServiceManifestImport.Length > 0);
            updatedBuildLayoutInfo2.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = version3;
            Verify.IsTrue(updatedBuildLayoutInfo2.ServiceManifestTypes.Count > 0);
            updatedBuildLayoutInfo2.ServiceManifestTypes[0].Version = version3;

            Verify.IsTrue(updatedBuildLayoutInfo2.ServiceManifestTypes[0].CodePackage.Length > 0);
            updatedBuildLayoutInfo2.ServiceManifestTypes[0].CodePackage[0].Version = version2;

            Verify.IsTrue(updatedBuildLayoutInfo2.ServiceManifestTypes[0].ConfigPackage.Length > 0);
            updatedBuildLayoutInfo2.ServiceManifestTypes[0].ConfigPackage[0].Version = version3;

            // Do not include version2
            applicationTypeName = updatedBuildLayoutInfo2.CreateDiffBuildLayout(new string[] { version1, version3 });
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Check that all versions have been provisioned to store
            TestUtility.VerifyStoreLayout(buildLayoutInfo);
            TestUtility.VerifyStoreLayout(updatedBuildLayoutInfo);
            TestUtility.VerifyStoreLayout(updatedBuildLayoutInfo2);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType_Diff_Invalid_ChangedServiceManifestNotIncluded()
        {
            // Provision an application with version 1.0 (full package)
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Provision updated version, same app manifest but changed service manifest version, without service manifest
            string version2 = "2.0";
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            updatedBuildLayoutInfo.UpdateApplicationVersion(version2);
            
            Verify.IsTrue(updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport.Length > 0);
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = version2;
            
            // Create build layout with just entries at new version - no service manifest included
            applicationTypeName = updatedBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version2 });

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType_Diff_Invalid_ChangedCodePackageNotIncluded()
        {
            // Provision an application with version 1.0 (full package)
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string version1 = "1.0";
            buildLayoutInfo.UpdateApplicationVersion(version1);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();            
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Provision updated version
            string version2 = "2.0";
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            
            updatedBuildLayoutInfo.UpdateApplicationVersion(version2);

            Verify.IsTrue(updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport.Length > 0);
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = version2;

            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes.Count > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].Version = version2;

            // Change the expected version for a code package
            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage.Length > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Version = "3.0";

            // Create build layout - the new code package with version 3.0 is not included
            applicationTypeName = updatedBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version1, version2 });

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType_Diff_Invalid_ChangedConfigPackageNotIncluded()
        {
            // Provision an application with version 1.0 (full package)
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string version1 = "1.0";
            buildLayoutInfo.UpdateApplicationVersion(version1);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Provision updated version, same app manifest but changed service manifest version, without service manifest
            string version2 = "2.0";
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            updatedBuildLayoutInfo.UpdateApplicationVersion(version2);

            Verify.IsTrue(updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport.Length > 0);
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = version2;

            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes.Count > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].Version = version2;

            // Change the expected version for a config package
            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes[0].ConfigPackage.Length > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].ConfigPackage[0].Version = "3.0";

            // Create build layout - the new config package with version 3.0 is not included
            applicationTypeName = updatedBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version1, version2 });

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType_Diff_Invalid_ChangedDataPackageNotIncluded()
        {
            // Provision an application with version 1.0 (full package)
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string version1 = "1.0";
            buildLayoutInfo.UpdateApplicationVersion(version1);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Provision updated version, same app manifest but changed service manifest version, without service manifest
            string version2 = "2.0";
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            updatedBuildLayoutInfo.UpdateApplicationVersion(version2);

            Verify.IsTrue(updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport.Length > 0);
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = version2;

            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes.Count > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].Version = version2;

            // Change the expected version for a data package
            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage.Length > 0);
            updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage[0].Version = "3.0";

            // Create build layout with just entries at new version - the new data package with version 3.0 is not included
            applicationTypeName = updatedBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version2 });

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType_Diff_Invalid_ConflictingCodePackage()
        {
            // Provision an application with version 1.0 (full package)
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string version1 = "1.0";
            buildLayoutInfo.UpdateApplicationVersion(version1);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            // Provision updated version, app manifest version 2.0 pointing to service manifest 1.0
            string version2 = "2.0";
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            updatedBuildLayoutInfo.UpdateApplicationVersion(version2);
            applicationTypeName = updatedBuildLayoutInfo.CreateDiffBuildLayout(new string[] { version1, version2 });

            Verify.IsTrue(updatedBuildLayoutInfo.ServiceManifestTypes.Count > 0);
            var serviceManifestType = updatedBuildLayoutInfo.ServiceManifestTypes[0];

            // Modify the code package without changing the version
            Verify.IsTrue(serviceManifestType.CodePackage.Length > 0);
            TestUtility.ModifyCodePackage(serviceManifestType.CodePackage[0].Name, serviceManifestType, updatedBuildLayoutInfo);

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplication()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("portvalue", "500");
            userParameters.Add("DomainGroup", "DomainGroup");
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TimeSpan.MaxValue);

            RolloutVersion expectedRolloutVersion = RolloutVersion.CreateRolloutVersion();

            TestUtility.VerifyApplicationDigestLayout(buildLayoutInfo, applicationId, 1, expectedRolloutVersion);

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, expectedRolloutVersion);

            Verify.AreEqual(applicationId, applicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(applicationInstance.NameUri));
            Verify.AreEqual(expectedRolloutVersion.ToString(), applicationInstance.ApplicationPackageRef.RolloutVersion);
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestGetStoreContentWithMissingFile()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            Verify.Throws<FileNotFoundException>(
                () => buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(@"c:\InvalidFolder\InvalidFile", TestUtility.ImageStoreDefaultTimeout),
                @"The file 'c:\InvalidFolder\InvalidFile' was not found in the Image Store");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationWithNoServices()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.DefaultServices = null;
            buildLayoutInfo.ApplicationManifestType.ServiceTemplates = null;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("DomainGroup", "DomainGroup");
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TimeSpan.MaxValue);

            TestUtility.VerifyApplicationDigestLayout(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestWithNoPackage()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].DataPackage = null;
            buildLayoutInfo.ServiceManifestTypes[0].ConfigPackage = null;

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ConfigOverrides = null;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("DomainGroup", "DomainGroup");
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TimeSpan.MaxValue);

            TestUtility.VerifyApplicationDigestLayout(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationWithNoConfigOverride()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            foreach (ApplicationManifestTypeServiceManifestImport serviceImport in buildLayoutInfo.ApplicationManifestType.ServiceManifestImport)
            {
                serviceImport.ConfigOverrides = null;
            }

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TimeSpan.MaxValue);

            TestUtility.VerifyApplicationDigestLayout(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceTemplatesWithSpecialInstanceCount()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            ((StatelessServiceType)(buildLayoutInfo.ApplicationManifestType.ServiceTemplates[0])).InstanceCount = "-1";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDefaultServicesWithInvalidInstanceCount()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            ((StatelessServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[0])).Item).InstanceCount = "0";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDefaultServicesWithEmptyAffinityServiceName()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ServiceTypeServiceCorrelation c = new ServiceTypeServiceCorrelation() { ServiceName = "", Scheme = ServiceTypeServiceCorrelationScheme.Affinity };
            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).ServiceCorrelations = new ServiceTypeServiceCorrelation[1] { c };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDefaultServicesWithMoreThanOneAffinity()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            ServiceTypeServiceCorrelation c1 = new ServiceTypeServiceCorrelation() { ServiceName = "RequiredQueueService1", Scheme = ServiceTypeServiceCorrelationScheme.Affinity };
            ServiceTypeServiceCorrelation c2 = new ServiceTypeServiceCorrelation() { ServiceName = "RequiredQueueService2", Scheme = ServiceTypeServiceCorrelationScheme.NonAlignedAffinity };

            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).ServiceCorrelations = new ServiceTypeServiceCorrelation[2] { c1, c2 };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationManifestSchemaValidation()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ApplicationTypeName = null;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<FabricImageBuilderValidationException>(
                () => this.imageBuilder.GetApplicationTypeInfo(applicationTypeName, TestUtility.ImageStoreDefaultTimeout));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationManifestWithNoServiceImport()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport = null;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestWithNoCodePackage()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage = null;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestWithDuplicateDataPackage()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            Verify.IsTrue(buildLayoutInfo.ServiceManifestTypes.Count > 0);
            Verify.IsTrue(buildLayoutInfo.ServiceManifestTypes[0].DataPackage.Length > 0);

            // Add another data package with the same name as an existing one and an invalid version
            var pkgs = buildLayoutInfo.ServiceManifestTypes[0].DataPackage;
            var newDataPkg = new DataPackageType() { Name = pkgs[0].Name, Version = "Invalid" };

            var newPkgs = new DataPackageType[pkgs.Length + 1];
            for (int i = 0; i < pkgs.Length; i++)
            {
                newPkgs[i] = pkgs[i];
            }

            newPkgs[newPkgs.Length - 1] = newDataPkg;

            buildLayoutInfo.ServiceManifestTypes[0].DataPackage = newPkgs;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            var ex = Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
            Verify.IsNotNull(ex.InnerException);
            Verify.IsTrue(ex.InnerException is FabricImageBuilderValidationException);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestWithDuplicateConfigPackage()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            Verify.IsTrue(buildLayoutInfo.ServiceManifestTypes.Count > 0);
            Verify.IsTrue(buildLayoutInfo.ServiceManifestTypes[0].ConfigPackage.Length > 0);

            // Add another config package with the same name and version as an existing one
            var pkgs = buildLayoutInfo.ServiceManifestTypes[0].ConfigPackage;
            var newConfigPkg = new ConfigPackageType() { Name = pkgs[0].Name, Version = pkgs[0].Version };

            var newPkgs = new ConfigPackageType[pkgs.Length + 1];
            for (int i = 0; i < pkgs.Length; i++)
            {
                newPkgs[i] = pkgs[i];
            }

            newPkgs[newPkgs.Length - 1] = newConfigPkg;

            buildLayoutInfo.ServiceManifestTypes[0].ConfigPackage = newPkgs;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            var ex = Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
            Verify.IsNotNull(ex.InnerException);
            Verify.IsTrue(ex.InnerException is FabricImageBuilderValidationException);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestWithDuplicateCodePackage()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            Verify.IsTrue(buildLayoutInfo.ServiceManifestTypes.Count > 0);
            Verify.IsTrue(buildLayoutInfo.ServiceManifestTypes[0].CodePackage.Length > 0);

            // Add another config package with the same name as an existing one and an invalid version
            var pkgs = buildLayoutInfo.ServiceManifestTypes[0].CodePackage;
            var newCodePkg = new CodePackageType() { Name = pkgs[0].Name, Version = "Invalid" };

            var newPkgs = new CodePackageType[pkgs.Length + 1];
            for (int i = 0; i < pkgs.Length; i++)
            {
                newPkgs[i] = pkgs[i];
            }

            newPkgs[newPkgs.Length - 1] = newCodePkg;

            buildLayoutInfo.ServiceManifestTypes[0].CodePackage = newPkgs;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            var ex = Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
            Verify.IsNotNull(ex.InnerException);
            Verify.IsTrue(ex.InnerException is FabricImageBuilderValidationException);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestRelativeApplicationTypeName()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ApplicationTypeName = "..\\MasterApplicationType";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDuplicateServiceManifestImport()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport = new ApplicationManifestTypeServiceManifestImport[]
            {
                buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0],
                buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0]
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDuplicateServiceType()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ServiceManifestType additionalServiceManifest = new ServiceManifestType();
            additionalServiceManifest.Name = "AdditionalService";
            additionalServiceManifest.Version = "1.0";
            additionalServiceManifest.CodePackage = buildLayoutInfo.ServiceManifestTypes[0].CodePackage;
            ((UnmanagedDllType)((DllHostEntryPointType)additionalServiceManifest.CodePackage[0].EntryPoint.Item).Items[0]).Value = "System.Fabric.Management.AdditionalService.dll";
            additionalServiceManifest.ServiceTypes = buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes;

            buildLayoutInfo.ServiceManifestTypes.Add(additionalServiceManifest);

            var additionalServiceManifestImport = new ApplicationManifestTypeServiceManifestImport();
            additionalServiceManifestImport.ServiceManifestRef = new ServiceManifestRefType()
            {
                ServiceManifestName = additionalServiceManifest.Name,
                ServiceManifestVersion = additionalServiceManifest.Version
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport = new ApplicationManifestTypeServiceManifestImport[] { buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0], additionalServiceManifestImport };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDuplicateSettingNameInSettings()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            SettingsTypeSectionParameter setting = new SettingsTypeSectionParameter() { Name = "port", Value = "500" };
            SettingsTypeSectionParameter duplicatSetting = new SettingsTypeSectionParameter() { Name = "port", Value = "600" };

            buildLayoutInfo.SettingsType.Section[0].Parameter = new SettingsTypeSectionParameter[] { setting, duplicatSetting };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDuplicateSectionNameInSettings()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            SettingsTypeSection section = new SettingsTypeSection()
            {
                Name = "UserSection",
                Parameter = new SettingsTypeSectionParameter[] { new SettingsTypeSectionParameter() { Name = "port", Value = "500" } }
            };

            SettingsTypeSection duplicateSection = new SettingsTypeSection()
            {
                Name = "UserSection",
                Parameter = new SettingsTypeSectionParameter[] { new SettingsTypeSectionParameter() { Name = "maxConnections", Value = "10" } }
            };

            buildLayoutInfo.SettingsType.Section = new SettingsTypeSection[] { section, duplicateSection };
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDuplicateDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => ((DefaultServicesTypeService)i))
                .First();

            buildLayoutInfo.ApplicationManifestType.DefaultServices = new DefaultServicesType
            {
                Items = new object[]
                {
                    defaultService, 
                    defaultService
                }
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidServiceManifestImport()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestName = "Invalid";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidServiceTypeNameReferenceInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => ((DefaultServicesTypeService)i))
                .First();

            defaultService.Item.ServiceTypeName = "InvalidServiceTypeName";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDuplicateNamedPartitionsInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ServiceTypeNamedPartition namedPartition = new ServiceTypeNamedPartition();
            namedPartition.Partition = new ServiceTypeNamedPartitionPartition[2];
            namedPartition.Partition[0] = new ServiceTypeNamedPartitionPartition();
            namedPartition.Partition[0].Name = "Test";
            namedPartition.Partition[1] = new ServiceTypeNamedPartitionPartition();
            namedPartition.Partition[1].Name = "Test";

            StatefulServiceType service = new StatefulServiceType();
            service.ServiceTypeName = "WcfPerformanceStoreService";
            service.MinReplicaSetSize = "1";
            service.TargetReplicaSetSize = "3";
            service.NamedPartition = namedPartition;

            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            defaultService.Item = service;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestParameterizedNamedPartitionsInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            string parameterName = "FirstPartitionName";
            string parameterValue = "MyParameterizedValue";
            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add(parameterName, parameterValue);
            userParameters.Add("portvalue", "500");
            userParameters.Add("DomainGroup", "DomainGroup");
            userParameters.Add("InstanceCountParam", "100");

            ServiceTypeNamedPartition namedPartition = new ServiceTypeNamedPartition();
            namedPartition.Partition = new ServiceTypeNamedPartitionPartition[1];
            namedPartition.Partition[0] = new ServiceTypeNamedPartitionPartition();
            namedPartition.Partition[0].Name = "[" + parameterName + "]";

            StatefulServiceType service = new StatefulServiceType();
            service.ServiceTypeName = "WcfPerformanceStoreService";
            service.MinReplicaSetSize = "1";
            service.TargetReplicaSetSize = "3";
            service.NamedPartition = namedPartition;

            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            Verify.AreNotEqual(defaultService.Name, "XYZ");

            defaultService.Item = service;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            var reqServices = applicationInstance.DefaultServices;
            Verify.IsTrue(reqServices != null && reqServices.Items.Length >= 1, "Default services should have at least one service");
            var partitionNames = ((ServiceTypeNamedPartition)((StatefulServiceType)reqServices.Items.Select(i => (DefaultServicesTypeService)i).First().Item).NamedPartition).Partition;
            Verify.IsTrue(partitionNames != null && partitionNames.Length == 1, "There should be one named partition");
            Verify.AreEqual(partitionNames[0].Name, parameterValue, "The parameterized named partition has the correct value");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidServicePackageActivationModeInDefaultService()
        {
            var buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            defaultService.ServicePackageActivationMode = "InvalidServicePackageActivationMode";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            userParameters.Add("InstanceCountParam", "100");

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServicePackageActivationModeParameterizedValueInDefaultService()
        {
            var parameterName = "ServicePackageActivationModeParameter";
            var parameterDefaultValue = "SharedProcess";
            var parameterValue = "ExclusiveProcess";

            var buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            this.AddParam(buildLayoutInfo, parameterName, parameterDefaultValue);

            var userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add(parameterName, parameterValue);

            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            Verify.AreNotEqual(defaultService.ServicePackageActivationMode, "ExclusiveProcess");

            defaultService.ServicePackageActivationMode= "[" + parameterName + "]";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TimeSpan.MaxValue);

            var applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            var updatedDefaultService = applicationInstance.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService && ((DefaultServicesTypeService)i).Name == defaultService.Name)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            Verify.IsTrue(
                updatedDefaultService.ServicePackageActivationMode == "ExclusiveProcess", 
                "ServicePackageActivationMode should be set to ExclusiveProcess");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServicePackageActivationModeParameterizedInvalidValueInDefaultService()
        {
            var parameterName = "ServicePackageActivationModeParameter";
            var parameterDefaultValue = "SharedProcess";
            var parameterValue = "InvalidServicePackageActivationMode";
            
            var buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            this.AddParam(buildLayoutInfo, parameterName, parameterDefaultValue);

            var userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add(parameterName, parameterValue);

            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            defaultService.ServicePackageActivationMode = "[" + parameterName + "]";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            userParameters.Add("InstanceCountParam", "100");

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEmptyNamedPartitionsInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ServiceTypeNamedPartition namedPartition = new ServiceTypeNamedPartition();
            namedPartition.Partition = new ServiceTypeNamedPartitionPartition[1];
            namedPartition.Partition[0] = new ServiceTypeNamedPartitionPartition();
            namedPartition.Partition[0].Name = "";

            StatefulServiceType service = new StatefulServiceType();
            service.ServiceTypeName = "WcfPerformanceStoreService";
            service.MinReplicaSetSize = "1";
            service.TargetReplicaSetSize = "3";
            service.NamedPartition = namedPartition;

            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            defaultService.Item = service;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue),
                "Service type named partition must not have an empty name");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceTypeNamedPartitionWithNoPartitionInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ServiceTypeNamedPartition namedPartition = new ServiceTypeNamedPartition();

            StatefulServiceType service = new StatefulServiceType();
            service.ServiceTypeName = "WcfPerformanceStoreService";
            service.MinReplicaSetSize = "1";
            service.TargetReplicaSetSize = "3";
            service.NamedPartition = namedPartition;

            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            defaultService.Item = service;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue),
                "Service type named partition must contain at least a partition");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestManagedAssemblyEntryPoint()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EntryPoint.Item =
                new DllHostEntryPointType() { Items = new object[] { new ManagedAssemblyType() { Value = "System.Fabric.Management.CoreServices, Version=1.0.0.0, Culture=en-US, PublicKeyToken=null" } } };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidManagedAssemblyEntryPoint()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EntryPoint.Item = new DllHostEntryPointType() { Items = new object[] { new ManagedAssemblyType() { Value = "System.Fabric.Management.InvalidCoreServices, Version=1.0.0.0, Culture=en-US, PublicKeyToken=null" } } };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidCodePackageVersion()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Version = "invalidVersion ";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidSetupEntryPoint()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].SetupEntryPoint = new CodePackageTypeSetupEntryPoint()
            {
                ExeHost = new ExeHostEntryPointType()
                {
                    Program = "InvalidSetup.exe",
                    Arguments = null,
                    WorkingFolder = ExeHostEntryPointTypeWorkingFolder.Work
                }
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidCodePackageRefInEndpointResource()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[0].CodePackageRef = "InvalidCodePackageName";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidServiceTypeNameReferenceInServiceTemplate()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceTemplates[0].ServiceTypeName = "InvalidServiceTypeName";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidConfigOverridesName()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ConfigOverrides[0].Name = "InvalidConfigOverridesName";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidConfigOverridesSectionName()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ConfigOverrides[0].Settings[0].Name = "InvalidConfigOverridesSectionName";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidConfigOverridesSettingName()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ConfigOverrides[0].Settings[0].Parameter[0].Name = "InvalidConfigOverridesSettingName";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestConfigPackageWithNoSettingsFile()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.SettingsType = null;
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ConfigOverrides = null;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestNoEnvironmentProperties()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.Policies = null;
            buildLayoutInfo.ApplicationManifestType.Principals = null;

            foreach (ApplicationManifestTypeServiceManifestImport serviceImport in buildLayoutInfo.ApplicationManifestType.ServiceManifestImport)
            {
                serviceImport.Policies = null;
            }

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TimeSpan.MaxValue);

            TestUtility.VerifyApplicationDigestLayout(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEmptyParameterValue()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ConfigOverrides[0].Settings[0].Parameter[0].Value = string.Empty;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TimeSpan.MaxValue);

            TestUtility.VerifyApplicationDigestLayout(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidServicePolicies1()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            foreach (ApplicationManifestTypeServiceManifestImport serviceImport in buildLayoutInfo.ApplicationManifestType.ServiceManifestImport)
            {
                foreach (object servicePolicy in serviceImport.Policies)
                {
                    if (servicePolicy.GetType() == typeof(RunAsPolicyType))
                    {
                        ((RunAsPolicyType)servicePolicy).CodePackageRef = "InvalidCodePackageRef";
                    }
                }

            }

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidServicePolicies2()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            foreach (ApplicationManifestTypeServiceManifestImport serviceImport in buildLayoutInfo.ApplicationManifestType.ServiceManifestImport)
            {
                foreach (object servicePolicy in serviceImport.Policies)
                {
                    if (servicePolicy.GetType() == typeof(SecurityAccessPolicyType))
                    {
                        ((SecurityAccessPolicyType)servicePolicy).ResourceRef = "InvalidResourceRef";
                    }
                }

            }

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidServicePolicies3()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            foreach (ApplicationManifestTypeServiceManifestImport serviceImport in buildLayoutInfo.ApplicationManifestType.ServiceManifestImport)
            {
                foreach (object servicePolicy in serviceImport.Policies)
                {
                    if (servicePolicy.GetType() == typeof(SecurityAccessPolicyType))
                    {
                        ((SecurityAccessPolicyType)servicePolicy).PrincipalRef = "InvalidPrincipalRef";
                    }
                }

            }

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUseSFReplicatedStoreUsage()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            
            foreach (ApplicationManifestTypeServiceManifestImport serviceImport in buildLayoutInfo.ApplicationManifestType.ServiceManifestImport)
            {
                // Loop through all the policies in this service to locate the ServiceFabricRuntimeAccessPolicy.
                foreach (object servicePolicy in serviceImport.Policies)
                {
                    if (servicePolicy.GetType() == typeof(ServiceFabricRuntimeAccessPolicyType))
                    {
                        // We found the policy, so update it.
                        ((ServiceFabricRuntimeAccessPolicyType)servicePolicy).UseServiceFabricReplicatedStore = true;

                        // Set the flag that the policy object is already present
                        AppTrace.TraceSource.WriteInfo("ImageBuilderTest", "TestUseSFReplicatedStoreUsage - Found and updated ServiceFabricRuntimeAccessPolicy.");
                    }
                }
            }
            
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            bool initialIsSFVolumeDiskServiceEnabled = this.imageBuilder.IsSFVolumeDiskServiceEnabled;

            try
            {
                this.imageBuilder.IsSFVolumeDiskServiceEnabled = true;

                // Since Imagebuilder has been configured to indicate that SFVolumeDisk is enabled on the cluster,
                // we will expect ApplicationType to be successfully built.
                this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);
                AppTrace.TraceSource.WriteInfo("ImageBuilderTest", "TestUseSFReplicatedStoreUsage - Successfully validated the positive case.");

                // And for the negative case, we expect to get an exception since SFVolumeDisk is disabled on the cluster
                // via the configuration change below.
                this.imageBuilder.IsSFVolumeDiskServiceEnabled = false;
                
                Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
                AppTrace.TraceSource.WriteInfo("ImageBuilderTest", "TestUseSFReplicatedStoreUsage - Successfully validated the negative case.");
            }
            finally
            {
                // reset the original state
                this.imageBuilder.IsSFVolumeDiskServiceEnabled = initialIsSFVolumeDiskServiceEnabled;
                AppTrace.TraceSource.WriteInfo("ImageBuilderTest", "TestUseSFReplicatedStoreUsage - Exiting validations.");
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDuplicateSecurityAccessPolicy()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[2];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] =
                new SecurityAccessPolicyType() { ResourceRef = "EndpointResource", PrincipalRef = "DefaultUser", GrantRights = SecurityAccessPolicyTypeGrantRights.Read };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[1] =
                new SecurityAccessPolicyType() { ResourceRef = "EndpointResource", PrincipalRef = "PrivilegedUser", GrantRights = SecurityAccessPolicyTypeGrantRights.Read };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEndpointBindingPolicy()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints = new EndpointType[3];

            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[0] = new EndpointType
            {
                Name = "SecureEndpointResource",
                Protocol = EndpointTypeProtocol.https
            };
            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[1] = new EndpointType
            {
                Name = "EndpointResource",
                Protocol = EndpointTypeProtocol.http
            };
            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[2] = new EndpointType
            {
                Name = "EndpointResource1",
                Protocol = EndpointTypeProtocol.tcp
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[2];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] =
                new EndpointBindingPolicyType() {EndpointRef = "SecureEndpointResource", CertificateRef = "TestCert1"};

            buildLayoutInfo.ApplicationManifestType.Certificates = new ApplicationManifestTypeCertificates()
            {
                EndpointCertificate = new EndpointCertificateType[1]
            };
            buildLayoutInfo.ApplicationManifestType.Certificates.EndpointCertificate[0] = new EndpointCertificateType()
            {
                Name = "TestCert1",
                X509FindValue = "2d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TestUtility.ImageStoreDefaultTimeout);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            Verify.IsTrue(servicepackage.DigestedResources.DigestedCertificates.Length == 1,
                "Expected 1 DigestedCertificate in resources");
            foreach (var endpoint in servicepackage.DigestedResources.DigestedEndpoints)
            {
                if (endpoint.Endpoint.Name == "SecureEndpointResource")
                {
                    Verify.IsTrue(endpoint.EndpointBindingPolicy != null &&
                                  endpoint.EndpointBindingPolicy.CertificateRef == "TestCert1",
                        "Expected EndpointBindingPolicy associated with EndpointResource");
                }
                else
                {
                    Verify.IsTrue(endpoint.EndpointBindingPolicy == null ||
                                  string.IsNullOrEmpty(endpoint.EndpointBindingPolicy.CertificateRef) ,
                        "Expected no EndpointBindingPolicy associated with EndpointResource");
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEndpointBindingPolicyInvalidCertRef()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[2];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] =
                new EndpointBindingPolicyType() { EndpointRef = "EndpointResource", CertificateRef = "TestCertInvalid" };

            buildLayoutInfo.ApplicationManifestType.Certificates = new ApplicationManifestTypeCertificates()
            {
                EndpointCertificate = new EndpointCertificateType[1]
            };
            buildLayoutInfo.ApplicationManifestType.Certificates.EndpointCertificate[0] = new EndpointCertificateType()
            {
                Name = "TestCert1",
                X509FindValue = "2d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEndpointBindingPolicyNoCert()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[2];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] =
                new EndpointBindingPolicyType() { EndpointRef = "EndpointResource", CertificateRef = "TestCertInvalid" };

            buildLayoutInfo.ApplicationManifestType.Certificates = new ApplicationManifestTypeCertificates();
           
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEndpointBindingPolicyWithAppParameters()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints = new EndpointType[3];

            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[0] = new EndpointType
            {
                Name = "SecureEndpointResource",
                Protocol = EndpointTypeProtocol.https
            };
            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[1] = new EndpointType
            {
                Name = "SecureEndpointResource1",
                Protocol = EndpointTypeProtocol.https
            };
            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[2] = new EndpointType
            {
                Name = "SecureEndpointResource2",
                Protocol = EndpointTypeProtocol.https
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[3];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] =
                new EndpointBindingPolicyType() { EndpointRef = "SecureEndpointResource", CertificateRef = "TestCert1" };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[1] =
                new EndpointBindingPolicyType() { EndpointRef = "[EndpointResource]", CertificateRef = "TestCert2" };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[2] =
                new EndpointBindingPolicyType() { EndpointRef = "[EndpointResource1]", CertificateRef = "TestCert3" };

            buildLayoutInfo.ApplicationManifestType.Certificates = new ApplicationManifestTypeCertificates()
            {
                SecretsCertificate = new FabricCertificateType[1],
                EndpointCertificate = new EndpointCertificateType[3]
            };
            buildLayoutInfo.ApplicationManifestType.Certificates.EndpointCertificate[0] = new EndpointCertificateType()
            {
                Name = "TestCert1",
                X509FindValue = "2d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"
            };
            buildLayoutInfo.ApplicationManifestType.Certificates.EndpointCertificate[1] = new EndpointCertificateType()
            {
                Name = "TestCert2",
                X509FindValue = "3d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"
            };
            buildLayoutInfo.ApplicationManifestType.Certificates.EndpointCertificate[2] = new EndpointCertificateType()
            {
                Name = "TestCert3",
                X509FindValue = "4d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"
            };
            buildLayoutInfo.ApplicationManifestType.Certificates.SecretsCertificate[0] = new FabricCertificateType()
            {
                Name = "SecretCert",
                X509FindType = FabricCertificateTypeX509FindType.FindByThumbprint,
                X509FindValue = "4d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"
            };
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");
            userParameters.Add("EndpointResource", "SecureEndpointResource1");
            userParameters.Add("EndpointResource1", "SecureEndpointResource2");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TestUtility.ImageStoreDefaultTimeout);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            foreach (var endpoint in servicepackage.DigestedResources.DigestedEndpoints)
            {
                if (endpoint.Endpoint.Name == "SecureEndpointResource")
                {
                    Verify.IsTrue(endpoint.EndpointBindingPolicy != null &&
                                  endpoint.EndpointBindingPolicy.CertificateRef == "TestCert1",
                        "Expected EndpointBindingPolicy associated with EndpointResource");
                }
                else if (endpoint.Endpoint.Name == "SecureEndpointResource1")
                {
                    Verify.IsTrue(endpoint.EndpointBindingPolicy != null &&
                                  endpoint.EndpointBindingPolicy.CertificateRef == "TestCert2",
                        "Expected EndpointBindingPolicy associated with EndpointResource");
                }
                else if (endpoint.Endpoint.Name == "SecureEndpointResource2")
                {
                    Verify.IsTrue(endpoint.EndpointBindingPolicy != null &&
                                  endpoint.EndpointBindingPolicy.CertificateRef == "TestCert3",
                        "Expected EndpointBindingPolicy associated with EndpointResource");
                }
                else
                {
                    Verify.IsTrue(endpoint.EndpointBindingPolicy == null ||
                                  string.IsNullOrEmpty(endpoint.EndpointBindingPolicy.CertificateRef),
                        "Expected no EndpointBindingPolicy associated with EndpointResource");
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestContainerHostPolicy()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            AddParam(buildLayoutInfo, "ConfigPackageName", "ConfigPackage");
            AddParam(buildLayoutInfo, "ConfigPackageVersion", "X.X");

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ContainerHostPoliciesType containerHostPolicy =  new ContainerHostPoliciesType { CodePackageRef = codePackageName, UseDefaultRepositoryCredentials = "true" };

            containerHostPolicy.Items = new object[4];
            containerHostPolicy.Items[0] = new PortBindingType { ContainerPort = 8905, EndpointRef = buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[0].Name };
            containerHostPolicy.Items[1] = new RepositoryCredentialsType 
                {
                    AccountName = "[MyUserParam]",
                    // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Not a real secret")]
                    Password = "[RepoPassword]",
                    Email = "test@test.com",
                    PasswordEncrypted = true,
                    PasswordEncryptedSpecified = true
                };

            var logConfig = new ContainerLoggingDriverType { Driver = "etwlogs", Items = new DriverOptionType[2] };
            logConfig.Items[0] = new DriverOptionType { Name = "[DriverOptionName1]", Value = "[DriverOptionValue1]" };
            logConfig.Items[1] = new DriverOptionType { Name = "Name2", Value = "Value2" };
            containerHostPolicy.Items[2] = logConfig;

            var volConfig = new ContainerVolumeType { Source = "[Source]", Destination = "[Destination]", Driver = "[VolumeDriver]", IsReadOnly = true, Items = new DriverOptionType[2] };
            volConfig.Items[0] = new DriverOptionType { Name = "[DriverOptionName1]", Value = "[DriverOptionValue1]" };
            volConfig.Items[1] = new DriverOptionType { Name = "Name2", Value = "Value2" };
            containerHostPolicy.Items[3] = volConfig;

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");
            userParameters.Add("RepoPassword", "password1");
            userParameters.Add("Destination", @"c:\destination");
            userParameters.Add("Source", @"c:\source");
            userParameters.Add("MyUserParam", "testuser");
            userParameters.Add("DriverOptionName1", "Name1");
            userParameters.Add("DriverOptionValue1", "Value1");
            userParameters.Add("VolumeDriver", "azurefile");
            userParameters.Add("ConfigPackageName", "UserServiceHost.Config");
            userParameters.Add("ConfigPackageVersion", "1.0");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TestUtility.ImageStoreDefaultTimeout);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            foreach(var codePackage in servicepackage.DigestedCodePackage)
            {
                if(codePackageName.Equals(codePackage.CodePackage.Name))
                {
                    ContainerHostPoliciesType containerHostPolices = codePackage.ContainerHostPolicies;

                    Verify.IsTrue(containerHostPolices != null &&
                                containerHostPolices.Items != null,
                      String.Format( "{0} {1}", "Expected containerhostpolicy with valid items for the code package ", codePackageName));

                    Verify.IsTrue(!String.IsNullOrEmpty(containerHostPolices.Isolation) &&
                        containerHostPolices.Isolation.Equals("process"),
                        String.Format("Expected Isolation {0} got {1} codepackageref {2}", "process", containerHostPolices.Isolation, codePackageName));

                    Verify.IsTrue(!String.IsNullOrEmpty(containerHostPolices.UseDefaultRepositoryCredentials) &&
                        containerHostPolices.UseDefaultRepositoryCredentials.Equals("true"),
                        String.Format("Expected TRUE for usedefaultrepositorycredentis, instead got {0}", containerHostPolices.UseDefaultRepositoryCredentials));

                    foreach (var item in containerHostPolices.Items)
                    {
                        if (item.GetType().Equals(typeof(RepositoryCredentialsType)))
                        {
                            var repositoryCredentials = item as RepositoryCredentialsType;
                            Verify.IsTrue(repositoryCredentials.AccountName == "testuser" &&
                             repositoryCredentials.Password == "password1" &&
                             repositoryCredentials.Email == "test@test.com" &&
                             repositoryCredentials.PasswordEncrypted == true,
                             String.Format("Unexpected RepositoryCredentials {0} {1} {2} {3}",
                             repositoryCredentials.AccountName,
                             repositoryCredentials.Password,
                             repositoryCredentials.Email,
                             repositoryCredentials.PasswordEncrypted));
                        }
                        else if (item.GetType().Equals(typeof(ContainerLoggingDriverType)))
                        {
                            logConfig = item as ContainerLoggingDriverType;
                            Verify.IsTrue(logConfig.Driver == "etwlogs",
                                String.Format("Unexpected Logging Config {0}",
                                logConfig.Driver));
                            foreach (DriverOptionType option in logConfig.Items)
                            {
                                if (option.Name == "Name1")
                                {
                                    Verify.IsTrue(option.Value == "Value1",
                                        String.Format("Unexpected driver option for Name {0}, value {1}",
                                        option.Name,
                                        option.Value));
                                }
                                else if (option.Name == "Name2")
                                {
                                    Verify.IsTrue(option.Value == "Value2",
                                        String.Format("Unexpected driver option for Name {0}, value {1}",
                                        option.Name,
                                        option.Value));
                                }
                            }
                        }
                        else if (item.GetType().Equals(typeof(ContainerVolumeType)))
                        {
                            var volume = item as ContainerVolumeType;
                            Verify.IsTrue(volume.Destination == @"c:\destination" &&
                                volume.Source == @"c:\source" &&
                                volume.Driver == "azurefile",
                                String.Format("Unexpected volume Config source {0}, destination {1}",
                                volume.Source,
                                volume.Destination));
                            foreach (DriverOptionType option in volume.Items)
                            {
                                if (option.Name == "Name1")
                                {
                                    Verify.IsTrue(option.Value == "Value1",
                                        String.Format("Unexpected driver option for Name {0}, value {1}",
                                        option.Name,
                                        option.Value));
                                }
                                else if (option.Name == "Name2")
                                {
                                    Verify.IsTrue(option.Value == "Value2",
                                        String.Format("Unexpected driver option for Name {0}, value {1}",
                                        option.Name,
                                        option.Value));
                                }
                            }
                        }
                        else if (item.GetType().Equals(typeof(PortBindingType)))
                        {
                            var portBinding = item as PortBindingType;
                            Verify.IsTrue(portBinding.ContainerPort == 8905,
                                String.Format("Unexpected portbinding {0}, endpointref {1}",
                                portBinding.ContainerPort,
                                portBinding.EndpointRef));
                        }
                    }
                }
            }
           
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestContainerHostIsolationPolicy()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ContainerHostPoliciesType containerHostPolicy = new ContainerHostPoliciesType {
                CodePackageRef = codePackageName,
                Isolation = "[MyUserParam]",
                ContainersRetentionCount ="[ContainerRetentionCount]" };

            containerHostPolicy.Items = new object[4];
            containerHostPolicy.Items[0] = new PortBindingType { ContainerPort = 8905, EndpointRef = buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[0].Name };
            containerHostPolicy.Items[1] = new RepositoryCredentialsType
            {
                AccountName = "[MyUserParam]",
                // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Not a real secret")]
                Password = "[RepoPassword]",
                Email = "test@test.com",
                PasswordEncrypted = true,
                PasswordEncryptedSpecified = true
            };

            var logConfig = new ContainerLoggingDriverType { Driver = "etwlogs", Items = new DriverOptionType[2] };
            logConfig.Items[0] = new DriverOptionType { Name = "[DriverOptionName1]", Value = "[DriverOptionValue1]" };
            logConfig.Items[1] = new DriverOptionType { Name = "Name2", Value = "Value2" };
            containerHostPolicy.Items[2] = logConfig;

            var volConfig = new ContainerVolumeType { Source = "[Source]", Destination = "[Destination]", Driver = "[VolumeDriver]", IsReadOnly = true, Items = new DriverOptionType[2] };
            volConfig.Items[0] = new DriverOptionType { Name = "[DriverOptionName1]", Value = "[DriverOptionValue1]" };
            volConfig.Items[1] = new DriverOptionType { Name = "Name2", Value = "Value2" };
            containerHostPolicy.Items[3] = volConfig;

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");
            userParameters.Add("RepoPassword", "password1");
            userParameters.Add("Destination", @"c:\destination");
            userParameters.Add("Source", @"c:\source");
            userParameters.Add("MyUserParam", "hyperv");
            userParameters.Add("DriverOptionName1", "Name1");
            userParameters.Add("DriverOptionValue1", "Value1");
            userParameters.Add("VolumeDriver", "azurefile");
            userParameters.Add("ContainerRetentionCount", "5");
            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TestUtility.ImageStoreDefaultTimeout);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            foreach (var codePackage in servicepackage.DigestedCodePackage)
            {
                if (codePackageName.Equals(codePackage.CodePackage.Name))
                {
                    ContainerHostPoliciesType containerHostPolices = codePackage.ContainerHostPolicies;
                    Verify.IsTrue(containerHostPolices != null &&
                                containerHostPolices.Items != null,
                      String.Format("{0} {1}", "Expected containerhostpolicy with valid items for the code package ", codePackageName));
                    Verify.IsTrue(!String.IsNullOrEmpty(containerHostPolices.Isolation) &&
                        containerHostPolices.Isolation.Equals("hyperv"),
                        String.Format("Expected Isolation {0} got {1} codepackageref {2}", "hyperv", containerHostPolices.Isolation, codePackageName));
                    Verify.IsTrue(!String.IsNullOrEmpty(containerHostPolices.ContainersRetentionCount) &&
                        containerHostPolices.ContainersRetentionCount.Equals("5"),
                        String.Format("Expected ContainerRetentionCount {0} got {1} codepackageref {2}", "5", containerHostPolices.ContainersRetentionCount, codePackageName));
                    foreach (var item in containerHostPolices.Items)
                    {
                        if (item.GetType().Equals(typeof(ContainerLoggingDriverType)))
                        {
                            logConfig = item as ContainerLoggingDriverType;
                            Verify.IsTrue(logConfig.Driver == "etwlogs",
                                String.Format("Unexpected Logging Config {0}",
                                logConfig.Driver));
                            foreach (DriverOptionType option in logConfig.Items)
                            {
                                if (option.Name == "Name1")
                                {
                                    Verify.IsTrue(option.Value == "Value1",
                                        String.Format("Unexpected driver option for Name {0}, value {1}",
                                        option.Name,
                                        option.Value));
                                }
                                else if (option.Name == "Name2")
                                {
                                    Verify.IsTrue(option.Value == "Value2",
                                        String.Format("Unexpected driver option for Name {0}, value {1}",
                                        option.Name,
                                        option.Value));
                                }
                            }
                        }
                        else if (item.GetType().Equals(typeof(ContainerVolumeType)))
                        {
                            var volume = item as ContainerVolumeType;
                            Verify.IsTrue(volume.Destination == @"c:\destination" &&
                                volume.Source == @"c:\source" &&
                                volume.Driver == "azurefile",
                                String.Format("Unexpected volume Config source {0}, destination {1}",
                                volume.Source,
                                volume.Destination));
                            foreach (DriverOptionType option in volume.Items)
                            {
                                if (option.Name == "Name1")
                                {
                                    Verify.IsTrue(option.Value == "Value1",
                                        String.Format("Unexpected driver option for Name {0}, value {1}",
                                        option.Name,
                                        option.Value));
                                }
                                else if (option.Name == "Name2")
                                {
                                    Verify.IsTrue(option.Value == "Value2",
                                        String.Format("Unexpected driver option for Name {0}, value {1}",
                                        option.Name,
                                        option.Value));
                                }
                            }
                        }
                        else if (item.GetType().Equals(typeof(PortBindingType)))
                        {
                            var portBinding = item as PortBindingType;
                            Verify.IsTrue(portBinding.ContainerPort == 8905,
                                String.Format("Unexpected portbinding {0}, endpointref {1}",
                                portBinding.ContainerPort,
                                portBinding.EndpointRef));
                        }
                    }
                }
            }

        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestContainerHostInvalidIsolationPolicy()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ContainerHostPoliciesType containerHostPolicy = new ContainerHostPoliciesType { CodePackageRef = codePackageName, Isolation = "[MyUserParam]" };

            containerHostPolicy.Items = new object[4];
            containerHostPolicy.Items[0] = new PortBindingType { ContainerPort = 8905, EndpointRef = buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[0].Name };
            containerHostPolicy.Items[1] = new RepositoryCredentialsType
            {
                AccountName = "[MyUserParam]",
                // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Not a real secret")]
                Password = "[RepoPassword]",
                Email = "test@test.com",
                PasswordEncrypted = true,
                PasswordEncryptedSpecified = true
            };

            var logConfig = new ContainerLoggingDriverType { Driver = "etwlogs", Items = new DriverOptionType[2] };
            logConfig.Items[0] = new DriverOptionType { Name = "[DriverOptionName1]", Value = "[DriverOptionValue1]" };
            logConfig.Items[1] = new DriverOptionType { Name = "Name2", Value = "Value2" };
            containerHostPolicy.Items[2] = logConfig;

            var volConfig = new ContainerVolumeType { Source = "[Source]", Destination = "[Destination]", Driver = "[VolumeDriver]", IsReadOnly = true, Items = new DriverOptionType[2] };
            volConfig.Items[0] = new DriverOptionType { Name = "[DriverOptionName1]", Value = "[DriverOptionValue1]" };
            volConfig.Items[1] = new DriverOptionType { Name = "Name2", Value = "Value2" };
            containerHostPolicy.Items[3] = volConfig;

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");
            userParameters.Add("RepoPassword", "password1");
            userParameters.Add("Destination", @"c:\destination");
            userParameters.Add("Source", @"c:\source");
            userParameters.Add("MyUserParam", "bogus");
            userParameters.Add("DriverOptionName1", "Name1");
            userParameters.Add("DriverOptionValue1", "Value1");
            userParameters.Add("VolumeDriver", "azurefile");

           Verify.Throws<AggregateException>(
                () =>  this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TestUtility.ImageStoreDefaultTimeout));        
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestContainerHostCertificateRefPolicy()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ContainerHostPoliciesType containerHostPolicy = new ContainerHostPoliciesType { CodePackageRef = codePackageName};

            containerHostPolicy.Items = new object[2];

            containerHostPolicy.Items[0] = new ContainerCertificateType
            {
                Name = "[ContainerCertificateRef]",
                X509FindValue = "4d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13",
                X509StoreName = "My"
            };

            containerHostPolicy.Items[1] = new ContainerCertificateType
            {
                Name = "cert2",
                X509FindValue = "3d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13",
                X509StoreName = "My"
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("ContainerCertificateRef", "cert1");
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TestUtility.ImageStoreDefaultTimeout);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            foreach (var codePackage in servicepackage.DigestedCodePackage)
            {
                if (codePackageName.Equals(codePackage.CodePackage.Name))
                {
                    ContainerHostPoliciesType containerHostPolices = codePackage.ContainerHostPolicies;
                    Verify.IsTrue(containerHostPolices != null &&
                                containerHostPolices.Items != null,
                      String.Format("{0} {1}", "Expected containerhostpolicy for the code package ", codePackageName));

                    Verify.IsTrue(!String.IsNullOrEmpty(containerHostPolices.Isolation) &&
                        containerHostPolices.Isolation.Equals("process"),
                        String.Format("Expected Isolation {0} got {1} codepackageref {2}", "process", containerHostPolices.Isolation, codePackageName));

                    foreach (var item in containerHostPolices.Items)
                    {
                        if (item.GetType().Equals(typeof(ContainerCertificateType)))
                        {
                            var certificate = item as ContainerCertificateType;
                            if (certificate.Name.Equals("cert1"))
                            {
                                Verify.IsTrue(certificate.X509StoreName.Equals("My"),
                                String.Format("Unexpected certificate.X509StoreName {0}, expected {1}",
                                certificate.X509StoreName,
                                "My"));

                                Verify.IsTrue(certificate.X509FindValue.Equals("4d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"),
                                String.Format("Unexpected certificate.X509StoreName {0}, expected {1}",
                                certificate.X509FindValue,
                                "4d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"));
                            }
                            else if (certificate.Name.Equals("cert2"))
                            {
                                Verify.IsTrue(certificate.X509StoreName.Equals("My"),
                                String.Format("Unexpected certificate.X509StoreName {0}, expected {1}",
                                certificate.X509StoreName,
                                "My"));

                                Verify.IsTrue(certificate.X509FindValue.Equals("3d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"),
                                String.Format("Unexpected certificate.X509StoreName {0}, expected {1}",
                                certificate.X509FindValue,
                                "3d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"));
                            }
                            else
                            {
                                throw new Exception(string.Format("Unrecognised certificate.Name {0}", certificate.Name));
                            }                       
                        }
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServicePackageContainerHostPolicy()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ServicePackageContainerPolicyType containerHostPolicy = new ServicePackageContainerPolicyType { Hostname = "foo" };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TestUtility.ImageStoreDefaultTimeout);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            Verify.IsTrue(servicepackage.ServicePackageContainerPolicy != null && servicepackage.ServicePackageContainerPolicy.Hostname.Equals("foo"),
                               String.Format("Unexpected servicepackagecontainerpolicy, policy is null"));
            if(servicepackage.ServicePackageContainerPolicy != null &&
                servicepackage.ServicePackageContainerPolicy.Hostname != null)
            {

                Verify.IsTrue(servicepackage.ServicePackageContainerPolicy.Hostname.Equals("foo"),
                                   String.Format("Unexpected servicepackagecontainerpolicy, hostname {0}", servicepackage.ServicePackageContainerPolicy.Hostname));
            }

        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestContainerHostInvalidCertificateRefPolicy()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ContainerHostPoliciesType containerHostPolicy = new ContainerHostPoliciesType { CodePackageRef = codePackageName };

            containerHostPolicy.Items = new object[2];

            containerHostPolicy.Items[0] = new ContainerCertificateType
            {
                Name = "cert1",
                X509FindValue = "4d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13",
                X509StoreName = "My"
            };

            containerHostPolicy.Items[1] = new ContainerCertificateType
            {
                Name = "cert1",
                X509FindValue = "ad 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13",
                X509StoreName = "My"
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TestUtility.ImageStoreDefaultTimeout));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestContainerHostCertificateRefPolicyForDataPackage()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ContainerHostPoliciesType containerHostPolicy = new ContainerHostPoliciesType { CodePackageRef = codePackageName };

            containerHostPolicy.Items = new object[2];

            containerHostPolicy.Items[0] = new ContainerCertificateType
            {
                Name = "[ContainerCertificateRef]",
                DataPackageRef = "[MyUserParam]",
                DataPackageVersion = "1.0",
                RelativePath = "path\\to\\certificate"
            };

            containerHostPolicy.Items[1] = new ContainerCertificateType
            {
                Name = "cert2",
                DataPackageRef = "[MyUserParam]",
                DataPackageVersion = "1.0",
                RelativePath = "path\\to\\certificate2",
                // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Not a real secret")]
                Password = "password",
                IsPasswordEncrypted = false
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("ContainerCertificateRef", "cert1");
            userParameters.Add("MyUserParam", "XContainerHost.Data");
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TestUtility.ImageStoreDefaultTimeout);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            foreach (var codePackage in servicepackage.DigestedCodePackage)
            {
                if (codePackageName.Equals(codePackage.CodePackage.Name))
                {
                    ContainerHostPoliciesType containerHostPolices = codePackage.ContainerHostPolicies;
                    Verify.IsTrue(containerHostPolices != null &&
                                containerHostPolices.Items != null,
                      String.Format("{0} {1}", "Expected containerhostpolicy for the code package ", codePackageName));

                    Verify.IsTrue(!String.IsNullOrEmpty(containerHostPolices.Isolation) &&
                        containerHostPolices.Isolation.Equals("process"),
                        String.Format("Expected Isolation {0} got {1} codepackageref {2}", "process", containerHostPolices.Isolation, codePackageName));

                    foreach (var item in containerHostPolices.Items)
                    {
                        if (item.GetType().Equals(typeof(ContainerCertificateType)))
                        {
                            var certificate = item as ContainerCertificateType;
                            if (certificate.Name.Equals("cert1"))
                            {
                                Verify.IsTrue(certificate.DataPackageRef.Equals("XContainerHost.Data"),
                                String.Format("Unexpected certificate.DataPackageRef {0}, expected {1}",
                                certificate.DataPackageRef,
                                "XContainerHost.Data"));

                                Verify.IsTrue(certificate.DataPackageVersion.Equals("1.0"),
                                String.Format("Unexpected certificate.DataPackageRef {0}, expected {1}",
                                certificate.DataPackageVersion,
                                "1.0"));

                                Verify.IsTrue(certificate.RelativePath.Equals("path\\to\\certificate"),
                                String.Format("Unexpected certificate.RelativePath {0}, expected {1}",
                                certificate.RelativePath,
                                "path\\to\\certificate"));
                            }
                            else if (certificate.Name.Equals("cert2"))
                            {
                                Verify.IsTrue(certificate.DataPackageRef.Equals("XContainerHost.Data"),
                                String.Format("Unexpected certificate.DataPackageRef {0}, expected {1}",
                                certificate.DataPackageRef,
                                "XContainerHost.Data"));

                                Verify.IsTrue(certificate.DataPackageVersion.Equals("1.0"),
                                String.Format("Unexpected certificate.DataPackageRef {0}, expected {1}",
                                certificate.DataPackageVersion,
                                "1.0"));

                                Verify.IsTrue(certificate.RelativePath.Equals("path\\to\\certificate2"),
                                String.Format("Unexpected certificate.RelativePath {0}, expected {1}",
                                certificate.RelativePath,
                                "path\\to\\certificate2"));

                                Verify.IsTrue(certificate.Password.Equals("password"),
                                String.Format("Unexpected certificate.Password {0}, expected {1}",
                                certificate.Password,
                                "password"));

                                Verify.IsFalse(certificate.IsPasswordEncrypted,
                                String.Format("Unexpected certificate.IsPasswordEncrypted {0}, expected {1}",
                                certificate.IsPasswordEncrypted,
                                "false"));
                            }
                            else
                            {
                                throw new Exception(string.Format("Unrecognised certificate.Name {0}", certificate.Name));
                            }
                        }
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestContainerHostInvalidCertificateRefPolicyForDataPackage()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ContainerHostPoliciesType containerHostPolicy = new ContainerHostPoliciesType { CodePackageRef = codePackageName };

            containerHostPolicy.Items = new object[2];

            containerHostPolicy.Items[0] = new ContainerCertificateType
            {
                Name = "cert1",
                DataPackageRef = "XContainerHost.Data",
                DataPackageVersion = "1.0",
                RelativePath = "path\\to\\certificate",
                X509FindValue = "4d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TestUtility.ImageStoreDefaultTimeout));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestContainerHostInvalidNetworkConfig()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ContainerHostPoliciesType containerHostPolicy = new ContainerHostPoliciesType { CodePackageRef = codePackageName };

            containerHostPolicy.Items = new object[2];

            containerHostPolicy.Items[0] = new PortBindingType
            {
                ContainerPort=1234,
                EndpointRef = buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[0].Name
            };

            containerHostPolicy.Items[1] = new ContainerNetworkConfigType
            {
                NetworkType="Public"
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TestUtility.ImageStoreDefaultTimeout));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestContainerHostInvalidHealthConfig()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var codePackageName = "XContainerHost";
            
            var containerHostPolicy = new ContainerHostPoliciesType
            {
                CodePackageRef = codePackageName,
                Items = new object[1]
            };
            
            containerHostPolicy.Items[0] = new ContainerHealthConfigType
            {
                IncludeDockerHealthStatusInSystemHealthReport = false,
                RestartContainerOnUnhealthyDockerHealthStatus = true
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestContainerHostInvalidNetworkConfigOption()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ContainerHostPoliciesType containerHostPolicy = new ContainerHostPoliciesType { CodePackageRef = codePackageName };

            containerHostPolicy.Items = new object[1];

            containerHostPolicy.Items[0] = new ContainerNetworkConfigType
            {
                NetworkType = "Internal"
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TestUtility.ImageStoreDefaultTimeout));
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidApplicationPolicies()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.Policies.DefaultRunAsPolicy.UserRef = "InvalidUserRef";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidApplicationHealthPolicy()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            {
                buildLayoutInfo.ApplicationManifestType.Policies.HealthPolicy.MaxPercentUnhealthyDeployedApplications = "101";
                string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
                Verify.Throws<AggregateException>(
                    () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
                buildLayoutInfo.ApplicationManifestType.Policies.HealthPolicy.MaxPercentUnhealthyDeployedApplications = "0";
            }

            this.TestInvalidServiceTypeHealthPolicy(buildLayoutInfo, buildLayoutInfo.ApplicationManifestType.Policies.HealthPolicy.DefaultServiceTypeHealthPolicy);

            foreach (var serviceTypeHealthPolicy in buildLayoutInfo.ApplicationManifestType.Policies.HealthPolicy.ServiceTypeHealthPolicy)
            {
                this.TestInvalidServiceTypeHealthPolicy(buildLayoutInfo, serviceTypeHealthPolicy);
            }
        }

        private void TestInvalidServiceTypeHealthPolicy(BuildLayoutInfo buildLayoutInfo, ServiceTypeHealthPolicyType serviceTypeHealthPolicy)
        {
            {
                serviceTypeHealthPolicy.MaxPercentUnhealthyServices = "101";
                string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
                Verify.Throws<AggregateException>(
                    () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
                serviceTypeHealthPolicy.MaxPercentUnhealthyServices = "0";
            }

            {
                serviceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService = "101";
                string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
                Verify.Throws<AggregateException>(
                    () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
                serviceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService = "0";
            }

            {
                serviceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition = "101";
                string applicationTypeName = buildLayoutInfo.CreateBuildLayout();
                Verify.Throws<AggregateException>(
                    () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
                serviceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition = "0";
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEmptyServiceTypeName()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.DefaultServices = null;
            buildLayoutInfo.ApplicationManifestType.ServiceTemplates = null;

            ((ServiceTypeType)buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes[0]).ServiceTypeName = "";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceTypeMismatch()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes[0] = new StatefulServiceTypeType() { ServiceTypeName = "WcfAnagramService" };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidApplicationTypeName()
        {
            Verify.Throws<DirectoryNotFoundException>(
                () => this.imageBuilder.BuildApplicationType("unknownAppTypeName", TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidUniformInt64Partition()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var requiredQueueService = (DefaultServicesTypeService)buildLayoutInfo.ApplicationManifestType.DefaultServices.Items.First(
                defaultService => (defaultService is DefaultServicesTypeService) && ((DefaultServicesTypeService)defaultService).Name.Equals("RequiredQueueService"));

            requiredQueueService.Item.UniformInt64Partition.LowKey = "0";
            requiredQueueService.Item.UniformInt64Partition.HighKey = "3";
            requiredQueueService.Item.UniformInt64Partition.PartitionCount = "5";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidOverrideOfEncryptedParameter()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.SettingsType.Section[0].Parameter[2].IsEncrypted = true;
            buildLayoutInfo.SettingsType.Section[0].Parameter[2].MustOverride = true;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEncryptedParametersWithoutSecretsCertificate()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.SettingsType.Section[0].Parameter[2].IsEncrypted = true;
            buildLayoutInfo.SettingsType.Section[0].Parameter[2].MustOverride = true;

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ConfigOverrides[0].Settings[0].Parameter[0].IsEncrypted = true;

            buildLayoutInfo.ApplicationManifestType.Certificates = null;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEncryptedParametersWithoutSecretsCertificateLocalSystemUser()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.SettingsType.Section[0].Parameter[2].IsEncrypted = true;
            buildLayoutInfo.SettingsType.Section[0].Parameter[2].MustOverride = true;

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ConfigOverrides[0].Settings[0].Parameter[0].IsEncrypted = true;

            buildLayoutInfo.ApplicationManifestType.Certificates = null;

            buildLayoutInfo.ApplicationManifestType.Principals.Users = new SecurityPrincipalsTypeUser[1];
            buildLayoutInfo.ApplicationManifestType.Principals.Users[0] = new SecurityPrincipalsTypeUser
            {
                Name = "LocalSystem",
                AccountType = SecurityPrincipalsTypeUserAccountType.LocalSystem,
                LoadUserProfile = false,
                PerformInteractiveLogon = false
            };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.All
            };
            buildLayoutInfo.ApplicationManifestType.Policies.DefaultRunAsPolicy = null;
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty("Data:DiagnosticsStatus", "{DiagnosticsAbsent, DiagnosticsIncomplete}")]
        public void TestServiceWithDiagnosticsAbsentOrIncomplete()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            if (this.testContextInstance.DataRow["DiagnosticsStatus"].Equals("DiagnosticsAbsent"))
            {
                buildLayoutInfo.ServiceManifestTypes[0].Diagnostics = null;
            }
            else if (this.testContextInstance.DataRow["DiagnosticsStatus"].Equals("DiagnosticsIncomplete"))
            {
                buildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages = null;
            }
            else
            {
                Verify.Fail("Unexpected DiagnosticsStatus value");
            }

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("portvalue", "500");
            userParameters.Add("DomainGroup", "DomainGroup");
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TimeSpan.MaxValue);

            RolloutVersion expectedRolloutVersion = RolloutVersion.CreateRolloutVersion();

            TestUtility.VerifyApplicationDigestLayout(buildLayoutInfo, applicationId, 1, expectedRolloutVersion);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [DataSource("Table:System.Fabric.Management.TestData.xml#InvalidDiagnosticsElement")]
        public void TestInvalidDiagnosticsElement()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("portvalue", "500");
            userParameters.Add("DomainGroup", "DomainGroup");
            userParameters.Add("InstanceCountParam", "100");
            string invalidName = (string)this.testContextInstance.DataRow["InvalidName"];
            string invalidValue = (string)this.testContextInstance.DataRow["InvalidValue"];
            userParameters.Add(invalidName, invalidValue);

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplication(
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                        buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                        applicationId,
                        nameUri,
                        userParameters,
                        TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestReplicaRestartWaitDurationValidValueZeroInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).ReplicaRestartWaitDurationSeconds = "0";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestReplicaRestartWaitDurationValidValueInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).ReplicaRestartWaitDurationSeconds = "10";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestReplicaRestartWaitDurationInvalidNegativeValueInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).ReplicaRestartWaitDurationSeconds = "-1";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestReplicaRestartWaitDurationInvalidRandomValueInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).ReplicaRestartWaitDurationSeconds = "foo";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestReplicaRestartWaitDurationParameterizedInvalidValueInDefaultService()
        {
            string paramName = "RRWT";
            string paramDefaultValue = "";
            string paramValue = "-1";

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            AddParam(buildLayoutInfo, paramName, paramDefaultValue);

            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).ReplicaRestartWaitDurationSeconds = "[" + paramName + "]";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add(paramName, paramValue);

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestQuorumLossWaitDurationValidValueZeroInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).QuorumLossWaitDurationSeconds = "0";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestQuorumLossWaitDurationValidValueInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).QuorumLossWaitDurationSeconds = "10";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestQuorumLossWaitDurationInvalidNegativeValueInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).QuorumLossWaitDurationSeconds = "-1";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestQuorumLossWaitDurationInvalidRandomValueInDefaultService()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).QuorumLossWaitDurationSeconds = "foo";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestQuorumLossWaitDurationParameterizedInvalidValueInDefaultService()
        {
            string paramName = "QLWD";
            string paramDefaultValue = "";
            string paramValue = "-1";

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            AddParam(buildLayoutInfo, paramName, paramDefaultValue);

            ((StatefulServiceType)((DefaultServicesTypeService)(buildLayoutInfo.ApplicationManifestType.DefaultServices.Items[1])).Item).QuorumLossWaitDurationSeconds = "[" + paramName + "]";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add(paramName, paramValue);

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestParameterizedPlacementConstraints()
        {
            string paramName = "PlacementConstraintsParameter";
            string paramDefaultValue = "";
            string paramValue = "NodeType == NodeType1";

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            AddParam(buildLayoutInfo, paramName, paramDefaultValue);

            var requiredQueueDefaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items.First(defaultService => ((DefaultServicesTypeService)(defaultService)).Name.Equals("RequiredQueueService"));

            (((DefaultServicesTypeService)(requiredQueueDefaultService)).Item).PlacementConstraints = "[" + paramName + "]";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add(paramName, paramValue);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();

            string applicationInstanceFile = storeLayoutSpec.GetApplicationInstanceFile("MasterApplicationType", applicationId, "1");
            var applicationInstance = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ApplicationInstanceType>(applicationInstanceFile, TestUtility.ImageStoreDefaultTimeout);

            requiredQueueDefaultService = applicationInstance.DefaultServices.Items.First(defaultService => ((DefaultServicesTypeService)(defaultService)).Name.Equals("RequiredQueueService"));

            Verify.AreEqual((((DefaultServicesTypeService)(requiredQueueDefaultService)).Item).PlacementConstraints, paramValue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDefaultUserForCodePackage()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = null;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();

            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");
            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            Verify.AreEqual(servicepackage.DigestedCodePackage[0].RunAsPolicy[0].UserRef, buildLayoutInfo.ApplicationManifestType.Policies.DefaultRunAsPolicy.UserRef);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestAllEntryPointRunAsForCodePackage()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[2];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.All
            };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[1] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[1].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.All
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();            

            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");
            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);
            Verify.AreEqual(servicepackage.DigestedCodePackage[0].RunAsPolicy[0].EntryPointType, RunAsPolicyTypeEntryPointType.All);
            Verify.AreEqual(servicepackage.DigestedCodePackage[1].RunAsPolicy[0].EntryPointType, RunAsPolicyTypeEntryPointType.All);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPackageSharingPolicyForAllPackage()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[3];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                PackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name
            };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[1] = new PackageSharingPolicyType
            {
                PackageRef = buildLayoutInfo.ServiceManifestTypes[0].ConfigPackage[0].Name
            };
            string packageName = buildLayoutInfo.ServiceManifestTypes[0].DataPackage[0].Name;
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[2] = new PackageSharingPolicyType
            {
                PackageRef = packageName
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();            

            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");
            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            Verify.IsTrue(servicepackage.DigestedCodePackage[0].IsShared,
                String.Format("Expected package sharing policy {0} for {1}", true, servicepackage.DigestedCodePackage[0].CodePackage.Name));

            Verify.IsFalse(servicepackage.DigestedCodePackage[1].IsShared,
                String.Format("Expected package sharing policy {0} for {1}", false, servicepackage.DigestedCodePackage[1].CodePackage.Name));
            
            Verify.IsTrue(servicepackage.DigestedConfigPackage[0].IsShared,
                String.Format("Expected package sharing policy {0} for {1}", true, servicepackage.DigestedConfigPackage[0].ConfigPackage.Name));

            foreach (ServicePackageTypeDigestedDataPackage dataPackage in servicepackage.DigestedDataPackage)
            {
                if (String.Equals(dataPackage.DataPackage.Name, packageName, StringComparison.OrdinalIgnoreCase))
                {
                    Verify.IsTrue(dataPackage.IsShared,
                        String.Format("Expected package sharing policy {0} for {1}", true, servicepackage.DigestedDataPackage[0].DataPackage.Name));
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidPackageSharingRef()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                PackageRef = "InvalidPackageRef"
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
               () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDuplicatePackageSharingRef()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[2];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                PackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[1] = new PackageSharingPolicyType
            {
                PackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
               () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidWorkFolderWithPackageSharing()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage = new CodePackageType[1];
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0] = new CodePackageType()
            {
                Name = "CodePackage1",
                Version = "1.0",
                EntryPoint = new EntryPointDescriptionType()
                {
                    Item = new EntryPointDescriptionTypeExeHost()
                    {
                        Program = "Setup.exe",
                        WorkingFolder = ExeHostEntryPointTypeWorkingFolder.CodePackage
                    }
                }
            };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                PackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name
            };


            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
               () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));

            EntryPointDescriptionTypeExeHost exeHost = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EntryPoint.Item as EntryPointDescriptionTypeExeHost;
            exeHost.WorkingFolder = ExeHostEntryPointTypeWorkingFolder.CodeBase;

            applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
               () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPackageSharingPolicyScopeAll()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                Scope = PackageSharingPolicyTypeScope.All
            };
           

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();

            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");
            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            for(int i = 0; i < servicepackage.DigestedCodePackage.Length; i++)
            {
                Verify.IsTrue(servicepackage.DigestedCodePackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", true, servicepackage.DigestedCodePackage[i].CodePackage.Name));
            }
            for(int i = 0; i < servicepackage.DigestedConfigPackage.Length; i++)
            {
                Verify.IsTrue(servicepackage.DigestedConfigPackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", true, servicepackage.DigestedConfigPackage[i].ConfigPackage.Name));
            }
            for(int i = 0; i < servicepackage.DigestedDataPackage.Length; i++)
            {
                Verify.IsTrue(servicepackage.DigestedDataPackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", true, servicepackage.DigestedDataPackage[i].DataPackage.Name));
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPackageSharingPolicyScopeCode()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                Scope = PackageSharingPolicyTypeScope.Code
            };


            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");
            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            for (int i = 0; i < servicepackage.DigestedCodePackage.Length; i++)
            {
                Verify.IsTrue(servicepackage.DigestedCodePackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", true, servicepackage.DigestedCodePackage[i].CodePackage.Name));
            }
            for (int i = 0; i < servicepackage.DigestedConfigPackage.Length; i++)
            {
                Verify.IsFalse(servicepackage.DigestedConfigPackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", false, servicepackage.DigestedConfigPackage[i].ConfigPackage.Name));
            }
            for (int i = 0; i < servicepackage.DigestedDataPackage.Length; i++)
            {
                Verify.IsFalse(servicepackage.DigestedDataPackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", false, servicepackage.DigestedDataPackage[i].DataPackage.Name));
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPackageSharingPolicyScopeConfig()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                Scope = PackageSharingPolicyTypeScope.Config
            };


            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");
            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            for (int i = 0; i < servicepackage.DigestedCodePackage.Length; i++)
            {
                Verify.IsFalse(servicepackage.DigestedCodePackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", false, servicepackage.DigestedCodePackage[i].CodePackage.Name));
            }
            for (int i = 0; i < servicepackage.DigestedConfigPackage.Length; i++)
            {
                Verify.IsTrue(servicepackage.DigestedConfigPackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", true, servicepackage.DigestedConfigPackage[i].ConfigPackage.Name));
            }
            for (int i = 0; i < servicepackage.DigestedDataPackage.Length; i++)
            {
                Verify.IsFalse(servicepackage.DigestedDataPackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", false, servicepackage.DigestedDataPackage[i].DataPackage.Name));
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPackageSharingPolicyScopeData()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                Scope = PackageSharingPolicyTypeScope.Data
            };


            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");
            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            for (int i = 0; i < servicepackage.DigestedCodePackage.Length; i++)
            {
                Verify.IsFalse(servicepackage.DigestedCodePackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", false, servicepackage.DigestedCodePackage[i].CodePackage.Name));
            }
            for (int i = 0; i < servicepackage.DigestedConfigPackage.Length; i++)
            {
                Verify.IsFalse(servicepackage.DigestedConfigPackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", false, servicepackage.DigestedConfigPackage[i].ConfigPackage.Name));
            }
            for (int i = 0; i < servicepackage.DigestedDataPackage.Length; i++)
            {
                Verify.IsTrue(servicepackage.DigestedDataPackage[i].IsShared,
                    String.Format("Expected package sharing policy {0} for {1}", true, servicepackage.DigestedDataPackage[i].DataPackage.Name));
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestPackageSharingPolicyInvalidScopeWithRef()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                PackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                Scope = PackageSharingPolicyTypeScope.Data
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
               () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                PackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                Scope = PackageSharingPolicyTypeScope.Config
            };

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                PackageRef = buildLayoutInfo.ServiceManifestTypes[0].ConfigPackage[0].Name,
                Scope = PackageSharingPolicyTypeScope.Code
            };

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDifferentEntryPointRunAsForCodePackage()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[3];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.All
            };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[1] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[1].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.Setup
            };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[2] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[1].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.Main
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");
            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            Verify.AreEqual(servicepackage.DigestedCodePackage[0].RunAsPolicy[0].EntryPointType, RunAsPolicyTypeEntryPointType.All);
            Verify.AreNotEqual(servicepackage.DigestedCodePackage[1].RunAsPolicy[0].EntryPointType, RunAsPolicyTypeEntryPointType.All);
            Verify.AreNotEqual(servicepackage.DigestedCodePackage[1].RunAsPolicy[0].EntryPointType, RunAsPolicyTypeEntryPointType.All);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestCodePackageRunasPolicyEntryPointCounts()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[3];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.All
            };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[1] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[1].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.All
            };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[2] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.Setup
            };
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
               () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestSetupEntryRunAsForCodePackage()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            foreach (ApplicationManifestTypeServiceManifestImport serviceImport in buildLayoutInfo.ApplicationManifestType.ServiceManifestImport)
            {
                foreach (object servicePolicy in serviceImport.Policies)
                {
                    if (servicePolicy.GetType() == typeof(RunAsPolicyType))
                    {
                        ((RunAsPolicyType)servicePolicy).EntryPointType = RunAsPolicyTypeEntryPointType.Setup;
                    }
                }

            }

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");
            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            Verify.AreEqual(servicepackage.DigestedCodePackage[0].RunAsPolicy[0].EntryPointType, RunAsPolicyTypeEntryPointType.Setup);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidRunAsPolicyCount()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[3];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] =
                new RunAsPolicyType
                {
                    CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                    UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                    EntryPointType = RunAsPolicyTypeEntryPointType.Setup
                };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[1] =
                new RunAsPolicyType
                {
                    CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                    UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                    EntryPointType = RunAsPolicyTypeEntryPointType.All
                };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
               () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidSetupEntryRunAsForCodePackage()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].SetupEntryPoint = null;

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] =
                new RunAsPolicyType
                {
                    CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                    UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                    EntryPointType = RunAsPolicyTypeEntryPointType.Setup
                };


            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
               () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidLogonTypeForPrincipal()
        {

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.Principals.Users = new SecurityPrincipalsTypeUser[1];
            buildLayoutInfo.ApplicationManifestType.Principals.Users[0] = new SecurityPrincipalsTypeUser
            {
                Name = "NetworkService",
                AccountType = SecurityPrincipalsTypeUserAccountType.NetworkService,
                LoadUserProfile = true,
                PerformInteractiveLogon = true
            };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.All
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
              () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationProvisionWithAbsolutePath()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            string appDirectory = Path.Combine(TestUtility.TestDirectory, Guid.NewGuid().ToString());
            
            try
            {
                Directory.CreateDirectory(appDirectory);

                TestUtility.WriteXml<ApplicationManifestType>(Path.Combine(appDirectory, "ApplicationManifest.xml"), buildLayoutInfo.ApplicationManifestType);

                Verify.Throws<FabricImageBuilderValidationException>(
                    () => this.imageBuilder.BuildApplicationType(appDirectory, TimeSpan.MaxValue));
            }
            finally 
            {
                if (Directory.Exists(appDirectory))
                {
                    Directory.Delete(appDirectory, true);
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationProvisionWithInvalidRelativePath()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<FabricImageBuilderValidationException>(
                () => this.imageBuilder.BuildApplicationType(Path.Combine(this.imageBuilderTestContext.FabricWorkingDir, "..", applicationTypeName), TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestSecurityUserParameters()
        {
            string accountParamName = "DomUser";
            string passwordParamName = "Password";

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.Principals.Users = new SecurityPrincipalsTypeUser[1];
            buildLayoutInfo.ApplicationManifestType.Principals.Users[0] = new SecurityPrincipalsTypeUser
            {
                Name = "DomUser",
                AccountType = SecurityPrincipalsTypeUserAccountType.DomainUser,
                AccountName = "[DomUser]",
                // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Not a real secret")]
                Password = "[Password]",
                PasswordEncrypted = false
            };
            buildLayoutInfo.ApplicationManifestType.Policies.DefaultRunAsPolicy = null;
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.All
            };

            AddParam(buildLayoutInfo, accountParamName, "");
            AddParam(buildLayoutInfo, passwordParamName, "");

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");
            userParameters.Add("DomUser", @"redmond\testuser");
            userParameters.Add("Password", "DummyPassword");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);


            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string appPackageFile = storeLayoutSpec.GetApplicationPackageFile("MasterApplicationType", applicationId, "1.0");
            var appPackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ApplicationPackageType>(appPackageFile, TestUtility.ImageStoreDefaultTimeout);

            Verify.AreEqual(appPackage.DigestedEnvironment.Principals.Users[0].AccountName, @"redmond\testuser");
            Verify.AreEqual(appPackage.DigestedEnvironment.Principals.Users[0].Password, "DummyPassword");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestDebugParameters()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));
            string env = "ab=cd.de=ef.hi=jk..";
            string envBlock = env.Replace('.', '\0');

            string debugArguments = "-p [ProcessId] -t [ThreadId]";
            string debugExe = @"c:\debuggers\windbg.exe";
            string codePackageName = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name;
            string serviceManifestName = buildLayoutInfo.ServiceManifestTypes[0].Name;
            CodePackageDebugParameters debugParameters = new CodePackageDebugParameters(
                serviceManifestName,
                codePackageName,
                string.Empty,
                string.Empty,
                "All",
                debugExe,
                debugArguments,
                string.Empty,
                string.Empty,
                string.Empty,
                string.Empty,
                string.Empty,
                string.Empty,
                envBlock,
                new ContainerDebugParameters(),
                false /*DisableReliableCollectionReplicationMode*/);

            List<CodePackageDebugParameters> dparams = new List<CodePackageDebugParameters>();
            dparams.Add(debugParameters);
            string debugString = CodePackageDebugParameters.GetDebugParameters(dparams.ToArray());

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("_WFDebugParams_", debugString);
            userParameters.Add("InstanceCountParam", "100");
            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);


            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();

            //string appPackageFile = storeLayoutSpec.GetApplicationPackageFile("MasterApplicationType", applicationId, "1.0");
            //var appPackage = TestUtility.ReadXml<ApplicationPackageType>(appPackageFile);

            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, serviceManifestName, "1.0");
            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);
            foreach(ServicePackageTypeDigestedCodePackage codePackage in servicepackage.DigestedCodePackage)
            {
                if(String.Equals(codePackage.CodePackage.Name, codePackageName, StringComparison.OrdinalIgnoreCase))
                {
                    Verify.IsNotNull(codePackage.DebugParameters, "DebugParameters cannot be null");
                    if (codePackage.DebugParameters != null)
                    {
                        Verify.AreEqual(codePackage.DebugParameters.ProgramExePath, debugExe, "DebugExe path should match expected");
                        Verify.AreEqual(codePackage.DebugParameters.Arguments, debugArguments, "Debug arguments must match expected");
                    }
                }
            }

        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationSecurityAccessPolicies()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.Principals.Users = new SecurityPrincipalsTypeUser[3];
            buildLayoutInfo.ApplicationManifestType.Principals.Users[0] = new SecurityPrincipalsTypeUser
            {
                Name = "LocalUser1",
            };
            buildLayoutInfo.ApplicationManifestType.Principals.Users[1] = new SecurityPrincipalsTypeUser
            {
                Name = "LocalUser2",
            };
            buildLayoutInfo.ApplicationManifestType.Principals.Users[2] = new SecurityPrincipalsTypeUser
            {
                Name = "LocalUser3",
            };
            buildLayoutInfo.ApplicationManifestType.Policies.DefaultRunAsPolicy = null;
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new RunAsPolicyType
            {
                CodePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name,
                UserRef = buildLayoutInfo.ApplicationManifestType.Principals.Users[0].Name,
                EntryPointType = RunAsPolicyTypeEntryPointType.All
            };

            buildLayoutInfo.ApplicationManifestType.Policies.SecurityAccessPolicies = new ApplicationPoliciesTypeSecurityAccessPolicies();
            buildLayoutInfo.ApplicationManifestType.Policies.SecurityAccessPolicies.SecurityAccessPolicy = new SecurityAccessPolicyType[1];
            buildLayoutInfo.ApplicationManifestType.Policies.SecurityAccessPolicies.SecurityAccessPolicy[0] = new SecurityAccessPolicyType
            {
                ResourceRef = "cert1",
                ResourceType = SecurityAccessPolicyTypeResourceType.Certificate,
                PrincipalRef = "LocalUser1"
            };

            buildLayoutInfo.ApplicationManifestType.Certificates = new ApplicationManifestTypeCertificates();
            buildLayoutInfo.ApplicationManifestType.Certificates.SecretsCertificate = new FabricCertificateType[3];
            buildLayoutInfo.ApplicationManifestType.Certificates.SecretsCertificate[0] = new FabricCertificateType
            {
                Name = "cert1",
                X509FindValue = "6d 39 9b a5 66 f3 f5 88 a0 34 f6 18 9d 0f 4e 96 d9 d5 69 b4"
            };
            buildLayoutInfo.ApplicationManifestType.Certificates.SecretsCertificate[1] = new FabricCertificateType
            {
                Name = "cert2",
                X509FindValue = "7d 39 9b a5 66 f3 f5 88 a0 34 f6 18 9d 0f 4e 96 d9 d5 69 b4"
            };
            buildLayoutInfo.ApplicationManifestType.Certificates.SecretsCertificate[2] = new FabricCertificateType
            {
                Name = "cert3",
                X509FindValue = "8d 39 9b a5 66 f3 f5 88 a0 34 f6 18 9d 0f 4e 96 d9 d5 69 b4"
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                    buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                    applicationId,
                    nameUri,
                    userParameters,
                    TimeSpan.MaxValue);


            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string appPackageFile = storeLayoutSpec.GetApplicationPackageFile("MasterApplicationType", applicationId, "1.0");
            var appPackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ApplicationPackageType>(appPackageFile, TestUtility.ImageStoreDefaultTimeout);

            Verify.AreEqual(appPackage.DigestedEnvironment.Policies.SecurityAccessPolicies.SecurityAccessPolicy[0].ResourceRef, "cert1");
            Verify.AreEqual(appPackage.DigestedEnvironment.Policies.SecurityAccessPolicies.SecurityAccessPolicy[0].ResourceType, SecurityAccessPolicyTypeResourceType.Certificate);
            Verify.AreEqual(appPackage.DigestedCertificates.SecretsCertificate[0].Name, "cert1");
            Verify.AreEqual(appPackage.DigestedCertificates.SecretsCertificate[1].Name, "cert2");

        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidResourceRefSecurityAccessPolicy()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.Policies.SecurityAccessPolicies = new ApplicationPoliciesTypeSecurityAccessPolicies();
            buildLayoutInfo.ApplicationManifestType.Policies.SecurityAccessPolicies.SecurityAccessPolicy = new SecurityAccessPolicyType[1];
            buildLayoutInfo.ApplicationManifestType.Policies.SecurityAccessPolicies.SecurityAccessPolicy[0] = new SecurityAccessPolicyType
            {
                ResourceRef = "cert1",
                ResourceType = SecurityAccessPolicyTypeResourceType.Certificate,
                PrincipalRef = "LocalUser1"
            };

            buildLayoutInfo.ApplicationManifestType.Certificates = new ApplicationManifestTypeCertificates();
            buildLayoutInfo.ApplicationManifestType.Certificates.SecretsCertificate = new FabricCertificateType[1];
            buildLayoutInfo.ApplicationManifestType.Certificates.SecretsCertificate[0] = new FabricCertificateType
            {
                Name = "cert2",
                X509FindValue = "6d 39 9b a5 66 f3 f5 88 a0 34 f6 18 9d 0f 4e 96 d9 d5 69 b4"
            };

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestCodePackageWithFabricBinaries()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            var codePackagePath = buildLayoutInfo.BuildLayoutSpecification.GetCodePackageFolder(buildLayoutInfo.ServiceManifestTypes[0].Name, buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name);            

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            buildLayoutInfo.ImageStore.UploadContent(
                Path.Combine(codePackagePath, "FabricCommon.dll"),
                Path.Combine(TestUtility.TestDirectory, "System.Fabric.Management.dll"),
                TestUtility.ImageStoreDefaultTimeout,
                CopyFlag.AtomicCopy,
                false);

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationManifestForEndPointOverride()
        {
            /*
             * This test covers how Endpoint in the Resources section of ServiceManifest can be overridden.
             * In the ApplicationManifest if you specify a ResourceOverride then the corresponding entry for endpoint in Service Manifest is overridden
             * while creating digested resources.
             */
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string servicePort = "ServicePort";
            string serviceProtocol = "ServiceProtocol";
            string uri = "Uri";
            string serviceType = "ServiceType";
            string suffix = "Suffix";

            AddParam(buildLayoutInfo, servicePort, "");
            AddParam(buildLayoutInfo, serviceProtocol, "tcp");
            AddParam(buildLayoutInfo, uri, "");
            AddParam(buildLayoutInfo, serviceType, "");
            AddParam(buildLayoutInfo, suffix, "");

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ResourceOverrides = new ResourceOverridesType();
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ResourceOverrides.Endpoints = new EndpointOverrideType[1];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ResourceOverrides.Endpoints[0] = new EndpointOverrideType
            {
                Name = "EndpointResource2",
                Port = "["+ servicePort +"]",
                Protocol = "[" + serviceProtocol +"]",
                Type = "[" + serviceType +"]",
                UriScheme = "[" + uri +"]",
                PathSuffix = "[" + suffix + "]"
            };

            // [Test1]: Get the default values
            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            var endpointOverride = servicepackage.DigestedResources.DigestedEndpoints.FirstOrDefault(
                    endpoint => endpoint.Endpoint.GetType().Equals(typeof(EndpointType)) && ImageBuilderUtility.Equals(((EndpointType)endpoint.Endpoint).Name, "EndpointResource2"));

            Verify.AreEqual(endpointOverride.Endpoint.Port, 0);
            Verify.AreEqual(endpointOverride.Endpoint.Protocol, EndpointTypeProtocol.tcp);
            Verify.AreEqual(endpointOverride.Endpoint.Type, EndpointTypeType.Internal);
            Verify.AreEqual(endpointOverride.Endpoint.PathSuffix, null);
            Verify.AreEqual(endpointOverride.Endpoint.UriScheme, null);

            //[Test2]: Override the protocol
            userParameters.Add(serviceProtocol, "http");

            this.imageBuilder.BuildApplication(
              buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
              buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
              applicationId,
              nameUri,
              userParameters,
              TimeSpan.MaxValue);

            servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            endpointOverride = servicepackage.DigestedResources.DigestedEndpoints.FirstOrDefault(
                    endpoint => endpoint.Endpoint.GetType().Equals(typeof(EndpointType)) && ImageBuilderUtility.Equals(((EndpointType)endpoint.Endpoint).Name, "EndpointResource2"));

            Verify.AreEqual(endpointOverride.Endpoint.Port, 0);
            Verify.AreEqual(endpointOverride.Endpoint.Protocol, EndpointTypeProtocol.http);
            Verify.AreEqual(endpointOverride.Endpoint.Type, EndpointTypeType.Internal);
            Verify.AreEqual(endpointOverride.Endpoint.PathSuffix, null);
            Verify.AreEqual(endpointOverride.Endpoint.UriScheme, null);

            // Add a certificate to override to https
            buildLayoutInfo.ApplicationManifestType.Certificates = new ApplicationManifestTypeCertificates();
            buildLayoutInfo.ApplicationManifestType.Certificates.EndpointCertificate = new EndpointCertificateType[1];
            buildLayoutInfo.ApplicationManifestType.Certificates.EndpointCertificate[0] = new EndpointCertificateType
            {
                Name = "TestCert1",
                X509FindValue = "FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF F0",
                X509StoreName = "MY1"
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new EndpointBindingPolicyType
            {
                CertificateRef = "TestCert1",
                EndpointRef = "EndpointResource2"
            };

            applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            applicationId = Guid.NewGuid().ToString();
            nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            storeLayoutSpec = StoreLayoutSpecification.Create();
            servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            // [Test 3]: Override the protocol to https and do not override port so should be 0 by default. Override uri as well.
            userParameters.Clear();
            userParameters.Add("InstanceCountParam", "100");
            userParameters.Add(serviceProtocol, "https");
            userParameters.Add(uri, "ftp");

            this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TimeSpan.MaxValue);

            servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            endpointOverride = servicepackage.DigestedResources.DigestedEndpoints.FirstOrDefault(
                    endpoint => endpoint.Endpoint.GetType().Equals(typeof(EndpointType)) && ImageBuilderUtility.Equals(((EndpointType)endpoint.Endpoint).Name, "EndpointResource2"));

            Verify.AreEqual(endpointOverride.Endpoint.Port, 0);
            Verify.AreEqual(endpointOverride.Endpoint.Protocol, EndpointTypeProtocol.https);
            Verify.AreEqual(endpointOverride.Endpoint.Type, EndpointTypeType.Internal);
            Verify.AreEqual(endpointOverride.Endpoint.PathSuffix, null);
            Verify.AreEqual(endpointOverride.Endpoint.UriScheme, "ftp");

            endpointOverride = servicepackage.DigestedResources.DigestedEndpoints.FirstOrDefault(
                     endpoint => endpoint.Endpoint.GetType().Equals(typeof(EndpointType)) && ImageBuilderUtility.Equals(((EndpointType)endpoint.Endpoint).Name, "EndpointResource3"));

            Verify.AreEqual(endpointOverride, null);

            // [Test 4]: Override every possible value
            userParameters.Clear();
            userParameters.Add(servicePort, "4000");
            userParameters.Add(serviceProtocol, "https");
            userParameters.Add(serviceType, "input");
            userParameters.Add(uri, "http");
            userParameters.Add(suffix, "/test");
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
              buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
              buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
              applicationId,
              nameUri,
              userParameters,
              TimeSpan.MaxValue);

            servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            endpointOverride = servicepackage.DigestedResources.DigestedEndpoints.FirstOrDefault(
                    endpoint => endpoint.Endpoint.GetType().Equals(typeof(EndpointType)) && ImageBuilderUtility.Equals(((EndpointType)endpoint.Endpoint).Name, "EndpointResource2"));

            Verify.AreEqual(endpointOverride.Endpoint.Port, 4000);
            Verify.AreEqual(endpointOverride.Endpoint.Protocol, EndpointTypeProtocol.https);
            Verify.AreEqual(endpointOverride.Endpoint.Type, EndpointTypeType.Input);
            Verify.AreEqual(endpointOverride.Endpoint.PathSuffix, "/test");
            Verify.AreEqual(endpointOverride.Endpoint.UriScheme, "http");

            // [Test 5]: Other endpoints except the one overridden should not be effected.
            endpointOverride = servicepackage.DigestedResources.DigestedEndpoints.FirstOrDefault(
                  endpoint => endpoint.Endpoint.GetType().Equals(typeof(EndpointType)) && ImageBuilderUtility.Equals(((EndpointType)endpoint.Endpoint).Name, "EndpointResource1"));

            Verify.AreEqual(endpointOverride.Endpoint.Port, 0);
            Verify.AreEqual(endpointOverride.Endpoint.Protocol, EndpointTypeProtocol.tcp);
            Verify.AreEqual(endpointOverride.Endpoint.Type, EndpointTypeType.Internal);
            Verify.AreEqual(endpointOverride.Endpoint.PathSuffix, null);
            Verify.AreEqual(endpointOverride.Endpoint.UriScheme, null);

            //There will not be a validation error. This is because having an endpoint as https without EndpointBindingRef is valid scenario.
            endpointOverride = servicepackage.DigestedResources.DigestedEndpoints.FirstOrDefault(
                endpoint => endpoint.Endpoint.GetType().Equals(typeof(EndpointType)) && ImageBuilderUtility.Equals(((EndpointType)endpoint.Endpoint).Name, "EndpointResource4"));

            Verify.AreEqual(endpointOverride.Endpoint.Port, 0);
            Verify.AreEqual(endpointOverride.Endpoint.Protocol, EndpointTypeProtocol.https);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationManifestForEndPointOverrideFromhttpsTohttp()
        {
            //[Test6]: add an endpoint with https and a certificate. Override it to http and it should not throw validation error
            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);


            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ResourceOverrides = new ResourceOverridesType();
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ResourceOverrides.Endpoints = new EndpointOverrideType[1];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ResourceOverrides.Endpoints[0] = new EndpointOverrideType
            {
                Name = "SecureEndpointResource",
                Protocol = "http"
            };

            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints = new EndpointType[2];

            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[0] = new EndpointType
            {
                Name = "SecureEndpointResource",
                Protocol = EndpointTypeProtocol.https
            };
            buildLayoutInfo.ServiceManifestTypes[0].Resources.Endpoints[1] = new EndpointType
            {
                Name = "SecureEndpointResource1",
                Protocol = EndpointTypeProtocol.https
            };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[2];

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] =
                new EndpointBindingPolicyType() { EndpointRef = "SecureEndpointResource", CertificateRef = "TestCert1" };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[1] =
                new EndpointBindingPolicyType() { EndpointRef = "SecureEndpointResource1", CertificateRef = "TestCert1" };

            buildLayoutInfo.ApplicationManifestType.Certificates = new ApplicationManifestTypeCertificates()
            {
                EndpointCertificate = new EndpointCertificateType[1]
            };
            buildLayoutInfo.ApplicationManifestType.Certificates.EndpointCertificate[0] = new EndpointCertificateType()
            {
                Name = "TestCert1",
                X509FindValue = "2d 33 e5 74 4a 83 44 ab fd 96 24 bf f7 79 27 e2 97 2a 3d 13"
            };

            userParameters.Add("InstanceCountParam", "100");

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            var nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TestUtility.ImageStoreDefaultTimeout);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            var servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            Verify.IsTrue(servicepackage.DigestedResources.DigestedCertificates.Length == 1,
                "Expected 1 DigestedCertificate in resources");
            foreach (var endpoint in servicepackage.DigestedResources.DigestedEndpoints)
            {
                if (endpoint.Endpoint.Name == "SecureEndpointResource")
                {
                    Verify.IsTrue(endpoint.EndpointBindingPolicy == null,
                        "Did not expect EndpointBindingPolicy associated with EndpointResource");
                }
                if (endpoint.Endpoint.Name == "SecureEndpointResource1")
                {
                    Verify.IsTrue(endpoint.EndpointBindingPolicy != null &&
                                  endpoint.EndpointBindingPolicy.CertificateRef == "TestCert1",
                        "Expected EndpointBindingPolicy associated with EndpointResource");
                }
            }

        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestCodePackageWithAzureBinaries()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            var codePackagePath = buildLayoutInfo.BuildLayoutSpecification.GetCodePackageFolder(buildLayoutInfo.ServiceManifestTypes[0].Name, buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name);

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            buildLayoutInfo.ImageStore.UploadContent(
                Path.Combine(codePackagePath, "Microsoft.WindowsAzure.Storage.dll"),
                Path.Combine(TestUtility.TestDirectory, "System.Fabric.Management.dll"),
                TestUtility.ImageStoreDefaultTimeout,
                CopyFlag.AtomicCopy,
                false);

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestOverrideBooleanValueInDriverConfig()
        {
            /*
             * This test covers overriding a boolean value in the ApplicationManifest with some junk value.
             * In that case it should throw a validation exception.
             */
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            AddParam(buildLayoutInfo, "Encrypted", "false");

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ContainerHostPoliciesType containerHostPolicy = new ContainerHostPoliciesType { CodePackageRef = codePackageName };

            containerHostPolicy.Items = new object[1];

            var logConfig = new ContainerLoggingDriverType { Driver = "etwlogs", Items = new DriverOptionType[4] };
            logConfig.Items[0] = new DriverOptionType { Name = "[DriverOptionName1]", Value = "[DriverOptionValue1]" };
            logConfig.Items[1] = new DriverOptionType { Name = "Name2", Value = "Value2" };
            logConfig.Items[2] = new DriverOptionType { Name = "Name3", Value = "Value3", IsEncrypted = "false" };
            logConfig.Items[3] = new DriverOptionType { Name = "Name4", Value = "Value4", IsEncrypted = "[Encrypted]" };
            containerHostPolicy.Items[0] = logConfig;

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("DriverOptionName1", "Name1");
            userParameters.Add("DriverOptionValue1", "Value1");
            userParameters.Add("Encrypted", "True");
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TestUtility.ImageStoreDefaultTimeout);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            foreach (var codePackage in servicepackage.DigestedCodePackage)
            {
                if (codePackageName.Equals(codePackage.CodePackage.Name))
                {
                    ContainerHostPoliciesType containerHostPolices = codePackage.ContainerHostPolicies;
                    Verify.IsTrue(containerHostPolices != null &&
                                containerHostPolices.Items != null,
                      String.Format("{0} {1}", "Expected containerhostpolicy with valid items for the code package ", codePackageName));
                    Verify.IsTrue(!String.IsNullOrEmpty(containerHostPolices.Isolation) &&
                        containerHostPolices.Isolation.Equals("process"),
                        String.Format("Expected Isolation {0} got {1} codepackageref {2}", "process", containerHostPolices.Isolation, codePackageName));
                    foreach (var item in containerHostPolices.Items)
                    {
                        if (item.GetType().Equals(typeof(ContainerLoggingDriverType)))
                        {
                            logConfig = item as ContainerLoggingDriverType;
                            Verify.IsTrue(logConfig.Driver == "etwlogs",
                                String.Format("Unexpected Logging Config {0}",
                                logConfig.Driver));
                            foreach (DriverOptionType option in logConfig.Items)
                            {
                                if (option.Name == "Name1")
                                {
                                    Verify.IsTrue(option.Value == "Value1",
                                        String.Format("Unexpected driver option for Name {0}, value {1}",
                                        option.Name,
                                        option.Value));
                                }
                                else if (option.Name == "Name2")
                                {
                                    Verify.IsTrue(option.Value == "Value2",
                                        String.Format("Unexpected driver option for Name {0}, value {1}",
                                        option.Name,
                                        option.Value));
                                    Verify.IsTrue(option.IsEncrypted == "false",
                                        String.Format("Unexpected driver option IsEncrypted for Name {0}, value {1}, isEncrypted {2}",
                                        option.Name,
                                        option.Value,
                                        option.IsEncrypted));

                                }
                                else if (option.Name == "Name3")
                                {
                                    Verify.IsTrue(option.IsEncrypted == "false",
                                        String.Format("Unexpected driver option IsEncrypted for Name {0}, value {1}, isEncrypted {2}",
                                        option.Name,
                                        option.Value,
                                        option.IsEncrypted));
                                }
                                else if (option.Name == "Name4")
                                {
                                    Verify.IsTrue(option.IsEncrypted == "True",
                                        String.Format("Unexpected driver option IsEncrypted for Name {0}, value {1}, isEncrypted {2}",
                                        option.Name,
                                        option.Value,
                                        option.IsEncrypted));
                                }
                            }
                        }
                    }
                }
            }

            //Test Case 2: Override value with some junk value. It will throw an error
            userParameters.Clear();
            userParameters.Add("DriverOptionName1", "Name1");
            userParameters.Add("DriverOptionValue1", "Value1");
            userParameters.Add("Encrypted", "NotaBool");
            userParameters.Add("InstanceCountParam", "100");


            //This will throw an error since it will fail the validation of IsEncrypted being a bool value.
            Verify.Throws<AggregateException>(
                  () => this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TestUtility.ImageStoreDefaultTimeout));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationManifestForImageOverride()
        {
            /*
             * This test covers that the application instance should be build correctly if ImageOverrides is specified.
             */
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];
            ContainerHostPoliciesType containerHostPolicy = new ContainerHostPoliciesType { CodePackageRef = codePackageName };

            containerHostPolicy.Items = new object[1];

            var imageOverrides = new ImageOverridesType { Image = new ImageType[3] };
            imageOverrides.Image[0] = new ImageType { Name = "TestImage", Os = "16299" };
            imageOverrides.Image[1] = new ImageType { Name = "TestImage1", Os = "14393" };
            imageOverrides.Image[2] = new ImageType { Name = "TestImage1" };

            containerHostPolicy.Items[0] = imageOverrides;

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = containerHostPolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                TestUtility.ImageStoreDefaultTimeout);

        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationManifestForEnvironmentOverride()
        {
            /*
             * This test covers that the application instance should be build correctly if EnvironmentOverrides is specified.
             */
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            AddParam(buildLayoutInfo, "SomeTestEnvValue", "Something4");
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ResourceOverrides = new ResourceOverridesType();
            
            EnvironmentOverridesType[] envOverrides = new EnvironmentOverridesType[1];
            EnvironmentVariableOverrideType[] envVars = new EnvironmentVariableOverrideType[3];
            envVars[0] = new EnvironmentVariableOverrideType { Name = "TestEnvVar", Value = "[SomeTestEnvValue]", Type = "SecretsStoreRef" };
            envVars[1] = new EnvironmentVariableOverrideType { Name = "TestEnvVar2", Value = "TestVal2" };
            envVars[2] = new EnvironmentVariableOverrideType { Name = "TestEnvVar3", Value = "DefaultVal3" };
            // You cannot add below because it is not present in ServiceManifest and ImageBuilder throws an error.
            // envVars[3] = new EnvironmentVariableType { Name = "NotpresentInManifest", Value = "RandomValue" };

            envOverrides[0] = new EnvironmentOverridesType { EnvironmentVariable = envVars, CodePackageRef = "ZDummyProgram" };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].EnvironmentOverrides = envOverrides;


            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            userParameters.Add("SomeTestEnvValue", "EnvValOverride");
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);
            
            ServicePackageTypeDigestedCodePackage digestedCodePackage = servicepackage.DigestedCodePackage.FirstOrDefault(
                codePackage => ImageBuilderUtility.Equals(codePackage.CodePackage.Name, "ZDummyProgram"));

            EnvironmentVariableType[] environmentVariables = digestedCodePackage.CodePackage.EnvironmentVariables;

            Verify.AreEqual(environmentVariables.Length, 4);

            var defaultEnv = environmentVariables.FirstOrDefault(
                  env => ImageBuilderUtility.Equals(env.Name, "TestEnvVar"));

            Verify.AreEqual(defaultEnv.Value, "EnvValOverride");
            Verify.AreEqual(defaultEnv.Type, EnvironmentVariableTypeType.SecretsStoreRef);

            var env2 = environmentVariables.FirstOrDefault(
                  env => ImageBuilderUtility.Equals(env.Name, "TestEnvVar1"));

            Verify.AreEqual(env2.Value, "SomeOtherValue");
            Verify.AreEqual(env2.Type, EnvironmentVariableTypeType.Encrypted);

            var env3 = environmentVariables.FirstOrDefault(
                    env => ImageBuilderUtility.Equals(env.Name, "NotpresentInManifest"));

            Verify.AreEqual(env3, null);

            var env4 = environmentVariables.FirstOrDefault(
                    env => ImageBuilderUtility.Equals(env.Name, "TestEnvVar2"));

            Verify.AreEqual(env4.Value, "TestVal2");
            Verify.AreEqual(env4.Type, EnvironmentVariableTypeType.SecretsStoreRef);

            var defaultEnv2 = environmentVariables.FirstOrDefault(
                     env => ImageBuilderUtility.Equals(env.Name, "TestEnvVar3"));

            Verify.AreEqual(defaultEnv2.Value, "DefaultVal3");
            Verify.AreEqual(defaultEnv2.Type, EnvironmentVariableTypeType.PlainText);

        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationManifestForConfigPackagePolicies()
        {
            /*
             * This test covers that the application instance should be build correctly if ImageOverrides is specified.
             */
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            AddParam(buildLayoutInfo, "ConfigSection1", "X.X");
            AddParam(buildLayoutInfo, "MountPoint2", "");

            var codePackageName = "XContainerHost";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new object[1];

            int configPoliciesCount = 2;
            var conifgPackagePolicy = new ConfigPackagePoliciesType { CodePackageRef = codePackageName, ConfigPackage = new ConfigPackageDescriptionType[configPoliciesCount] };

            conifgPackagePolicy.ConfigPackage[0] = new ConfigPackageDescriptionType { Name = "UserServiceHost.Config", SectionName = "[ConfigSection1]", MountPoint = "C:\\SfDev" };
            conifgPackagePolicy.ConfigPackage[1] = new ConfigPackageDescriptionType { Name = "UserServiceHost.Config", SectionName = "CustomSection", MountPoint = "[MountPoint2]" };

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = conifgPackagePolicy;

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri =
                new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");
            userParameters.Add("ConfigSection1", "UserServiceHostSection");
            userParameters.Add("MountPoint2", "C:\\SfDev");

            //This will throw an error since we have MyParam as duplicate in the sections "UserServiceHostSection" and "CustomSection"
            Verify.Throws<AggregateException>(
                  () => this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TestUtility.ImageStoreDefaultTimeout));

            //Test 2
            userParameters.Clear();
            userParameters.Add("InstanceCountParam", "100");
            userParameters.Add("ConfigSection1", "UserServiceHostSection");
            userParameters.Add("MountPoint2", "/var/secrets");

            this.imageBuilder.BuildApplication(
              buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
              buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
              applicationId,
              nameUri,
              userParameters,
              TimeSpan.MaxValue);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile("MasterApplicationType", applicationId, "CoreServices", "1.0");

            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            foreach (var codePackage in servicepackage.DigestedCodePackage)
            {
                if (codePackageName.Equals(codePackage.CodePackage.Name))
                {
                    var policies = codePackage.ConfigPackagePolicies;

                    Verify.AreNotEqual(policies.ConfigPackage, null);
                    Verify.AreEqual(policies.ConfigPackage.Length, configPoliciesCount);

                    var policy1 = policies.ConfigPackage.FirstOrDefault(
                                 policy => ImageBuilderUtility.Equals(policy.Name, "UserServiceHost.Config")
                                 && ImageBuilderUtility.Equals(policy.SectionName, "UserServiceHostSection"));

                    Verify.AreNotEqual(policy1, null);
                    Verify.AreEqual(policy1.MountPoint, "C:\\SfDev");


                    var policy2 = policies.ConfigPackage.FirstOrDefault(
                       policy => ImageBuilderUtility.Equals(policy.Name, "UserServiceHost.Config")
                       && ImageBuilderUtility.Equals(policy.SectionName, "CustomSection"));

                    Verify.AreNotEqual(policy2, null);
                    Verify.AreEqual(policy2.MountPoint, "/var/secrets");
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vivekagg,dariopb")]
        public void TestServiceDnsNameParameterizedValueInDefaultService()
        {
            var parameterName = "ServiceDns_Name";
            var parameterDefaultValue = "defaultservicednsname";

            var buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            this.AddParam(buildLayoutInfo, parameterName, parameterDefaultValue);
     
            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            Verify.IsTrue(String.IsNullOrEmpty(defaultService.ServiceDnsName));

            defaultService.ServiceDnsName = "[" + parameterName + "]";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TimeSpan.MaxValue);

            var applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            var updatedDefaultService = applicationInstance.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService && ((DefaultServicesTypeService)i).Name == defaultService.Name)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            Verify.IsTrue(
                updatedDefaultService.ServiceDnsName.Equals(parameterDefaultValue),
                "ServiceDnsName should be set to " + parameterDefaultValue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vivekagg,dariopb")]
        public void TestServiceDnsNameParameterizedValueOverridenInDefaultService()
        {
            var parameterName = "ServiceDns_Name";
            var parameterDefaultValue = "";
            var parameterValue = "overridenservicednsname";

            var buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            this.AddParam(buildLayoutInfo, parameterName, parameterDefaultValue);

            var userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add(parameterName, parameterValue);

            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            Verify.IsTrue(String.IsNullOrEmpty(defaultService.ServiceDnsName));

            defaultService.ServiceDnsName = "[" + parameterName + "]";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TimeSpan.MaxValue);

            var applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            var updatedDefaultService = applicationInstance.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService && ((DefaultServicesTypeService)i).Name == defaultService.Name)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            Verify.IsTrue(
                updatedDefaultService.ServiceDnsName.Equals(parameterValue),
                "ServiceDnsName should be set to " + parameterValue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vivekagg,dariopb")]
        public void TestServiceDnsNameParameterizedValueOverridenWithEmptyValueInDefaultService()
        {
            var parameterName = "ServiceDns_Name";
            var parameterDefaultValue = "defaultservicednsname";
            var parameterValue = "";

            var buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            this.AddParam(buildLayoutInfo, parameterName, parameterDefaultValue);

            var userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add(parameterName, parameterValue);

            var defaultService = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            Verify.IsTrue(String.IsNullOrEmpty(defaultService.ServiceDnsName));

            defaultService.ServiceDnsName = "[" + parameterName + "]";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            userParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TimeSpan.MaxValue);

            var applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            var updatedDefaultService = applicationInstance.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService && ((DefaultServicesTypeService)i).Name == defaultService.Name)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            Verify.IsTrue(
                updatedDefaultService.ServiceDnsName.Equals(parameterValue),
                "ServiceDnsName should be set to " + parameterValue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vivekagg,dariopb")]
        public void TestServiceDnsNameParameterizedDupilcateDefaultValuesInDefaultService()
        {
            var service1ParameterName = "Service1_DnsName";
            var service2ParameterName = "Service2_DnsName";
            var serviceParameterValue = "defaultservicednsname";

            var buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            this.AddParam(buildLayoutInfo, service1ParameterName, serviceParameterValue);
            this.AddParam(buildLayoutInfo, service2ParameterName, serviceParameterValue);

            List<object> items = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items.ToList();

            Verify.IsTrue(items.Count > 0);

            DefaultServicesTypeService defaultService1 = (DefaultServicesTypeService)items[0];
            DefaultServicesTypeService defaultService2 = (DefaultServicesTypeService)items[1];

            Verify.IsTrue(String.IsNullOrEmpty(defaultService1.ServiceDnsName));
            Verify.IsTrue(String.IsNullOrEmpty(defaultService2.ServiceDnsName));

            defaultService1.ServiceDnsName = "[" + service1ParameterName + "]";
            defaultService2.ServiceDnsName = "[" + service2ParameterName + "]";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(
                    applicationTypeName, 
                    TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vivekagg,dariopb")]
        public void TestServiceDnsNameParameterizedDupilcateOverridenValuesInDefaultService()
        {
            var service1ParameterName = "Service1_DnsName";
            var service2ParameterName = "Service2_DnsName";
            var service1DefaultParameterValue = "defaultservice1Value";
            var service2DefaultParameterValue = "defaultservice2Value";
            var serviceParameterValue = "overriderndnsname";

            var buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            this.AddParam(buildLayoutInfo, service1ParameterName, service1DefaultParameterValue);
            this.AddParam(buildLayoutInfo, service2ParameterName, service2DefaultParameterValue);

            List<object> items = buildLayoutInfo.ApplicationManifestType.DefaultServices.Items.ToList();

            Verify.IsTrue(items.Count > 0);

            DefaultServicesTypeService defaultService1 = (DefaultServicesTypeService)items[0];
            DefaultServicesTypeService defaultService2 = (DefaultServicesTypeService)items[1];

            Verify.IsTrue(String.IsNullOrEmpty(defaultService1.ServiceDnsName));
            Verify.IsTrue(String.IsNullOrEmpty(defaultService2.ServiceDnsName));

            defaultService1.ServiceDnsName = "[" + service1ParameterName + "]";
            defaultService2.ServiceDnsName = "[" + service2ParameterName + "]";

            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue);

            string applicationId = Guid.NewGuid().ToString();
            Uri nameUri = new Uri(string.Format("fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName));

            var userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add(service1ParameterName, serviceParameterValue);
            userParameters.Add(service2ParameterName, serviceParameterValue);

            Verify.Throws <AggregateException> (
                () => this.imageBuilder.BuildApplication(
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                 buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                 applicationId,
                 nameUri,
                 userParameters,
                 TimeSpan.MaxValue));
        }

        private void AddParam(BuildLayoutInfo buildLayoutInfo, string paramName, string paramDefaultValue)
        {
            int paramCount = buildLayoutInfo.ApplicationManifestType.Parameters.Length;

            ApplicationManifestTypeParameter[] appParams = new ApplicationManifestTypeParameter[paramCount + 1];

            for (int i = 0; i < paramCount; i++)
            {
                appParams[i] = buildLayoutInfo.ApplicationManifestType.Parameters[i];
            }

            ApplicationManifestTypeParameter appParam = new ApplicationManifestTypeParameter();
            appParam.Name = paramName;
            appParam.DefaultValue = paramDefaultValue;

            appParams[paramCount] = appParam;

            buildLayoutInfo.ApplicationManifestType.Parameters = appParams;
        }
                
        internal static void CreateDropAndSetFabricCodePath()
        {
            var RootLocation = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "log", "FD.Tests");
            var DropLocation = "Drop";
            if (!Directory.Exists(RootLocation))
            {
                Directory.CreateDirectory(RootLocation);
            }

            if (!Directory.Exists(DropLocation))
            {
                string makeDropScriptPath = Environment.ExpandEnvironmentVariables(@"%SRCROOT%\prod\Setup\scripts\MakeDrop.cmd");
                if (File.Exists(makeDropScriptPath))
                {
                    int exitCode = FabricDeployerDeploymentTests.ExecuteProcess(
                        makeDropScriptPath,
                        string.Format(CultureInfo.InvariantCulture, @"/dest:{0} /symbolsDestination:nul", DropLocation));
                    Verify.AreEqual<int>(0, exitCode, "Unable to create Drop");
                }
                else
                {
                    string fabricDropPath = Environment.ExpandEnvironmentVariables(@"%_NTTREE%\FabricDrop");
                    FabricDeployerDeploymentTests.DirectoryCopy(fabricDropPath, DropLocation, true);
                }
            }

            string fabricCodePackage = null;
            string fabricAppDirectory = Path.Combine(Directory.GetCurrentDirectory(), DropLocation, @"bin\Fabric");
            foreach (string dir in Directory.EnumerateDirectories(fabricAppDirectory, "Fabric.Code"))
            {
                fabricCodePackage = dir;
            }

            string path = Environment.GetEnvironmentVariable("path");
            path = string.Format(CultureInfo.InvariantCulture, "{0};{1}", path, fabricCodePackage);
            Environment.SetEnvironmentVariable("path", path);
            Environment.SetEnvironmentVariable("FabricCodePath", fabricCodePackage);
        }

        private static string GetTempDirectoryPath(string directoryName)
        {
            string path = Path.Combine(TestUtility.TestDirectory, directoryName);
            if (Directory.Exists(path))
            {
                Directory.Delete(path, true);
            }

            return path;
        }

        internal static string GenerateSfpkgWithValidation(string appFolder, string destFolder, bool applyCompression, string sfpkgName)
        {
            string expectedSfpkgName = String.IsNullOrEmpty(sfpkgName) ? Path.GetFileName(appFolder) : sfpkgName;
            expectedSfpkgName = Path.ChangeExtension(expectedSfpkgName, ".sfpkg");

            string sfpkgFilePath = ImageStoreUtility.GenerateSfpkg(appFolder, destFolder, applyCompression, sfpkgName);

            Verify.IsTrue(File.Exists(sfpkgFilePath));
            Verify.AreEqual(Path.GetFileName(sfpkgFilePath), expectedSfpkgName);
            Verify.IsTrue(Path.GetExtension(sfpkgFilePath) == ".sfpkg");

            // Destination folder exists and contains the sfpkg.
            Verify.IsTrue(Directory.Exists(destFolder));
            Verify.IsTrue(Directory.GetFiles(destFolder).Length >= 1);

            Verify.IsTrue(Directory.Exists(appFolder));

            return sfpkgFilePath;
        }
    }
}