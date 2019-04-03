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
    using System.Globalization;
    using System.Fabric.Management.ImageStore;

    [TestClass]
    public class ImageBuilderExeTest
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

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUnexpectedError()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string buildPath = buildLayoutInfo.CreateBuildLayout();
            string errorDetailsFile = "ErrorDetails.txt";

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            try
            {
                string applicationTypeName = null, applicationTypeVersion = null;
                int exitCode = exeWrapper.GetApplicationTypeInfo(
                    buildPath,
                    errorDetailsFile,
                    out applicationTypeName, 
                    out applicationTypeVersion);

                Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
                Verify.IsTrue(File.Exists(errorDetailsFile));

                using (StreamReader reader = new StreamReader(errorDetailsFile))
                {
                    var tokens = reader.ReadLine().Split(new char[]{ ',' });
                    Verify.AreEqual(tokens.Length, 2);

                    int fileErrorCode = int.Parse(tokens[0]);
                    Verify.AreEqual(fileErrorCode, (int)ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
                    Verify.AreEqual(tokens[1], System.Fabric.ImageBuilderExe.StringConstants.TestErrorDetails);
                }
            }
            finally
            {
                if (File.Exists(errorDetailsFile))
                {
                    File.Delete(errorDetailsFile);
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestGetApplicationTypeInfo()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            string applicationTypeName = null, applicationTypeVersion = null;
            int exitCode = exeWrapper.GetApplicationTypeInfo(
                buildPath,                 
                out applicationTypeName, 
                out applicationTypeVersion);

            Verify.AreEqual(exitCode, 0);
            Verify.AreEqual(buildLayoutInfo.ApplicationManifestType.ApplicationTypeName, applicationTypeName);
            Verify.AreEqual(buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion, applicationTypeVersion);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationType()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, 0);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplication()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, 0);

            string applicationId = Guid.NewGuid().ToString();
            string nameUri = string.Format(CultureInfo.InvariantCulture, "fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName);

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("portvalue", "500");
            userParameters.Add("DomainGroup", "DomainGroup");
            userParameters.Add("MyUserParam", "Foo");
            userParameters.Add("InstanceCountParam", "100");

            string outputFolder = "CMOutputFolder";

            exitCode = exeWrapper.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                outputFolder);

            Verify.AreEqual(exitCode, 0);
            Verify.IsTrue(Directory.Exists(outputFolder));

            if (Directory.Exists(outputFolder))
            {
                Directory.Delete(outputFolder, true);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationWithMissingParameterSuccess()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, 0);

            string applicationId = Guid.NewGuid().ToString();
            string nameUri = string.Format(CultureInfo.InvariantCulture, "fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName);

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            // we do not validate against user parameters being null/empty
            userParameters.Add("InstanceCountParam", "100");

            string outputFolder = "CMOutputFolder";

            exitCode = exeWrapper.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                outputFolder);

            Verify.AreEqual(exitCode, 0);
            Verify.IsTrue(Directory.Exists(outputFolder));

            if (Directory.Exists(outputFolder))
            {
                Directory.Delete(outputFolder, true);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationWithEmptyParameterSuccess()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, 0);

            string applicationId = Guid.NewGuid().ToString();
            string nameUri = string.Format(CultureInfo.InvariantCulture, "fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName);

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            // we do not validate against user parameters being null/empty
            userParameters.Add("MyUserParam", string.Empty);
            userParameters.Add("InstanceCountParam", "100");

            string outputFolder = "CMOutputFolder";

            exitCode = exeWrapper.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                outputFolder);

            Verify.AreEqual(exitCode, 0);
            Verify.IsTrue(Directory.Exists(outputFolder));

            if (Directory.Exists(outputFolder))
            {
                Directory.Delete(outputFolder, true);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationWithMissingParameterFail()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, 0);

            string applicationId = Guid.NewGuid().ToString();
            string nameUri = string.Format(CultureInfo.InvariantCulture, "fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName);

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            // "InstanceCountParam" is used to generate the digested manifest - we validate this against being null/empty
            string outputFolder = "CMOutputFolder";

            exitCode = exeWrapper.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                outputFolder);

            Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
            Verify.IsTrue(!Directory.Exists(outputFolder));

            if (Directory.Exists(outputFolder))
            {
                Directory.Delete(outputFolder, true);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestTypeWithNoCodePackage()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage = null;

            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestTypeWithNoServiceTypes()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes = null;

            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestTypeWithInValidPlacementConstraints()
        {
            this.TestServiceManifestPlacementConstraints("(abc=", false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestTypeWithValidPlacementConstraints()
        {
            this.TestServiceManifestPlacementConstraints("(abc=1)", true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationManifestTypeWithNoServiceImport()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport = null;

            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationManifestTypeInvalidPlacementConstraint()
        {
            this.TestApplicationManifestTypeWithPlacementConstraint("a!Param)", false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestApplicationManifestTypeValidPlacementConstraint()
        {
            this.TestApplicationManifestTypeWithPlacementConstraint("a>100.00", true);
        }

        public void TestApplicationManifestTypeWithPlacementConstraint(string placementConstraint, bool expectSuccess)
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceTemplates.First().PlacementConstraints = placementConstraint;

            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, expectSuccess ? 0 : ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestBuildApplicationTypeWithInvalidConfigOverride()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.SettingsType.Section = null;

            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestWithInvalidCodePackageName()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name = "InvalidCodePackageName";

            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestWithInvalidCodePackageEntryPoints()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            {
                var entryPoint = new DllHostEntryPointType();
                entryPoint.Items = new object[] 
                {
                    new UnmanagedDllType { Value = "C:\\temp\\InvalidDll.dll" }
                };

                buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EntryPoint.Item = entryPoint;

                string buildPath = buildLayoutInfo.CreateBuildLayout();                

                int exitCode = exeWrapper.GetApplicationType(buildPath);
                Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
            }

            {
                var entryPoint = new DllHostEntryPointType();
                entryPoint.Items = new object[] 
                {
                    new UnmanagedDllType { Value = "\"MyLib.dll" }
                };

                buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EntryPoint.Item = entryPoint;

                string buildPath = buildLayoutInfo.CreateBuildLayout();

                int exitCode = exeWrapper.GetApplicationType(buildPath);
                Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
            }

            {
                var entryPoint = new DllHostEntryPointType();
                entryPoint.Items = new object[] 
                {
                    new UnmanagedDllType { Value = "\"\"" }
                };

                buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EntryPoint.Item = entryPoint;

                string buildPath = buildLayoutInfo.CreateBuildLayout();

                int exitCode = exeWrapper.GetApplicationType(buildPath);
                Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestWithValidCodePackageEntryPoints()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            {
                var entryPoint = new EntryPointDescriptionTypeExeHost
                {
                    Program = "Setup.exe",
                    Arguments = "-port:80 -testonly",
                    WorkingFolder = ExeHostEntryPointTypeWorkingFolder.Work
                };

                buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EntryPoint = new EntryPointDescriptionType { Item = entryPoint };

                string buildPath = buildLayoutInfo.CreateBuildLayout();

                ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

                int exitCode = exeWrapper.GetApplicationType(buildPath);
                Verify.AreEqual(exitCode, 0);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestServiceManifestWithValidCodePackageEntryPoints2()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            {
                var entryPoint = new EntryPointDescriptionTypeExeHost
                {
                    Program = "\"Test Path\\TestLib.exe\"",
                    Arguments = "/name:\"Test Name\" /opt",
                    WorkingFolder = ExeHostEntryPointTypeWorkingFolder.Work
                };

                buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EntryPoint = new EntryPointDescriptionType { Item = entryPoint };

                string buildPath = buildLayoutInfo.CreateBuildLayout();

                ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

                int exitCode = exeWrapper.GetApplicationType(buildPath);
                Verify.AreEqual(exitCode, 0);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidBuildPath()
        {
            string buildPath = "InvalidBuildLayout";

            string applicationTypeName, applicationTypeVersion;

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationTypeInfo(buildPath, out applicationTypeName, out applicationTypeVersion);
            Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidBuildApplication()
        {
            string appTypeName = "InvalidAppTypeName";
            string appTypeVersion = "InvalidAppTypeVersion";
            string appId = "AppId_App0";
            string nameUri = "fabric:/app";
            Dictionary<string, string> parameters = new Dictionary<string, string>();
            string output = ".\\";

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.BuildApplication(
                appTypeName,
                appTypeVersion,
                appId,
                nameUri,
                parameters,
                output);

            Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestInvalidUpgradeApplication()
        {
            string appTypeName = "InvalidAppTypeName";
            string currentInstanceVersion = "1";
            string targetTypeVersion = "InvalidAppTypeVersion";
            string appId = "AppId_App0";
            Dictionary<string, string> parameters = new Dictionary<string, string>();
            string output = ".\\";

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.UpgradeApplication(
                appTypeName,
                currentInstanceVersion,
                targetTypeVersion,
                appId,
                parameters,
                output);

            Verify.AreEqual(exitCode, ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpgradeApplication()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, 0);

            string applicationId = Guid.NewGuid().ToString();
            string nameUri = string.Format(CultureInfo.InvariantCulture, "fabric:/{0}", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName);

            Dictionary<string, string> userParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            userParameters.Add("portvalue", "500");
            userParameters.Add("DomainGroup", "DomainGroup");
            userParameters.Add("MyUserParam", "Foo");
            userParameters.Add("InstanceCountParam", "100");

            exitCode = exeWrapper.BuildApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                nameUri,
                userParameters,
                null);

            Verify.AreEqual(exitCode, 0);

            buildLayoutInfo.DeleteBuildLayout();

            // Upgrade Application
            Collection<object> updatedServiceTypes = new Collection<object>(buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes.ToList());
            updatedServiceTypes.Add(new StatelessServiceTypeType() { ServiceTypeName = "UpdatedServiceType" });
            buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes = updatedServiceTypes.ToArray();
            buildLayoutInfo.ServiceManifestTypes[0].Version = "1.1";

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = "1.1";
            buildLayoutInfo.ApplicationManifestType.ServiceTemplates = null;
            buildLayoutInfo.ApplicationManifestType.DefaultServices = null;
            buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2.0";

            string updatedBuildPath = buildLayoutInfo.CreateBuildLayout();

            exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, 0);

            string outputFolder = "CMUpdatedOutput";

            exitCode = exeWrapper.UpgradeApplication(
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                "1",
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                applicationId,
                new Dictionary<string, string>(),
                outputFolder);
            Verify.AreEqual(exitCode, 0);

            Verify.IsTrue(Directory.Exists(outputFolder));

            if (Directory.Exists(outputFolder))
            {
                Directory.Delete(outputFolder, true);
            }
        }
        
        private void TestServiceManifestPlacementConstraints(string expression, bool expectSuccess)
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            ServiceTypeType sericeType = buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes.First() as ServiceTypeType;
            Verify.IsNotNull(sericeType);
            sericeType.PlacementConstraints = expression;

            string buildPath = buildLayoutInfo.CreateBuildLayout();

            ImageBuilderExeTestWrapper exeWrapper = new ImageBuilderExeTestWrapper(this.imageStore);

            int exitCode = exeWrapper.GetApplicationType(buildPath);
            Verify.AreEqual(exitCode, expectSuccess ? 0 : ImageBuilderExeTestWrapper.ImageBuilderErrorCode);
        }
    }   
}