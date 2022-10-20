// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Collections.Generic;
    using Microsoft.WindowsAzure.ServiceRuntime.Management;

    internal class MockManagementNotificationContext : IManagementNotificationContext
    {
        public MockManagementNotificationContext()
        {
            NotificationId = Guid.NewGuid().ToString();
            ActiveJobStepId = Guid.NewGuid().ToString();
            ActiveJobId = Guid.NewGuid().ToString();
            ImpactedInstances = new List<ImpactedInstance>();
            ActiveJobContextId = Guid.NewGuid().ToString();
        }

        public string NotificationId { get; set; }
        public string ActiveJobStepId { get; set; }
        public string ActiveJobId { get; set; }
        public NotificationType NotificationType { get; set; }
        public DateTime NotificationDeadline { get; set; }
        public ManagementNotificationStatus NotificationStatus { get; set; }
        public int ActiveJobStepTargetUD { get; set; }
        public JobType ActiveJobType { get; set; }
        public IEnumerable<ImpactedInstance> ImpactedInstances { get; set; }
        public bool ActiveJobIncludesTopologyChange { get; set; }
        public ulong Incarnation { get; set; }
        public ManagementJobDetailedStatus ActiveJobDetailedStatus { get; set; }
        public string ActiveJobContextId { get; set; }
        public ManagementJobStepStatus ActiveJobStepStatus { get; set; }

        public Action SignalReadyAction { get; set; }

        public Action<string> SignalErrorAction { get; set; }

        public void SignalReady()
        {
            if (SignalReadyAction != null)
                SignalReadyAction();
        }

        public void SignalError(string errorDescription)
        {
            if (SignalErrorAction != null)
                SignalErrorAction(errorDescription);
        }
    }
}