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

    [TestClass]
    public class ImageBuilderUpgradeTest
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
        }

        [TestCleanup]
        public virtual void TestCleanup()
        {
            this.imageBuilderTestContext.TestCleanup();
        }

        /***************** Upgrade Tests *****************************/
        [ImageStoreType(ImageStoreEnum.XStore)]
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpgradeApplication()
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

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            buildLayoutInfo.DeleteBuildLayout();

            // Upgrade the application          
            BuildLayoutInfo upgradedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            upgradedBuildLayoutInfo.ApplicationManifestType.Policies.DefaultRunAsPolicy.UserRef = "[RunAsUser]";
            upgradedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2.0";
            upgradedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = "2.0";

            upgradedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage = null;
            upgradedBuildLayoutInfo.ServiceManifestTypes[0].Version = "2.0";

            string updatedApplicationTypeName = upgradedBuildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(updatedApplicationTypeName, TimeSpan.MaxValue);

            this.imageBuilder.UpgradeApplication(
                applicationId,
                upgradedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                applicationInstance.Version,
                upgradedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                userParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType upgradedApplicationInstance = TestUtility.GetApplicationInstance(upgradedBuildLayoutInfo, applicationId, 2, RolloutVersion.CreateRolloutVersion());

            Verify.AreEqual(applicationId, upgradedApplicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(upgradedApplicationInstance.NameUri));

            // Verify RolloutVersion on ApplicationInstance
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ApplicationPackageRef.RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ApplicationPackageRef.RolloutVersion).NextMajorRolloutVersion());

            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ServicePackageRef[0].RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ServicePackageRef[0].RolloutVersion).NextMajorRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpgradeApplicationWithNewServiceManifest()
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

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            buildLayoutInfo.DeleteBuildLayout();

            // Upgrade the application

            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            string newServiceManifestName = "NewCalcService";
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestName = newServiceManifestName;
            updatedBuildLayoutInfo.ServiceManifestTypes[0].Name = newServiceManifestName;
            CodePackageType codePackage =  updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage[0];
            updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage = new CodePackageType[1] { codePackage };
            ((UnmanagedDllType)((DllHostEntryPointType)updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EntryPoint.Item).Items[0]).Value = string.Format("System.Fabric.Management.{0}.dll", newServiceManifestName);

            string updatedApplicationTypeName = updatedBuildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(updatedApplicationTypeName, TimeSpan.MaxValue);

            this.imageBuilder.UpgradeApplication(
                applicationId,
                updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                applicationInstance.Version,
                updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                userParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType upgradedApplicationInstance = TestUtility.GetApplicationInstance(updatedBuildLayoutInfo, applicationId, 2, RolloutVersion.CreateRolloutVersion());

            Verify.AreEqual(applicationId, upgradedApplicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(upgradedApplicationInstance.NameUri));

            // Verify RolloutVersion on ApplicationInstance
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ApplicationPackageRef.RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ApplicationPackageRef.RolloutVersion));

            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ServicePackageRef[0].RolloutVersion),
                RolloutVersion.CreateRolloutVersion(2));
        }

        [ImageStoreType(ImageStoreEnum.XStore)]
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpgradeApplicationWithUpdatedParameters()
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

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            // Upgrade the application                                   
            Dictionary<string, string> upgradeUserParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            upgradeUserParameters.Add("portvalue", "600");
            upgradeUserParameters.Add("DomainGroup", "UpgradedDomainGroup");
            upgradeUserParameters.Add("InstanceCountParam", "300");

            this.imageBuilder.UpgradeApplication(
                applicationId,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                applicationInstance.Version,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                upgradeUserParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType upgradedApplicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 2, RolloutVersion.CreateRolloutVersion());

            Verify.AreEqual(applicationId, upgradedApplicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(upgradedApplicationInstance.NameUri));

            // Verify RolloutVersion on ApplicationInstance
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ApplicationPackageRef.RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ApplicationPackageRef.RolloutVersion).NextMajorRolloutVersion());

            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ServicePackageRef[0].RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ServicePackageRef[0].RolloutVersion).NextMinorRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpgradeApplicationWithUpdatedServiceManifest()
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

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            buildLayoutInfo.DeleteBuildLayout();

            // Upgrade the application   
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            Collection<DataPackageType> updatedDataPackages = new Collection<DataPackageType>(updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage.ToList());
            updatedDataPackages.Add(new DataPackageType() { Name = "UpdatedDataPackageName", Version = "1.1" });
            updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage = updatedDataPackages.ToArray();

            CodePackageType updatedCodePackage = new CodePackageType()
            {
                Name = "UpdatedCodePackage",
                Version = "1.0",
                EntryPoint = updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EntryPoint
            };

            Collection<CodePackageType> updatedCodePackages = new Collection<CodePackageType>();
            updatedCodePackages.Add(updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage[0]);
            updatedCodePackages.Add(updatedCodePackage);

            updatedBuildLayoutInfo.ServiceManifestTypes[0].CodePackage = updatedCodePackages.ToArray();

            updatedBuildLayoutInfo.ServiceManifestTypes[0].ConfigPackage = null;

            updatedBuildLayoutInfo.ServiceManifestTypes[0].Version = "2.0";

            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ConfigOverrides = null;
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = "2.0";

            var defaultService = updatedBuildLayoutInfo.ApplicationManifestType.DefaultServices.Items
                .Where(i => i is DefaultServicesTypeService)
                .Select(i => (DefaultServicesTypeService)i)
                .First();

            defaultService.Name = "UpdatedDefaultServicesName";

            updatedBuildLayoutInfo.ApplicationManifestType.ServiceTemplates = null;
            updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2.0";

            string updatedApplicationTypeName = updatedBuildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(updatedApplicationTypeName, TimeSpan.MaxValue);

            this.imageBuilder.UpgradeApplication(
                applicationId,
                updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                applicationInstance.Version,
                updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                userParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType upgradedApplicationInstance = TestUtility.GetApplicationInstance(updatedBuildLayoutInfo, applicationId, 2, RolloutVersion.CreateRolloutVersion());

            Verify.AreEqual(applicationId, upgradedApplicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(upgradedApplicationInstance.NameUri));

            // Verify RolloutVersion on ApplicationInstance            
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ApplicationPackageRef.RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ApplicationPackageRef.RolloutVersion));
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ServicePackageRef[0].RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ServicePackageRef[0].RolloutVersion).NextMinorRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpgradeApplicationWithServiceTypeOutofOrder()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.DefaultServices = null;
            buildLayoutInfo.ApplicationManifestType.ServiceTemplates = null;

            buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes = new object[2];
            buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes[0] = new StatelessServiceTypeType()
            {
                ServiceTypeName = "StatelessServiceType"
            };
            buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes[1] = new StatefulServiceTypeType()
            {
                ServiceTypeName = "StatefulServiceType",
                HasPersistedState = true
            };

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

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            buildLayoutInfo.DeleteBuildLayout();

            // Upgrade the application   
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            updatedBuildLayoutInfo.ApplicationManifestType.DefaultServices = null;
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceTemplates = null;

            updatedBuildLayoutInfo.ServiceManifestTypes[0].ServiceTypes = new object[2];
            updatedBuildLayoutInfo.ServiceManifestTypes[0].ServiceTypes[0] = buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes[1];
            updatedBuildLayoutInfo.ServiceManifestTypes[0].ServiceTypes[1] = buildLayoutInfo.ServiceManifestTypes[0].ServiceTypes[0];

            updatedBuildLayoutInfo.ServiceManifestTypes[0].Version = "2.0";            
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = "2.0";
            
            updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2.0";

            string updatedApplicationTypeName = updatedBuildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(updatedApplicationTypeName, TimeSpan.MaxValue);

            this.imageBuilder.UpgradeApplication(
                applicationId,
                updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                applicationInstance.Version,
                updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                userParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType upgradedApplicationInstance = TestUtility.GetApplicationInstance(updatedBuildLayoutInfo, applicationId, 2, RolloutVersion.CreateRolloutVersion());

            Verify.AreEqual(applicationId, upgradedApplicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(upgradedApplicationInstance.NameUri));

            // Verify RolloutVersion on ApplicationInstance            
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ApplicationPackageRef.RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ApplicationPackageRef.RolloutVersion));
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ServicePackageRef[0].RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ServicePackageRef[0].RolloutVersion));
        }

        [ImageStoreType(ImageStoreEnum.XStore)]
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpgradeApplicationWithoutPrincipals()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            string applicationTypeName = buildLayoutInfo.CreateBuildLayout();

            Verify.NoThrow(() => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue), "BuildApplicationType");

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

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            buildLayoutInfo.DeleteBuildLayout();

            // Upgrade the application   
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            updatedBuildLayoutInfo.ApplicationManifestType.Principals = null;
            updatedBuildLayoutInfo.ApplicationManifestType.Diagnostics = null;
            updatedBuildLayoutInfo.ApplicationManifestType.Policies.DefaultRunAsPolicy = null;
            updatedBuildLayoutInfo.ApplicationManifestType.Parameters = new ApplicationManifestTypeParameter[]
            {
                new ApplicationManifestTypeParameter() { Name ="PortValue", DefaultValue = "40000"},
                new ApplicationManifestTypeParameter() { Name ="MyUserParam", DefaultValue = ""},
                new ApplicationManifestTypeParameter() { Name ="InstanceCountParam", DefaultValue = ""},
                new ApplicationManifestTypeParameter() { Name ="MyUserParamType", DefaultValue = "SecretsStoreRef"}
            };
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = null;
            updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2.0";

            string updatedApplicationTypeName = updatedBuildLayoutInfo.CreateBuildLayout();

            Verify.NoThrow(() => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue), "BuildApplicationType");

            this.imageBuilder.BuildApplicationType(updatedApplicationTypeName, TimeSpan.MaxValue);

            Dictionary<string, string> upgradeUserParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            upgradeUserParameters.Add("InstanceCountParam", "100");

            this.imageBuilder.UpgradeApplication(
                applicationId,
                updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                applicationInstance.Version,
                updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                upgradeUserParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType upgradedApplicationInstance = TestUtility.GetApplicationInstance(updatedBuildLayoutInfo, applicationId, 2, RolloutVersion.CreateRolloutVersion());

            Verify.AreEqual(applicationId, upgradedApplicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(upgradedApplicationInstance.NameUri));

            // Verify RolloutVersion on ApplicationInstance
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ApplicationPackageRef.RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ApplicationPackageRef.RolloutVersion).NextMajorRolloutVersion());

            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ServicePackageRef[0].RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ServicePackageRef[0].RolloutVersion).NextMajorRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpgradeApplicationSecretsCertificate()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.SettingsType.Section[0].Parameter[2].IsEncrypted = true;
            buildLayoutInfo.SettingsType.Section[0].Parameter[2].MustOverride = true;

            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ConfigOverrides[0].Settings[0].Parameter[0].IsEncrypted = true;

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

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            buildLayoutInfo.DeleteBuildLayout();

            // Upgrade the application               
            buildLayoutInfo.ApplicationManifestType.Certificates.SecretsCertificate[0].X509FindValue = "55 db 3c cb ab 8f 4b 54 fa f3 35 bc 87 d4 88 e5 66 2d 24 df";
            buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2.0";

            string updatedApplicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(updatedApplicationTypeName, TimeSpan.MaxValue);

            this.imageBuilder.UpgradeApplication(
                applicationId,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                applicationInstance.Version,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                userParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType upgradedApplicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 2, RolloutVersion.CreateRolloutVersion());

            Verify.AreEqual(applicationId, upgradedApplicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(upgradedApplicationInstance.NameUri));

            // Verify RolloutVersion on ApplicationInstance
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ApplicationPackageRef.RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ApplicationPackageRef.RolloutVersion).NextMajorRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpgradeServiceManifestWithoutVersionChange()
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

            buildLayoutInfo.DeleteBuildLayout();

            // Upgrade the application   
            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);

            updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage = null;
            updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2.0";

            string updatedApplicationTypeName = updatedBuildLayoutInfo.CreateBuildLayout();

            Verify.Throws<AggregateException>(
                () => this.imageBuilder.BuildApplicationType(applicationTypeName, TimeSpan.MaxValue));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [DataSource("Table:System.Fabric.Management.TestData.xml#UpdateDiagnosticsElement")]
        public void TestUpgradeApplicationWithUpdatedDiagnosticsParameters()
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

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            // Upgrade the application                                   
            Dictionary<string, string> upgradeUserParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            upgradeUserParameters.Add("portvalue", "500");
            upgradeUserParameters.Add("DomainGroup", "DomainGroup");
            upgradeUserParameters.Add("InstanceCountParam", "100");
            string parameterName = (string)this.testContextInstance.DataRow["Name"];
            string updatedValue = (string)this.testContextInstance.DataRow["UpdatedValue"];
            upgradeUserParameters.Add(parameterName, updatedValue);

            this.imageBuilder.UpgradeApplication(
                applicationId,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                applicationInstance.Version,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                upgradeUserParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType upgradedApplicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 2, RolloutVersion.CreateRolloutVersion());

            Verify.AreEqual(applicationId, upgradedApplicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(upgradedApplicationInstance.NameUri));

            // Verify RolloutVersion on ApplicationInstance
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ApplicationPackageRef.RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ApplicationPackageRef.RolloutVersion).NextMajorRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty(
            "Data:DiagnosticsChange", 
            "{AddProvider, RemoveProvider, ReplaceProvider, AddManifestPackage, RemoveManifestPackage, ReplaceManifestPackage}")]
        public void TestUpgradeServiceWithUpdatedDiagnosticsParameters()
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

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            buildLayoutInfo.DeleteBuildLayout();

            // Upgrade the application

            BuildLayoutInfo updatedBuildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            updatedBuildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].ServiceManifestRef.ServiceManifestVersion = "2.0";
            updatedBuildLayoutInfo.ServiceManifestTypes[0].Version = "2.0";

            ServiceDiagnosticsTypeETWProviderGuid newProviderGuid = 
                new ServiceDiagnosticsTypeETWProviderGuid() { Value = "0F0EA8A6-6A35-4027-8601-A543457CF764" };
            if (this.testContextInstance.DataRow["DiagnosticsChange"].Equals("AddProvider"))
            {
                ServiceDiagnosticsTypeETWProviderGuid[] newProviders = 
                    new ServiceDiagnosticsTypeETWProviderGuid[updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ProviderGuids.Length + 1];
                Array.Copy(
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ProviderGuids,
                    newProviders,
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ProviderGuids.Length);
                newProviders[newProviders.Length - 1] = newProviderGuid;
                updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ProviderGuids = newProviders;
            }
            else if (this.testContextInstance.DataRow["DiagnosticsChange"].Equals("RemoveProvider"))
            {
                Verify.IsGreaterThan(updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ProviderGuids.Length, 1);
                ServiceDiagnosticsTypeETWProviderGuid[] newProviders = 
                    new ServiceDiagnosticsTypeETWProviderGuid[updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ProviderGuids.Length - 1];
                Array.Copy(
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ProviderGuids,
                    newProviders,
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ProviderGuids.Length-1);
                updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ProviderGuids = newProviders;
            }
            else if (this.testContextInstance.DataRow["DiagnosticsChange"].Equals("ReplaceProvider"))
            {
                updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ProviderGuids[
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ProviderGuids.Length - 1] =
                        newProviderGuid;
            }
            else if (this.testContextInstance.DataRow["DiagnosticsChange"].Equals("AddManifestPackage"))
            {
                Verify.IsGreaterThan(
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage.Length, 
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages.Length);
                updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages = updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage;
            }
            else if (this.testContextInstance.DataRow["DiagnosticsChange"].Equals("RemoveManifestPackage"))
            {
                Verify.IsGreaterThan(updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages.Length, 1);
                DataPackageType[] newPackages = new DataPackageType[updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages.Length - 1];
                Array.Copy(
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages,
                    newPackages,
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages.Length - 1);
                updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages = newPackages;
            }
            else if (this.testContextInstance.DataRow["DiagnosticsChange"].Equals("ReplaceManifestPackage"))
            {
                Verify.AreNotEqual(
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages[
                      updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages.Length - 1].Name,
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage[
                      updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage.Length - 1].Name);

                updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages[
                    updatedBuildLayoutInfo.ServiceManifestTypes[0].Diagnostics.ETW.ManifestDataPackages.Length - 1].Name =
                        updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage[
                            updatedBuildLayoutInfo.ServiceManifestTypes[0].DataPackage.Length - 1].Name;
            }
            else
            {
                Verify.Fail("Unexpected ProviderListChange value");
            }

            string updatedApplicationTypeName = updatedBuildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(updatedApplicationTypeName, TimeSpan.MaxValue);

            this.imageBuilder.UpgradeApplication(
                applicationId,
                updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                applicationInstance.Version,
                updatedBuildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                userParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType upgradedApplicationInstance = TestUtility.GetApplicationInstance(updatedBuildLayoutInfo, applicationId, 2, RolloutVersion.CreateRolloutVersion());

            Verify.AreEqual(applicationId, upgradedApplicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(upgradedApplicationInstance.NameUri));

            // Verify RolloutVersion on ApplicationInstance
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ApplicationPackageRef.RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ApplicationPackageRef.RolloutVersion));

            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ServicePackageRef[0].RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ServicePackageRef[0].RolloutVersion).NextMinorRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpgradeApplicationSharingPolicy()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = null;
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

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            buildLayoutInfo.DeleteBuildLayout();

            // Upgrade the application               
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies = new Object[1];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].Policies[0] = new PackageSharingPolicyType
            {
                PackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name
            };

            buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2.0";

            string updatedApplicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(updatedApplicationTypeName, TimeSpan.MaxValue);

            this.imageBuilder.UpgradeApplication(
                applicationId,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                applicationInstance.Version,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                userParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType upgradedApplicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 2, RolloutVersion.CreateRolloutVersion());

            Verify.AreEqual(applicationId, upgradedApplicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(upgradedApplicationInstance.NameUri));

            // Verify RolloutVersion on ApplicationInstance
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ServicePackageRef[0].RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ServicePackageRef[0].RolloutVersion).NextMinorRolloutVersion());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpgradeWithEnvironmentVariables()
        {
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore);
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EnvironmentVariables = new EnvironmentVariableType[3];
            var codePackageRef = buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].Name;
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EnvironmentVariables[0] = new EnvironmentVariableType
            {
                Name = "TestVariable1",
                Value = "Value1"
            };
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EnvironmentVariables[1] = new EnvironmentVariableType
            {
                Name = "TestVariable2",
                Value = "Value2"
            };
            buildLayoutInfo.ServiceManifestTypes[0].CodePackage[0].EnvironmentVariables[2] = new EnvironmentVariableType
            {
                Name = "TestVariable3",
                Value = "Value3"
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
                TimeSpan.MaxValue);

            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1, RolloutVersion.CreateRolloutVersion());

            buildLayoutInfo.DeleteBuildLayout();

            // Upgrade the application               
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].EnvironmentOverrides = new EnvironmentOverridesType[1];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].EnvironmentOverrides[0] = new EnvironmentOverridesType { CodePackageRef = codePackageRef };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].EnvironmentOverrides[0].EnvironmentVariable = new EnvironmentVariableOverrideType[2];
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].EnvironmentOverrides[0].EnvironmentVariable[0] =
                new EnvironmentVariableOverrideType { Name = "TestVariable2", Value = "Override2" };
            buildLayoutInfo.ApplicationManifestType.ServiceManifestImport[0].EnvironmentOverrides[0].EnvironmentVariable[1] =
                new EnvironmentVariableOverrideType { Name = "TestVariable3", Value = "Override3" };

            buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion = "2.0";

            string updatedApplicationTypeName = buildLayoutInfo.CreateBuildLayout();

            this.imageBuilder.BuildApplicationType(updatedApplicationTypeName, TimeSpan.MaxValue);

            this.imageBuilder.UpgradeApplication(
                applicationId,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeName,
                applicationInstance.Version,
                buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion,
                userParameters,
                TimeSpan.MaxValue);

            ApplicationInstanceType upgradedApplicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 2, RolloutVersion.CreateRolloutVersion());

            Verify.AreEqual(applicationId, upgradedApplicationInstance.ApplicationId);
            Verify.AreEqual<Uri>(nameUri, new Uri(upgradedApplicationInstance.NameUri));

            // Verify RolloutVersion on ApplicationInstance
            Verify.AreEqual(
                RolloutVersion.CreateRolloutVersion(upgradedApplicationInstance.ServicePackageRef[0].RolloutVersion),
                RolloutVersion.CreateRolloutVersion(applicationInstance.ServicePackageRef[0].RolloutVersion).NextMinorRolloutVersion());
        }
    }
}