
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM
{
    public enum MonitoredUpgradeFailureAction
    {
        Invalid = 0x0,
        Rollback = 0x1,
        Manual = 0x2,
    }

    public enum RollingUpgradeMode
    {
        Invalid = 0x0,
        UnmonitoredAuto = 0x1,
        UnmonitoredManual = 0x2,
        Monitored = 0x3,
    }

    public enum UpgradeType
    {
        Invalid = 0x0,
        Rolling = 0x1,
        Rolling_ForceRestart = 0x2,
        Rolling_NotificationOnly = 0x3,
    }

    public enum UpgradeFailureReason
    {
        None = 0x0,
        Interrupted = 0x1,
        HealthCheck = 0x2,
        UpgradeDomainTimeout = 0x3,
        OverallUpgradeTimeout = 0x4,
        ProcessingFailure = 0x5,
    }

    public enum FabricUpgradeState
    {
        Invalid = 0x0,
        CompletedRollforward = 0x1,
        CompletedRollback = 0x2,
        RollingForward = 0x3,
        RollingBack = 0x4,
        Interrupted = 0x5,
        Failed = 0x6,
    }

    public enum ApplicationUpgradeState
    {
        Invalid = 0x0,
        CompletedRollforward = 0x1,
        CompletedRollback = 0x2,
        RollingForward = 0x3,
        RollingBack = 0x4,
        Interrupted = 0x5,
        Failed = 0x6,
    }
}
