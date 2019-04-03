// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Fabric.Management.ServiceModel;

    [Serializable]
    public class KtlLogger
    {
        public string SharedLogFileId { get; set; }

        public string SharedLogFilePath { get; set; }

        public int SharedLogFileSizeInMB { get; set; }

        public FabricKtlLoggerSettingsType ToFabricKtlLoggerSettingsType()
        {
            return new FabricKtlLoggerSettingsType()
            {
                SharedLogFileId = new FabricKtlLoggerSettingsTypeSharedLogFileId() { Value = this.SharedLogFileId },
                SharedLogFilePath = new FabricKtlLoggerSettingsTypeSharedLogFilePath() { Value = this.SharedLogFilePath },
                SharedLogFileSizeInMB = new FabricKtlLoggerSettingsTypeSharedLogFileSizeInMB() { Value = this.SharedLogFileSizeInMB }
            };
        }
    }
}