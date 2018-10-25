// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;

    internal static class MdsConstants
    {
        internal const string EnabledParamName = "IsEnabled";
        internal const string DirectoryParamName = "DirectoryName";
        internal const string TableParamName = "TableName";
        internal const string UploadIntervalParamName = "UploadIntervalInMinutes";
        internal const string TablePriorityParamName = "TablePriority";
        internal const string DiskQuotaParamName = "DiskQuotaInMB";
        internal const string DataDeletionAgeParamName = "DataDeletionAgeInDays";
        internal const string TestDataDeletionAgeParamName = "TestOnlyDataDeletionAgeInMinutes";
        internal const string LogFilterParamName = "LogFilter";

        internal const bool MdsUploadEnabledByDefault = false;
        internal const string DefaultTableName = "etwlogs";
        internal const string DefaultTablePriority = "PriNormal";
        internal const int DefaultDiskQuotaInMB = 4096;

        // Regular expression describing valid table names
        // NOTE: We use similar rules to Azure tables, but allow a shorter length in order to
        // account for any prefixes or suffixes MDS might want to add.
        internal const string TableNameRegularExpression = "^[A-Za-z][A-Za-z0-9]{2,40}$";

        // Regular expression describing valid table names specified in the cluster manifest
        // NOTE 1: We use similar rules to Azure tables, but allow a shorter length in order to
        // account for any prefixes or suffixes MDS might want to add.
        // NOTE 2: The maximum allowed length for this is even shorter than TableNameRegularExpression
        // because we need to account for the fact that the uploader needs to append the subdirectory
        // name (e.g. Fabric, Lease) to the table name
        internal const string TableNameRegularExpressionManifest = "^[A-Za-z][A-Za-z0-9]{2,32}$";

        internal static readonly TimeSpan DefaultUploadInterval = TimeSpan.FromMinutes(5);
        internal static readonly TimeSpan DefaultDataDeletionAge = TimeSpan.FromDays(14);

        internal static readonly TimeSpan MaxDataDeletionAge = TimeSpan.FromDays(1000);
        internal static readonly TimeSpan MaxUploadInterval = MaxDataDeletionAge;
    }
}