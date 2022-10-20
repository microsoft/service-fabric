// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using SingleInstance = Management.ImageBuilder.SingleInstance;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Collections.Generic;
    using System.Fabric.FabricDeployer;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Management.Automation.Runspaces;
    using WEX.TestExecution;
    using Newtonsoft.Json;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Management.ImageBuilder.SingleInstance;
    using System.Xml;

    [TestClass]
    public class SingleInstanceApplicationTest
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
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void UpgradeWithSetting()
        {
            string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            string jsonFilePath = Path.Combine(currentExecutingDirectory, "Application_No_Setting.json");
            string descriptionJson = File.ReadAllText(jsonFilePath);
            SingleInstance.Application application = JsonConvert.DeserializeObject<SingleInstance.Application>(descriptionJson);

            string applicationTypeName = "MeshApplicationType";
            string applicationTypeVersion = "v1";
            Uri applicationName = new Uri("fabric:/myapp");
            string tempBuildPath = Path.Combine(TestUtility.TestDirectory, Guid.NewGuid().ToString());
            string outputPath = Path.Combine(TestUtility.TestDirectory, Guid.NewGuid().ToString());

            string applicationId = Guid.NewGuid().ToString();
            this.imageBuilder.BuildSingleInstanceApplication(
                application,
                applicationTypeName,
                applicationTypeVersion,
                applicationId,
                applicationName,
                true, //generateDNS
                TimeSpan.MaxValue,
                tempBuildPath,
                outputPath, // output pkg build path
                false, //use open network
                false, //use local nat network
                "C:\\",
                new SingleInstance.GenerationConfig());
            
            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore, tempBuildPath);
            ApplicationInstanceType applicationInstance = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 1);
            TestUtility.VerifyStoreLayout(buildLayoutInfo);

            StoreLayoutSpecification storeLayoutSpec = StoreLayoutSpecification.Create();
            storeLayoutSpec.SetRoot(outputPath);
            string servicePackageFile = storeLayoutSpec.GetServicePackageFile(applicationTypeName, applicationId, "myBackendServicePkg", applicationInstance.ServicePackageRef[0].RolloutVersion);
            var servicepackage = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile, TestUtility.ImageStoreDefaultTimeout);

            string rolloutVersion1 = servicepackage.RolloutVersion;

            // Test Case 1: Instance Count change
            string jsonFilePath2 = Path.Combine(currentExecutingDirectory, "Application_No_Setting2.json");
            string descriptionJson2 = File.ReadAllText(jsonFilePath2);
            SingleInstance.Application application2 = JsonConvert.DeserializeObject<SingleInstance.Application>(descriptionJson2);

            string applicationTypeVersion2 = "v2";
            this.imageBuilder.BuildSingleInstanceApplicationForUpgrade(
                application2,
                applicationTypeName,
                applicationTypeVersion,
                applicationTypeVersion2,
                applicationId,
                applicationInstance.Version,
                applicationName,
                true, //generateDNS
                TimeSpan.MaxValue,
                tempBuildPath,
                outputPath,
                false, //use open network
                false, //use local nat network
                "C:\\",
                new SingleInstance.GenerationConfig());

            ApplicationInstanceType applicationInstance2 = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 2);
            string servicePackageFile2 = storeLayoutSpec.GetServicePackageFile(applicationTypeName, applicationId, "myBackendServicePkg", applicationInstance2.ServicePackageRef[0].RolloutVersion);
            var servicepackage2 = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile2, TestUtility.ImageStoreDefaultTimeout);
            Verify.AreEqual(servicepackage2.ManifestVersion, "v1");

            string rolloutVersion2 = servicepackage2.RolloutVersion;
            Verify.AreEqual(rolloutVersion1, rolloutVersion2);
            TestUtility.VerifyStoreLayout(buildLayoutInfo);

            // Test Case 2: Adding settings
            string jsonSettingFilePath1 = Path.Combine(currentExecutingDirectory, "Application_Setting1.json");
            string settingDescriptionJson1 = File.ReadAllText(jsonSettingFilePath1);
            SingleInstance.Application applicationWithSetting1 = JsonConvert.DeserializeObject<SingleInstance.Application>(settingDescriptionJson1);

            string applicationTypeVersion3 = "v3";
            this.imageBuilder.BuildSingleInstanceApplicationForUpgrade(
                applicationWithSetting1,
                applicationTypeName,
                applicationTypeVersion2,
                applicationTypeVersion3,
                applicationId,
                applicationInstance2.Version,
                applicationName,
                true, //generateDNS
                TimeSpan.MaxValue,
                tempBuildPath,
                outputPath,
                false, //use open network
                false, //use localnat network
                "C:\\",
                new SingleInstance.GenerationConfig());
            ApplicationInstanceType applicationInstance3 = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 3);
            string servicePackageFile3 = storeLayoutSpec.GetServicePackageFile(applicationTypeName, applicationId, "myBackendServicePkg", applicationInstance3.ServicePackageRef[0].RolloutVersion);
            var servicepackage3 = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile3, TestUtility.ImageStoreDefaultTimeout);

            Verify.AreEqual(servicepackage3.ManifestVersion, applicationTypeVersion3);

            string rolloutVersion3 = servicepackage3.RolloutVersion;
            Verify.AreNotEqual(rolloutVersion2, rolloutVersion3);
            TestUtility.VerifyStoreLayout(buildLayoutInfo);

            // Test Case 3: Changing settings only
            string jsonSettingFilePath2 = Path.Combine(currentExecutingDirectory, "Application_Setting2.json");
            string settingDescriptionJson2 = File.ReadAllText(jsonSettingFilePath2);
            SingleInstance.Application applicationWithSetting2 = JsonConvert.DeserializeObject<SingleInstance.Application>(settingDescriptionJson2);

            string applicationTypeVersion4 = "v4";
            this.imageBuilder.BuildSingleInstanceApplicationForUpgrade(
                applicationWithSetting2,
                applicationTypeName,
                applicationTypeVersion3,
                applicationTypeVersion4,
                applicationId,
                applicationInstance3.Version,
                applicationName,
                true, //generateDNS
                TimeSpan.MaxValue,
                tempBuildPath,
                outputPath,
                false, //use open network
                false, //use localnat network
                "C:\\",
                new SingleInstance.GenerationConfig());
            ApplicationInstanceType applicationInstance4 = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 4);
            string servicePackageFile4 = storeLayoutSpec.GetServicePackageFile(applicationTypeName, applicationId, "myBackendServicePkg", applicationInstance4.ServicePackageRef[0].RolloutVersion);
            var servicepackage4 = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile4, TestUtility.ImageStoreDefaultTimeout);

            Verify.AreEqual(servicepackage4.ManifestVersion, applicationTypeVersion4);

            // Code package version remains the same for settings upgrade
            foreach (var codePackage in servicepackage4.DigestedCodePackage)
            {
                Verify.AreEqual(applicationTypeVersion3, codePackage.CodePackage.Version);
            }
            // Config package version is set to target version for settings upgrade
            foreach (var configPackage in servicepackage4.DigestedConfigPackage)
            {
                Verify.AreEqual(applicationTypeVersion4, configPackage.ConfigPackage.Version);
            }

            string rolloutVersion4 = servicepackage4.RolloutVersion;
            Verify.AreNotEqual(rolloutVersion3, rolloutVersion4);
            TestUtility.VerifyStoreLayout(buildLayoutInfo);

            // Test Case 4: Keep settings
            string jsonSettingFilePath3 = Path.Combine(currentExecutingDirectory, "Application_Setting2InstanceCount.json");
            string settingDescriptionJson3 = File.ReadAllText(jsonSettingFilePath3);
            SingleInstance.Application applicationWithSetting3 = JsonConvert.DeserializeObject<SingleInstance.Application>(settingDescriptionJson3);

            string applicationTypeVersion5 = "v5";
            this.imageBuilder.BuildSingleInstanceApplicationForUpgrade(
                applicationWithSetting3,
                applicationTypeName,
                applicationTypeVersion4,
                applicationTypeVersion5,
                applicationId,
                applicationInstance4.Version,
                applicationName,
                true, //generateDNS
                TimeSpan.MaxValue,
                tempBuildPath,
                outputPath,
                false, //use open network
                false, //use localnat network
                "C:\\",
                new SingleInstance.GenerationConfig());
            ApplicationInstanceType applicationInstance5 = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 5);
            string servicePackageFile5 = storeLayoutSpec.GetServicePackageFile(applicationTypeName, applicationId, "myBackendServicePkg", applicationInstance5.ServicePackageRef[0].RolloutVersion);
            var servicepackage5 = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile5, TestUtility.ImageStoreDefaultTimeout);

            // SP manifest version remains the same
            Verify.AreEqual(servicepackage5.ManifestVersion, applicationTypeVersion4);

            // SP rollout version remains the same
            string rolloutVersion5 = servicepackage5.RolloutVersion;
            Verify.AreEqual(rolloutVersion4, rolloutVersion5);
            TestUtility.VerifyStoreLayout(buildLayoutInfo);

            // Test Case 5: Autoscaling - Instance Count change
            string jsonSettingFilePath4 = Path.Combine(currentExecutingDirectory, "Application_Setting3Autoscaling.json");
            string settingDescriptionJson4 = File.ReadAllText(jsonSettingFilePath4);
            SingleInstance.Application applicationWithSetting4 = JsonConvert.DeserializeObject<SingleInstance.Application>(settingDescriptionJson4);
            
            string applicationTypeVersion6 = "v6";
            this.imageBuilder.BuildSingleInstanceApplicationForUpgrade(
                applicationWithSetting4,
                applicationTypeName,
                applicationTypeVersion5,
                applicationTypeVersion6,
                applicationId,
                applicationInstance5.Version,
                applicationName,
                true, //generateDNS
                TimeSpan.MaxValue,
                tempBuildPath,
                outputPath,
                false, //use open network
                false, //use localnat network
                "C:\\",
                new SingleInstance.GenerationConfig());

            ApplicationInstanceType applicationInstance6 = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 6);
            string servicePackageFile6 = storeLayoutSpec.GetServicePackageFile(applicationTypeName, applicationId, "myBackendServicePkg", applicationInstance6.ServicePackageRef[0].RolloutVersion);
            var servicepackage6 = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile6, TestUtility.ImageStoreDefaultTimeout);
            Verify.AreEqual(servicepackage6.ManifestVersion, "v4");
            
            string rolloutVersion6 = servicepackage6.RolloutVersion;
            Verify.AreEqual(rolloutVersion5, rolloutVersion6);
            TestUtility.VerifyStoreLayout(buildLayoutInfo);

            // Test Case 6: Changing settings and image at the same time
            string jsonSettingFilePath5 = Path.Combine(currentExecutingDirectory, "Application_Setting3.json");
            string settingDescriptionJson5 = File.ReadAllText(jsonSettingFilePath5);
            SingleInstance.Application applicationWithSetting5 = JsonConvert.DeserializeObject<SingleInstance.Application>(settingDescriptionJson5);

            string applicationTypeVersion7 = "v7";
            this.imageBuilder.BuildSingleInstanceApplicationForUpgrade(
                applicationWithSetting5,
                applicationTypeName,
                applicationTypeVersion6,
                applicationTypeVersion7,
                applicationId,
                applicationInstance6.Version,
                applicationName,
                true, //generateDNS
                TimeSpan.MaxValue,
                tempBuildPath,
                outputPath,
                false, //use open network
                false, //use localnat network
                "C:\\",
                new SingleInstance.GenerationConfig());

            ApplicationInstanceType applicationInstance7 = TestUtility.GetApplicationInstance(buildLayoutInfo, applicationId, 7);
            string servicePackageFile7 = storeLayoutSpec.GetServicePackageFile(applicationTypeName, applicationId, "myBackendServicePkg", applicationInstance7.ServicePackageRef[0].RolloutVersion);
            var servicepackage7 = buildLayoutInfo.ImageStoreWrapper.GetFromStore<ServicePackageType>(servicePackageFile7, TestUtility.ImageStoreDefaultTimeout);
            Verify.AreEqual(servicepackage7.ManifestVersion, applicationTypeVersion7);

            // Code package version and config package version are set to target version applicationTypeVersion7
            foreach (var codePackage in servicepackage7.DigestedCodePackage)
            {
                Verify.AreEqual(applicationTypeVersion7, codePackage.CodePackage.Version);
            }
            foreach (var configPackage in servicepackage7.DigestedConfigPackage)
            {
                Verify.AreEqual(applicationTypeVersion7, configPackage.ConfigPackage.Version);
            }

            string rolloutVersion7 = servicepackage7.RolloutVersion;
            Verify.AreNotEqual(rolloutVersion6, rolloutVersion7);
            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateNetworkPoliciesForManifestGenerationForComposeScenario()
        {
            ApplicationTypeInformation applicationTypeInfo = new ApplicationTypeInformation("AppType", "1.0");
            Application app = new Application();
            app.name = "TestApp";
            Management.ImageBuilder.SingleInstance.CodePackage cpkg = new Management.ImageBuilder.SingleInstance.CodePackage();
            cpkg.image = "image";
            cpkg.name = "CodePackage";
            var endpoints = new List<Endpoint>();
            var endpoint = new Endpoint();
            endpoint.name = "publicListener";
            endpoint.port = 8080;
            endpoints.Add(endpoint);
            cpkg.endpoints = endpoints;
            var resource = new Resources();
            var req = new ResourceDescription();
            req.cpu = 0.5;
            req.memoryInGB = 1;
            resource.requests = req;
            cpkg.resources = resource;
            Service svc = new Service();
            svc.properties.codePackages = new List<Management.ImageBuilder.SingleInstance.CodePackage>();
            svc.properties.codePackages.Add(cpkg);
            svc.name = "Svc1";
            NetworkRef nref = new NetworkRef();
            nref.name = "Network1";
            EndpointRef endpointRef = new EndpointRef();
            endpointRef.name = "publicListener";
            nref.endpointRefs = new List<EndpointRef>() { endpointRef };
            svc.properties.networkRefs = new List<NetworkRef>();
            svc.properties.networkRefs.Add(nref);
            app.properties.services = new List<Service>();
            app.properties.services.Add(svc);

            string applicationTestDir = Path.Combine(TestUtility.TestDirectory, Guid.NewGuid().ToString());
            string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            this.imageBuilder.BuildSingleInstanceApplication(app, "AppType", "1.0", "TestApp", new Uri("fabric:/TestApp"), false, TimeSpan.MaxValue, applicationTestDir, null, true, false, null, new GenerationConfig());
            string expectedAppManifest = Path.Combine(applicationTestDir, "ApplicationManifest.xml");

            var validatingXmlReaderSettings = new XmlReaderSettings();
            validatingXmlReaderSettings.ValidationType = ValidationType.Schema;
            validatingXmlReaderSettings.Schemas.Add(StringConstants.FabricNamespace, Path.Combine(currentExecutingDirectory, StringConstants.DefaultSchemaPath));
            validatingXmlReaderSettings.XmlResolver = null;
            var applicationManifestType = ImageBuilderUtility.ReadXml<ApplicationManifestType>(expectedAppManifest, validatingXmlReaderSettings);

            ApplicationManifestTypeServiceManifestImport[] servicemanifestimports = applicationManifestType.ServiceManifestImport;
            foreach (var serviceManifestImport in servicemanifestimports)
            {
                var policies = serviceManifestImport.Policies;
                foreach (var policy in policies)
                {
                    if (policy.GetType() == typeof(NetworkPoliciesType))
                    {
                        var containerNetworkPolicyTypes = ((NetworkPoliciesType)policy).Items;
                        Verify.AreEqual("Network1", containerNetworkPolicyTypes[0].NetworkRef);
                        var containerNetworkPolicyEndpointBindingType = containerNetworkPolicyTypes[0].Items;
                        Verify.AreEqual("publicListener", containerNetworkPolicyEndpointBindingType[0].EndpointRef);
                    }
                }
            }
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateOpenNetworkPoliciesForManifestGenerationOnNatForComposeScenario()
        {
            ApplicationTypeInformation applicationTypeInfo = new ApplicationTypeInformation("AppType", "1.0");
            Application app = new Application();
            app.name = "TestApp";
            Management.ImageBuilder.SingleInstance.CodePackage cpkg = new Management.ImageBuilder.SingleInstance.CodePackage();
            cpkg.image = "image";
            cpkg.name = "CodePackage";
            var endpoints = new List<Endpoint>();
            var endpoint = new Endpoint();
            endpoint.name = "publicListener";
            endpoint.port = 8080;
            endpoints.Add(endpoint);
            cpkg.endpoints = endpoints;
            var resource = new Resources();
            var req = new ResourceDescription();
            req.cpu = 0.5;
            req.memoryInGB = 1;
            resource.requests = req;
            cpkg.resources = resource;
            Service svc = new Service();
            svc.properties.codePackages = new List<Management.ImageBuilder.SingleInstance.CodePackage>();
            svc.properties.codePackages.Add(cpkg);
            svc.name = "Svc1";
            NetworkRef nref = new NetworkRef();
            nref.name = StringConstants.OpenNetworkName;
            EndpointRef endpointRef = new EndpointRef();
            endpointRef.name = "publicListener";
            nref.endpointRefs = new List<EndpointRef>() { endpointRef };
            svc.properties.networkRefs = new List<NetworkRef>();
            svc.properties.networkRefs.Add(nref);
            app.properties.services = new List<Service>();
            app.properties.services.Add(svc);

            string applicationTestDir = Path.Combine(TestUtility.TestDirectory, Guid.NewGuid().ToString());
            string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            this.imageBuilder.BuildSingleInstanceApplication(app, "AppType", "1.0", "TestApp", new Uri("fabric:/TestApp"), false, TimeSpan.MaxValue, applicationTestDir, null, false, false, null, new GenerationConfig());
            string expectedAppManifest = Path.Combine(applicationTestDir, "ApplicationManifest.xml");

            var validatingXmlReaderSettings = new XmlReaderSettings();
            validatingXmlReaderSettings.ValidationType = ValidationType.Schema;
            validatingXmlReaderSettings.Schemas.Add(StringConstants.FabricNamespace, Path.Combine(currentExecutingDirectory, StringConstants.DefaultSchemaPath));
            validatingXmlReaderSettings.XmlResolver = null;
            var applicationManifestType = ImageBuilderUtility.ReadXml<ApplicationManifestType>(expectedAppManifest, validatingXmlReaderSettings);

            ApplicationManifestTypeServiceManifestImport[] servicemanifestimports = applicationManifestType.ServiceManifestImport;
            foreach (var serviceManifestImport in servicemanifestimports)
            {
                var policies = serviceManifestImport.Policies;
                foreach (var policy in policies)
                {
                    if (policy.GetType() == typeof(NetworkPoliciesType))
                    {
                        var containerNetworkPolicyTypes = ((NetworkPoliciesType)policy).Items;
                        Verify.AreEqual(StringConstants.NatNetworkName, containerNetworkPolicyTypes[0].NetworkRef);
                        var containerNetworkPolicyEndpointBindingType = containerNetworkPolicyTypes[0].Items;
                        Verify.AreEqual("publicListener", containerNetworkPolicyEndpointBindingType[0].EndpointRef);
                    }
                    if (policy.GetType() == typeof(ContainerHostPoliciesType))
                    {
                        var ContainerHostPoliciesTypeItems = ((ContainerHostPoliciesType)policy).Items;
                        foreach (var item in ContainerHostPoliciesTypeItems)
                        {
                            if (item.GetType() == typeof(PortBindingType))
                            {
                                var portBindingType = (PortBindingType)item;
                                Verify.AreEqual("publicListener", portBindingType.EndpointRef);
                                Verify.AreEqual(8080, portBindingType.ContainerPort);
                            }
                        }
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestValidateNATNetworkPoliciesForManifestGenerationOnOpenForComposeScenario()
        {
            ApplicationTypeInformation applicationTypeInfo = new ApplicationTypeInformation("AppType", "1.0");
            Application app = new Application();
            app.name = "TestApp";
            Management.ImageBuilder.SingleInstance.CodePackage cpkg = new Management.ImageBuilder.SingleInstance.CodePackage();
            cpkg.image = "image";
            cpkg.name = "CodePackage";
            var endpoints = new List<Endpoint>();
            var endpoint = new Endpoint();
            endpoint.name = "publicListener";
            endpoint.port = 8080;
            endpoints.Add(endpoint);
            cpkg.endpoints = endpoints;
            var resource = new Resources();
            var req = new ResourceDescription();
            req.cpu = 0.5;
            req.memoryInGB = 1;
            resource.requests = req;
            cpkg.resources = resource;
            Service svc = new Service();
            svc.properties.codePackages = new List<Management.ImageBuilder.SingleInstance.CodePackage>();
            svc.properties.codePackages.Add(cpkg);
            svc.name = "Svc1";
            NetworkRef nref = new NetworkRef();
            nref.name = StringConstants.NatNetworkName;
            EndpointRef endpointRef = new EndpointRef();
            endpointRef.name = "publicListener";
            nref.endpointRefs = new List<EndpointRef>() { endpointRef };
            svc.properties.networkRefs = new List<NetworkRef>();
            svc.properties.networkRefs.Add(nref);
            app.properties.services = new List<Service>();
            app.properties.services.Add(svc);

            string applicationTestDir = Path.Combine(TestUtility.TestDirectory, Guid.NewGuid().ToString());
            string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            this.imageBuilder.BuildSingleInstanceApplication(app, "AppType", "1.0", "TestApp", new Uri("fabric:/TestApp"), false, TimeSpan.MaxValue, applicationTestDir, null, true, false, null, new GenerationConfig());
            string expectedAppManifest = Path.Combine(applicationTestDir, "ApplicationManifest.xml");

            var validatingXmlReaderSettings = new XmlReaderSettings();
            validatingXmlReaderSettings.ValidationType = ValidationType.Schema;
            validatingXmlReaderSettings.Schemas.Add(StringConstants.FabricNamespace, Path.Combine(currentExecutingDirectory, StringConstants.DefaultSchemaPath));
            validatingXmlReaderSettings.XmlResolver = null;
            var applicationManifestType = ImageBuilderUtility.ReadXml<ApplicationManifestType>(expectedAppManifest, validatingXmlReaderSettings);

            ApplicationManifestTypeServiceManifestImport[] servicemanifestimports = applicationManifestType.ServiceManifestImport;
            foreach (var serviceManifestImport in servicemanifestimports)
            {
                var policies = serviceManifestImport.Policies;
                foreach (var policy in policies)
                {
                    if (policy.GetType() == typeof(ContainerHostPoliciesType))
                    {
                        var ContainerHostPoliciesTypeItems = ((ContainerHostPoliciesType)policy).Items;
                        foreach (var item in ContainerHostPoliciesTypeItems)
                        {
                            if (item.GetType() == typeof(PortBindingType))
                            {
                                var portBindingType = (PortBindingType)item;
                                Verify.AreEqual("publicListener", portBindingType.EndpointRef);
                                Verify.AreEqual(8080, portBindingType.ContainerPort);
                            }
                        }
                    }
                }
            }
        }
    }
}