// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    internal static class V1Constants
    {
        internal const string EtlProducerSectionNameV1 = "LocalLogStore";
        internal const string EtlProducerEnabledParamNameV1 = "LocalLogDeletionEnabled";
        internal const string AzureBlobConsumerSectionNameV1 = "DiagnosticFileStore";
        internal const string AzureTableConsumerSectionNameV1 = "DiagnosticTableStore";
        internal const string AzureConsumerEnabledParamNameV1 = "IsEnabled";
        internal const string AzureConsumerUploadIntervalParamName = "UploadIntervalInMinutes";

        internal const string V1DataDeletionAgeParamName = "LogDeletionAgeInDays";
        internal const string V1DataDeletionAgeTestParamName = "TestOnlyLogDeletionAgeInMinutes";
    }
}