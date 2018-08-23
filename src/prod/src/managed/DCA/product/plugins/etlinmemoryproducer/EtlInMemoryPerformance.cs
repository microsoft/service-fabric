// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;

    // Class for gathering information related to the performance of ETL processing
    internal class EtlInMemoryPerformance
    {
        private const string TraceType = "EtlInMemoryPerformance";

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Count of ETW events processed
        private ulong etwEventsProcessed;

        // The beginning tick count for ETL processing pass
        private long etlPassBeginTime;

        // Count of unordered events (events with an older timestamp than expected)
        // while reading events from ETL file.
        private ulong unorderedEventsReceived;

        internal EtlInMemoryPerformance(FabricEvents.ExtensionsEvents traceSource)
        {
            this.traceSource = traceSource;
        }

        internal void EtwEventProcessed()
        {
            this.etwEventsProcessed++;
        }

        internal void EtlUnorderedEventReceived()
        {
            this.unorderedEventsReceived++;
        }

        internal void EtlReadPassBegin()
        {
            this.etwEventsProcessed = 0;
            this.unorderedEventsReceived = 0;
            this.etlPassBeginTime = DateTime.Now.Ticks;
        }

        internal void EtlReadPassEnd(string providerFriendlyName)
        {
            long etlPassEndTime = DateTime.Now.Ticks;
            double etlPassTimeSeconds = ((double)(etlPassEndTime - this.etlPassBeginTime)) / (10 * 1000 * 1000);
            
            this.traceSource.WriteInfo(
                TraceType,
                "ETLPassInfo - {0}:{1},{2}", // WARNING: The performance test parses this message,
                                             // so changing the message can impact the test.
                providerFriendlyName,
                etlPassTimeSeconds,
                this.etwEventsProcessed);

            this.traceSource.WriteInfo(
                TraceType,
                "ETLPassMoreInfo - {0}:{1}",
                providerFriendlyName,
                this.unorderedEventsReceived);
        }

        internal void RecordEtlPassBacklog(string providerFriendlyName, int backlogSize)
        {
            this.traceSource.WriteInfo(
                TraceType,
                "ETLPassBacklog - {0}:{1}", // WARNING: The performance test parses this message,
                                            // so changing the message can impact the test.
                providerFriendlyName,
                backlogSize);
        }
    }
}