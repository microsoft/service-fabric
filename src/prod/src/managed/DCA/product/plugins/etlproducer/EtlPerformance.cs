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
    internal class EtlPerformance
    {
        // Constants
        private const string TraceType = "Performance";

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Counter to count the number of ETW events processed
        private ulong etwEventsProcessed;

        // The beginning tick count for ETL processing pass
        private long etlPassBeginTime;

        // Counter to count the number of out of order events (events with an older timestamp than expected)
        // while reading events from ETL file.
        private ulong unorderedEventsRecieved;

        internal EtlPerformance(FabricEvents.ExtensionsEvents traceSource)
        {
            this.traceSource = traceSource;
        }

        internal void EtwEventProcessed()
        {
            this.etwEventsProcessed++;
        }

        internal void EtlReadPassBegin()
        {
            this.etwEventsProcessed = 0;
            this.etlPassBeginTime = DateTime.Now.Ticks;
            this.unorderedEventsRecieved = 0;
        }

        internal void EtlUnorderedEventRecieved()
        {
            this.unorderedEventsRecieved++;
        }

        internal void EtlReadPassEnd(string providerFriendlyName)
        {
            long etlPassEndTime = DateTime.Now.Ticks;
            double etlPassTimeSeconds = ((double)(etlPassEndTime - this.etlPassBeginTime)) / (10 * 1000 * 1000);
            
            FabricEvents.Events.EtlPassPerformance(
                providerFriendlyName,
                etlPassTimeSeconds,
                (long)this.etwEventsProcessed);

            this.traceSource.WriteInfo(
                TraceType,
                "ETLPassMoreInfo - {0}:{1}",
                providerFriendlyName,
                this.unorderedEventsRecieved);
        }

        internal void RecordEtlPassBacklog(string providerFriendlyName, int backlogSize)
        {
            FabricEvents.Events.EtlPassBacklogPerformance(
                providerFriendlyName,
                backlogSize);
        }
    }
}