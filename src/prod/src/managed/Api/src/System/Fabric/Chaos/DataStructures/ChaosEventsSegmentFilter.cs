//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using Fabric.Chaos.Common;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Text;

    /// <summary>
    ///  <para>Represents the filter to choose the ChaosEvent's to include in the <see cref="System.Fabric.Chaos.DataStructures.ChaosEventsSegment" /></para>
    /// </summary>
    [DataContract]
    [Serializable]
    public sealed class ChaosEventsSegmentFilter : ByteSerializable
    {
        private const string TraceComponent = "Chaos.ChaosEventsSegmentFilter";

        internal ChaosEventsSegmentFilter()
        {
            this.StartTimeUtc = DateTime.SpecifyKind(ChaosConstants.FileTimeMinDateTime, DateTimeKind.Utc);
            this.EndTimeUtc = DateTime.MaxValue;
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosEventsSegmentFilter" /> class.</para>
        /// </summary>
        /// <param name="startTimeUtc">Include events starting from this time.</param>
        /// <param name="endTimeUtc">Include events starting before this time.</param>
        public ChaosEventsSegmentFilter(
            DateTime startTimeUtc,
            DateTime endTimeUtc)
        {
            ThrowIf.IsTrue(startTimeUtc > endTimeUtc, "Start time must before end time for filter.");

            this.StartTimeUtc = startTimeUtc;
            this.EndTimeUtc = endTimeUtc;
        }

        /// <summary>
        /// <para>Gets the <see cref="System.DateTime"/> representing earliest time of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent" /> this filter will include.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.DateTime"/> representing earliest time of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent" /> this filter will include.</para>
        /// </value>
        [DataMember]
        public DateTime StartTimeUtc { get; private set; }

        /// <summary>
        /// <para>Gets the <see cref="System.DateTime"/> representing latest time of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent" /> this filter will include.</para>
        /// </summary>
        /// <value>
        /// <para>Gets the <see cref="System.DateTime"/> representing latest time of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent" /> this filter will include.</para>
        /// </value>
        [DataMember]
        public DateTime EndTimeUtc { get; private set; }

        /// <summary>
        /// Gets a string representation of the ChaosEventsSegmentFilter object.
        /// </summary>
        /// <returns>A string representation of the ChaosEventsSegmentFilter object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendFormat("startTime={0}, endTime={1}",
                this.StartTimeUtc.ToString(CultureInfo.InvariantCulture),
                this.EndTimeUtc.ToString(CultureInfo.InvariantCulture));

            return sb.ToString();
        }

        internal bool Match(long key)
        {
            var eventTimeStamp = new DateTime(key);

            return eventTimeStamp >= this.StartTimeUtc && eventTimeStamp <= this.EndTimeUtc;
        }

        #region Interop Helpers

        internal unsafe IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeEventsSegmentFilter = new NativeTypes.FABRIC_CHAOS_EVENTS_SEGMENT_FILTER();

            nativeEventsSegmentFilter.StartTimeUtc = ChaosUtility.ToNativeFILETIMENormalTimeStamp(this.StartTimeUtc);
            nativeEventsSegmentFilter.EndTimeUtc = ChaosUtility.ToNativeFILETIMENormalTimeStamp(this.EndTimeUtc);

            return pinCollection.AddBlittable(nativeEventsSegmentFilter);
        }

        internal static unsafe ChaosEventsSegmentFilter FromNative(IntPtr pointer)
        {
            NativeTypes.FABRIC_CHAOS_EVENTS_SEGMENT_FILTER nativeEventsSegment = *(NativeTypes.FABRIC_CHAOS_EVENTS_SEGMENT_FILTER*)pointer;

            var startTimeUtc = NativeTypes.FromNativeFILETIME(nativeEventsSegment.StartTimeUtc);
            var endTimeUtc = NativeTypes.FromNativeFILETIME(nativeEventsSegment.EndTimeUtc);

            return new ChaosEventsSegmentFilter(startTimeUtc, endTimeUtc);
        }

        #endregion

        #region ByteSerializable

        /// <summary>
        /// Reads the state of this object from byte array.
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        public override void Read(BinaryReader br)
        {
            decimal objectVersion = br.ReadDecimal();

            if (objectVersion >= ChaosConstants.ApiVersion62)
            {
                this.StartTimeUtc = new DateTime(br.ReadInt64());
                this.EndTimeUtc = new DateTime(br.ReadInt64());
            }
        }

        /// <summary>
        /// Writes the state of this object into a byte array.
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        public override void Write(BinaryWriter bw)
        {
            bw.Write(ChaosConstants.ApiVersionCurrent);

            // Api Version >= 6.2
            bw.Write(this.StartTimeUtc.Ticks);
            bw.Write(this.EndTimeUtc.Ticks);
        }

        #endregion
    }
}
