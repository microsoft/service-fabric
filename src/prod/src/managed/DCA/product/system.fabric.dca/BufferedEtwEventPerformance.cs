// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper 
{
    using System;
    using System.Fabric.Common.Tracing;

    // Class for gathering information related to the performance of buffering ETW events
    // locally before delivering them to the consumer
    internal class BufferedEtwEventPerformance
    {
        // Constants
        private const string TraceTypePerformance = "Performance";

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;
        
        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Counter to count the number of ETW events processed
        private ulong etwEventsProcessed;
        
        // Counter to count the number of ETW events written locally for eventual 
        // delivery to the consumer
        private ulong etwEventsWrittenLocallyForConsumer;

        // The beginning tick count for ETL processing pass
        private long etlPassBeginTime;

        // Counter to count the number of ETW events delivered to the consumer
        private ulong etwEventsDeliveredToConsumer;
        
        // The beginning tick count for event delivery pass
        private long eventDeliveryPassBeginTime;

        internal BufferedEtwEventPerformance(FabricEvents.ExtensionsEvents traceSource, string logSourceId)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
        }
        
        internal void EtwEventProcessed()
        {
            this.etwEventsProcessed++;
        }

        internal void EventWrittenToLocalBuffer()
        {
            this.etwEventsWrittenLocallyForConsumer++;
        }

        internal void EtlReadPassBegin()
        {
            this.etwEventsProcessed = 0;
            this.etwEventsWrittenLocallyForConsumer = 0;
            this.etlPassBeginTime = DateTime.Now.Ticks;
        }

        internal void EtlReadPassEnd()
        {
            long etlPassEndTime = DateTime.Now.Ticks;
            double etlPassTimeSeconds = ((double)(etlPassEndTime - this.etlPassBeginTime)) / (10 * 1000 * 1000);
            
            this.traceSource.WriteInfo(
                TraceTypePerformance,
                "ETLPassInfo - {0}:{1},{2},{3}", // WARNING: The performance test parses this message,
                                                 // so changing the message can impact the test.
                this.logSourceId,
                etlPassTimeSeconds,
                this.etwEventsProcessed,
                this.etwEventsWrittenLocallyForConsumer);
        }

        internal void EventDeliveredToConsumer()
        {
            this.etwEventsDeliveredToConsumer++;
        }

        internal void EventDeliveryPassBegin()
        {
            this.etwEventsDeliveredToConsumer = 0;
            this.eventDeliveryPassBeginTime = DateTime.Now.Ticks;
        }

        internal void EventDeliveryPassEnd()
        {
            long eventDeliveryPassEndTime = DateTime.Now.Ticks;
            double eventDeliveryPassTimeSeconds = ((double)(eventDeliveryPassEndTime - this.eventDeliveryPassBeginTime)) / (10 * 1000 * 1000);
            
            this.traceSource.WriteInfo(
                TraceTypePerformance,
                "EventDeliveryPassInfo - {0}: {1},{2}", // WARNING: The performance test parses this message,
                                                        // so changing the message can impact the test.
                this.logSourceId,
                eventDeliveryPassTimeSeconds,
                this.etwEventsDeliveredToConsumer);
        }

        internal void RecordEventDeliveryBacklog(int backlogSize)
        {
            this.traceSource.WriteInfo(
                TraceTypePerformance,
                "EventDeliveryBacklog - {0}: {1}", // WARNING: The performance test parses this message,
                                                   // so changing the message can impact the test.
                this.logSourceId,
                backlogSize);
        }
    }
}