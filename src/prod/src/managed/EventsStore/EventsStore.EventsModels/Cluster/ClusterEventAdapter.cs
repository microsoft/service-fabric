// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Cluster
{
    using System;
    using System.Globalization;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    public static class ClusterEventAdapter
    {
        public static ClusterEvent Convert(TraceRecord traceRecord)
        {
            Assert.IsNotNull(traceRecord, "traceRecord != null");
            if (traceRecord.GetType() == typeof(ClusterUpgradeStartTraceRecord))
            {
                return new ClusterUpgradeStartEvent((ClusterUpgradeStartTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ClusterUpgradeCompleteTraceRecord))
            {
                return new ClusterUpgradeCompleteEvent((ClusterUpgradeCompleteTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ClusterUpgradeDomainCompleteTraceRecord))
            {
                return new ClusterUpgradeDomainCompleteEvent((ClusterUpgradeDomainCompleteTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ClusterUpgradeRollbackStartTraceRecord))
            {
                return new ClusterUpgradeRollbackStartEvent((ClusterUpgradeRollbackStartTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ClusterUpgradeRollbackCompleteTraceRecord))
            {
                return new ClusterUpgradeRollbackCompleteEvent((ClusterUpgradeRollbackCompleteTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ClusterHealthReportCreatedTraceRecord))
            {
                return new ClusterHealthReportCreatedEvent((ClusterHealthReportCreatedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ClusterHealthReportExpiredTraceRecord))
            {
                return new ClusterHealthReportExpiredEvent((ClusterHealthReportExpiredTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ChaosStartedTraceRecord))
            {
                return new ChaosStartedEvent((ChaosStartedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ChaosStoppedTraceRecord))
            {
                return new ChaosStoppedEvent((ChaosStoppedTraceRecord)traceRecord);
            }

            throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Cluster Event of Type: {0} Not supported", traceRecord.GetType()));
        }
    }
}
