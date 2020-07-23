// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Service
{
    using System;
    using System.Globalization;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM;

    public static class ServiceEventAdapter
    {
        public static ServiceEvent Convert(TraceRecord traceRecord)
        {
            Assert.IsNotNull(traceRecord, "traceRecord != null");
            if (traceRecord.GetType() == typeof(ServiceCreatedTraceRecord))
            {
                return new ServiceCreatedEvent((ServiceCreatedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ServiceDeletedTraceRecord))
            {
                return new ServiceDeletedEvent((ServiceDeletedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ServiceHealthReportCreatedTraceRecord))
            {
                return new ServiceHealthReportCreatedEvent((ServiceHealthReportCreatedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ServiceHealthReportExpiredTraceRecord))
            {
                return new ServiceHealthReportExpiredEvent((ServiceHealthReportExpiredTraceRecord)traceRecord);
            }

            throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Service Event of Type: {0} Not supported", traceRecord.GetType()));
        }
    }
}
