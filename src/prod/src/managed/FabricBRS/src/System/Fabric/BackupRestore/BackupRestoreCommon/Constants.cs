// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System;
    using System.Fabric.Common;
    using System.Globalization;

    internal static class Constants
    {
        internal const string BackupPolicyStoreName = "BackupPolicyStore";
        internal const string BackupMappingStoreName = "BackupMappingStore";
        internal const string WorkItemStoreName = "WorkItemStore";
        internal const string ConfigStoreName = "BRSConfigStore";
        internal const string WorkItemInProcessStoreName = "WorkItemInProcessStore";
        internal const string BackupFolderNameFormat = "yyyy-MM-dd HH.mm.ss";
        internal const string RestoreStoreName = "RestoreStore";
        internal const string SuspendBackupStoreName = "SuspendStore";
        internal const string BackupPartitionStoreName = "BackupPartitionStore";
        internal const string RetentionStoreName = "RetentionStore";
        internal const string CleanUpStoreName = "CleanUpStore";

        internal const string WorkItemQueue = "WorkItemQueue";
        internal const string WorkItemQueue1Min = "WorkItemQueue1Min";
        internal const string WorkItemQueue2Min = "WorkItemQueue2Min";
        internal const string WorkItemQueue4Min = "WorkItemQueue4Min";
        internal const string WorkItemQueue8Min = "WorkItemQueue8Min";

        internal const string EncryptionCertThumbprintKey = "EncryptionCertThumprint";
        internal const string SecretUpdateInProgress = "SecretUpdateInProgress";

        internal static TimeSpan StoreTimeOut = new TimeSpan(5000);
        internal static TimeSpan GetDataLossProgressTimeout = new TimeSpan(5000);
        internal static TimeSpan RequestDataLossTimeout = new TimeSpan(5000);
        internal static TimeSpan CheckRestoreTimeSpan = new TimeSpan(0, 1, 0);

        internal const char ContinuationTokenSeparatorChar = '+';

        internal const int MaxWorkItemProcess = 5;
        internal const int HttpNotFoundStatusCode = 404;

        internal static readonly double RuntimeVersion;

        static Constants()
        {
            RuntimeVersion = GetRuntimeVersion();
        }

        static double GetRuntimeVersion()
        {
            var currentVersion = FabricRuntime.FabricRuntimeFactory.GetCurrentRuntimeVersion();
            ReleaseAssert.AssertIf(String.IsNullOrEmpty(currentVersion), "Runtime version fetched is empty or null");

            var splitVersions = currentVersion.Split('.');
            ReleaseAssert.AssertIf(splitVersions.Length < 2, "Runtime version fetched incorrectly");

            var majorMinorVersion = splitVersions[0] + "." + splitVersions[1];
            return Double.Parse(majorMinorVersion, CultureInfo.InvariantCulture);
        }
    }

    enum WorkItemQueueStatus
    {
        ContinueProcessing,
        ContinueWithWait
    }
    enum FabricBackupResourceType
    {
        Error,
        ApplicationUri,
        ServiceUri,
        PartitionUri
    }

    enum BackupProtectionLevel
    {
        Application,
        Service,
        Partition
    }

    enum WorkItemPropogationType
    {
        Enable,
        Disable,
        UpdateBackupPolicy,
        UpdateProtection,
        SuspendPartition,
        ResumePartition,
        UpdatePolicyAfterRestore
    }

    enum WorkItemQueueRunType
    {
        WorkItemQueue,
        WorkItemQueue1MinDelay,
        WorkItemQueue2MinDelay,
        WorkItemQueue4MinDelay,
        WorkItemQueue8MinDelay,
    }
}