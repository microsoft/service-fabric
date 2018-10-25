// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.AzureCliProxy
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.ImageStore;
    using System.Fabric.Management;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ImageStore;
    using System.Fabric.Security;
    using System.Globalization;
    using System.IO;
    using System.Security.Cryptography.X509Certificates;

    class AzureCliProxy
    {
        private static readonly TimeSpan DefaultTimeSpan = TimeSpan.FromMinutes(29);

        public void CopyApplicationPackage(string connectionEndpoint, string applicationPackagePath, string imageStoreConnectionString, string relativeDestinationPath, string certThumbprint)
        {
            IImageStore imageStore = CreateImageStore(connectionEndpoint, imageStoreConnectionString, Path.GetTempPath(), certThumbprint);

            if (string.IsNullOrEmpty(relativeDestinationPath))
            {
                relativeDestinationPath = new DirectoryInfo(applicationPackagePath).Name;
            }

            ImageBuilderUtility.ValidateRelativePath(relativeDestinationPath);

            imageStore.UploadContent(
                relativeDestinationPath,
                applicationPackagePath,
                AzureCliProxy.DefaultTimeSpan,
                CopyFlag.AtomicCopy,
                false);
        }

        public void RemoveApplicationPackage(string connectionEndpoint, string imageStoreConnectionString, string applicationPackagePath, string certThumbprint)
        {
            IImageStore imageStore = this.CreateImageStore(connectionEndpoint, imageStoreConnectionString, Path.GetTempPath(), certThumbprint);

            imageStore.DeleteContent(applicationPackagePath, AzureCliProxy.DefaultTimeSpan);
        }

        public void TestApplicationPackage(string connectionEndpoint, string applicationPackagePath, string applicationParameter, string imageStoreConnectionString, string certThumbprint)
        {
            string[] parameters = applicationParameter.Split(',');
            IDictionary<string, string> dictionary = new Dictionary<string, string>();
            for (int i = 0; i < parameters.Length / 2; i += 2)
            {
                dictionary.Add(parameters[i], parameters[i + 1]);
            }

            var testRootPath = Path.Combine(Path.GetTempPath(), string.Format(CultureInfo.InvariantCulture, "{0}_{1}", "TestApplicationPackage", DateTime.Now.Ticks));

            try
            {
                var applicationName = new DirectoryInfo(applicationPackagePath).Name;
                var testRootApplicationPath = Path.Combine(testRootPath, applicationName);
                FabricDirectory.Copy(applicationPackagePath, testRootApplicationPath, true);
                ImageBuilderUtility.RemoveReadOnlyFlag(testRootApplicationPath);

                var fileImageStoreConnectionString = string.Format(CultureInfo.InvariantCulture, "{0}{1}", "file:", testRootPath.TrimEnd(Path.DirectorySeparatorChar));
                var sourceImageStore = ImageStoreFactoryProxy.CreateImageStore(fileImageStoreConnectionString);

                IImageStore destinationImageStore = null;
                if (string.IsNullOrEmpty(imageStoreConnectionString))
                {
                    destinationImageStore = sourceImageStore;
                }
                else
                {
                    destinationImageStore = this.CreateImageStore(connectionEndpoint, imageStoreConnectionString, testRootPath, certThumbprint);
                }

                var imagebuilder = new ImageBuilder(sourceImageStore, destinationImageStore, this.GetFabricFilePath("ServiceFabricServiceModel.xsd"), testRootPath);

                imagebuilder.ValidateApplicationPackage(
                    applicationName,
                    dictionary);
            }
            catch (Exception e)
            {
                throw e;
            }
            finally
            {
                FabricDirectory.Delete(testRootPath, true, true);
            }
        }

        private void InvokeEncryptText(string text, string path, string algorithmOid)
        {
            Console.WriteLine(EncryptionUtility.EncryptTextByCertFile(text, path, algorithmOid));
        }

        private void InvokeDecryptText(string cipherText)
        {
            Console.WriteLine(EncryptionUtility.DecryptText(cipherText));
        }

        private IImageStore CreateImageStore(string connectionEndpoint, string imageStoreConnectionString, string workingDirectory, string certThumbprint)
        {
            if (!imageStoreConnectionString.StartsWith("file:", StringComparison.OrdinalIgnoreCase)
                && !imageStoreConnectionString.StartsWith("fabric:", StringComparison.OrdinalIgnoreCase))
            {
                throw new ArgumentException();
            }

            string[] connectionEndpoints = { connectionEndpoint };
            X509Credentials securityCredentials = null;
            if (certThumbprint != null && certThumbprint.Length > 0)
            {
                securityCredentials = new X509Credentials();
                securityCredentials.RemoteCertThumbprints.Add(certThumbprint);
                securityCredentials.FindType = X509FindType.FindByThumbprint;
                securityCredentials.FindValue = certThumbprint;
                securityCredentials.StoreLocation = StoreLocation.CurrentUser;
                securityCredentials.StoreName = "MyStore";
            }

            IImageStore imageStore = ImageStoreFactoryProxy.CreateImageStore(
                imageStoreConnectionString,
                null,
                connectionEndpoints,
                securityCredentials,
                workingDirectory,
                false);

            return imageStore;
        }

        private string GetFabricFilePath(string fileName)
        {
            var fabricfilePath = Path.Combine(FabricEnvironment.GetCodePath(), fileName);
            if (string.IsNullOrEmpty(fabricfilePath) || !File.Exists(fabricfilePath))
            {
                throw new FileNotFoundException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        "FabricCodeFileNotFound: {0}", fabricfilePath));
            }

            return fabricfilePath;
        }

        static int Main(string[] args)
        {
            string command = string.Empty;

            try
            {
                command = args[0].ToLower();
                AzureCliProxy azureCliProxy = new AzureCliProxy();
                switch (command)
                {
                    case "copyapplicationpackage":
                        azureCliProxy.CopyApplicationPackage(args[1], args[2], args[3], args.Length >= 5 ? args[4] : null, args.Length >= 6 ? args[5] : null);

                        break;
                    case "testapplicationpackage":
                        azureCliProxy.TestApplicationPackage(args[1], args[2], args.Length >= 4 ? args[3] : null, args.Length >= 5 ? args[4] : null, args.Length >= 6 ? args[5] : null);

                        break;
                    case "removeapplicationpackage":
                        azureCliProxy.RemoveApplicationPackage(args[1], args[2], args[3], args.Length >= 5 ? args[4] : null);

                        break;
                    case "invokeencrypttext":
                        azureCliProxy.InvokeEncryptText(args[1], args[2], args[3]);

                        break;
                    case "invokedecrypttext":
                        azureCliProxy.InvokeDecryptText(args[1]);

                        break;
                    default:
                        Console.WriteLine("Command not found");
                        break;
                }
            }
            catch(Exception e)
            {
                Console.WriteLine(e);
                return 1;
            }

            return 0;
        }
    }
}