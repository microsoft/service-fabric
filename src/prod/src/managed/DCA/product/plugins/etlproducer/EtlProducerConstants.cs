// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;

    internal static class EtlProducerConstants
    {
        internal const bool EtlProducerEnabledByDefault = false;

        internal const int DefaultTraceSessionFlushIntervalSeconds = 60;
        internal const double TraceSessionFlushIntervalMultiplier = 1.25;

        internal const string TracesDirectory = "Traces";
        internal const string MarkerFileDirectory = "ArchivedTraces";
        internal const string LastEtlReadDirectory = "LastEtlReadInfo";
        internal const string LastEndTimeFormatVersionString = "Version: 1";
        internal const string QueryTracesDirectory = "QueryTraces";
        internal const string OperationalTracesDirectory = "OperationalTraces";
        internal const string QueryMarkerFileDirectory = "ArchivedQueryTraces";
        internal const string OperationalMarkerFileDirectory = "ArchivedOperationalTraces";

        internal static readonly TimeSpan DefaultEtlReadInterval = TimeSpan.FromMinutes(5);
        internal static readonly TimeSpan DefaultDataDeletionAge = TimeSpan.FromDays(7);
        internal static readonly TimeSpan MaxDataDeletionAge = TimeSpan.FromDays(1000);
    }
}