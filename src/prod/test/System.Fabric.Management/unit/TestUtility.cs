// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using Microsoft.WindowsAzure.Storage;

    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
#if !DotNetCoreClr
    using System.Management.Automation.Runspaces;
#endif
    using System.Net;
    using System.Reflection;
    using System.Security;
    using System.Security.Permissions;
#if !DotNetCoreClr
    using System.Security.Policy;
#endif
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.WindowsAzure.Storage.Blob;

    using KeyVault.Wrapper;

    internal class TestUtility
    {
        public static readonly string TestResourcesStorageAccountName = "sftestresources";

        public static readonly string TestResourcesStorageAccountKeyName = "sftestresources-StorageKey";

        public static readonly string TestResourcesTempContainerName = "tempuploads";

        public static string TestDirectory = new FileInfo(Assembly.GetExecutingAssembly().Location).DirectoryName;

#if !DotNetCoreClr // unused variable warning in coreclr.
        public static readonly string ConfigurationCsvFilePath = Path.Combine(TestUtility.TestDirectory, "Configurations.csv");

        private static readonly string FabricNamespace = "http://schemas.microsoft.com/2011/01/fabric";
#endif

        public static readonly string XStoreAccountName = "winfabcit";

        public static readonly string XStoreContainerName = string.Format("sfmt-{0}-{1}", Environment.UserName, Environment.MachineName).ToLower();

        public static readonly string LogicalDirectoriesBackupDir = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "jbod", "backup");

        public static readonly string LogicalDirectoriesLogDir = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "jbod", "log");

        public static readonly string LogicalDirectoriesApplicationCheckPointFilesDir = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "jbod", "applicationCheckPoint");

        public static readonly string LogicalDirectoriesUserDefined1Dir = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "jbod", "userDefined1");

        public static readonly string LogicalDirectoriesUserDefined2Dir = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "jbod", "userDefined2");

        public static readonly string LogicalDirectoriesBackupDir2 = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "jbod", "backup2");

        public static readonly string LogicalDirectoriesLogDir2 = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "jbod", "log2");

        public static readonly string LogicalDirectoriesApplicationCheckPointFilesDir2 = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "jbod", "applicationCheckPoint2");

        public static readonly string LogicalDirectoriesUserDefined1Dir2 = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "jbod", "userDefined3");

        public static readonly string LogicalDirectoriesUserDefined2Dir2 = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "jbod", "userDefined4");

        internal static readonly TimeSpan ImageStoreDefaultTimeout = TimeSpan.FromMinutes(10);

#if !DotNetCoreClr
        public static Runspace PowershellRunspace;

        static TestUtility()
        {
            var sessionState = InitialSessionState.CreateDefault();
            var location = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            Assert.IsNotNull(location, "Current assembly location");

            var powershellAssemblyLocation = Path.Combine(location, "Microsoft.ServiceFabric.Powershell.dll");
            Assert.IsTrue(File.Exists(powershellAssemblyLocation), "Powershell assembly location");

            sessionState.ImportPSModule(new[] { powershellAssemblyLocation });

            TestUtility.PowershellRunspace = RunspaceFactory.CreateRunspace(sessionState);
            TestUtility.PowershellRunspace.Open();
        }

        public static T ReadXml<T>(string path)
        {
            using (FileStream fileStream = new FileStream(path, FileMode.Open))
            {
                XmlUrlResolver resolver = new XmlUrlResolver();
                resolver.Credentials = CredentialCache.DefaultCredentials;
                PermissionSet ps = new PermissionSet(PermissionState.None);
                ps.AddPermission(new FileIOPermission(FileIOPermissionAccess.Read, path));

                XmlReaderSettings readerSettings = new XmlReaderSettings();
                readerSettings.XmlResolver = new XmlSecureResolver(resolver, ps);
                readerSettings.ValidationType = ValidationType.Schema;
                readerSettings.Schemas.Add(FabricNamespace, Path.Combine(TestUtility.TestDirectory, "ServiceFabricServiceModel.xsd"));

                XmlSerializer xmlSerializer = new XmlSerializer(typeof(T));
                XmlReader reader = XmlReader.Create(fileStream, readerSettings);

                return (T)xmlSerializer.Deserialize(reader);
            }
        }

        public static void WriteXml<T>(string path, T value)
        {
            using (FileStream fileStream = new FileStream(path, FileMode.Create))
            {
                XmlSerializer xmlSerializer = new XmlSerializer(typeof(T));
                xmlSerializer.Serialize(fileStream, value);
            }
        }
#endif

        public static IImageStore GetFileImageStore(string storeRoot)
        {
            string path = "file:" + Path.Combine(storeRoot);

            if (Directory.Exists(storeRoot)) { Directory.Delete(storeRoot, true); }

            Directory.CreateDirectory(storeRoot);

            return ImageStoreFactoryProxy.CreateImageStore(path);
        }

#if !DotNetCoreClr
        public static string GetXStoreTestResourcesConnectionString()
        {
            var keyVaultUri = KeyVaultWrapper.GetTestInfraKeyVaultUri();
            var kvw = KeyVaultWrapper.GetTestInfraKeyVaultWrapper();
            var storageKey = kvw.GetSecret(keyVaultUri, TestUtility.TestResourcesStorageAccountKeyName);
            return string.Format(
                CultureInfo.InvariantCulture,
                "DefaultEndpointsProtocol=https;AccountName={0};AccountKey={1}",
                TestUtility.TestResourcesStorageAccountName,
                storageKey);
        }

        public static string UploadFileToXStoreTempTestResources(string srcFilePath, string blobName)
        {
            var storageConnectionString = TestUtility.GetXStoreTestResourcesConnectionString();

            var storageAccount = CloudStorageAccount.Parse(storageConnectionString);
            var blobClient = storageAccount.CreateCloudBlobClient();
            var container = blobClient.GetContainerReference(TestUtility.TestResourcesTempContainerName);

            if (container.Exists())
            {
                var oldBlob = container.GetBlockBlobReference(blobName);
                oldBlob.DeleteIfExists();
            }

            container.CreateIfNotExists();
            container.SetPermissions(
                new BlobContainerPermissions { PublicAccess = BlobContainerPublicAccessType.Blob });

            var blob = container.GetBlockBlobReference(blobName);
            using (var stream = FabricFile.Open(srcFilePath, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                blob.UploadFromStreamAsync(stream).Wait();
            }

            return blob.Uri.OriginalString;
        }

        public static bool DeleteXStoreTempTestResourcesBlob(string blobName)
        {
            var storageConnectionString = TestUtility.GetXStoreTestResourcesConnectionString();

            var storageAccount = CloudStorageAccount.Parse(storageConnectionString);
            var blobClient = storageAccount.CreateCloudBlobClient();
            var container = blobClient.GetContainerReference(TestUtility.TestResourcesTempContainerName);
            if (container.Exists())
            {
                var blob = container.GetBlockBlobReference(blobName);
                return blob.DeleteIfExists();
            }

            return false;
        }

#endif
        public static string GetAzureImageStoreConnectionString(string accountName, string containerName)
        {
            const string KeyVaultClientId = "33e3af95-cf38-49be-9db9-909bb5b3cb2e";
            const string KeyVaultCliedIdAppCertThumbprint = "6108D2170756888910B43787F0FE8C6DD9162FE7";
            const string KeyVaultUri = "https://TestInfraKeyVault.vault.azure.net";
            const string WinFabCitStoreAccountKeySecretName = "AccountKey-winfabcit";

            var kvw = new KeyVaultWrapper(KeyVaultClientId, KeyVaultCliedIdAppCertThumbprint);
            var accountKey = kvw.GetSecret(KeyVaultUri, WinFabCitStoreAccountKeySecretName);
            return string.Format(
                CultureInfo.InvariantCulture,
                "xstore:DefaultEndpointsProtocol=https;AccountName={0};AccountKey={1};Container={2}",
                accountName,
                accountKey,
                containerName);
        }

        public static IImageStore GetAzureImageStore(string accountName, string containerName)
        {
            var path = TestUtility.GetAzureImageStoreConnectionString(accountName, containerName);
            // When the image store type is XStore, XStoreImageStore should be created directly from System.Fabric.Managemnt to avoid reflection 
            return new System.Fabric.Management.ImageStore.XStoreImageStore(path, null);
        }

#if !DotNetCoreClr
        public static void ModifyCodePackage(string codePackageName, ServiceManifestType serviceManifestType, BuildLayoutInfo buildLayoutInfo)
        {
            string codeFolderName = buildLayoutInfo.BuildLayoutSpecification.GetCodePackageFolder(
                        serviceManifestType.Name,
                        codePackageName);

            buildLayoutInfo.ImageStore.UploadContent(
                Path.Combine(codeFolderName, string.Format("AdditionalAssembly.dll", serviceManifestType.Name)),
                Path.Combine(TestDirectory, "System.Fabric.Management.dll"),
                TimeSpan.MaxValue,
                CopyFlag.AtomicCopy,
                false);
        }

        public static void VerifyStoreLayout(BuildLayoutInfo buildLayoutInfo)
        {
            string applicationPathStoreLayout = Path.Combine("Store", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName);

            var fileList = buildLayoutInfo.ImageStore.ListContent(applicationPathStoreLayout, TimeSpan.MaxValue);
            bool containsFabricAssembly = fileList.Any(file => file.EndsWith("FabricCommon.dll"));
            Assert.IsFalse(containsFabricAssembly, "StoreLayout should not contain fabric assembly");

            string applicationManifestFile = Path.Combine(applicationPathStoreLayout, string.Format("ApplicationManifest.{0}.xml", buildLayoutInfo.ApplicationManifestType.ApplicationTypeVersion));
            Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(applicationManifestFile, TimeSpan.MaxValue), applicationManifestFile);

            foreach (ServiceManifestType serviceManifestType in buildLayoutInfo.ServiceManifestTypes)
            {
                string serviceManifestFile = Path.Combine(applicationPathStoreLayout, string.Format("{0}.Manifest.{1}.xml", serviceManifestType.Name, serviceManifestType.Version));
                Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(serviceManifestFile, TimeSpan.MaxValue), applicationPathStoreLayout);

                string serviceManifestChecksumPath = string.Format("{0}.{1}", serviceManifestFile, "checksum");
                Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(serviceManifestChecksumPath, TimeSpan.MaxValue), serviceManifestChecksumPath);

                if (serviceManifestType.CodePackage != null)
                {
                    foreach (var codePackage in serviceManifestType.CodePackage)
                    {
                        if (codePackage.EntryPoint.Item is ContainerHostEntryPointType)
                        {
                            continue;
                        }

                        string codePackagePath = Path.Combine(applicationPathStoreLayout, string.Format("{0}.{1}.{2}", serviceManifestType.Name, codePackage.Name, codePackage.Version));
                        Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(codePackagePath, TimeSpan.MaxValue), codePackagePath);

                        string checksumPath = string.Format("{0}.{1}", codePackagePath, "checksum");
                        Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(checksumPath, TimeSpan.MaxValue), checksumPath);
                    }
                }

                if (serviceManifestType.DataPackage != null)
                {
                    foreach (var dataPackage in serviceManifestType.DataPackage)
                    {
                        string dataPackagePath = Path.Combine(applicationPathStoreLayout, string.Format("{0}.{1}.{2}", serviceManifestType.Name, dataPackage.Name, dataPackage.Version));
                        Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(dataPackagePath, TimeSpan.MaxValue), dataPackagePath);

                        string checksumPath = string.Format("{0}.{1}", dataPackagePath, "checksum");
                        Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(checksumPath, TimeSpan.MaxValue), checksumPath);
                    }
                }

                if (serviceManifestType.ConfigPackage != null)
                {
                    foreach (var configPackage in serviceManifestType.ConfigPackage)
                    {
                        string configPackagePath = Path.Combine(applicationPathStoreLayout, string.Format("{0}.{1}.{2}", serviceManifestType.Name, configPackage.Name, configPackage.Version));
                        Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(configPackagePath, TimeSpan.MaxValue), configPackagePath);

                        string settingPath = Path.Combine(configPackagePath, "Settings.xml");
                        Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(settingPath, TimeSpan.MaxValue), settingPath);

                        string checksumPath = string.Format("{0}.{1}", configPackagePath, "checksum");
                        Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(checksumPath, TimeSpan.MaxValue), checksumPath);
                    }
                }
            }
        }

        public static void VerifyWinFabLayout(FabricLayoutInfo fabircLayoutInfo)
        {
            string winFabLayoutPath = "WindowsFabricStore";

            if (fabircLayoutInfo.ClusterManifest != null)
            {
                string configPath = Path.Combine(winFabLayoutPath, string.Format("ClusterManifest.{0}.xml", fabircLayoutInfo.ClusterManifest.Version));
                Assert.IsTrue(fabircLayoutInfo.ImageStore.DoesContentExist(configPath, TimeSpan.MaxValue));
            }

            if (!string.IsNullOrEmpty(fabircLayoutInfo.PatchVersion))
            {
                if (fabircLayoutInfo.UseMSI)
                {
                    string codePath = Path.Combine(winFabLayoutPath, string.Format("WindowsFabric.{0}.msi", fabircLayoutInfo.PatchVersion));
                    Assert.IsTrue(fabircLayoutInfo.ImageStore.DoesContentExist(codePath, TimeSpan.MaxValue));
                }
                else
                {
                    string codePackageFolder = Path.Combine(winFabLayoutPath, string.Format(FabricLayoutInfo.WindowsFabricCodePackageFolderNamePattern, fabircLayoutInfo.PatchVersion));
                    Assert.IsTrue(fabircLayoutInfo.ImageStore.DoesContentExist(codePackageFolder, TimeSpan.MaxValue));
                }
            }
        }

        public static void VerifyApplicationDigestLayout(BuildLayoutInfo buildLayoutInfo, string applicationId, int applicationInstanceVersion, RolloutVersion rolloutVersion)
        {
            string applicationDigestPath = Path.Combine("Store", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName, "apps", applicationId);

            string applicationInstanceFile = Path.Combine(applicationDigestPath, string.Format("ApplicationInstance.{0}.xml", applicationInstanceVersion));
            Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(applicationInstanceFile, TimeSpan.MaxValue));

            string applicationPackageFile = Path.Combine(applicationDigestPath, string.Format("ApplicationPackage.{0}.xml", rolloutVersion));
            Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(applicationPackageFile, TimeSpan.MaxValue));

            foreach (var serviceManifestType in buildLayoutInfo.ServiceManifestTypes)
            {
                string servicePackageFile = Path.Combine(applicationDigestPath, string.Format("{0}.Package.{1}.xml", serviceManifestType.Name, rolloutVersion));
                Assert.IsTrue(buildLayoutInfo.ImageStore.DoesContentExist(servicePackageFile, TimeSpan.MaxValue));
            }
        }

        public static ApplicationInstanceType GetApplicationInstance(BuildLayoutInfo buildLayoutInfo, string applicationId, int applicationInstanceVersion, RolloutVersion rolloutVersion = null)
        {
            string applicationDigestPath = Path.Combine("Store", buildLayoutInfo.ApplicationManifestType.ApplicationTypeName, "apps", applicationId);
            string applicationInstanceFile = Path.Combine(applicationDigestPath, string.Format("ApplicationInstance.{0}.xml", applicationInstanceVersion));
            return buildLayoutInfo.ImageStoreWrapper.GetFromStore<ApplicationInstanceType>(applicationInstanceFile, TestUtility.ImageStoreDefaultTimeout);
        }

        public static void AddToFabricSettings(
            string sectionName,
            string parameterName,
            string parameterValue,
            bool isEncrypted,
            ClusterManifestType clusterManifest)
        {
            SettingsOverridesTypeSection matchingSection = clusterManifest.FabricSettings.FirstOrDefault(section => section.Name == sectionName);
            if (matchingSection == null)
            {
                Collection<SettingsOverridesTypeSection> sections = new Collection<SettingsOverridesTypeSection>();
                foreach (var section in clusterManifest.FabricSettings)
                {
                    sections.Add(section);
                }

                matchingSection = new SettingsOverridesTypeSection() { Name = sectionName, Parameter = new SettingsOverridesTypeSectionParameter[0] };
                sections.Add(matchingSection);

                clusterManifest.FabricSettings = sections.ToArray();
            }

            SettingsOverridesTypeSectionParameter matchingParameter = matchingSection.Parameter.FirstOrDefault(parameter => parameter.Name == parameterName);
            if (matchingParameter == null)
            {
                Collection<SettingsOverridesTypeSectionParameter> parameters = new Collection<SettingsOverridesTypeSectionParameter>();
                foreach (var parameter in matchingSection.Parameter)
                {
                    parameters.Add(parameter);
                }

                matchingParameter = new SettingsOverridesTypeSectionParameter() { Name = parameterName, Value = parameterValue, IsEncrypted = isEncrypted };
                parameters.Add(matchingParameter);

                matchingSection.Parameter = parameters.ToArray();
            }
            else
            {
                matchingParameter.Value = parameterValue;
                matchingParameter.IsEncrypted = isEncrypted;
            }
        }
#endif
    }
}