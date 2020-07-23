// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.RetryPolicies;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Azure Blob Store Helper
    /// </summary>
    internal static class AzureBlobStoreHelper
    {
        private const string AzureStoragePathSeparator = "/";
        private const int RetryCount = 3;
        private const string TraceType = "AzureBlobStoreHelper";
        private static readonly FabricEvents.ExtensionsEvents TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.BackupRestoreService);

        public static string ConvertToAzureBlobStorePath(string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                return string.Empty;
            }

            string[] pathParts = path.Split(Path.DirectorySeparatorChar);

            return string.Join(AzureStoragePathSeparator, pathParts);
        }

        public static CloudBlobContainer GetContainer(CloudStorageAccount storageAccount, string containerName)
        {
            try
            {
                return BackupRestoreUtility.PerformWithRetries<string, CloudBlobContainer>(
                    (string name) =>
                    {
                        CloudBlobContainer container = GetContainerRef(storageAccount, name);

                        // Create the container
                        if (!container.Exists())
                        {
                            throw new ArgumentException(string.Format("Given container {0} doesn't exist.", name), "containerName");
                        }

                        return container;
                    },
                    containerName,
                    new RetriableOperationExceptionHandler(AzureStorageExceptionHandler),
                    RetryCount);
            }
            catch (Exception e)
            {
                TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to get container {0} from Azure storage.",
                    containerName);

                throw;
            }
        }

        public static CloudBlobContainer GetContainer(string connectionString, string containerName)
        {
            try
            {
                return BackupRestoreUtility.PerformWithRetries<string, CloudBlobContainer>(
                    (string name) =>
                    {
                        CloudBlobContainer container = GetContainerRef(connectionString, name);

                        // Create the container
                        if (!container.Exists())
                        {
                            throw new ArgumentException(string.Format("Given container {0} doesn't exist.", name), "containerName");
                        }

                        return container;
                    },
                    containerName,
                    new RetriableOperationExceptionHandler(AzureStorageExceptionHandler),
                    RetryCount);
            }
            catch (Exception e)
            {
                TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to get container {0} from Azure storage.",
                    containerName);

                throw;
            }
        }

        public static CloudBlobDirectory GetCloudBlobDirectoryRef(CloudBlobContainer container, string directoryPath)
        {
            try
            {
                return BackupRestoreUtility.PerformWithRetries<string, CloudBlobDirectory>(
                    (string relDirName) =>
                    {
                        return container.GetDirectoryReference(relDirName);
                    },
                    directoryPath,
                    new RetriableOperationExceptionHandler(AzureStorageExceptionHandler),
                    RetryCount);
            }
            catch (Exception e)
            {
                TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to get directory reference {0} from Azure storage.",
                    directoryPath);

                throw;
            }
        }

        public static CloudBlockBlob GetCloudBlockBlobRef(CloudBlobContainer container, string blockBlobPath)
        {
            try
            {
                return BackupRestoreUtility.PerformWithRetries<string, CloudBlockBlob>(
                    (string blobPath) =>
                    {
                        return container.GetBlockBlobReference(blobPath);
                    },
                    blockBlobPath,
                    new RetriableOperationExceptionHandler(AzureStorageExceptionHandler),
                    RetryCount);
            }
            catch (Exception e)
            {
                TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to get block blob reference {0} from Azure storage container.",
                    blockBlobPath);

                throw;
            }
        }

        public static IEnumerable<IListBlobItem> GetBlobList(CloudBlobDirectory directory, bool useFlatBlobListing)
        {
            try
            {
                return BackupRestoreUtility.PerformWithRetries<bool, IEnumerable<IListBlobItem>>(
                    (bool flatBlobListing) =>
                    {
                        return directory.ListBlobs(flatBlobListing);
                    },
                    useFlatBlobListing,
                    new RetriableOperationExceptionHandler(AzureStorageExceptionHandler),
                    RetryCount);
            }
            catch (Exception e)
            {
                TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to list blobs under directory {0} from Azure storage.",
                    directory.Prefix);

                throw;
            }
        }

        public static BlobResultSegment GetBlobList(CloudBlobDirectory directory, bool useFlatBlobListing, BlobContinuationToken blobContinuationToken, int maxResults)
        {
            try
            {
                return BackupRestoreUtility.PerformWithRetries<bool, BlobResultSegment>(
                    (bool flatBlobListing) =>
                    {
                        return directory.ListBlobsSegmented(flatBlobListing, BlobListingDetails.None, maxResults, blobContinuationToken, null, null);
                    },
                    useFlatBlobListing,
                    new RetriableOperationExceptionHandler(AzureStorageExceptionHandler),
                    RetryCount);
            }
            catch (Exception e)
            {
                TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to list blobs under directory {0} from Azure storage.",
                    directory.Prefix);

                throw;
            }
        }

        public static void DeleteBlob(CloudBlockBlob cloudBlockBlob)
        {
            try
            {
                BackupRestoreUtility.PerformIOWithRetries(
                    () =>
                    {
                        cloudBlockBlob.DeleteIfExists();
                    },
                    RetryCount);
            }
            catch (Exception e)
            {
                TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to list blobs under directory {0} .",
                    cloudBlockBlob.Name);

                throw;
            }
        }

        public static Task DownloadToStreamAsync(CloudBlob blob, Stream target, CancellationToken cancellationToken)
        {
            try
            {
                return BackupRestoreUtility.PerformActionWithRetriesAsync<CloudBlob, Stream>(
                    (CloudBlob blockBlob, Stream targetStream, CancellationToken ct) =>
                    {
                        return blockBlob.DownloadToStreamAsync(targetStream, cancellationToken);
                    },
                    blob,
                    target,
                    cancellationToken,
                    new RetriableOperationExceptionHandler(AzureStorageExceptionHandler));
            }
            catch (Exception e)
            {
                TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to download blob {0} to stream.",
                    blob.Name);

                throw;
            }
        }

        private static CloudBlobContainer GetContainerRef(string connectionString, string containerName)
        {
            CloudStorageAccount storageAccount = CloudStorageAccount.Parse(connectionString);

            return AzureBlobStoreHelper.GetContainerRef(storageAccount, containerName);
        }

        private static CloudBlobContainer GetContainerRef(CloudStorageAccount storageAccount, string containerName)
        {
            // Create the blob client object
            CloudBlobClient blobClient = storageAccount.CreateCloudBlobClient();

            // Set the retry policy
            blobClient.DefaultRequestOptions.RetryPolicy = new ExponentialRetry();

            // Get the container reference
            return blobClient.GetContainerReference(containerName);
        }

        private static RetriableOperationExceptionHandler.Response AzureStorageExceptionHandler(Exception e)
        {
            if (e is StorageException)
            {
                // If this was a network error, we'll retry later. Else, we'll
                // just give up.
                StorageException storageException = (StorageException)e;
                if (IsNetworkError(storageException.RequestInformation.HttpStatusCode))
                {
                    return RetriableOperationExceptionHandler.Response.Retry;
                }

                TraceSource.WriteExceptionAsWarning(
                    TraceType,
                    e,
                    "Encountered an unhandled storage exception with http code {0}",
                    storageException.RequestInformation.HttpStatusCode);
            }
            else if ((e is IOException) || (e is FabricException))
            {
                // If there was an error accessing the file, but it was not because
                // the file was not found, we'll retry later. Else, we'll just give up.
                if (false == ((e is FileNotFoundException) || (e is DirectoryNotFoundException)))
                {
                    return RetriableOperationExceptionHandler.Response.Retry;
                }
            }

            return RetriableOperationExceptionHandler.Response.Abort;
        }

        private static bool IsNetworkError(int httpStatusCode)
        {
            const int HttpCodeInternalServerError = 500;
            const int HttpCodeBadGateway = 502;
            const int HttpCodeServiceUnavailable = 503;
            const int HttpCodeGatewayTimeout = 504;

            return (httpStatusCode == HttpCodeInternalServerError) ||
                    (httpStatusCode == HttpCodeBadGateway) ||
                    (httpStatusCode == HttpCodeServiceUnavailable) ||
                    (httpStatusCode == HttpCodeGatewayTimeout);
        }
    }
}