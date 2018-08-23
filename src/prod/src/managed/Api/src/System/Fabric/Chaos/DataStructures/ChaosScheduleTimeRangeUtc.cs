//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.IO;
    using System.Text;
    using Fabric.Chaos.Common;
    using Fabric.Common;
    using Threading.Tasks;
    using Interop;
    using Collections.Generic;

    /// <summary>
    /// <para>Represents a time range in a 24 hour day in UTC time.</para>
    /// </summary>
    [DataContract]
    [Serializable]
    public sealed class ChaosScheduleTimeRangeUtc : ByteSerializable
    {
        /// <summary>
        /// <para>A <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeRangeUtc" /> representing the entire day.</para>
        /// </summary>
        public static readonly ChaosScheduleTimeRangeUtc WholeDay = new ChaosScheduleTimeRangeUtc(ChaosScheduleTimeUtc.StartOfDay, ChaosScheduleTimeUtc.EndOfDay);

        private const string TraceComponent = "Chaos.ChaosScheduleTimeRangeUtc";

        internal ChaosScheduleTimeRangeUtc()
        {
            this.StartTime = new ChaosScheduleTimeUtc();
            this.EndTime = new ChaosScheduleTimeUtc();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeRangeUtc" /> class by copying another time range.</para>
        /// </summary>
        /// <param name="other">Another time range to copy.</param>
        internal ChaosScheduleTimeRangeUtc(ChaosScheduleTimeRangeUtc other)
        {
            this.StartTime = new ChaosScheduleTimeUtc(other.StartTime);
            this.EndTime = new ChaosScheduleTimeUtc(other.EndTime);
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeRangeUtc" /> class with a provided start and end time.</para>
        /// </summary>
        /// <param name="startTime">Start of time range.</param>
        /// <param name="endTime">End of time range.</param>
        public ChaosScheduleTimeRangeUtc(ChaosScheduleTimeUtc startTime, ChaosScheduleTimeUtc endTime)
        {
            if (startTime == null)
            {
                throw new System.ArgumentNullException(
                    "StartTime",
                    StringResources.ChaosScheduler_ScheduleJobTimeIsNull);
            }

            if (endTime == null)
            {
                throw new System.ArgumentNullException(
                    "EndTime",
                    StringResources.ChaosScheduler_ScheduleJobTimeIsNull);
            }

            this.StartTime = new ChaosScheduleTimeUtc(startTime);
            this.EndTime = new ChaosScheduleTimeUtc(endTime);
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeUtc"/> representing the UTC starting time of the day for a run of Chaos defined by a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob"/>.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeUtc"/> representing the UTC starting time of the day for a run of Chaos defined by a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob"/>.</para>
        /// </value>
        [DataMember]
        public ChaosScheduleTimeUtc StartTime
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeUtc"/> representing the UTC ending time of the day for a run of Chaos defined by a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob"/>.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeUtc"/> representing the UTC ending time of the day for a run of Chaos defined by a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob"/>.</para>
        /// </value>
        [DataMember]
        public ChaosScheduleTimeUtc EndTime
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets a string representation of the ChaosScheduleTimeRangeUtc object.
        /// </summary>
        /// <returns>A string representation of the ChaosScheduleTimeRangeUtc object.</returns>
        public override string ToString()
        {
            var description = new StringBuilder();
            description.AppendLine(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "StartTime: {0}",
                    this.StartTime.ToString()));

            description.AppendLine(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "EndTime: {0}",
                    this.EndTime.ToString()));
            description.AppendLine();

            return description.ToString();
        }

        #region ByteSerializable
        /// <summary>
        /// Reads the state of this object from byte array.
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        /// <exception cref="EndOfStreamException">The end of the stream is reached. </exception>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Read(BinaryReader br)
        {
            decimal objectVersion = br.ReadDecimal();

            if (objectVersion >= ChaosConstants.ApiVersion62)
            {
                this.StartTime = new ChaosScheduleTimeUtc(0, 0);
                this.StartTime.Read(br);

                this.EndTime = new ChaosScheduleTimeUtc(0, 0);
                this.EndTime.Read(br);
            }
            else
            {
                TestabilityTrace.TraceSource.WriteError(TraceComponent, "Attempting to read a version of object below lowest version. Saw {0}. Expected >=6.2", objectVersion);
                ReleaseAssert.Fail("Failed to read byte serialization of ChaosScheduleTimeRangeUtc");
            }
        }

        /// <summary>
        /// Writes the state of this object into a byte array.
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Write(BinaryWriter bw)
        {
            bw.Write(ChaosConstants.ApiVersionCurrent);

            // Api Version >= 6.2
            this.StartTime.Write(bw);
            this.EndTime.Write(bw);
        }
        #endregion

        #region Interop Helpers

        internal static unsafe ChaosScheduleTimeRangeUtc FromNative(IntPtr pointer)
        {
            var nativeTimeRange = *(NativeTypes.FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC*)pointer;
            var startTime = ChaosScheduleTimeUtc.FromNative(nativeTimeRange.StartTime);
            var endTime = ChaosScheduleTimeUtc.FromNative(nativeTimeRange.EndTime);
            return new ChaosScheduleTimeRangeUtc(startTime, endTime);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeTimeRange = new NativeTypes.FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC
            {
                StartTime = this.StartTime.ToNative(pinCollection),
                EndTime = this.EndTime.ToNative(pinCollection),
            };

            return pinCollection.AddBlittable(nativeTimeRange);
        }

        internal static unsafe IntPtr ToNativeList(PinCollection pinCollection, List<ChaosScheduleTimeRangeUtc> times)
        {

            var timesArray = new NativeTypes.FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC[times.Count];

            for (int i=0; i<times.Count; i++)
            {
                timesArray[i].StartTime = times[i].StartTime.ToNative(pinCollection);
                timesArray[i].EndTime = times[i].EndTime.ToNative(pinCollection);
            }

            var nativeTimeRangeList = new NativeTypes.FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC_LIST
            {
                Count = (uint)times.Count,
                Items = pinCollection.AddBlittable(timesArray)
            };

            return pinCollection.AddBlittable(nativeTimeRangeList);
        }
        #endregion
    }
}