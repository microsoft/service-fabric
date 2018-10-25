// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper
{
    internal static class EtlConsumerConstants
    {
        internal const string LogsForUploadDirectoryPrefix = "FormattedEtwFiles_";
        internal const string TableUploadCacheDirectoryPrefix = "FormattedEtwEvents_";

        internal const string EtwEventCacheFormatVersionString = "Version: 2";
        internal const string FilteredEtwTraceFileExtension = "dtr";
        internal const string FilteredEtwTraceSearchPattern = "*." + FilteredEtwTraceFileExtension + "*";
        internal const string CompressedFilteredEtwTraceFileExtension = ".zip";
        internal const string BootstrapTraceFileSuffix = ".trace";
        internal const string BootstrapTraceSearchPattern = "*" + BootstrapTraceFileSuffix;

        internal const string TraceTypePerformance = "Performance";

        internal const long OldLogDeletionInterval = 3 * 60 * 60 * 1000; // 3 hours
        internal const long OldLogDeletionIntervalForTest = 60 * 1000; // 1 minute
    }
}