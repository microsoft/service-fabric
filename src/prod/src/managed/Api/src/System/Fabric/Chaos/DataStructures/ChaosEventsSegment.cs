//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Threading;

    /// <summary>
    /// <para>Represents the events of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent" /> caused by Chaos.</para>
    /// </summary>
    [DataContract]
    public sealed class ChaosEventsSegment
    {
        private const string TraceComponent = "Chaos.ChaosEventsSegment";

        internal ChaosEventsSegment()
        {
            this.History = new List<ChaosEvent>();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosEventsSegment" /> class.</para>
        /// </summary>
        /// <param name="history">The list of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent" /> that match the <see cref="System.Fabric.Chaos.DataStructures.ChaosEventsSegmentFilter" />.</param>
        /// <param name="continuationToken">If too many events match the filter, the events will be returned in multiple batches linked by this token.</param>
        internal ChaosEventsSegment(
            List<ChaosEvent> history,
            string continuationToken)
        {
            this.History = history;
            this.ContinuationToken = continuationToken;
        }

        /// <summary>
        /// <para>Gets the list of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent" /> that were produced during the timerange
        /// specified in <see cref="System.Fabric.FabricClient.TestManagementClient.GetChaosEventsAsync(ChaosEventsSegmentFilter, long, TimeSpan, CancellationToken)" /></para>.
        /// </summary>
        /// <value>
        /// <para>The list of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent" /> that were produced during the timerange specified in <see cref="System.Fabric.FabricClient.TestManagementClient.GetChaosEventsAsync(ChaosEventsSegmentFilter, long, TimeSpan, CancellationToken)" /></para>.
        /// </value>
        [DataMember]
        public List<ChaosEvent> History
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>If the number of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent" /> is too large, then those will be returned in segments. To get the next batch of events the ContinuationToken must be passed in the API call, <see cref="System.Fabric.FabricClient.TestManagementClient.GetChaosEventsAsync(string, long, TimeSpan, CancellationToken)" />. </para>
        /// </summary>
        /// <value>
        /// <para>The string to pass into <see cref="System.Fabric.FabricClient.TestManagementClient.GetChaosEventsAsync(string, long, TimeSpan, CancellationToken)" /> when the resulting number of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent" /> returned is too large to fit into one response.</para>
        /// </value>
        [DataMember]
        public string ContinuationToken
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Chaos.DataStructures.ChaosEventsSegment" /> object.
        /// </summary>
        /// <returns>A string representation of the ChaosEventsSegment object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            if (this.History.Any())
            {
                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "History:"));
                foreach (ChaosEvent ce in this.History)
                {
                    sb.AppendLine(ce.ToString());
                }
            }

            if (!string.IsNullOrEmpty(this.ContinuationToken))
            {
                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ContinuationToken={0}", this.ContinuationToken));
            }

            return sb.ToString();
        }

        #region Interop Helpers

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeReport = new NativeTypes.FABRIC_CHAOS_EVENTS_SEGMENT
            {
                ContinuationToken = pinCollection.AddObject(this.ContinuationToken),
                History = ToNativeEvents(pinCollection, this.History)
            };

            return pinCollection.AddBlittable(nativeReport);
        }

        private static IntPtr ToNativeEvents(PinCollection pin, List<ChaosEvent> history)
        {
            if (history == default(List<ChaosEvent>))
            {
                return IntPtr.Zero;
            }

            var eventArray = new NativeTypes.FABRIC_CHAOS_EVENT[history.Count];
            for (int i = 0; i < history.Count; ++i)
            {
                eventArray[i].Value = history[i].ToNative(pin);
                eventArray[i].Kind = ChaosUtility.GetNativeEventType(history[i]);
            }

            var nativeEvents = new NativeTypes.FABRIC_CHAOS_EVENT_LIST
            {
                Count = (uint)eventArray.Length,
                Items = pin.AddBlittable(eventArray)
            };

            return pin.AddBlittable(nativeEvents);
        }

        internal static unsafe ChaosEventsSegment FromNative(IntPtr pointer)
        {
            var nativeSegment = *(NativeTypes.FABRIC_CHAOS_EVENTS_SEGMENT*)pointer;
            var continuationToken = NativeTypes.FromNativeString(nativeSegment.ContinuationToken);
            var history = FromNativeEvents(nativeSegment.History);
            return new ChaosEventsSegment(history, continuationToken);
        }

        private static unsafe List<ChaosEvent> FromNativeEvents(IntPtr pointer)
        {
            var nativeEventsPtr = (NativeTypes.FABRIC_CHAOS_EVENT_LIST*)pointer;
            var retval = new List<ChaosEvent>();
            var nativeEventArrayPtr = (NativeTypes.FABRIC_CHAOS_EVENT*)nativeEventsPtr->Items;
            for (int i = 0; i < nativeEventsPtr->Count; ++i)
            {
                var nativeItem = *(nativeEventArrayPtr + i);
                ChaosEvent chaosEvent = ChaosUtility.FromNativeEvent(nativeItem);
                retval.Add(chaosEvent);
            }

            return retval;
        }

        #endregion
    }
}
