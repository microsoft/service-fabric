// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ServiceModel;
    using System.IO;

    public class BuildLayoutInfo : IDisposable
    {
        static readonly string TestDirectory = TestUtility.TestDirectory;

        public ApplicationManifestType ApplicationManifestType { get; set; }
        public Collection<ServiceManifestType> ServiceManifestTypes { get; set; }
        public SettingsType SettingsType { get; set; }

        public string ApplicationFolderName { get; private set; }

        public string ApplicationManifestPath { get; set; }

        internal IImageStore ImageStore { get; private set; }

        internal ImageStoreWrapper ImageStoreWrapper { get; private set; }

        internal BuildLayoutSpecification BuildLayoutSpecification { get; set; }
        
        internal BuildLayoutInfo(IImageStore imageStore)
        {
            this.ImageStore = imageStore;
            this.ImageStoreWrapper = new ImageStoreWrapper(imageStore, TestDirectory);
            this.InitializeManifest();
        }

        internal BuildLayoutInfo(IImageStore imageStore, string applicationBuildPath)
        {
            this.ImageStore = imageStore;
            this.ImageStoreWrapper = new ImageStoreWrapper(imageStore, TestDirectory);
            this.InitializeManifest(applicationBuildPath);
        }

        public void InitializeManifest(string applicationBuildPath = null)
        {
            if (string.IsNullOrEmpty(applicationBuildPath))
            {
                this.ApplicationManifestType =
                    TestUtility.ReadXml<ApplicationManifestType>(Path.Combine(TestDirectory, "ApplicationManifest.xml"));

                this.ServiceManifestTypes = new Collection<ServiceManifestType>();
                this.ServiceManifestTypes.Add(
                    TestUtility.ReadXml<ServiceManifestType>(Path.Combine(TestDirectory, "ServiceManifest.xml")));

                this.SettingsType = TestUtility.ReadXml<SettingsType>(Path.Combine(TestDirectory, "Settings.xml"));

                this.GenerateApplicationFolderName();

                this.BuildLayoutSpecification = BuildLayoutSpecification.Create();
                this.BuildLayoutSpecification.SetRoot(this.ApplicationFolderName);

                this.ApplicationManifestPath = this.BuildLayoutSpecification.GetApplicationManifestFile();
            }
            else
            {
                this.ApplicationManifestType = 
                    TestUtility.ReadXml<ApplicationManifestType>(Path.Combine(applicationBuildPath, "ApplicationManifest.xml"));

                var servicePkgDirectories = Directory.EnumerateDirectories(applicationBuildPath);
                this.ServiceManifestTypes = new Collection<ServiceManifestType>();
                foreach (var directory in servicePkgDirectories)
                {
                    if (File.Exists(Path.Combine(directory, "ServiceManifest.xml")))
                    {
                        this.ServiceManifestTypes.Add(
                            TestUtility.ReadXml<ServiceManifestType>(Path.Combine(directory, "ServiceManifest.xml")));
                    }
                }

                this.BuildLayoutSpecification = BuildLayoutSpecification.Create();
                this.BuildLayoutSpecification.SetRoot(Path.GetDirectoryName(applicationBuildPath));

                this.ApplicationManifestPath = this.BuildLayoutSpecification.GetApplicationManifestFile();
            }
        }

        public string GetSfpkgFileName()
        {
            return string.Format("{0}.sfpkg", this.ApplicationFolderName);
        }
        
        public string GetApplicationFolderFullPath()
        {
            string rootUri = this.ImageStore.RootUri.Replace("file:", "");
            return Path.Combine(rootUri, this.ApplicationFolderName);
        }

        public string GenerateSfpkg(string sfPkgFolderPath, bool applyCompression)
        {
            string sfPkgFileName = this.GetSfpkgFileName();
            string rootUri = this.ImageStore.RootUri.Replace("file:", "");
            string buildLayoutPath = Path.Combine(rootUri, this.ApplicationFolderName);

            return ImageBuilderTest.GenerateSfpkgWithValidation(buildLayoutPath, sfPkgFolderPath, applyCompression, sfPkgFileName);
        }
        
        public void UpdateApplicationVersion(string applicationTypeVersion)
        {
            this.ApplicationManifestType.ApplicationTypeVersion = applicationTypeVersion;

            this.GenerateApplicationFolderName();
            this.BuildLayoutSpecification.SetRoot(this.ApplicationFolderName);
            this.ApplicationManifestPath = this.BuildLayoutSpecification.GetApplicationManifestFile();
        }

        public string GetServiceManifestPath(string serviceManifestTypeName)
        {
            return this.BuildLayoutSpecification.GetServiceManifestFile(serviceManifestTypeName);
        }

        public string GetServiceManifestFolder(string serviceManifestTypeName)
        {
            FileInfo serviceManifestFileInfo = new FileInfo(this.GetServiceManifestPath(serviceManifestTypeName));
            return serviceManifestFileInfo.DirectoryName;
        }

        public void DeleteBuildLayout()
        {
            this.ImageStore.DeleteContent(this.ApplicationFolderName, TimeSpan.MaxValue);            
        }

        public string CreateBuildLayout()
        {
            string tempDirectory = Path.Combine(TestDirectory, Guid.NewGuid().ToString());
            try
            {
                Directory.CreateDirectory(tempDirectory);

                BuildLayoutSpecification tempBuildLayoutSpec = BuildLayoutSpecification.Create();
                tempBuildLayoutSpec.SetRoot(tempDirectory);

                foreach (var serviceManifestType in this.ServiceManifestTypes)
                {
                    if (serviceManifestType.CodePackage != null)
                    {
                        foreach (CodePackageType codePackage in serviceManifestType.CodePackage)
                        {
                            string codeFolderName = tempBuildLayoutSpec.GetCodePackageFolder(
                                serviceManifestType.Name,
                                codePackage.Name);
                            string pathInCodeFolder = Path.Combine(codeFolderName, "Test Path");

                            Directory.CreateDirectory(codeFolderName);
                            Directory.CreateDirectory(pathInCodeFolder);

                            File.Copy(
                               Path.Combine(TestDirectory, "System.Fabric.Management.dll"),
                               Path.Combine(codeFolderName, string.Format("System.Fabric.Management.{0}.dll", serviceManifestType.Name)),
                               true);

                            File.Copy(
                                Path.Combine(TestDirectory, "System.Fabric.Management.dll"),
                                Path.Combine(codeFolderName, "Setup.exe"),
                                true);

                            File.Copy(
                                Path.Combine(TestDirectory, "System.Fabric.Management.dll"),
                                Path.Combine(pathInCodeFolder, "TestLib.exe"),
                                true);
                        }
                    }

                    if (serviceManifestType.ConfigPackage != null)
                    {
                        foreach (ConfigPackageType configPackage in serviceManifestType.ConfigPackage)
                        {
                            string configFolderName = tempBuildLayoutSpec.GetConfigPackageFolder(
                                serviceManifestType.Name,
                                configPackage.Name);

                            Directory.CreateDirectory(configFolderName);

                            if (this.SettingsType != null)
                            {
                                string settingsFileName = tempBuildLayoutSpec.GetSettingsFile(configFolderName);
                                TestUtility.WriteXml<SettingsType>(settingsFileName, this.SettingsType);
                            }
                            else
                            {
                                File.Copy(
                                    Path.Combine(TestDirectory, "System.Fabric.Management.dll"),
                                    Path.Combine(configFolderName, "MyConfig.xml"),
                                    true);
                            }
                        }
                    }

                    if (serviceManifestType.DataPackage != null)
                    {
                        foreach (DataPackageType dataPackage in serviceManifestType.DataPackage)
                        {
                            string dataFolderName = tempBuildLayoutSpec.GetDataPackageFolder(
                                serviceManifestType.Name,
                                dataPackage.Name);

                            Directory.CreateDirectory(dataFolderName);

                            File.Copy(
                                Path.Combine(TestDirectory, "System.Fabric.Management.dll"),
                                Path.Combine(dataFolderName, "MyData.exe"),
                                true);
                        }
                    }

                    TestUtility.WriteXml<ServiceManifestType>(
                        tempBuildLayoutSpec.GetServiceManifestFile(serviceManifestType.Name),
                        serviceManifestType);
                }

                TestUtility.WriteXml<ApplicationManifestType>(
                    tempBuildLayoutSpec.GetApplicationManifestFile(),
                    this.ApplicationManifestType);

                this.ImageStore.DeleteContent(this.ApplicationFolderName, TimeSpan.MaxValue);

                this.ImageStore.UploadContent(this.ApplicationFolderName, tempDirectory, TimeSpan.MaxValue, CopyFlag.AtomicCopy, false);
            }
            finally
            {
                if (Directory.Exists(tempDirectory))
                {
                    Directory.Delete(tempDirectory, true);
                }
            }

            return this.ApplicationFolderName;
        }

        public string CreateDiffBuildLayout(string[] includedComponentVersions)
        {
            string tempDirectory = Path.Combine(TestDirectory, "FabricUnitTests", Guid.NewGuid().ToString());

            try
            {
                Directory.CreateDirectory(tempDirectory);

                BuildLayoutSpecification tempBuildLayoutSpec = BuildLayoutSpecification.Create();
                tempBuildLayoutSpec.SetRoot(tempDirectory);

                foreach (var serviceManifestType in this.ServiceManifestTypes)
                {
                    if (!ShouldIncludeVersion(includedComponentVersions, serviceManifestType.Name, serviceManifestType.Version))
                    {
                        continue;
                    }

                    if (serviceManifestType.CodePackage != null)
                    {
                        foreach (CodePackageType codePackage in serviceManifestType.CodePackage)
                        {
                            if (!ShouldIncludeVersion(includedComponentVersions, codePackage.Name, codePackage.Version))
                            {
                                continue;
                            }

                            string codeFolderName = tempBuildLayoutSpec.GetCodePackageFolder(
                                serviceManifestType.Name,
                                codePackage.Name);
                            string pathInCodeFolder = Path.Combine(codeFolderName, "Test Path");

                            Directory.CreateDirectory(codeFolderName);
                            Directory.CreateDirectory(pathInCodeFolder);

                            File.Copy(
                               Path.Combine(TestDirectory, "System.Fabric.Management.dll"),
                               Path.Combine(codeFolderName, string.Format("System.Fabric.Management.{0}.dll", serviceManifestType.Name)),
                               true);

                            File.Copy(
                                Path.Combine(TestDirectory, "System.Fabric.Management.dll"),
                                Path.Combine(codeFolderName, "Setup.exe"),
                                true);

                            File.Copy(
                                Path.Combine(TestDirectory, "System.Fabric.Management.dll"),
                                Path.Combine(pathInCodeFolder, "TestLib.exe"),
                                true);
                        }
                    }

                    if (serviceManifestType.ConfigPackage != null)
                    {
                        foreach (ConfigPackageType configPackage in serviceManifestType.ConfigPackage)
                        {
                            if (!ShouldIncludeVersion(includedComponentVersions, configPackage.Name, configPackage.Version))
                            {
                                continue;
                            }

                            string configFolderName = tempBuildLayoutSpec.GetConfigPackageFolder(
                                serviceManifestType.Name,
                                configPackage.Name);

                            Directory.CreateDirectory(configFolderName);

                            if (this.SettingsType != null)
                            {
                                string settingsFileName = tempBuildLayoutSpec.GetSettingsFile(configFolderName);
                                TestUtility.WriteXml<SettingsType>(settingsFileName, this.SettingsType);
                            }
                            else
                            {
                                File.Copy(
                                    Path.Combine(TestDirectory, "System.Fabric.Management.dll"),
                                    Path.Combine(configFolderName, "MyConfig.xml"),
                                    true);
                            }
                        }
                    }

                    if (serviceManifestType.DataPackage != null)
                    {
                        foreach (DataPackageType dataPackage in serviceManifestType.DataPackage)
                        {
                            if (!ShouldIncludeVersion(includedComponentVersions, dataPackage.Name, dataPackage.Version))
                            {
                                continue;
                            }

                            string dataFolderName = tempBuildLayoutSpec.GetDataPackageFolder(
                                serviceManifestType.Name,
                                dataPackage.Name);

                            Directory.CreateDirectory(dataFolderName);

                            File.Copy(
                                Path.Combine(TestDirectory, "System.Fabric.Management.dll"),
                                Path.Combine(dataFolderName, "MyData.exe"),
                                true);
                        }
                    }

                    string serviceManifestFilePath = tempBuildLayoutSpec.GetServiceManifestFile(serviceManifestType.Name);
                    string serviceManifestDirectory = Path.GetDirectoryName(serviceManifestFilePath);
                    if (!Directory.Exists(serviceManifestDirectory)) { Directory.CreateDirectory(serviceManifestDirectory); }

                    TestUtility.WriteXml<ServiceManifestType>(
                            tempBuildLayoutSpec.GetServiceManifestFile(serviceManifestType.Name),
                            serviceManifestType);
                }

                string appManifestFilePath = tempBuildLayoutSpec.GetApplicationManifestFile();
                string appManifestDirectory = Path.GetDirectoryName(appManifestFilePath);
                if (!Directory.Exists(appManifestDirectory)) { Directory.CreateDirectory(appManifestDirectory); }

                TestUtility.WriteXml<ApplicationManifestType>(
                    tempBuildLayoutSpec.GetApplicationManifestFile(),
                    this.ApplicationManifestType);

                this.ImageStore.DeleteContent(this.ApplicationFolderName, TimeSpan.MaxValue);
                this.ImageStore.UploadContent(this.ApplicationFolderName, tempDirectory, TimeSpan.MaxValue, CopyFlag.AtomicCopy, false);
            }
            finally
            {
                if (Directory.Exists(tempDirectory))
                {
                    Directory.Delete(tempDirectory, true);
                }
            }

            return this.ApplicationFolderName;
        }
              
        public void Dispose()
        {
            this.DeleteBuildLayout();
        }

        private static bool ShouldIncludeVersion(string[] includedComponentVersions, string componentName, string componentVersion)
        {
            foreach (var version in includedComponentVersions)
            {
                if (componentVersion == version)
                {
                    Console.WriteLine("CreateDiffBuildLayout: Add {0} ({1})", componentName, componentVersion);
                    return true;
                }
            }

            Console.WriteLine("CreateDiffBuildLayout: Skip {0} ({1})", componentName, componentVersion);
            return false;
        }

        private void GenerateApplicationFolderName()
        {
            this.ApplicationFolderName = this.ApplicationManifestType.ApplicationTypeName + "+" + this.ApplicationManifestType.ApplicationTypeVersion;
        }
    }
}