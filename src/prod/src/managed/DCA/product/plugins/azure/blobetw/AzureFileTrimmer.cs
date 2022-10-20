// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.FileUploader;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using Microsoft.WindowsAzure.Storage.RetryPolicies;
    
    // This class implements the logic to delete old blobs from Azure blob storage.
    internal class AzureFileTrimmer : IDisposable
    {
        // Constants
        private const string TraceType = "AzureFileTrimmer";
        private const string OldLogDeletionTimerIdSuffix = "_OldBlobDeletionTimer";

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Container in the blob storage account where data needs to be uploaded
        private readonly string containerName;

        // Time after which the data in the container becomes a candidate for deletion
        private readonly TimeSpan blobDeletionAge;

        // Fabric node name of the current node
        private readonly string fabricNodeInstanceName;

        // Override value for deploymentId from cluster manifest.
        private readonly string deploymentId;

        // Whether or not the Azure runtime interfaces are available
        private readonly bool azureInterfaceAvailable;

        // Object that measures performance
        private readonly AzureBlobPerformance perfHelper;

        // Timer to delete old logs
        private readonly DcaTimer oldLogDeletionTimer;

        // Helper objects used in deleting old files
        private readonly FolderTrimmer folderTrimmer;
        private readonly LocalMapTrimmingHelper localMapTrimmingHelper;

        // Folder containing CSV files to trim files from
        private readonly string sourceFolderName;

        // Local folder containing a map of files that we uploaded
        private readonly string localMap;

        private StorageAccountFactory storageAccountFactory;

        // Whether or not the we are in the process of stopping
        private bool stopping;

        // Whether or not the object has been disposed
        private bool disposed;

        internal AzureFileTrimmer(
                    string sourceFolderName,
                    string workFolder,
                    StorageAccountFactory storageAccountFactory,
                    string containerName,
                    TimeSpan blobDeletionAge,
                    string fabricNodeInstanceName,
                    string deploymentId,
                    bool azureInterfaceAvailable)
        {
            // Initialization
            this.stopping = false;
            this.sourceFolderName = sourceFolderName;
            this.localMap = Path.Combine(workFolder, AzureBlobUploader.LocalMapFolder);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.logSourceId = string.Concat(TraceType, "-", Guid.NewGuid().ToString("P"));
            this.storageAccountFactory = storageAccountFactory;
            this.containerName = containerName;
            this.fabricNodeInstanceName = fabricNodeInstanceName;
            this.blobDeletionAge = blobDeletionAge;
            this.deploymentId = deploymentId;
            this.azureInterfaceAvailable = azureInterfaceAvailable;
            this.localMapTrimmingHelper = new LocalMapTrimmingHelper(sourceFolderName);
            this.folderTrimmer = new FolderTrimmer(
                                        this.traceSource,
                                        this.logSourceId,
                                        blobDeletionAge);
            this.perfHelper = new AzureBlobPerformance(this.traceSource, this.logSourceId);
            
            // Deletion and query are currently synchronous, so the concurrency 
            // count is 1.
            this.perfHelper.ExternalOperationInitialize(
                ExternalOperationTime.ExternalOperationType.BlobQuery, 
                1);
            this.perfHelper.ExternalOperationInitialize(
                ExternalOperationTime.ExternalOperationType.BlobDeletion, 
                1);

            // Create a timer to delete old logs
            string timerId = string.Concat(
                                 this.logSourceId,
                                 OldLogDeletionTimerIdSuffix);
            var oldLogDeletionInterval =
                (this.blobDeletionAge < TimeSpan.FromDays(1))
                    ? AzureConstants.OldLogDeletionIntervalForTest
                    : AzureConstants.OldLogDeletionInterval;
            this.oldLogDeletionTimer = new DcaTimer(
                                               timerId,
                                               this.DeleteOldLogsHandler,
                                               oldLogDeletionInterval);
            this.oldLogDeletionTimer.Start();
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            // Keep track of the fact that the consumer is stopping
            this.stopping = true;

            if (null != this.folderTrimmer)
            {
                this.folderTrimmer.Dispose();
            }

            if (null != this.oldLogDeletionTimer)
            {
                // Stop and dispose timers
                this.oldLogDeletionTimer.StopAndDispose();

                // Wait for timer callbacks to finish executing
                this.oldLogDeletionTimer.DisposedEvent.WaitOne();
            }

            GC.SuppressFinalize(this);
        }

        #region Private Fields
        private bool ShouldDeleteBlob(IListBlobItem blobInterface, DateTime cutoffTime)
        {
            // We delete the blob if it is old enough
            ICloudBlob blob = (ICloudBlob)blobInterface;
            return ((false == blob.Properties.LastModified.HasValue) ||
                    (blob.Properties.LastModified.Value.CompareTo(cutoffTime) > 0)) ?
                        false :
                        true;
        }
        
        private void DeleteOldBlobsFromContainer(CloudBlobClient blobClient, string containerName)
        {
            // Figure out the timestamp before which all blobs will be deleted
            DateTime cutoffTime = DateTime.UtcNow.Add(-this.blobDeletionAge);
            this.traceSource.WriteInfo(
                this.logSourceId,
                "Deleting blobs older than {0}.", 
                cutoffTime);
        
            // List all blobs in the container that need to be deleted
            //
            // Notes: 
            // #1. The blobs have a virtual directory hierarchy inside the 
            //     container, so we explicitly opt for a flat listing of blobs.
            // #2. If the Azure runtime interfaces are available, then the top-
            //     level virtual directory we create is inside the container 
            //     is the deployment ID. In this case, we only delete blobs under 
            //     the virtual container whose deployment ID matches our current
            //     deployment ID.
            string prefix = string.Format(
                                CultureInfo.InvariantCulture,
                                "{0}{1}{2}{3}{4}{5}{6}{7}",
                                containerName,
                                blobClient.DefaultDelimiter,
                                this.azureInterfaceAvailable ? (string.IsNullOrEmpty(this.deploymentId) ? AzureUtility.DeploymentId : this.deploymentId) : this.fabricNodeInstanceName,
                                blobClient.DefaultDelimiter,
                                this.azureInterfaceAvailable ? AzureUtility.RoleName : string.Empty,
                                this.azureInterfaceAvailable ? blobClient.DefaultDelimiter : string.Empty,
                                this.azureInterfaceAvailable ? AzureUtility.RoleInstanceId : string.Empty,
                                this.azureInterfaceAvailable ? blobClient.DefaultDelimiter : string.Empty);

            BlobContinuationToken continuationToken = null;
            BlobRequestOptions requestOptions = new BlobRequestOptions();
            OperationContext operationContext = new OperationContext();
            do
            {
                BlobResultSegment resultSegment;
                try
                {
                    this.perfHelper.ExternalOperationBegin(
                        ExternalOperationTime.ExternalOperationType.BlobQuery,
                        0);

                    // Get the blobs that were uploaded by the current node. The prefix
                    // helps us identify them.
                    resultSegment = blobClient.ListBlobsSegmented(
                                         prefix,
                                         true, // useFlatBlobListing
                                         BlobListingDetails.All,
                                         null,
                                         continuationToken,
                                         requestOptions,
                                         operationContext);

                    this.perfHelper.ExternalOperationEnd(
                        ExternalOperationTime.ExternalOperationType.BlobQuery,
                        0);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Exception encountered when attempting to enumerate blobs for deletion in container {0} in storage account {1}. Exception information: {2}",
                        containerName,
                        this.storageAccountFactory.Connection.AccountName,
                        e);

                    // If we encounter an error during enumeration of blobs for deletion, 
                    // we give up.
                    this.BlobQueryAndDeleteExceptionHandler(e);
                    break;
                }

                continuationToken = resultSegment.ContinuationToken;
                if (!resultSegment.Results.Any())
                {
                    continue;
                }

                this.perfHelper.AzureBlobsQueried((ulong)resultSegment.Results.Count());
        
                // Get the blobs that are old enough to be deleted
                IEnumerable<IListBlobItem> blobsToDelete = resultSegment.Results
                                                           .Where(blob => this.ShouldDeleteBlob(
                                                                              blob,
                                                                              cutoffTime));

                // Go through the list and delete the blobs
                foreach (IListBlobItem blobInterface in blobsToDelete)
                {
                    ICloudBlob blob = (ICloudBlob)blobInterface;

                    try
                    {
                        this.perfHelper.ExternalOperationBegin(
                            ExternalOperationTime.ExternalOperationType.BlobDeletion,
                            0);

                        // DeleteIfExists allows for the case where the blob is being
                        // deleted by other means (possibly by another instance of
                        // the DCA) at the same time that we are trying to delete it.
                        blob.DeleteIfExists();

                        this.perfHelper.ExternalOperationEnd(
                            ExternalOperationTime.ExternalOperationType.BlobDeletion,
                            0);

                        this.perfHelper.AzureBlobDeleted();
                    }
                    catch (Exception e)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Exception encountered when attempting to delete blob {0} in container {1} in storage account {2}. Exception information: {3}",
                            blob.Name,
                            containerName,
                            this.storageAccountFactory.Connection.AccountName,
                            e);

                        // If we encounter an error during the deletion of one blob,
                        // we'll still try and delete the others
                        this.BlobQueryAndDeleteExceptionHandler(e);    
                    }
        
                    if (this.stopping)
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "The consumer is being stopped. Therefore, no more old blobs will be deleted from container {0} in storage account {1}.",
                            containerName,
                            this.storageAccountFactory.Connection.AccountName);
                        break;
                    }
                }

                if (this.stopping)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "The consumer is being stopped. Therefore, no more blobs will be enumerated for deletion from container {0} in storage account {1}.",
                        containerName,
                        this.storageAccountFactory.Connection.AccountName);
                    break;
                }
            } 
            while (continuationToken != null);
        }

        private void BlobQueryAndDeleteExceptionHandler(Exception e)
        {
            if (e is StorageException)
            {
                // If this was a network error, we'll retry later. Else, we'll
                // just give up.
                StorageException storageException = (StorageException)e;
                if (Utility.IsNetworkError(storageException.RequestInformation.HttpStatusCode))
                {
                    // We encountered a network error that wasn't resolved even
                    // after retries.
                    throw new MaxRetriesException();
                }
            }
        }
        
        private void DeleteOldLogsHandler(object state)
        {
            try
            {
                this.perfHelper.AzureBlobDeletionPassBegin();
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Starting deletion of old blobs from blob storage ...");

                var blobClient = this.storageAccountFactory.GetCloudBlobClient();
                
                // Delete old blobs from the container
                this.DeleteOldBlobsFromContainer(
                    blobClient, 
                    this.containerName);
                    
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Finished deletion of old blobs from blob storage.");
                this.perfHelper.AzureBlobDeletionPassEnd();
            }
            catch (MaxRetriesException)
            {
                // A failure that we are designed to handle did not get resolved
                // even after the maximum number of retries. Let's abort this
                // pass and retry on the next pass.
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Deletion of old blobs from blob storage aborted due to errors.");
            }

            // Delete old files
            this.folderTrimmer.DeleteOldFilesFromFolder(
                this.sourceFolderName,
                this.SetFolderDeletionContext,
                this.OkToDeleteFile, 
                null);

            // Delete old files from local map
            this.folderTrimmer.DeleteOldFilesFromFolder(
                this.localMap,
                this.localMapTrimmingHelper.SetFolderDeletionContext,
                this.localMapTrimmingHelper.OkToDeleteFile,
                null);

            // Schedule the next pass
            this.oldLogDeletionTimer.Start();
        }

        private void SetFolderDeletionContext(FolderTrimmer.FolderDeletionInfo folderInfo, FolderTrimmer.FolderDeletionInfo parentFolderInfo, string folderNameWithoutPath)
        {
            if (null == parentFolderInfo)
            {
                folderInfo.Context = string.Empty;
            }
            else
            {
                string parentRelativePath = (string)parentFolderInfo.Context;
                if (string.IsNullOrEmpty(parentRelativePath))
                {
                    folderInfo.Context = folderNameWithoutPath;
                }
                else
                {
                    folderInfo.Context = Path.Combine(parentRelativePath, folderNameWithoutPath);
                }
            }
        }

        private bool OkToDeleteFile(FolderTrimmer.FolderDeletionInfo folderInfo, string fileName)
        {
            string relativePath = (string)folderInfo.Context;
            string pathOnLocalNode;
            if (string.IsNullOrEmpty(relativePath))
            {
                pathOnLocalNode = Path.Combine(this.sourceFolderName, fileName);
            }
            else
            {
                pathOnLocalNode = Path.Combine(
                                        this.sourceFolderName,
                                        relativePath,
                                        fileName);
            }

            return FabricFile.Exists(pathOnLocalNode);
        }
#endregion
    }
}