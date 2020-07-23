// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Partition
{
    using System;
    using System.Globalization;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    public static class PartitionEventAdapter
    {
        public static PartitionEvent Convert(TraceRecord traceRecord)
        {
            Assert.IsNotNull(traceRecord, "traceRecord != null");
            if (traceRecord.GetType() == typeof(PartitionHealthReportCreatedTraceRecord))
            {
                return new PartitionHealthReportCreatedEvent((PartitionHealthReportCreatedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(PartitionHealthReportExpiredTraceRecord))
            {
                return new PartitionHealthReportExpiredEvent((PartitionHealthReportExpiredTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ReconfigurationCompletedTraceRecord))
            {
                return new PartitionReconfigurationCompletedEvent((ReconfigurationCompletedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(PrimaryMoveAnalysisTraceRecord))
            {
                return new PartitionPrimaryMoveAnalysisEvent((PrimaryMoveAnalysisTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ChaosMovePrimaryFaultScheduledTraceRecord))
            {
                return new ChaosMovePrimaryFaultScheduledEvent((ChaosMovePrimaryFaultScheduledTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ChaosMoveSecondaryFaultScheduledTraceRecord))
            {
                return new ChaosMoveSecondaryFaultScheduledEvent((ChaosMoveSecondaryFaultScheduledTraceRecord)traceRecord);
            }

            throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Partition Event of Type: {0} Not supported", traceRecord.GetType()));
        }
    }
}
