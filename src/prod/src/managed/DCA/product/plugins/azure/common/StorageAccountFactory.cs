//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace FabricDCA
{
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.RetryPolicies;
    using System;

    /// <summary>
    /// Represents the factory for creating CloudStorageAccount object used to upload.
    /// </summary>
    public abstract class StorageAccountFactory
    {
        public StorageAccountFactory(StorageConnection connection)
        {
            if (connection == null)
            {
                throw new ArgumentNullException(nameof(connection));
            }

            this.Connection = connection;
        }

        public StorageConnection Connection { get; private set; }

        public abstract CloudStorageAccount GetStorageAccount();

        internal CloudBlobClient GetCloudBlobClient()
        {
            CloudStorageAccount storageAccount = this.GetStorageAccount();
            CloudBlobClient blobClient = storageAccount.CreateCloudBlobClient();
            blobClient.DefaultRequestOptions.RetryPolicy = new ExponentialRetry();
            return blobClient;
        }

        internal CloudBlobContainer GetContainer(string containerName)
        {
            var blobClient = this.GetCloudBlobClient();
            return blobClient.GetContainerReference(containerName);
        }

        internal CloudBlobDirectory GetDirectory(string containerName, string directoryName)
        {
            CloudBlobContainer container = this.GetContainer(containerName);
            return container.GetDirectoryReference(directoryName);
        }

        internal CloudBlobDirectory GetDirectory(string containerName, string directoryName, string relativeDirectoryName)
        {
            var directory = this.GetDirectory(containerName, directoryName);
            return string.IsNullOrWhiteSpace(relativeDirectoryName)
                ? directory
                : directory.GetDirectoryReference(relativeDirectoryName);
        }
    }
}