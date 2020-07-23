// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Collections.Generic;
    using Microsoft.WindowsAzure.ServiceRuntime.Management;

    /// <summary>
    /// A wrapper for <see cref="ManagementNotification"/> in the Azure Management protocol (MR) SDK.
    /// </summary>
    internal interface IManagementNotificationContext
    {
        string NotificationId { get; }
        string ActiveJobStepId { get; }
        string ActiveJobId { get; }
        NotificationType NotificationType { get; }
        DateTime NotificationDeadline { get; }
        ManagementNotificationStatus NotificationStatus { get; }
        int ActiveJobStepTargetUD { get; }
        JobType ActiveJobType { get; }
        IEnumerable<ImpactedInstance> ImpactedInstances { get; }
        bool ActiveJobIncludesTopologyChange { get; }
        ulong Incarnation { get; }
        ManagementJobDetailedStatus ActiveJobDetailedStatus { get; }
        string ActiveJobContextId { get; }
        ManagementJobStepStatus ActiveJobStepStatus { get; }

        void SignalReady();
        void SignalError(string errorDescription);
    }
}