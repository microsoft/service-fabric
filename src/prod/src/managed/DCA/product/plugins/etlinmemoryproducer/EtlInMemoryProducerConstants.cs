// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;

    internal static class EtlInMemoryProducerConstants
    {
        internal const string EnabledParamName = "IsEnabled";
        internal const string EtlReadIntervalParamName = "EtlReadIntervalInMinutes";
        internal const string UploadIntervalParamName = "UploadIntervalInMinutes";
        internal const string DataDeletionAgeParamName = "DataDeletionAgeInDays";
        internal const string WindowsFabricEtlTypeParamName = "WindowsFabricEtlType";
        internal const string ServiceFabricEtlTypeParamName = "ServiceFabricEtlType";
        internal const string TestDataDeletionAgeParamName = "TestOnlyDataDeletionAgeInMinutes";
        internal const string TestEtlPathParamName = "TestOnlyEtlPath";
        internal const string TestEtlFilePatternsParamName = "TestOnlyEtlFilePatterns";
        internal const string TestProcessingWinFabEtlFilesParamName = "TestOnlyProcessingWinFabEtlFiles";

        internal const string DefaultEtl = "DefaultEtl";
        internal const string QueryEtl = "QueryEtl";
        internal const string OperationalEtl = "OperationalEtl";

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

        internal const int MethodExecutionMaxRetryCount = 3;
        internal const int MethodExecutionInitialRetryIntervalMs = 1000;
        internal const int MethodExecutionMaxRetryIntervalMs = int.MaxValue;

        internal static readonly TimeSpan DefaultEtlReadInterval = TimeSpan.FromMinutes(5);
        internal static readonly TimeSpan DefaultDataDeletionAge = TimeSpan.FromDays(7);
        internal static readonly TimeSpan MaxDataDeletionAge = TimeSpan.FromDays(1000);
    }
}