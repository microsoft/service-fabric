// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;

    internal static class AzureConstants
    {
        internal const string PerformanceTraceType = "Performance";
        internal const string AzureFileUploaderFolder = "AFU";

        // Namespace in which to create the mutex used to synchronize with other
        // processes that configure the Azure Diagnostic Manager
        internal const string WinFabNamespaceAlias = "WindowsFabricObjectPrivateNamespace-{0FC73FB6-F925-4340-A1C1-4425BCB56B31}";

        // Name of the mutex used to synchronize with other processes that configure
        // the Azure Diagnostic Manager
        internal const string AzureDiagnosticManagerMutexName = "AzureDiagnosticManagerMutex-{DE02F206-D4D5-4533-8C2D-2155DC7AEAF9}";

        // Connection string for Azure Storage Emulator
        internal const string DevelopmentStorageConnectionString = "UseDevelopmentStorage=true";
        
        // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Well known key for Azure storage enumulator")]
        internal const string DevelopmentStorageAccountWellKnownKey = "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw==";

        // Regular expression describing valid Azure table names
        internal const string AzureTableNameRegularExpression = "^[A-Za-z][A-Za-z0-9]{2,62}$";

        // Parameter names in Azure-specific sections in settings.xml
        internal const string EnabledParamName = "IsEnabled";
        internal const string ConnectionStringParamName = "StoreConnectionString";
        internal const string ContainerParamName = "ContainerName";
        internal const string UploadIntervalParamName = "UploadIntervalInMinutes";
        internal const string FileSyncIntervalParamName = "FileSyncIntervalInMinutes";
        internal const string DataDeletionAgeParamName = "DataDeletionAgeInDays";
        internal const string TestDataDeletionAgeParamName = "TestOnlyDataDeletionAgeInMinutes";
        internal const string LogFilterParamName = "LogFilter";
        internal const string EtwTraceTableParamName = "TableName";
        internal const string EtwTraceTablePrefixParamName = "TableNamePrefix";
        internal const string UploadConcurrencyParamName = "UploadConcurrencyCount";
        internal const string AppEtwLevelFilterParamName = "LevelFilter";
        internal const string AppManifestConnectionStringParamName = "ConnectionString";
        internal const string ConnectionStringIsEncryptedParamName = "ConnectionStringIsEncrypted";
        internal const string TestOnlyEtwManifestFilePatternParamName = "TestOnlyEtwManifestFilePattern";
        internal const string DeploymentId = "DeploymentId";

        // Default values for Azure-specific settings
        internal const bool BlobUploadEnabledByDefault = false;
        internal const bool TableUploadEnabledByDefault = false;
        internal const string DefaultEtwTraceContainerName = "etwlogs";
        internal const string DefaultLttTraceContainerName = "lttlogs";
        internal const string DefaultContainerName = "data";
        internal const string DefaultTableName = "etwlogs";
        internal const string DefaultTableNamePrefix = "etwlogs";
        internal const string DefaultAppEtwLevelFilter = "Informational";
        internal const string DefaultDeploymentId = "";

        internal const string DsmsTraceType = "Dsms";

        /// <summary>
        /// MaxBatchConcurrencyCount is the maximum length of the wait handle array
        /// that we can wait on.
        /// </summary>
        internal const int MaxBatchConcurrencyCount = 64;

        internal const int MaxTableNamePrefixLengthForDeploymentIdInclusion = 32;

        internal static readonly TimeSpan DefaultBlobUploadIntervalMinutes = TimeSpan.FromMinutes(10);
        internal static readonly TimeSpan DefaultFileSyncIntervalMinutes = TimeSpan.FromMinutes(30);
        internal static readonly TimeSpan DefaultDataDeletionAge = TimeSpan.FromDays(14);

        internal static readonly TimeSpan MaxDataDeletionAge = TimeSpan.FromDays(1000);

        internal static readonly TimeSpan OldLogDeletionInterval = TimeSpan.FromHours(3);
        internal static readonly TimeSpan OldLogDeletionIntervalForTest = TimeSpan.FromMinutes(1);
    }
}