// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.IO;
    using Microsoft.WindowsAzure.Storage;
    using Utility = System.Fabric.Dca.Utility;

    internal class AzureBlobEtwUploader : IDcaInMemoryConsumer
    {
        private const string LogDirName = "Log";
        private const string FileUploadTimerIdSuffix = "_FileUploadTimer";

        // Tag used to represent the source of the log message
        private readonly string logSourceId;
        
        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // String that represents the destination account and container
        private readonly string destinationKey;

        // Configuration reader
        private readonly AzureBlobEtwConfigReader configReader;

        // Settings for uploading data to blob storage
        private readonly BlobUploadSettings blobUploadSettings;

        // ETL to in-memory buffer writer interface
        private readonly EtlToInMemoryBufferWriter etlToInMemoryBufferWriter;

        // Consumer initialization params
        private readonly ConsumerInitializationParameters initParam;

        // Object that manages bookmarks
        private readonly ConsumerProgressManager progressManager;

        // Timer to sync local files to blob storage
        private readonly DcaTimer fileUploadTimer;

        // Azure blob uploader instance used to sync local files to blob storage
        private readonly AzureBlobUploader fileBlobUploader;

        // Object that measures performance of stream upload
        private readonly AzureBlobPerformance streamUploadPerfHelper;

        // Object that measures performance of file upload
        private readonly AzureBlobPerformance fileUploadPerfHelper;

        // Access condition used by in-memory stream uploader
        private readonly AccessCondition uploadStreamAccessCondition;

        // Access condition used by file stream uploader
        private readonly AccessCondition uploadFileAccessCondition;

        // Folder to hold compressed CSV files in case of 
        // failure to upload compressed memory stream to blob storage
        private readonly string etwLogDirName;

        // Helper object that will delete old local files and old blobs on blob storage.
        private readonly AzureFileTrimmer trimmer;

        // Work folder used by the Azure blob uploader
        private readonly string workFolder;

        // Whether or not the object has been disposed
        private bool disposed;

        // Whether or not we are in the process of stopping
        private bool stopping;

        // Helper object that will upload compressed memory stream to blob storage.
        // If this upload fails, it will write the compressed memory stream to disk.
        private AzureBlobUploader blobUploader;

        // Used to record bytes read when getting max index already processed.
        private long bytesReadGetMaxIndexAlreadyProcessed;

        public AzureBlobEtwUploader(ConsumerInitializationParameters initParam, AccessCondition uploadStreamAccessCondition, AccessCondition uploadFileAccessCondition)
            : this(initParam)
        {
            this.uploadStreamAccessCondition = uploadStreamAccessCondition;
            this.uploadFileAccessCondition = uploadFileAccessCondition;
        }

        public AzureBlobEtwUploader(ConsumerInitializationParameters initParam)
        {
            this.stopping = false;
            this.initParam = initParam;
            this.logSourceId = string.Concat(initParam.ApplicationInstanceId, "_", initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.progressManager = new ConsumerProgressManager(
                this.traceSource, 
                this.logSourceId,
                AzureBlobEtwConstants.MethodExecutionInitialRetryIntervalMs,
                AzureBlobEtwConstants.MethodExecutionMaxRetryCount,
                AzureBlobEtwConstants.MethodExecutionMaxRetryIntervalMs);

            this.configReader = new AzureBlobEtwConfigReader(
                new ConfigReader(initParam.ApplicationInstanceId),
                initParam.SectionName,
                this.traceSource,
                this.logSourceId);

            this.streamUploadPerfHelper = new AzureBlobPerformance(this.traceSource, this.logSourceId);
            this.fileUploadPerfHelper = new AzureBlobPerformance(this.traceSource, this.logSourceId);

            // Read blob-specific settings
            this.blobUploadSettings = this.GetSettings();
            if (false == this.blobUploadSettings.Enabled)
            {
                return;
            }

            // Create the destination key
            var accountName = this.blobUploadSettings.StorageAccountFactory.Connection.UseDevelopmentStorage
               ? AzureConstants.DevelopmentStorageConnectionString
               : this.blobUploadSettings.StorageAccountFactory.Connection.AccountName;

            this.destinationKey = string.Join(
                "_",
                StandardPluginTypes.AzureBlobEtwUploader,
                accountName,
                this.blobUploadSettings.EtwTraceContainerName);

            // initialize bookmark folders and files
            var initializeBookmarkFoldersAndFilesSuccess = this.progressManager.InitializeBookmarkFoldersAndFiles(
                this.initParam.WorkDirectory,
                this.destinationKey);
            if (false == initializeBookmarkFoldersAndFilesSuccess)
            {
                const string Message = "Failed to initialize bookmark folders and files.";
                this.traceSource.WriteError(
                    this.logSourceId,
                    Message);
                throw new InvalidOperationException(Message);
            }

            // Create etw log directory
            this.etwLogDirName = this.CreateEtwLogDirectory();
            if (string.IsNullOrEmpty(this.etwLogDirName))
            {
                const string Message = "Failed to create etw log directory.";
                this.traceSource.WriteError(
                    this.logSourceId,
                    Message);
                throw new InvalidOperationException(Message);
            }

            // Create a sub-directory for the blob uploader
            this.workFolder = this.CreateBlobUploaderWorkSubDirectory();
            if (string.IsNullOrEmpty(this.workFolder))
            {
                const string Message = "Failed to create work folder for the blob uploader.";
                this.traceSource.WriteError(
                    this.logSourceId,
                    Message);
                throw new InvalidOperationException(Message);
            }

            // Create the helper object that writes events delivered from ETL files into an in-memory buffer.
            this.etlToInMemoryBufferWriter = new EtlToInMemoryBufferWriter(
                new TraceEventSourceFactory(),
                this.logSourceId,
                initParam.FabricNodeId,
                this.etwLogDirName,
                true,
                this);

            // Set the event filter
            this.etlToInMemoryBufferWriter.SetEtwEventFilter(
                this.blobUploadSettings.Filter,
                WinFabDefaultFilter.StringRepresentation,
                WinFabSummaryFilter.StringRepresentation,
                true);

            // Create the helper object that syncs local files to blob storage.
            // Local files will be created when upload of compressed memory stream to blob storage fails.
            this.fileBlobUploader = new AzureBlobUploader(
                this.traceSource,
                this.logSourceId,
                this.etwLogDirName,
                this.workFolder,
                this.blobUploadSettings.StorageAccountFactory,
                this.blobUploadSettings.EtwTraceContainerName,
                this.initParam.FabricNodeInstanceName,
                this.blobUploadSettings.DeploymentId,
                this.fileUploadPerfHelper,
                null,
                this.uploadFileAccessCondition);

            // Create a timer to schedule the upload of local files to blob storage
            string timerId = string.Concat(
                this.logSourceId,
                FileUploadTimerIdSuffix);

            this.fileUploadTimer = new DcaTimer(
                timerId,
                this.UploadFilesToDestinationBlob,
                this.blobUploadSettings.FileSyncInterval);

            this.fileUploadTimer.Start();

            // Initialize trimmer
            this.trimmer = new AzureFileTrimmer(
                this.etwLogDirName,
                this.workFolder,
                this.blobUploadSettings.StorageAccountFactory,
                this.blobUploadSettings.EtwTraceContainerName,
                this.blobUploadSettings.BlobDeletionAge,
                this.initParam.FabricNodeInstanceName,
                this.blobUploadSettings.DeploymentId,
                AzureUtility.IsAzureInterfaceAvailable());

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Upload to blob storage is configured. Storage account: {0}, Trace container: {1}, Local trace path: {2}",
                this.blobUploadSettings.StorageAccountFactory.Connection.AccountName,
                this.blobUploadSettings.EtwTraceContainerName,
                this.etwLogDirName);

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Windows Fabric event filters for Azure blob uploader: {0}",
                this.blobUploadSettings.Filter);
        }

        public Action<DecodedEventWrapper, string> ConsumerProcessTraceEventAction
        {
            get 
            {
                return this.ProcessTraceEvent;
            }
        }

        public Func<string, EventIndex> MaxIndexAlreadyProcessed
        {
            get
            {
                return this.GetMaxIndexAlreadyProcessed;
            }
        }

        public object GetDataSink()
        {
            return this.blobUploadSettings.Enabled ? this.etlToInMemoryBufferWriter : null;
        }

        public void FlushData()
        {
            if (null != this.fileBlobUploader)
            {
                this.fileBlobUploader.UploadFilesToDestinationBlob();
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            this.stopping = true;

            if (null != this.etlToInMemoryBufferWriter)
            {
                this.etlToInMemoryBufferWriter.Dispose();
            }

            if (null != this.fileUploadTimer)
            {
                this.fileUploadTimer.StopAndDispose();
                this.fileUploadTimer.DisposedEvent.WaitOne();
            }

            if (null != this.fileBlobUploader)
            {
                this.fileBlobUploader.Dispose();
            }

            if (null != this.trimmer)
            {
                this.trimmer.Dispose();
            }

            GC.SuppressFinalize(this);
        }

        public void OnProcessingPeriodStart(string traceFileName, bool isActiveEtl, string traceFileSubFolder)
        {
            // Create a new azure blob uploader
            var blobUploader = new AzureBlobUploader(
                this.traceSource,
                this.logSourceId,
                this.etwLogDirName,
                this.workFolder,
                this.blobUploadSettings.StorageAccountFactory,
                traceFileName,
                isActiveEtl,
                this.blobUploadSettings.EtwTraceContainerName,
                this.initParam.FabricNodeId,
                this.initParam.FabricNodeInstanceName,
                this.blobUploadSettings.DeploymentId,
                this.streamUploadPerfHelper,
                this.uploadStreamAccessCondition,
                null);

            this.streamUploadPerfHelper.AzureBlobUploadPassBegin();

            // GetMaxIndexAlreadyProcessed is invoked before the perf helper pass begins. So we save the bytes read
            // during the call to GetMaxIndexAlreadyProcessed and then record it here.
            this.streamUploadPerfHelper.BytesRead(this.bytesReadGetMaxIndexAlreadyProcessed);

            this.blobUploader = blobUploader;

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Filtered traces from {0} will be written to the memory stream in the azure blob uploader.",
                traceFileName);
        }

        public void OnProcessingPeriodStop(string traceFileName, bool isActiveEtl, string traceFileSubFolder)
        {
            if (this.blobUploader == null)
            {
                return;
            }

            // retrieve the bookmark folder
            string bookmarkFolder = this.progressManager.BookmarkFolders[traceFileSubFolder];
            if (string.IsNullOrEmpty(bookmarkFolder))
            {
                return;
            }

            // Upload in-memory compressed stream to blob storage
            this.blobUploader.UploadStreamToDestinationBlob(traceFileSubFolder);

            // Update bookmark. Ensure we do not overwrite bookmark with default value for last event read.
            if (this.blobUploader.LastEventIndexProcessed.Timestamp != DateTime.MinValue && this.blobUploader.LastEventIndexProcessed.TimestampDifferentiator != -1)
            {
                // Last event read position
                long bookmarkLastEventReadPosition = this.progressManager.BookmarkLastEventReadPosition[traceFileSubFolder];
                long bytesWritten = 0;
                this.progressManager.UpdateBookmarkFile(
                    bookmarkFolder,
                    bookmarkLastEventReadPosition,
                    this.blobUploader.LastEventIndexProcessed,
                    out bytesWritten);

                this.streamUploadPerfHelper.BytesWritten(bytesWritten);
            }

            this.streamUploadPerfHelper.AzureBlobUploadPassEnd();
        }

        private void ProcessTraceEvent(DecodedEventWrapper decodedEventWrapper, string traceFileSubFolder)
        {
            if (this.stopping)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "The consumer is being stopped. Therefore, no more in-memory trace events will be processed in this pass.");
                return;
            }

            this.blobUploader.ProcessTraceEvent(decodedEventWrapper);
        }

        private BlobUploadSettings GetSettings()
        {
            var blobUploadSettings = new BlobUploadSettings();
            blobUploadSettings.Enabled = this.configReader.GetEnabled();

            if (!blobUploadSettings.Enabled)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Blob storage upload not enabled.");
                return blobUploadSettings;
            }

            blobUploadSettings.StorageAccountFactory = this.configReader.GetStorageAccountFactory();
            if (blobUploadSettings.StorageAccountFactory == null)
            {
                blobUploadSettings.Enabled = false;
                return blobUploadSettings;
            }

            blobUploadSettings.EtwTraceContainerName = this.configReader.GetEtwTraceContainerName();
            blobUploadSettings.FileSyncInterval = this.configReader.GetFileSyncInterval();
            blobUploadSettings.BlobDeletionAge = this.configReader.GetDataDeletionAge();
            blobUploadSettings.Filter = this.configReader.GetFilters();
            blobUploadSettings.DeploymentId = this.configReader.GetDeploymentId();

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Blob storage upload enabled: ETW trace container: {0}, File sync interval: {1}, Blob deletion age: {2}",
                blobUploadSettings.EtwTraceContainerName,
                blobUploadSettings.FileSyncInterval,
                blobUploadSettings.BlobDeletionAge);

            return blobUploadSettings;
        }

        private string CreateEtwLogDirectory()
        {
            string etwParentFolder = Path.Combine(
                this.initParam.WorkDirectory,
                Utility.ShortWindowsFabricIdForPaths);

            string etwFolder;
            bool success = Utility.CreateWorkSubDirectory(
                this.traceSource,
                this.logSourceId,
                this.destinationKey,
                string.Concat(Utility.EtwConsumerWorkSubFolderIdPrefix, this.logSourceId),
                etwParentFolder,
                out etwFolder);

            if (success)
            {
                return Path.Combine(etwFolder, LogDirName);
            }

            return null;
        }

        private string CreateBlobUploaderWorkSubDirectory()
        {
            string workParentFolder = Path.Combine(
                this.initParam.WorkDirectory,
                Utility.ShortWindowsFabricIdForPaths);

            string workFolder;
            bool success = Utility.CreateWorkSubDirectory(
                               this.traceSource,
                               this.logSourceId,
                               this.destinationKey,
                               this.etwLogDirName,
                               workParentFolder,
                               out workFolder);

            if (success)
            {
                return Path.Combine(workFolder, AzureConstants.AzureFileUploaderFolder);
            }

            return null;
        }

        private EventIndex GetMaxIndexAlreadyProcessed(string traceFileSubFolder)
        {
            string bookmarkFolder = this.progressManager.BookmarkFolders[traceFileSubFolder];
            this.bytesReadGetMaxIndexAlreadyProcessed = 0;
            var lastEventIndex = this.progressManager.ReadBookmarkFile(
                bookmarkFolder,
                out this.bytesReadGetMaxIndexAlreadyProcessed);

            return lastEventIndex;
        }

        private void UploadFilesToDestinationBlob(object state)
        {
            if (this.stopping)
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "The consumer is being stopped. Therefore, no more files will be uploaded to blob storage in this pass.");
                return;
            }

            this.fileUploadPerfHelper.AzureBlobUploadPassBegin();

            this.fileBlobUploader.UploadFilesToDestinationBlob();

            this.fileUploadPerfHelper.AzureBlobUploadPassEnd();

            this.fileUploadTimer.Start();
        }

        // Settings related to upload of data to blob storage
        internal struct BlobUploadSettings
        {
            internal bool Enabled;
            internal StorageAccountFactory StorageAccountFactory;
            internal string EtwTraceContainerName;
            internal TimeSpan FileSyncInterval;
            internal TimeSpan BlobDeletionAge;
            internal string Filter;
            internal string DeploymentId;
        }
    }
}