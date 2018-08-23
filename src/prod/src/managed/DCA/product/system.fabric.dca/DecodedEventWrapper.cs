// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper 
{
    using System;

    // This class represents a wrapper around a decoded ETW event given to us by
    // the producer
    public class DecodedEventWrapper
    {
        // Decoded event given to us by the producer
        private DecodedEtwEvent producerEvent;

        internal DecodedEventWrapper(DecodedEtwEvent producerEvent)
        {
            this.producerEvent = producerEvent;
        }

        // Number used to different among events **within a file** that have the
        // same timestamp. Events bearing the same timestamp within an ETL file
        // will have different timestamp differentiators. However, it is possible
        // that events with the same timestamp but belonging to different ETL 
        // files can have the same timestamp differentiator.
        internal int TimestampDifferentiator { get; set; }

        internal string TaskName
        {
            get
            {
                return this.producerEvent.TaskName;
            }
        }

        internal string EventType
        {
            get
            {
                return this.producerEvent.EventType;
            }
        }

        internal byte Level
        {
            get
            {
                return this.producerEvent.Level;
            }
        }

        internal DateTime Timestamp
        {
            get
            {
                return this.producerEvent.Timestamp;
            }
        }

        internal uint ThreadId
        {
            get
            {
                return this.producerEvent.ThreadId;
            }
        }

        internal uint ProcessId
        {
            get
            {
                return this.producerEvent.ProcessId;
            }
        }

        internal string EventText
        {
            get
            {
                return this.producerEvent.EventText;
            }
        }

        internal string StringRepresentation
        {
            get
            {
                return this.producerEvent.StringRepresentation;
            }
        }

        internal DecodedEtwEvent InternalEvent
        {
            get
            {
                return this.producerEvent;
            }
        }
    }
}