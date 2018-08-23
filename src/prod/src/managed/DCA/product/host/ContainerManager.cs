// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca;
    using System.IO;
    using System.Threading;

    // This class manages the service package table
    internal class ContainerManager : IDisposable
    {
        private const string TraceType = "ContainerManager";

        private AppInstanceManager appInstanceManager;

        private FileSystemWatcher watcher;

        private Dictionary<string, FileSystemWatcher> containerRemovalMarkerWatcher = new Dictionary<string, FileSystemWatcher>();

        private List<string> applicationInstancesToDelete = new List<string>();

        private DcaTimer cleanupTimer;

        public ContainerManager(AppInstanceManager appInstanceManager)
        {
            Utility.TraceSource.WriteInfo(TraceType, "Watching directory for creation of new container trace folders {0}", ContainerEnvironment.ContainerLogRootDirectory);
            this.appInstanceManager = appInstanceManager;
            if (Directory.Exists(ContainerEnvironment.ContainerLogRootDirectory))
            {
                foreach (var directory in Directory.EnumerateDirectories(ContainerEnvironment.ContainerLogRootDirectory))
                {
                    var subDirectory = Path.GetFileName(directory);
                    var appInstanceId = Path.Combine(ContainerEnvironment.ContainersRootFolderName, subDirectory);
                    Utility.TraceSource.WriteInfo(TraceType, "Reading container traces from {0}", appInstanceId);
                    if (!File.Exists(Path.Combine(directory, ContainerEnvironment.ContainerDeletionMarkerFileName)))
                    {
                        this.appInstanceManager.CreateApplicationInstance(appInstanceId, null);
                    }
                    else
                    {
                        this.applicationInstancesToDelete.Add(appInstanceId);
                    }
                }
            }
            else
            {
                Directory.CreateDirectory(ContainerEnvironment.ContainerLogRootDirectory);
            }

            // Arm FSW to watch for new folders.
            this.watcher = new FileSystemWatcher(ContainerEnvironment.ContainerLogRootDirectory);
            this.watcher.Created += this.OnFolderCreated;
            this.watcher.EnableRaisingEvents = true;

            // Arm the cleanup timer.
            this.cleanupTimer = new DcaTimer(
                ContainerManager.TraceType, 
                new TimerCallback(this.BackgroundCleanup), 
                (long)TimeSpan.FromMinutes(10.0).TotalMilliseconds);
            this.cleanupTimer.Start();
        }

        public void OnFolderCreated(object source, FileSystemEventArgs e)
        {
            var containerFolderName = Path.Combine(ContainerEnvironment.ContainersRootFolderName, e.Name);
            Utility.TraceSource.WriteInfo(TraceType, "Reading container traces from {0}", containerFolderName);
            this.appInstanceManager.CreateApplicationInstance(containerFolderName, null);

            // Arm a watcher to watch for the marker file to indicate that the folder is done.
            var markerFileWatcher = new FileSystemWatcher(e.FullPath);
            this.watcher.Created += this.OnFolderDeletionMarkerCreated;
            this.containerRemovalMarkerWatcher.Add(containerFolderName, markerFileWatcher);
        }

        public void OnFolderDeletionMarkerCreated(object source, FileSystemEventArgs e)
        {
            if (e.Name != ContainerEnvironment.ContainerDeletionMarkerFileName)
            {
                return;
            }

            Utility.TraceSource.WriteInfo(TraceType, "Container traces folder mark for removal {0}", e.Name);
            var containerFolderName = Path.GetFileName(Path.GetDirectoryName(e.FullPath));
            containerFolderName = Path.Combine(ContainerEnvironment.ContainersRootFolderName, containerFolderName);
            this.containerRemovalMarkerWatcher.Remove(containerFolderName);
            this.applicationInstancesToDelete.Add(containerFolderName);
        }

        public void BackgroundCleanup(object context)
        {
            foreach (var applicationInstanceId in this.applicationInstancesToDelete)
            {
                Utility.TraceSource.WriteInfo(TraceType, "Container traces folder being deleted {0}", applicationInstanceId);
                var fullPath = ContainerEnvironment.GetContainerLogFolder(applicationInstanceId);
                var files = Directory.GetFiles(fullPath);
                if (files.Length == 1 && Path.GetFileName(files[0]) == ContainerEnvironment.ContainerDeletionMarkerFileName)
                {
                    try
                    {
                        this.appInstanceManager.DeleteApplicationInstance(applicationInstanceId);
                        Directory.Delete(fullPath, true);
                    }
                    catch (Exception e)
                    {
                        Utility.TraceSource.WriteInfo(TraceType, "Container traces folder delete failed {0} becauuse {1}. Will retry later", applicationInstanceId, e);
                    }
                }
            }

            this.cleanupTimer.Start();
        }

        public void Dispose()
        {
            this.watcher.EnableRaisingEvents = false;
            this.cleanupTimer.StopAndDispose();
        }
    }
}