// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;

    /// <inheritdoc />
    /// <summary>
    /// Base Implementation of all Trace Records.
    /// </summary>
    [DataContract]
    [KnownType("GetKnownTypes")]
    public abstract class TraceRecord : ITraceObject
    {
        private static List<Type> knownTypes;

        [DataMember]
        private InstanceIdentity instanceIdentity;

        protected TraceRecord(int traceId, TaskName taskName)
        {
            if (traceId < 0)
            {
                throw new ArgumentOutOfRangeException("traceId");
            }

            this.TraceId = traceId;
            this.TaskName = taskName;
            this.instanceIdentity = null;
        }

        /// <summary>
        /// Gets or sets the delegate associated with this event.   
        /// </summary>
        public abstract Delegate Target { get; protected set; }

        /// <summary>
        /// Gets the Identify for this Event Type.
        /// </summary>
        public TraceIdentity Identity
        {
            get { return new TraceIdentity(this.TraceId, this.TaskName); }
        }

        /// <summary>
        /// Gets the Instance ID for this Event
        /// </summary>
        public virtual InstanceIdentity ObjectInstanceId
        {
            get { return this.instanceIdentity; }
            private set { this.instanceIdentity = value; }
        }

        /// <summary>
        /// Gets or set the next Record that is used to make multiple subscriber work.
        /// </summary>
        internal TraceRecord Next { get; set; }

        #region Trace Data

        /// <summary>
        /// Gets or sets the Property Value Reader
        /// </summary>
        [DataMember]
        public ValueReader PropertyValueReader { get; set; }

        [DataMember]
        public int TraceId { get; private set; }

        [DataMember]
        public uint ThreadId { get; set; }

        [DataMember]
        public uint TracingProcessId { get; set; }

        [DataMember]
        public TaskName TaskName { get; private set; }

        [DataMember]
        public TraceLevel Level { get; set; }

        [DataMember]
        public DateTime TimeStamp { get; set; }

        [DataMember]
        public byte Version { get; set; }

        #endregion

        /// <summary>
        /// Get the Minimum version of the product where this Event is present.
        /// </summary>
        /// <returns></returns>
        public virtual int GetMinimumVersionOfProductWherePresent()
        {
            // Today it's hard code as 1 - the version where these special events were added to product. Each Event 
            // can override it to specify their min supported version.
            return 1;
        }

        public abstract IEnumerable<PropertyDescriptor> GetPropertyDescriptors();

        public abstract object GetPropertyValue(PropertyDescriptor descriptor);

        public abstract object GetPropertyValue(int propertyIndex);

        public abstract object GetPropertyValue(string propertyName);

        /// <summary>
        /// Each TraceRecord items knows where it should Dispatch to. When records are being read by reader,
        /// the reader will call this function.
        /// </summary>
        public abstract Task DispatchAsync(CancellationToken token);

        /// <summary>
        /// Each TraceRecord item should provide a public facing string representation of the data.
        /// </summary>
        /// <returns>Public facing string representation of data</returns>
        public virtual string ToPublicString()
        {
            return this.ToStringCommonPrefix();
        }

        public string ToStringCommonPrefix()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "[{0}]: TimeStamp : {1}",
                this.GetType().Name,
                this.TimeStamp);
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "{0}, ProcessId : {1}, ThreadId : {2}, Level : {3}",
                this.ToStringCommonPrefix(),
                this.TracingProcessId,
                this.ThreadId,
                this.Level);
        }

        public virtual TraceRecord Clone()
        {
            TraceRecord clonedRecord = (TraceRecord)this.MemberwiseClone();
            if (clonedRecord.PropertyValueReader != null)
            {
                clonedRecord.PropertyValueReader = this.PropertyValueReader.Clone();
            }

            return clonedRecord;
        }

        public static IList<Type> GetKnownTypes()
        {
            if (knownTypes != null)
            {
                return knownTypes;
            }

            knownTypes = new List<Type> { typeof(TraceRecord), typeof(InstanceIdentity), typeof(ValueReader) };
            var assemblies = AppDomain.CurrentDomain.GetAssemblies();
            foreach (var assembly in assemblies)
            {
                try
                {
                    var types = assembly.GetTypes();
                    knownTypes.AddRange(types.Where(type => type.IsSubclassOf(typeof(TraceRecord))));
                }
                catch (Exception)
                {
                    // ignored
                }
            }

            return knownTypes;
        }

        [OnDeserialized]
        public void OnDeserialized(StreamingContext context)
        {
            this.Target = null;
            this.Next = null;
        }

        protected virtual void InternalOnDeserialized(StreamingContext context)
        {
        }
    }
}