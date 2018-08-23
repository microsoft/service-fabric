// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;

    internal static class FileShareUploaderConstants
    {
        internal const string PerformanceTraceType = "Performance";

        internal const string FileShareUploaderFolder = "FSU";
        internal const string V2FileShareUploaderPrefix = "FileShareUploader_";

        // Parameter names in file-share-specific sections in cluster/application manifest
        internal const string EnabledParamName = "IsEnabled";
        internal const string FileConnectionStringIdentifier = "file:";
        internal const string StoreConnectionStringParamName = "StoreConnectionString";
        internal const string UploadIntervalParamName = "UploadIntervalInMinutes";
        internal const string FileSyncIntervalParamName = "FileSyncIntervalInMinutes";
        internal const string DataDeletionAgeParamName = "DataDeletionAgeInDays";
        internal const string TestDataDeletionAgeParamName = "TestOnlyDataDeletionAgeInMinutes";
        internal const string LogFilterParamName = "LogFilter";
        internal const string AppRelativeFolderPathParamName = "RelativeFolderPath";
        internal const string AppDestinationPathParamName = "Path";
        internal const string AppEtwLevelFilterParamName = "LevelFilter";
        internal const string StoreAccessAccountName = "AccountName";
        internal const string StoreAccessAccountPassword = "Password";
        internal const string StoreAccessAccountPasswordIsEncrypted = "PasswordEncrypted";
        internal const string StoreAccessAccountType = "AccountType";
        internal const string AccountTypeDomainUser = "DomainUser";
        internal const string AccountTypeManagedServiceAccount = "ManagedServiceAccount";

        // Default values for file-share-specific settings
        internal const bool FileShareUploadEnabledByDefault = false;
        internal const string DefaultAppEtwLevelFilter = "Informational";
        internal const string PasswordEncryptedDefaultValue = "false";
        internal static readonly TimeSpan DefaultFileUploadInterval = TimeSpan.FromMinutes(10);
        internal static readonly TimeSpan DefaultFileSyncInterval = TimeSpan.FromMinutes(30);
        internal static readonly TimeSpan DefaultDataDeletionAgeDays = TimeSpan.FromDays(14);
        internal static readonly TimeSpan DefaultLocalFolderFlushInterval = TimeSpan.FromMinutes(3);

        internal static readonly TimeSpan MaxDataDeletionAge = TimeSpan.FromDays(1000);

        internal static readonly TimeSpan OldLogDeletionInterval = TimeSpan.FromHours(3);
        internal static readonly TimeSpan OldLogDeletionIntervalForTest = TimeSpan.FromMinutes(1);
    }
}