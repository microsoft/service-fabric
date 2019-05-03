// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Node
{
    using System;
    using System.Globalization;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FabricNode;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    public static class NodeEventAdapter
    {
        public static NodeEvent Convert(TraceRecord traceRecord)
        {
            Assert.IsNotNull(traceRecord, "traceRecord != null");
            if(traceRecord.GetType() == typeof(NodeDownTraceRecord))
            {
                return new NodeDownEvent((NodeDownTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(NodeUpTraceRecord))
            {
                return new NodeUpEvent((NodeUpTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(NodeAddedTraceRecord))
            {
                return new NodeAddedEvent((NodeAddedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(NodeRemovedTraceRecord))
            {
                return new NodeRemovedEvent((NodeRemovedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(NodeDeactivateStartTraceRecord))
            {
                return new NodeDeactivateStartEvent((NodeDeactivateStartTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(NodeDeactivateCompleteTraceRecord))
            {
                return new NodeDeactivateCompleteEvent((NodeDeactivateCompleteTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(NodeOpenedSuccessTraceRecord))
            {
                return new NodeOpenedSuccessEvent((NodeOpenedSuccessTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(NodeOpenedFailedTraceRecord))
            {
                return new NodeOpenFailedEvent((NodeOpenedFailedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(NodeClosedTraceRecord))
            {
                return new NodeClosedEvent((NodeClosedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(NodeAbortedTraceRecord))
            {
                return new NodeAbortedEvent((NodeAbortedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(NodeHealthReportCreatedTraceRecord))
            {
                return new NodeHealthReportCreatedEvent((NodeHealthReportCreatedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(NodeHealthReportExpiredTraceRecord))
            {
                return new NodeHealthReportExpiredEvent((NodeHealthReportExpiredTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ChaosRestartNodeFaultScheduledTraceRecord))
            {
                return new ChaosRestartNodeFaultScheduledEvent((ChaosRestartNodeFaultScheduledTraceRecord)traceRecord);
            }

            throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Node Event of Type: {0} Not supported", traceRecord.GetType()));
        }
    }
}
