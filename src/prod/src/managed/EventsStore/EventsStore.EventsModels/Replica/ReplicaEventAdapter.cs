// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Replica
{
    using System;
    using System.Globalization;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    public static class ReplicaEventAdapter
    {
        public static ReplicaEvent Convert(TraceRecord traceRecord)
        {
            Assert.IsNotNull(traceRecord, "traceRecord != null");
            if (traceRecord.GetType() == typeof(StatefulReplicaHealthReportCreatedTraceRecord))
            {
                return new StatefulReplicaHealthReportCreatedEvent((StatefulReplicaHealthReportCreatedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(StatefulReplicaHealthReportExpiredTraceRecord))
            {
                return new StatefulReplicaHealthReportExpiredEvent((StatefulReplicaHealthReportExpiredTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(StatelessReplicaHealthReportCreatedTraceRecord))
            {
                return new StatelessReplicaHealthReportCreatedEvent((StatelessReplicaHealthReportCreatedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(StatelessReplicaHealthReportExpiredTraceRecord))
            {
                return new StatelessReplicaHealthReportExpiredEvent((StatelessReplicaHealthReportExpiredTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ChaosRemoveReplicaFaultScheduledTraceRecord))
            {
                return new ChaosRemoveReplicaFaultScheduledEvent((ChaosRemoveReplicaFaultScheduledTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ChaosRestartReplicaFaultScheduledTraceRecord))
            {
                return new ChaosRestartReplicaFaultScheduledEvent((ChaosRestartReplicaFaultScheduledTraceRecord)traceRecord);
            }

            throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Replica Event of Type: {0} Not supported", traceRecord.GetType()));
        }
    }
}
