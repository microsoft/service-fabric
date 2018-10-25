// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System;
    using System.IO;

    internal static class ContainerEnvironment
    {
        internal const string ContainersRootFolderName = "Containers";

        internal const string ContainerDeletionMarkerFileName = "RemoveContainerLog.txt";

        internal static string ContainerLogRootDirectory
        {
            get
            {
                return Path.Combine(Utility.LogDirectory, ContainersRootFolderName);
            }
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
    }
}