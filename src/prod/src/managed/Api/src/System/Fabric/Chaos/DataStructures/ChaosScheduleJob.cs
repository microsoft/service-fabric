//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Text;
    using System.IO;
    using Fabric.Chaos.Common;
    using Fabric.Common;
    using Strings;

    /// <summary>
    /// <para>Represents a rule for when and how to run Chaos.</para>
    /// </summary>
    [Serializable]
    [DataContract]
    public sealed class ChaosScheduleJob : ByteSerializable
    {
        private const string TraceComponent = "Chaos.ChaosScheduleJob";

        internal ChaosScheduleJob()
        {
            this.ChaosParameters = string.Empty;
            this.Days = new ChaosScheduleJobActiveDays();
            this.Times = new List<ChaosScheduleTimeRangeUtc>();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> class.</para>
        /// </summary>
        /// <param name="chaosParameters">Parameters to run Chaos with.</param>
        /// <param name="days">Which days of the week this job will trigger runs of Chaos.</param>
        /// <param name="times">What times of the day Chaos will be running for.</param>
        public ChaosScheduleJob(string chaosParameters, ChaosScheduleJobActiveDays days, List<ChaosScheduleTimeRangeUtc> times)
        {
            if (chaosParameters == null)
            {
                throw new System.ArgumentNullException("ChaosParameters", StringResources.ChaosScheduler_ScheduleJobParametersIsNull);
            }

            if (days == null)
            {
                throw new System.ArgumentNullException("Days", StringResources.ChaosScheduler_ScheduleJobDaysIsNull);
            }

            if (times == null)
            {
                throw new System.ArgumentNullException("Times", StringResources.ChaosScheduler_ScheduleJobTimesIsNull);
            }

            this.ChaosParameters = chaosParameters;
            this.Days = days;
            this.Times = times;
        }

        /// <summary>
        /// <para>Gets the named reference to the <see cref="System.Fabric.Chaos.DataStructures.ChaosParameters" /> stored in the <see cref="System.Fabric.Chaos.DataStructures.ChaosSchedule" />. Automated runs of Chaos defined by this job will run with these parameters.</para>
        /// </summary>
        /// <value>
        /// <para>The named reference to the <see cref="System.Fabric.Chaos.DataStructures.ChaosParameters" /> stored in the <see cref="System.Fabric.Chaos.DataStructures.ChaosSchedule" />. Automated runs of Chaos defined by this job will run with these parameters.</para>
        /// </value>
        [DataMember]
        public string ChaosParameters
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJobActiveDays" /> for which days this job will automatically start runs of Chaos.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJobActiveDays" /> for which days this job will automatically start runs of Chaos.</para>
        /// </value>
        [DataMember]
        public ChaosScheduleJobActiveDays Days
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the list of <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeRangeUtc" /> representing the time ranges in a day for which this Chaos will be scheduled to run. The time ranges are treated as UTC time.</para>
        /// </summary>
        /// <value>
        /// <para>Gets the list of <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeRangeUtc" /> representing the time ranges in a day for which this Chaos will be scheduled to run. The time ranges are treated as UTC time.</para>
        /// </value>
        [DataMember]
        public List<ChaosScheduleTimeRangeUtc> Times
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Chaos.DataStructures.ChaosStatus" /> object.
        /// </summary>
        /// <returns>A string representation of the chaos status object.</returns>
        public override string ToString()
        {
            return this.ToStringIndented(0);
        }

        /// <summary>
        /// Gets a string representation of the chaos status object with indentation.
        /// </summary>
        /// <param name="indentLevel">How many indents (4 spaces) the output will have in front of each line.</param>
        /// <returns>A string representation of the chaos status object with indentation.</returns>
        internal string ToStringIndented(int indentLevel)
        {
            var description = new StringBuilder();

            var indentString = new string(' ', 4 * indentLevel);

            description.AppendLine(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "{0}ChaosParameters: {1}",
                    indentString,
                    this.ChaosParameters));

            description.AppendLine(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "{0}Days:\n    {0}{1}",
                    indentString,
                    this.Days));

            if (this.Times.Any())
            {
                description.AppendLine(string.Format("{0}Times:", indentString));

                foreach (var timerange in this.Times)
                {
                    description.AppendLine(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            "    {0}StartTime: {1}",
                            indentString,
                            timerange.StartTime.ToString()));

                    description.AppendLine(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            "    {0}EndTime: {1}",
                            indentString,
                            timerange.EndTime.ToString()));

                    description.AppendLine();
                }
            }

            return description.ToString();
        }

        #region ByteSerializable
        /// <summary>
        /// Reads the state of this object from byte array.
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        /// <exception cref="EndOfStreamException">The end of the stream is reached. </exception>
        public override void Read(BinaryReader br)
        {
            decimal objectVersion = br.ReadDecimal();

            if (objectVersion >= ChaosConstants.ApiVersion62)
            {
                this.ChaosParameters = br.ReadString();

                this.Days.Read(br);

                int timesCount = br.ReadInt32();
                for (int i = 0; i < timesCount; i++)
                {
                    ChaosScheduleTimeRangeUtc time = new ChaosScheduleTimeRangeUtc();
                    time.Read(br);
                    this.Times.Add(time);
                }
            }
            else
            {
                TestabilityTrace.TraceSource.WriteError(TraceComponent, "Attempting to read a version of object below lowest version. Saw {0}. Expected >=6.2", objectVersion);
                ReleaseAssert.Fail("Failed to read byte serialization of ChaosScheduleJob");
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
            bw.Write(this.ChaosParameters);

            this.Days.Write(bw);

            bw.Write(this.Times.Count);

            foreach (var time in this.Times)
            {
                time.Write(bw);
            }
        }
        #endregion

        #region Interop Helpers

        internal static unsafe ChaosScheduleJob FromNative(NativeTypes.FABRIC_CHAOS_SCHEDULE_JOB* pointer)
        {
            var nativeJob = *pointer;
            var chaosParameters = NativeTypes.FromNativeString(nativeJob.ChaosParameters);
            var days = ChaosScheduleJobActiveDays.FromNative(nativeJob.Days);
            var times = new List<ChaosScheduleTimeRangeUtc>();

            var nativeTimesList = *(NativeTypes.FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC_LIST*)nativeJob.Times;
            for (int i=0; i<nativeTimesList.Count; ++i )
            {
                var time = ChaosScheduleTimeRangeUtc.FromNative(nativeTimesList.Items + i * sizeof(NativeTypes.FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC));
                times.Add(time);
            }

            return new ChaosScheduleJob(chaosParameters, days, times);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            throw new NotImplementedException();
        }
        #endregion
    }
}