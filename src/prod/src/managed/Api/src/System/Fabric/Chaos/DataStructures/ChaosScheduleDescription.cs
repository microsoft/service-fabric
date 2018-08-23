
//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Fabric.Interop;
    using System.IO;
    using System.Runtime.Serialization;
    using Fabric.Chaos.Common;
    using Fabric.Common;
    using Globalization;
    using Strings;

    /// <summary>
    /// <para>Represents a versioned <see cref="System.Fabric.Chaos.DataStructures.ChaosSchedule" />. The version of a schedule is a number that gets updated when the schedule is updated.</para>
    /// </summary>
    [Serializable]
    [DataContract]
    public sealed class ChaosScheduleDescription : ByteSerializable
    {
        private const string TraceComponent = "Chaos.ChaosScheduleDescription";

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleDescription" /> class with Version set to 0 and an empty schedule.</para>
        /// </summary>
        internal ChaosScheduleDescription()
        {
            this.Version = 0;
            this.Schedule = new ChaosSchedule();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleDescription" /> class with a provided schedule and version.</para>
        /// </summary>
        /// <param name="version">The version of the schedule.</param>
        /// <param name="schedule">The schedule for scheduling runs of Chaos.</param>
        public ChaosScheduleDescription(int version, ChaosSchedule schedule)
        {
            if (version < 0)
            {
                throw new System.ArgumentException(
                    string.Format(
                        StringResources.ChaosScheduler_ScheduleDescriptionVersionIsNegative,
                        version),
                    "version");
            }

            if (schedule == null)
            {
                throw new System.ArgumentNullException(
                    "Schedule",
                    StringResources.ChaosScheduler_ScheduleIsNull);
            }

            this.Version = version;
            this.Schedule = schedule;
        }

        /// <summary>
        /// <para>Gets the version number of the <see cref="System.Fabric.Chaos.DataStructures.ChaosSchedule" />.</para>.
        /// </summary>
        /// <value>
        /// <para>The version of the <see cref="System.Fabric.Chaos.DataStructures.ChaosSchedule" />. A valid version number is greater than 0. The version of the schedule held by Chaos is updated whenever the schedule is updated.</para>
        /// </value>
        [DataMember]
        public int Version
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.Chaos.DataStructures.ChaosSchedule" /> describing the schedule of automated Chaos runs.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Chaos.DataStructures.ChaosSchedule" /> describing the schedule of automated Chaos runs.</para>
        /// </value>
        [DataMember]
        internal ChaosSchedule Schedule
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Increase the Version by 1, wrapping back to 0 if the Version would be greater than 2,147,483,648.</para>
        /// </summary>
        internal void IncrementVersion()
        {
            if (this.Version == int.MaxValue)
            {
                this.Version = 0;
            }
            else
            {
                this.Version++;
            }
        }

        #region ByteSerializable

        /// <summary>
        /// <para>Writes the state of this object into a byte array.</para>
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Write(BinaryWriter bw)
        {
            bw.Write(ChaosConstants.ApiVersionCurrent);

            // Api Version >= 6.2
            bw.Write(this.Version);
            this.Schedule.Write(bw);
        }

        /// <summary>
        /// <para>Reads the state of this object from byte array.</para>
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        /// <exception cref="EndOfStreamException">The end of the stream is reached. </exception>
        public override void Read(BinaryReader br)
        {
            decimal objectVersion = br.ReadDecimal();

            if (objectVersion >= ChaosConstants.ApiVersion62)
            {
                this.Version = br.ReadInt32();
                this.Schedule.Read(br);
            }
            else
            {
                TestabilityTrace.TraceSource.WriteError(TraceComponent, "Attempting to read a version of object below lowest version. Saw {0}. Expected >=6.2", objectVersion);
                ReleaseAssert.Fail("Failed to read byte serialization of ChaosScheduleDescription");
            }
        }

        #endregion

        /// <summary>
        /// <para>Gets a string representation of the chaos schedule description object.</para>
        /// </summary>
        /// <returns>A string representation of the chaos schedule description object.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Version={0}, Schedule={1}", this.Version, this.Schedule);
        }

        #region Interop Helpers

        internal static unsafe ChaosScheduleDescription FromNative(IntPtr pointer)
        {
            var nativeDescription = *(NativeTypes.FABRIC_CHAOS_SCHEDULE_DESCRIPTION*)pointer;
            var chaosSchedule = ChaosSchedule.FromNative(nativeDescription.Schedule);
            var version = (int)nativeDescription.Version;

            return new ChaosScheduleDescription(version, chaosSchedule);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_CHAOS_SCHEDULE_DESCRIPTION
            {
                Version = (uint)this.Version,
                Schedule = this.Schedule.ToNative(pinCollection)
            };

            return pinCollection.AddBlittable(nativeDescription);
        }

        #endregion
    }
}