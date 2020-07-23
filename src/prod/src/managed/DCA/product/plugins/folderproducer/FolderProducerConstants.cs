// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;

    internal static class FolderProducerConstants
    {
        internal const string CrashDumpsDirectory = "CrashDumps";
        internal const string PerformanceCountersDirectoryPrefix = "PerformanceCounters_";
        internal const string PerformanceCountersBinaryDirectoryName = "PerformanceCountersBinary";
        internal const string PerformanceCountersBinaryArchiveDirectoryName = "PerformanceCountersBinaryArchive";

        internal const bool FolderProducerEnabledByDefault = false;
        internal const bool KernelCrashUploadEnabledDefault = true;

        internal static readonly TimeSpan DefaultDataDeletionAge = TimeSpan.FromDays(7);
        internal static readonly TimeSpan MaxDataDeletionAge = TimeSpan.FromDays(1000);
        internal static readonly TimeSpan OldLogDeletionInterval = TimeSpan.FromMinutes(5);
        internal static readonly TimeSpan OldLogDeletionIntervalForTest = TimeSpan.FromMinutes(1);
    }
}