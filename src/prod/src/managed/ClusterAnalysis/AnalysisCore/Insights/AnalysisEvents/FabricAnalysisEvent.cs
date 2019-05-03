// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.AnalysisEvents
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.Serialization;
    using ClusterAnalysis.AnalysisCore.Callback;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    [KnownType("GetKnownTypes")]
    public abstract class FabricAnalysisEvent
    {
        private static List<Type> knownTypes;

        /// <summary>
        /// </summary>
        protected FabricAnalysisEvent(TraceRecord triggerTrace)
        {
            Assert.IsNotNull(triggerTrace, "triggerTrace != null");
            Assert.IsNotNull(triggerTrace.GetUniqueId(), "triggerTrace.InstanceId != null");

            this.CorrelatedTracesFromEventStore = new List<TraceRecord>();
            this.CorrelatedTraceRecords = new List<TraceRecord>();
            this.CorrelatedTracesFromEventStore.Add(triggerTrace);

            this.OriginTime = DateTimeOffset.UtcNow;

            // Create a new Random Guid for this object. This will serve as the unique instance ID.
            this.InstanceId = Guid.NewGuid();
        }

        /// <summary>
        /// Captures the Unique Instance ID for this event.
        /// </summary>
        [DataMember(IsRequired = true)]
        public Guid InstanceId { get; private set; }

        /// <summary>
        /// Time Event was Created
        /// </summary>
        [DataMember(IsRequired = true)]
        public DateTimeOffset OriginTime { get; private set; }

        /// <summary>
        /// Gets the cluster entity for which this event is.
        /// </summary>
        [DataMember(IsRequired = true)]
        public BaseEntity TargetEntity { get; internal set; }

        /// <summary>
        /// Gets the related Trace records from Query Store
        /// </summary>
        [DataMember(IsRequired = true)]
        public List<TraceRecord> CorrelatedTraceRecords { get; private set; }

        /// <summary>
        /// Gets the End user friendly formatted message that can be shown to the User.
        /// </summary>
        [DataMember(IsRequired = true)]
        public abstract string FormattedMessage { get; protected set; }

        /// <summary>
        /// Gets the Instance Ids of the correlated events from the event store.
        /// </summary>
        public IEnumerable<Guid> CorrelatedEvents
        {
            get { return this.CorrelatedTracesFromEventStore.Select(item => item.GetUniqueId()); }
        }

        /// <summary>
        /// Gets or Sets the list of Trace Records from Event store that are related to this analysis
        /// </summary>
        /// <remarks>
        /// This is not publicly exposed. Publicly we only exposed the event's Instance IDs.
        /// </remarks>
        [DataMember(IsRequired = true)]
        protected List<TraceRecord> CorrelatedTracesFromEventStore { get; private set; }

        /// <summary>
        /// Original invocation event
        /// </summary>
        protected TraceRecord OriginalInvocationEvent
        {
            get { return this.CorrelatedTracesFromEventStore.FirstOrDefault(); }
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "InstanceId: {0}, OriginTime: {1}, Entity: {2}", this.InstanceId, this.OriginTime, this.TargetEntity);
        }

        internal static IList<Type> GetKnownTypes()
        {
            if (knownTypes != null)
            {
                return knownTypes;
            }

            knownTypes = new List<Type> { typeof(FabricAnalysisEvent), typeof(TraceRecord), typeof(BaseEntity) };
            var assemblies = AppDomain.CurrentDomain.GetAssemblies();
            foreach (var assembly in assemblies)
            {
                try
                {
                    var types = assembly.GetTypes();
                    knownTypes.AddRange(types.Where(type => type.IsSubclassOf(typeof(FabricAnalysisEvent))));
                }
                catch (Exception)
                {
                    // ignored
                }
            }

            return knownTypes.ToArray();
        }

        internal void AddCorrelatedTraceRecord(TraceRecord record)
        {
            Assert.IsNotNull(record, "Record can't be null");
            this.CorrelatedTraceRecords.Add(record);
        }

        internal void AddCorrelatedTraceRecordRange(IEnumerable<TraceRecord> recordEnumerable)
        {
            Assert.IsNotNull(recordEnumerable, "Record can't be null");
            foreach (var record in recordEnumerable)
            {
                this.CorrelatedTraceRecords.Add(record);
            }
        }

        internal void AddCorrelatedEventStoreTraceRecord(TraceRecord record)
        {
            Assert.IsNotNull(record, "Record can't be null");
            this.CorrelatedTracesFromEventStore.Add(record);
        }
    }
}