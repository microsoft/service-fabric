// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Concurrent;
    using System.Fabric.Common;
    using System.Fabric.Dca;
    using System.IO;
    using System.Threading;

    // This class manages the service package table
    internal class ContainerManager : IDisposable
    {
        private const string TraceType = "ContainerManager";

        private AppInstanceManager appInstanceManager;
        private ContainerEnvironment containerEnvironment;
        private object appInstanceManagerLock = new object();
        private ReaderWriterLockSlim processingMarkersRWLock = new ReaderWriterLockSlim();

        private FileSystemWatcher containerFolderWatcher;
        private ConcurrentDictionary<string, ContainerFolderScheduledForRemoval> containersFoldersScheduledForRemoval = new ConcurrentDictionary<string, ContainerFolderScheduledForRemoval>();
        private DcaTimer cleanupTimer;

        internal ContainerManager(AppInstanceManager appInstanceManager, ContainerEnvironment containerEnvironment)
        {
            this.appInstanceManager = appInstanceManager;
            this.containerEnvironment = containerEnvironment;

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Watching directory for creation of new container trace folders {0}",
                this.containerEnvironment.ContainerLogRootDirectory);

            // Watcher needs to be created before startup listing of folders
            // Otherwise there is the risk a file will be created between the two and it will be missed.
            // WriteLock is created so prevent race condition with file watchers
            this.processingMarkersRWLock.EnterWriteLock();
            try
            {
                if (Directory.Exists(this.containerEnvironment.ContainerLogRootDirectory))
                {
                    this.CreateContainerFolderWacher();

                    foreach (var directory in Directory.EnumerateDirectories(this.containerEnvironment.ContainerLogRootDirectory))
                    {
                        this.ContainerLogFolderProcessingStartup(directory);
                    }
                }
                else
                {
                    Utility.TraceSource.WriteError(
                        TraceType,
                        "Not monitoring directory for creation of new container trace folders since directory doesn't exist {0}",
                        this.containerEnvironment.ContainerLogRootDirectory);
                }
            }
            finally
            {
                this.processingMarkersRWLock.ExitWriteLock();
            }

            // Arm the cleanup timer.
            this.cleanupTimer = new DcaTimer(
                ContainerManager.TraceType,
                new TimerCallback(this.BackgroundCleanup),
                (long)this.containerEnvironment.ContainerFolderCleanupTimerInterval.TotalMilliseconds);
            this.cleanupTimer.Start((long)this.containerEnvironment.ContainerFolderCleanupTimerInterval.TotalMilliseconds);
        }

        public void Dispose()
        {
            this.containerFolderWatcher.EnableRaisingEvents = false;
            this.containerFolderWatcher.Dispose();

            this.cleanupTimer.StopAndDispose();
        }

        private void ContainerLogFolderProcessingStartup(string containerLogFolderFullPath)
        {
            string appInstanceId = ContainerEnvironment.GetAppInstanceIdFromContainerFolderFullPath(containerLogFolderFullPath);
            Utility.TraceSource.WriteInfo(TraceType, "Initializing handling of container traces from {0}", appInstanceId);

            bool containsProcessLogMarker = File.Exists(Path.Combine(containerLogFolderFullPath, ContainerEnvironment.ContainerProcessLogMarkerFileName));
            if (containsProcessLogMarker)
            {
                this.InitializeProcessingOfContainerLogsFolder(containerLogFolderFullPath);
            }

            bool containsRemoveLogMarker = File.Exists(Path.Combine(containerLogFolderFullPath, ContainerEnvironment.ContainerRemoveLogMarkerFileName));
            if (containsRemoveLogMarker)
            {
                this.ScheduleRemovalOfContainerAppInstanceAndFolder(containerLogFolderFullPath);
            }
        }

        private void CreateContainerFolderWacher()
        {
            this.containerFolderWatcher = new FileSystemWatcher(
                this.containerEnvironment.ContainerLogRootDirectory);

            this.containerFolderWatcher.Filter = ContainerEnvironment.ContainerLogMarkerFileNameFilter;
            this.containerFolderWatcher.IncludeSubdirectories = true;
            this.containerFolderWatcher.Created += this.OnFileCreatedInContainerFolder;
            this.containerFolderWatcher.EnableRaisingEvents = true;
        }

        private void OnFileCreatedInContainerFolder(object source, FileSystemEventArgs e)
        {
            this.processingMarkersRWLock.EnterReadLock();
            try
            {
                switch (this.containerEnvironment.GetContainerMarkerFileType(e.FullPath))
                {
                    case ContainerMarkerFileType.ContainerProcessLogMarker:
                        this.OnProcessLogMarkerFileCreated(e.FullPath);
                        break;
                    case ContainerMarkerFileType.ContainerRemoveLogMarker:
                        this.OnRemoveLogMarkerFileCreated(e.FullPath);
                        break;
                    case ContainerMarkerFileType.NotInContainerFolder:
                        return;
                    default:
                        Utility.TraceSource.WriteWarning(
                            TraceType,
                            "Not a marker file created with path : {0}",
                            e.FullPath);
                        return;
                }
            }
            finally
            {
                this.processingMarkersRWLock.ExitReadLock();
            }
        }

        private void OnProcessLogMarkerFileCreated(string containerProcessLogMarkerFullPath)
        {
            Utility.TraceSource.WriteInfo(TraceType, "Processing container traces folder : {0}", FabricPath.GetDirectoryName(containerProcessLogMarkerFullPath));
            this.InitializeProcessingOfContainerLogsFolder(FabricPath.GetDirectoryName(containerProcessLogMarkerFullPath));
        }

        private void OnRemoveLogMarkerFileCreated(string containerRemoveLogMarkerFullPath)
        {
            Utility.TraceSource.WriteInfo(TraceType, "Removing container traces folder : {0}", FabricPath.GetDirectoryName(containerRemoveLogMarkerFullPath));
            this.ScheduleRemovalOfContainerAppInstanceAndFolder(FabricPath.GetDirectoryName(containerRemoveLogMarkerFullPath));
        }

        private void BackgroundCleanup(object context)
        {
            // wrapping in try/catch to avoid missing the rescheduling of next pass.
            try
            {
                foreach (var containerScheduleForRemovalKeyValue in this.containersFoldersScheduledForRemoval)
                {
                    this.TryCleanupContainerFolderScheduledForRemoval(containerScheduleForRemovalKeyValue.Value);
                }
            }
            catch (Exception e)
            {
                Utility.TraceSource.WriteInfo(TraceType, "Exception when trying to cleanup container log folders scheduled for removal. {0}", e);
            }

            this.cleanupTimer.Start((long)this.containerEnvironment.ContainerFolderCleanupTimerInterval.TotalMilliseconds);
        }

        private void TryCleanupContainerFolderScheduledForRemoval(ContainerFolderScheduledForRemoval containerScheduledForRemoval)
        {
            if (containerScheduledForRemoval.IsProcessingWaitTimeExpired(this.containerEnvironment.MaxContainerLogsProcessingWaitTime) &&
                this.TryRemovingContainerAppInstanceAndLogFolder(containerScheduledForRemoval))
            {
                ContainerFolderScheduledForRemoval unusedOutParameter;

                if (this.containersFoldersScheduledForRemoval.TryRemove(
                    containerScheduledForRemoval.ContainerLogFullPath,
                    out unusedOutParameter))
                {
                    Utility.TraceSource.WriteInfo(
                        TraceType,
                        "Successfully removed container traces folder: {0},  and AppInstance : {1}",
                        containerScheduledForRemoval.ContainerLogFullPath,
                        ContainerEnvironment.GetAppInstanceIdFromContainerFolderFullPath(containerScheduledForRemoval.ContainerLogFullPath));
                }
                else
                {
                    Utility.TraceSource.WriteInfo(
                        TraceType,
                        "Failed to removed container traces folder from ContainerFolderScheduledForRemoval list. Container folder path : {0}",
                        containerScheduledForRemoval.ContainerLogFullPath);
                }
            }
        }

        private void InitializeProcessingOfContainerLogsFolder(string containerLogFolderFullPath)
        {
            string appInstanceId = ContainerEnvironment.GetAppInstanceIdFromContainerFolderFullPath(containerLogFolderFullPath);

            lock (this.appInstanceManagerLock)
            {
                // this is needed because of the race condition we have at startup
                // where watcher could be triggered before initial folders are listed and processed.
                if (!this.appInstanceManager.Contains(appInstanceId))
                {
                    this.appInstanceManager.CreateApplicationInstance(appInstanceId, null);
                }
            }
        }

        private void ScheduleRemovalOfContainerAppInstanceAndFolder(string containerLogFolderFullPath)
        {
            bool containsProcessLogMarker = File.Exists(Path.Combine(containerLogFolderFullPath, ContainerEnvironment.ContainerProcessLogMarkerFileName));

            // schedule removal if contains contains marker for processing the folder or if removal of the folder failed.
            if (containsProcessLogMarker || !this.TryRemoveContainerFolder(containerLogFolderFullPath))
            {
                if (!this.containersFoldersScheduledForRemoval.TryAdd(containerLogFolderFullPath, new ContainerFolderScheduledForRemoval(containerLogFolderFullPath)))
                {
                    Utility.TraceSource.WriteInfo(
                        TraceType,
                        "Failed to add container traces folder to ContainerFolderScheduledForRemoval list. Possible duplication. Container folder path : {0}",
                        containerLogFolderFullPath);
                }
            }
        }

        private bool TryRemoveContainerFolder(string containerFolderFullPath)
        {
            try
            {
                Directory.Delete(containerFolderFullPath, recursive: true);
            }
            catch (Exception e)
            {
                Utility.TraceSource.WriteInfo(TraceType, "Failed to delete container folder with exception: {0}", e);
                return false;
            }

            return true;
        }

        private bool TryRemovingContainerAppInstanceAndLogFolder(ContainerFolderScheduledForRemoval deletedContainer)
        {
            try
            {
                lock (this.appInstanceManagerLock)
                {
                    this.appInstanceManager.DeleteApplicationInstance(ContainerEnvironment.GetAppInstanceIdFromContainerFolderFullPath(deletedContainer.ContainerLogFullPath));
                }
            }
            catch (Exception e)
            {
                Utility.TraceSource.WriteInfo(TraceType, "Failed to remove appInstance with exception: {0}", e);
            }

            return this.TryRemoveContainerFolder(deletedContainer.ContainerLogFullPath);
        }
    }
}