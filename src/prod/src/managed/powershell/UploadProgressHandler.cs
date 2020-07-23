// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if ServiceFabric
namespace Microsoft.ServiceFabric.Powershell
#else
namespace System.Fabric.Powershell
#endif
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Management.Automation;
    using System.Management.Automation.Host;

    internal class UploadProgressHandler : IImageStoreProgressHandler, IDisposable
    {
        private const int DefaultProgressIntervalMilliseconds = 500;

        private readonly int progressIntervalMilliseconds;
        private string appPackagePath;

        private PowerShell ps;
        private PSHostUserInterface ui;
        private State state;
        private long totalByteCountEstimate;
        private long totalFileCountEstimate;
        private bool hasCompressionStateChanged;

        private long completedTransferItems;
        private long totalTransferItems;
        private long completedReplicateItmes;
        private long totalReplicateItems;

        private bool disposed = false;

        public UploadProgressHandler(string appPackagePath, int progressIntervalMilliseconds)
        {
            this.appPackagePath = appPackagePath;
            this.progressIntervalMilliseconds = progressIntervalMilliseconds;

            this.ps = PowerShell.Create(RunspaceMode.CurrentRunspace);
            this.ui = this.ps
                .AddCommand("Get-Variable")
                .AddParameter("-ValueOnly")
                .AddArgument("Host")
                .Invoke<PSHost>()[0].UI;

            this.state = State.EstimatingPackageSize;
            this.totalByteCountEstimate = 0;
            this.totalFileCountEstimate = 0;

            this.completedTransferItems = 0;
            this.totalTransferItems = 0;
            this.completedReplicateItmes = 0;
            this.totalReplicateItems = 0;

            this.hasCompressionStateChanged = false;
        }

        ~UploadProgressHandler()
        {
            this.Dispose(false);
        }

        private enum State
        {
            EstimatingPackageSize,
            Copying,
            Compressing,
            Uncompressing,
            GeneratingChecksums,
            Uploading
        }

        public void Initialize()
        {
            this.EstimateTotalByteCount();
            this.totalTransferItems = this.totalByteCountEstimate;
            this.totalReplicateItems = this.totalFileCountEstimate;
        }

        public void Dispose()
        { 
            this.Dispose(true);
            GC.SuppressFinalize(this);           
        }

        public void SetStateCopying(string destPath)
        {
            this.state = State.Copying;
            
            this.WriteProgress(
                string.Format(CultureInfo.InvariantCulture, StringResources.Powershell_Activity_Copying, this.appPackagePath, destPath),
                string.Format(CultureInfo.InvariantCulture, StringResources.Powershell_Status_Copying, this.totalByteCountEstimate),
                0, // completed
                this.totalByteCountEstimate); // total
        }

        public void UpdateApplicationPackagePath(string destPath)
        {
            this.appPackagePath = destPath;
        }

        public void SetStateCompressing()
        {
            this.state = State.Compressing;
            this.hasCompressionStateChanged = true;
            
            this.WriteProgress(
                string.Format(CultureInfo.InvariantCulture, StringResources.Powershell_Activity_Compressing, this.appPackagePath),
                string.Format(CultureInfo.InvariantCulture, StringResources.Powershell_Status_Compressing, this.totalByteCountEstimate),
                0, // completed
                this.totalByteCountEstimate); // total
        }

        public void SetStateUncompressing()
        {
            this.state = State.Uncompressing;
            this.hasCompressionStateChanged = true;
            
            this.WriteProgress(
                string.Format(CultureInfo.InvariantCulture, StringResources.Powershell_Activity_Uncompressing, this.appPackagePath),
                string.Format(CultureInfo.InvariantCulture, StringResources.Powershell_Status_Uncompressing, this.totalByteCountEstimate),
                0, // completed
                this.totalByteCountEstimate); // total
        }

        public void SetStateGeneratingChecksums()
        {
            this.state = State.GeneratingChecksums;
        }

        public void SetStateUploading()
        {
            if (this.hasCompressionStateChanged)
            {
                // Update the byte count estimate after compression state has changed
                this.EstimateTotalByteCount();
            }

            this.state = State.Uploading;

            this.WriteProgress(
                string.Format(CultureInfo.InvariantCulture, StringResources.Powershell_Activity_Uploading, this.appPackagePath),
                string.Format(CultureInfo.InvariantCulture, StringResources.Powershell_Status_Uploading, this.totalByteCountEstimate),
                0, // completed
                this.totalByteCountEstimate); // total
        }

        TimeSpan IImageStoreProgressHandler.GetUpdateInterval()
        {
            var millis = this.progressIntervalMilliseconds > 0 ? this.progressIntervalMilliseconds : DefaultProgressIntervalMilliseconds;
            return TimeSpan.FromMilliseconds(millis);
        }

        // Status details (i.e. byte/package/file % progress) is only available for Native Image Store. Other image store
        // types will only display the high-level statuses set during the SetState*() methods. 
            void IImageStoreProgressHandler.UpdateProgress(long completedItems, long totalItems, ProgressUnitType itemType)
        {
            switch (this.state)
            {
            case State.Compressing:
                this.UpdateProgressCompressingDetails(completedItems, totalItems, itemType);
                break;
                
            case State.Uncompressing:
                this.UpdateProgressUncompressingDetails(completedItems, totalItems, itemType);
                break;
                
            case State.GeneratingChecksums:
                this.UpdateProgressChecksumDetails(completedItems, totalItems, itemType);
                break;
                
            case State.Uploading:
                this.UpdateProgressUploadingDetails(completedItems, totalItems, itemType);
                break;

            default:
                // Ignore unrecognized state details
                break;
            }
        }

        // Protected implementation of Dispose pattern.
        protected virtual void Dispose(bool disposing)
        {
            if (this.disposed)
            {
                return; 
            }

            if (disposing) 
            {
                this.ps.Dispose();
            }

            this.disposed = true;
        }

        private void UpdateProgressEstimatingPackageSizeDetails()
        {
            this.WriteProgress(
                string.Format(CultureInfo.InvariantCulture, StringResources.Powershell_Activity_Estimating, this.appPackagePath),
                string.Format(CultureInfo.InvariantCulture, StringResources.Powershell_Status_Estimating, this.totalByteCountEstimate),
                0, // completed
                this.totalByteCountEstimate); // total
        }

        private void UpdateProgressCompressingDetails(long completedItems, long totalItems, ProgressUnitType itemType)
        {
            var activity = string.Format(
                CultureInfo.InvariantCulture,
                StringResources.Powershell_Activity_Compressing,
                this.appPackagePath);

            var status = string.Format(
                CultureInfo.CurrentCulture,
                StringResources.Powershell_Status_Compressing_Details,
                completedItems,
                totalItems,
                itemType,
                (double)completedItems / totalItems);

            this.WriteProgress(activity, status, completedItems, totalItems);
        }

        private void UpdateProgressUncompressingDetails(long completedItems, long totalItems, ProgressUnitType itemType)
        {
            var activity = string.Format(
                CultureInfo.InvariantCulture,
                StringResources.Powershell_Activity_Uncompressing,
                this.appPackagePath);

            var status = string.Format(
                CultureInfo.CurrentCulture,
                StringResources.Powershell_Status_Uncompressing_Details,
                completedItems,
                totalItems,
                itemType,
                (double)completedItems / totalItems);

            this.WriteProgress(activity, status, completedItems, totalItems);
        }

        private void UpdateProgressChecksumDetails(long completedItems, long totalItems, ProgressUnitType itemType)
        {
            var activity = StringResources.Powershell_Activity_Checksum;

            var status = string.Format(
                CultureInfo.CurrentCulture,
                StringResources.Powershell_Status_Checksum_Details,
                completedItems,
                totalItems,
                itemType,
                (double)completedItems / totalItems);

            this.WriteProgress(activity, status, completedItems, totalItems);
        }

        private void UpdateProgressUploadingDetails(long completedItems, long totalItems, ProgressUnitType itemType)
        {
            if (itemType == ProgressUnitType.Bytes)
            {
                if (totalItems < this.totalByteCountEstimate)
                {
                    totalItems = this.totalByteCountEstimate;
                }

                this.completedTransferItems = completedItems;
                this.totalTransferItems = totalItems;
            }

            if (itemType == ProgressUnitType.Files)
            {
                if (totalItems < this.totalFileCountEstimate)
                {
                    totalItems = this.totalFileCountEstimate;
                }

                this.completedReplicateItmes = completedItems;
                this.totalReplicateItems = totalItems;
            }

            this.WriteUploadProgress();
        }

        private void WriteUploadProgress()
        {
           var transferActivity = string.Format(
           CultureInfo.InvariantCulture,
           StringResources.Powershell_Activity_TransferringToGateway,
           this.appPackagePath);

            var transferStatus = string.Format(
                CultureInfo.CurrentCulture,
                StringResources.Powershell_Status_Transferring_Details,
                this.completedTransferItems,
                this.totalTransferItems,
                ProgressUnitType.Bytes,
                (double)this.completedTransferItems / this.totalTransferItems);

            var replicateActivity = string.Format(
            CultureInfo.InvariantCulture,
            StringResources.Powershell_Activity_ReplicationInFileStoreService);

            var replicateStatus = string.Format(
                CultureInfo.CurrentCulture,
                StringResources.Powershell_Status_Replication_Details,
                this.completedReplicateItmes,
                this.totalReplicateItems,
                ProgressUnitType.Files,
                (double)this.completedReplicateItmes / this.totalReplicateItems);

            int transferActivityId = 1;
            var transferProgressRecord = new ProgressRecord(transferActivityId, transferActivity, transferStatus);
            transferProgressRecord.PercentComplete = (int)(((double)this.completedTransferItems / this.totalTransferItems) * 100);
            this.ui.WriteProgress(transferActivityId, transferProgressRecord);

            int replicateActivityId = 2;
            var replicateProgressRecord = new ProgressRecord(replicateActivityId, replicateActivity, replicateStatus);
            replicateProgressRecord.PercentComplete = (int)(((double)this.completedReplicateItmes / this.totalReplicateItems) * 100);
            this.ui.WriteProgress(replicateActivityId, replicateProgressRecord);
        }

        private void WriteProgress(string activity, string status, long completedItems, long totalItems)
        {
            int activityId = 1;

            var progressRecord = new ProgressRecord(activityId, activity, status);
            progressRecord.PercentComplete = (int)(((double)completedItems / totalItems) * 100);
            this.ui.WriteProgress(activityId, progressRecord);
        }

        private void EstimateTotalByteCount()
        {
            this.totalByteCountEstimate = 0;

            this.UpdateProgressEstimatingPackageSizeDetails();

            this.EstimateTotalByteCount(this.appPackagePath);
        }

        private void EstimateTotalByteCount(string dirFullPath)
        {
            var fileFullPaths = FabricDirectory.GetFiles(dirFullPath, "*", true, SearchOption.AllDirectories);
            this.totalFileCountEstimate += fileFullPaths.Length;
            foreach (var fileFullPath in fileFullPaths)
            {
                this.totalByteCountEstimate += FabricFile.GetSize(fileFullPath);

                this.UpdateProgressEstimatingPackageSizeDetails();
            }

            this.totalFileCountEstimate += FabricDirectory.GetDirectories(dirFullPath, "*", SearchOption.AllDirectories).Length;
            ++this.totalFileCountEstimate;
        }
    }
}