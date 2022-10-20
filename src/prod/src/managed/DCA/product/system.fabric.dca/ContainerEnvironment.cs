// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System;
    using System.IO;

    internal enum ContainerMarkerFileType
    {
        ContainerRemoveLogMarker,
        ContainerProcessLogMarker,
        NotInContainerFolder,
        None
    }

    internal class ContainerEnvironment
    {
        internal const string ContainersRootFolderName = "Containers";
        internal const string ContainerLogMarkerFileNameFilter = "*ContainerLog.txt";
        internal const string ContainerRemoveLogMarkerFileName = "RemoveContainerLog.txt";
        internal const string ContainerProcessLogMarkerFileName = "ProcessContainerLog.txt";

        private static readonly TimeSpan DefaultContainerFolderCleanupTimerInterval = TimeSpan.FromMinutes(10.0);
        private static readonly TimeSpan DefaultMaxContainerLogsProcessingWaitTime = TimeSpan.FromMinutes(30.0);

        internal virtual TimeSpan ContainerFolderCleanupTimerInterval
        {
            get
            {
                return ContainerEnvironment.DefaultContainerFolderCleanupTimerInterval;
            }
        }

        internal virtual TimeSpan MaxContainerLogsProcessingWaitTime
        {
            get
            {
                return ContainerEnvironment.DefaultMaxContainerLogsProcessingWaitTime;
            }
        }

        internal virtual string ContainerLogRootDirectory
        {
            get
            {
                return Path.Combine(Utility.LogDirectory, ContainersRootFolderName);
            }
        }

        internal static string GetAppInstanceIdFromContainerFolderFullPath(string containerFolderPath)
        {
            return Path.Combine(ContainerEnvironment.ContainersRootFolderName, Path.GetFileName(containerFolderPath));
        }

        internal static bool IsContainerApplication(string applicationInstanceId)
        {
            return applicationInstanceId.StartsWith(ContainersRootFolderName, StringComparison.OrdinalIgnoreCase);
        }

        internal static string GetContainerLogFolder(string applicationId)
        {
            return Path.Combine(Utility.LogDirectory, applicationId);
        }

        internal static string GetContainerApplicationInstanceId(string logSourceId)
        {
            var logSourceIdSpit = logSourceId.Split('_');
            return string.Join("_", logSourceIdSpit, 0, logSourceIdSpit.Length - 1);
        }

        internal ContainerMarkerFileType GetContainerMarkerFileType(string fileFullPath)
        {
            string fileName = Path.GetFileName(fileFullPath);
            string folderName = Path.GetFileName(Path.GetDirectoryName(fileFullPath));

            string expectedMarkerFilePath = Path.Combine(
                this.ContainerLogRootDirectory,
                folderName);

            // check whether file is inside a container folder root.
            if (!expectedMarkerFilePath.Equals(Path.GetDirectoryName(fileFullPath), StringComparison.OrdinalIgnoreCase))
            {
                return ContainerMarkerFileType.NotInContainerFolder;
            }

            if (fileName.Equals(ContainerProcessLogMarkerFileName, StringComparison.OrdinalIgnoreCase))
            {
                return ContainerMarkerFileType.ContainerProcessLogMarker;
            }
            else if (fileName.Equals(ContainerRemoveLogMarkerFileName, StringComparison.OrdinalIgnoreCase))
            {
                return ContainerMarkerFileType.ContainerRemoveLogMarker;
            }
            else
            {
                return ContainerMarkerFileType.None;
            }
        }
    }
}
