// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System.Diagnostics;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ServiceModel;
    using System.IO;

    class FabricLayoutInfo : IDisposable
    {
        static readonly string TestDirectory = TestUtility.TestDirectory;
        static readonly string RelativePath = "IncomingWindowsFabric";
       
        static readonly string InfrastructureManifestFileName = "InfrastructureManifest.xml";        
        static readonly string ClusterManifestFileName = "ClusterManifest.xml";        
        static readonly string WindowsFabricMSIFileName = "WindowsFabric.{0}.msi";
        static readonly string[] WindowsFabricCodePackageChildrenFolders = { "bin", "Eula", "FabricInstallerService.Code"};
        public static readonly string WindowsFabricCodePackageFolderNamePattern = "WindowsFabric.{0}";

        public ClusterManifestType ClusterManifest { get; set; }
        public InfrastructureInformationType InfrastructureManifest { get; set; }
        public string PatchVersion { get; set; }        
        public bool UseMSI { get; private set; }
        internal IImageStore ImageStore { get; private set; }
        internal ImageStoreWrapper ImageStoreWrapper { get; private set; }

        private string fullInfrastructureManifestFolderName;

        internal FabricLayoutInfo(IImageStore imageStore, string clusterManifestFileName)
            : this(imageStore, clusterManifestFileName, true)
        {
        }

        internal FabricLayoutInfo(IImageStore imageStore)
            : this(imageStore, "ClusterManifest-AllSettings.xml", true)
        {
        }

        internal FabricLayoutInfo(IImageStore imageStore, string clusterManifestFileName, bool useMSI)
        {
            this.ImageStore = imageStore;
            this.ImageStoreWrapper = new ImageStoreWrapper(imageStore, TestDirectory);
            this.fullInfrastructureManifestFolderName = Path.Combine(TestUtility.TestDirectory, Path.GetRandomFileName());
            this.UseMSI = useMSI;
            InitializeManifest(clusterManifestFileName);
        }

        public void Create(out string codePath, out string configPath, out string infrastructureManifestPath)
        {
            codePath = string.Empty;
            configPath = string.Empty;
            infrastructureManifestPath = string.Empty;

            Directory.CreateDirectory(this.fullInfrastructureManifestFolderName);

            if (this.ClusterManifest != null)
            {
                configPath = Path.Combine(FabricLayoutInfo.RelativePath, ClusterManifestFileName);
                this.ImageStoreWrapper.SetToStore<ClusterManifestType>(configPath, this.ClusterManifest, TestUtility.ImageStoreDefaultTimeout);
            }

            if (this.InfrastructureManifest != null)
            {
                infrastructureManifestPath = Path.Combine(this.fullInfrastructureManifestFolderName, InfrastructureManifestFileName);
                TestUtility.WriteXml<InfrastructureInformationType>(infrastructureManifestPath, this.InfrastructureManifest);
            }

            if (!string.IsNullOrEmpty(this.PatchVersion))
            {
                if (this.UseMSI)
                {
                    codePath = Path.Combine(FabricLayoutInfo.RelativePath, string.Format(WindowsFabricMSIFileName, this.PatchVersion));
                    this.ImageStore.DeleteContent(codePath, TestUtility.ImageStoreDefaultTimeout);                                        
                    this.ImageStore.UploadContent(
                        codePath,
                        Path.Combine(TestDirectory, "System.Fabric.Management.Test.dll.cfg"),
                        TestUtility.ImageStoreDefaultTimeout,
                        CopyFlag.AtomicCopy,
                        false);
                }
                else
                {
                    // Create a dummy xcopy package and copy fabric.exe into it.
                    var fabricCodePackageFolderName = string.Format(FabricLayoutInfo.WindowsFabricCodePackageFolderNamePattern, this.PatchVersion);

                    string tempDirectory = Path.Combine(TestDirectory, Guid.NewGuid().ToString());
                    var tempFabricCodePackageFolderPath = Path.Combine(tempDirectory, fabricCodePackageFolderName);
                    try
                    {
                        foreach (string childFolder in WindowsFabricCodePackageChildrenFolders)
                        {
                            string childFolderPath = Path.Combine(tempFabricCodePackageFolderPath, childFolder);
                            if (!Directory.Exists(childFolderPath))
                            {
                                Directory.CreateDirectory(childFolderPath);
                                File.Copy(
                                    Path.Combine(FabricLayoutInfo.TestDirectory, "Fabric.exe"),
                                    Path.Combine(childFolderPath, "Fabric.exe"),
                                    true);
                            }
                        }

                        codePath = Path.Combine(RelativePath, fabricCodePackageFolderName);
                        this.ImageStore.DeleteContent(codePath, TestUtility.ImageStoreDefaultTimeout);
                        this.ImageStore.UploadContent(
                            codePath,
                            tempFabricCodePackageFolderPath,
                            TestUtility.ImageStoreDefaultTimeout,
                            CopyFlag.AtomicCopy,
                            false);
                    }
                    finally
                    {
                        if (Directory.Exists(tempDirectory))
                        {
                            Directory.Delete(tempDirectory, true);
                        }
                    }
                }
            }
        }

        public void Delete()
        {
            this.ImageStore.DeleteContent(FabricLayoutInfo.RelativePath, TestUtility.ImageStoreDefaultTimeout);

            if (Directory.Exists(this.fullInfrastructureManifestFolderName))
            {
                Directory.Delete(this.fullInfrastructureManifestFolderName, true);
            }
        }

        public void Dispose()
        {
            this.Delete();
        }

        private void InitializeManifest(string clusterManifestFileName)
        {
            if (clusterManifestFileName == "ClusterManifest-AllSettings.xml")
            {
                this.ClusterManifest = new ClusterManifestHelper(true, true).ClusterManifest;
            }
            else
            {
                this.ClusterManifest = TestUtility.ReadXml<ClusterManifestType>(Path.Combine(TestDirectory, clusterManifestFileName));
            }
            this.InfrastructureManifest = TestUtility.ReadXml<InfrastructureInformationType>(Path.Combine(TestDirectory, "InfrastructureManifest.xml"));
            this.PatchVersion = this.GetVersion(Path.Combine(TestDirectory, "Fabric.exe"));
        }

        private string GetVersion(string fileName)
        {
            var productVersionString = FileVersionInfo.GetVersionInfo(fileName).ProductVersion;
            Version productVersion = null;
            if (!Version.TryParse(productVersionString, out productVersion))
            {
                throw new InvalidDataException(string.Format("Invalid product version {0}", productVersionString));
            }

            return productVersion.ToString();
        }
    }
}