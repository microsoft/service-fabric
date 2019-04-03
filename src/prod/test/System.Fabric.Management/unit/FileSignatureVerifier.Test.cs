// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System.Fabric.Common;
    using System.Globalization;
    using System.IO;
    using System.Reflection;

    using KeyVault.Wrapper;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using WEX.TestExecution;
    
    /// <summary>
    /// This class tests MSI validation.
    /// </summary>
    [TestClass]
    public class FileSignatureVerifierTest
    {
        private const string StorageContainerName = "testmsi";
        private const string BuggyFilename = "WindowsFabric.5.3.301.9590_Buggy.msi";
        private const string AuthenticFilename = "WindowsFabric.5.3.121.9494.msi";      

        private void DownloadFromAzureStorage(string sourceFilename, string destination)
        {
            if (!File.Exists(destination))
            {
                var storageConnectionString = TestUtility.GetXStoreTestResourcesConnectionString();
                
                CloudStorageAccount storageAccount = CloudStorageAccount.Parse(storageConnectionString);
                CloudBlobClient blobClient = storageAccount.CreateCloudBlobClient();
                CloudBlobContainer container = blobClient.GetContainerReference(StorageContainerName);
                CloudBlockBlob blockBlob = container.GetBlockBlobReference(sourceFilename);
                using (var fileStream = File.OpenWrite(destination))
                {
                    blockBlob.DownloadToStream(fileStream);
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifyMSISignatureBuggy()
        {
            string destPath = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), BuggyFilename);          
            DownloadFromAzureStorage(BuggyFilename, destPath);
            bool result = false;
            try
            {
                result = FileSignatureVerifier.IsSignatureValid(destPath);
            }
            catch (Exception)
            {
                Verify.IsFalse(result, "MSI Signature verification failed as expected.");
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void VerifyMSISignatureAuthentic()
        {
            string destPath = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), AuthenticFilename);
            DownloadFromAzureStorage(AuthenticFilename, destPath);
            bool result = FileSignatureVerifier.IsSignatureValid(destPath);
            Verify.IsTrue(result, "MSI Signature verification passed.");
        }
    }
}