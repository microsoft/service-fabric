//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;
    using System.Runtime.Serialization;
    using System.IO;
    using Fabric.Common;
    using Strings;

    /// <summary>
    /// <para>Represents a schedule that automates rus of Chaos.</para>
    /// </summary>
    [Serializable]
    [DataContract]
    public sealed class ChaosSchedule : ByteSerializable
    {
        /// <summary>
        /// <para>temp</para>
        /// </summary>
        public static readonly List<DayOfWeek> AllDaysOfWeek = new List<DayOfWeek>() { DayOfWeek.Sunday, DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Saturday };

        private const string TraceComponent = "Chaos.ChaosSchedule";

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosSchedule" /> representing an empty schedule.</para>
        /// </summary>
        internal ChaosSchedule()
        {
            this.StartDate = DateTime.SpecifyKind(ChaosConstants.FileTimeMinDateTime, DateTimeKind.Utc); // Since native Common::DateTime uses FileTime, the smallest date we can use in managed is FileTime epoch - January 1, 1601 (UTC)
            this.ExpiryDate = DateTime.SpecifyKind(DateTime.MaxValue, DateTimeKind.Utc);
            this.ChaosParametersDictionary = new Dictionary<string, ChaosParameters>();
            this.Jobs = new List<ChaosScheduleJob>();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosSchedule" /> class.</para>
        /// </summary>
        /// <param name="startDate">Date and time to begin using this schedule to automate Chaos runs.</param>
        /// <param name="expiryDate">Date and time to stop using this schedule to automate Chaos runs.</param>
        /// <param name="chaosParametersDictionary">A mapping of ChaosParameters to string names referenced by <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> in Jobs.</param>
        /// <param name="jobs">List of <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> describing when to run Chaos.</param>
        public ChaosSchedule(
            DateTime startDate,
            DateTime expiryDate,
            Dictionary<string, ChaosParameters> chaosParametersDictionary,
            List<ChaosScheduleJob> jobs)
        {
            if (startDate == null)
            {
                throw new System.ArgumentNullException("StartDate", StringResources.ChaosScheduler_ScheduleStartDateIsNull);
            }

            if (expiryDate == null)
            {
                throw new System.ArgumentNullException("ExpiryDate", StringResources.ChaosScheduler_ScheduleExpiryDateIsNull);
            }

            if (chaosParametersDictionary == null)
            {
                throw new System.ArgumentNullException("ChaosParametersDictionary", StringResources.ChaosScheduler_ScheduleParametersDictionaryIsNull);
            }

            if (jobs == null)
            {
                throw new System.ArgumentNullException("Jobs", StringResources.ChaosScheduler_ScheduleJobsIsNull);
            }

            this.StartDate = startDate;
            this.ExpiryDate = expiryDate;
            this.ChaosParametersDictionary = chaosParametersDictionary;
            this.Jobs = jobs;
        }

        /// <summary>
        /// <para>Gets the <see cref="System.DateTime"/> representing the date and time at which this schedule will start being used for scheduling runs of Chaos.</para>
        /// </summary>
        /// <value>
        /// <para>Gets the <see cref="System.DateTime"/> representing the date and time at which this schedule will start being used for scheduling runs runs of Chaos.</para>
        /// </value>
        [DataMember]
        public DateTime StartDate
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the <see cref="System.DateTime"/> representing the date and time at which this schedule will expire and no longer be used for scheduling runs of Chaos.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.DateTime"/> representing the date and time at which this schedule will expire and no longer be used for scheduling runs of Chaos.</para>
        /// </value>
        [DataMember]
        public DateTime ExpiryDate
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the mapping of names to <see cref="System.Fabric.Chaos.DataStructures.ChaosParameters" />. The parameters are referenced by name in <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" />.</para>
        /// </summary>
        /// <value>
        /// <para>The mapping of names to <see cref="System.Fabric.Chaos.DataStructures.ChaosParameters" />. The parameters are referenced by name in <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" />.</para>
        /// </value>
        [DataMember]
        public Dictionary<string, ChaosParameters> ChaosParametersDictionary
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the list of <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> that define when to run Chaos.</para>
        /// </summary>
        /// <value>
        /// <para>The list of <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> that define when to run Chaos.</para>
        /// </value>
        [DataMember]
        public List<ChaosScheduleJob> Jobs
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets a string representation of the Chaos schedule object.
        /// </summary>
        /// <returns>A string representation of the Chaos schedule object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine(string.Format(CultureInfo.CurrentCulture, "StartDate:  {0}", this.StartDate));
            sb.AppendLine(string.Format(CultureInfo.CurrentCulture, "ExpiryDate: {0}", this.ExpiryDate));
            sb.AppendLine("ChaosParametersDictionary:");
            foreach (var entry in this.ChaosParametersDictionary)
            {
                sb.AppendLine(string.Format("    Key: {0}", entry.Key));
                sb.AppendLine(string.Format("    Value: {0}", entry.Value));

            }

            sb.AppendLine("Jobs:");
            foreach (var job in this.Jobs)
            {
                sb.AppendLine(job.ToStringIndented(0));
            }
            return sb.ToString();
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
                this.StartDate = DateTime.ParseExact(
                    br.ReadString(),
                    ChaosConstants.DateTimeFormat_ISO8601,
                    CultureInfo.InvariantCulture,
                    DateTimeStyles.AdjustToUniversal);

                this.ExpiryDate = DateTime.ParseExact(
                    br.ReadString(),
                    ChaosConstants.DateTimeFormat_ISO8601,
                    CultureInfo.InvariantCulture,
                    DateTimeStyles.AdjustToUniversal);


                int keyValuePairCount = br.ReadInt32();

                for (int i = 0; i < keyValuePairCount; i++)
                {
                    string tempKey = br.ReadString();

                    ChaosParameters tempVal = new ChaosParameters();
                    tempVal.Read(br);
                    this.ChaosParametersDictionary.Add(tempKey, tempVal);
                }

                int jobsCount = br.ReadInt32();

                for (int i = 0; i < jobsCount; i++)
                {
                    ChaosScheduleJob job = new ChaosScheduleJob();
                    job.Read(br);
                    this.Jobs.Add(job);
                }
            }
            else
            {
                TestabilityTrace.TraceSource.WriteError(TraceComponent, "Attempting to read a version of object below lowest version. Saw {0}. Expected >=6.2", objectVersion);
                ReleaseAssert.Fail("Failed to read byte serialization of ChaosSchedule");
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
            bw.Write(this.StartDate.ToString(ChaosConstants.DateTimeFormat_ISO8601, CultureInfo.InvariantCulture));
            bw.Write(this.ExpiryDate.ToString(ChaosConstants.DateTimeFormat_ISO8601, CultureInfo.InvariantCulture));
            bw.Write(this.ChaosParametersDictionary.Count);

            foreach (var kvpair in this.ChaosParametersDictionary)
            {
                bw.Write(kvpair.Key);
                kvpair.Value.Write(bw);
            }

            bw.Write(this.Jobs.Count);

            foreach (var job in this.Jobs)
            {
                job.Write(bw);
            }
        }
        #endregion ByteSerializable

        #region Interop Helpers

        internal static unsafe ChaosSchedule FromNative(IntPtr pointer)
        {
            var nativeSchedule = *(NativeTypes.FABRIC_CHAOS_SCHEDULE*)pointer;
            DateTime startDate = NativeTypes.FromNativeFILETIME(nativeSchedule.StartDate);
            DateTime expiryDate =  NativeTypes.FromNativeFILETIME(nativeSchedule.ExpiryDate);
            Dictionary<string, ChaosParameters> chaosParametersDictionary = new Dictionary<string, ChaosParameters>();

            var chaosParametersMap = *(NativeTypes.FABRIC_CHAOS_SCHEDULE_CHAOS_PARAMETERS_MAP*)nativeSchedule.ChaosParametersMap;
            var nativechaosParametersMapArrayPtr = (NativeTypes.FABRIC_CHAOS_SCHEDULE_CHAOS_PARAMETERS_MAP_ITEM*)chaosParametersMap.Items;

            for (int i=0; i< chaosParametersMap.Count; i++)
            {
                var nativeItem = *(nativechaosParametersMapArrayPtr + i);
                var name = NativeTypes.FromNativeString(nativeItem.Name);
                var parameters = ChaosParameters.CreateFromNative(nativeItem.Parameters);

                chaosParametersDictionary.Add(name, parameters);
            }

            List<ChaosScheduleJob> jobs = new List<ChaosScheduleJob>();
            var nativeJobsList = *(NativeTypes.FABRIC_CHAOS_SCHEDULE_JOB_LIST*)nativeSchedule.Jobs;
            var nativeJobsArrayPtr = (NativeTypes.FABRIC_CHAOS_SCHEDULE_JOB*)nativeJobsList.Items;
            for (int i = 0; i < nativeJobsList.Count; ++i)
            {
                var nativeItem = (nativeJobsArrayPtr + i);
                ChaosScheduleJob job = ChaosScheduleJob.FromNative(nativeItem);
                jobs.Add(job);
            }

            return new ChaosSchedule(startDate, expiryDate, chaosParametersDictionary, jobs);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeSchedule = new NativeTypes.FABRIC_CHAOS_SCHEDULE
            {
                StartDate = NativeTypes.ToNativeFILETIME(this.StartDate),
                ExpiryDate = NativeTypes.ToNativeFILETIME(this.ExpiryDate),
                ChaosParametersMap = ToNativeParametersMap(pinCollection, this.ChaosParametersDictionary),
                Jobs = ToNativeJobsList(pinCollection, this.Jobs)
            };

            return pinCollection.AddBlittable(nativeSchedule);
        }

        private static IntPtr ToNativeParametersMap(PinCollection pinCollection, Dictionary<string, ChaosParameters> parametersDictionary)
        {
            var nativeChaosParametersMapItemsArray = new NativeTypes.FABRIC_CHAOS_SCHEDULE_CHAOS_PARAMETERS_MAP_ITEM[parametersDictionary.Count];

            int i = 0;
            foreach (var entry in parametersDictionary)
            {
                nativeChaosParametersMapItemsArray[i].Name = pinCollection.AddObject(entry.Key);
                nativeChaosParametersMapItemsArray[i].Parameters = entry.Value.ToNative(pinCollection);
                i++;
            };

            var nativeChaosParametersMap = new NativeTypes.FABRIC_CHAOS_SCHEDULE_CHAOS_PARAMETERS_MAP
            {
                Count = (uint)nativeChaosParametersMapItemsArray.Length,
                Items = pinCollection.AddBlittable(nativeChaosParametersMapItemsArray),
            };

            return pinCollection.AddBlittable(nativeChaosParametersMap);
        }

        private static IntPtr ToNativeJobsList(PinCollection pinCollection, List<ChaosScheduleJob> jobs)
        {
            var jobsArray = new NativeTypes.FABRIC_CHAOS_SCHEDULE_JOB[jobs.Count];

            for (int i=0; i<jobs.Count; ++i)
            {
                jobsArray[i].ChaosParameters = pinCollection.AddObject(jobs[i].ChaosParameters);
                jobsArray[i].Days = jobs[i].Days.ToNative(pinCollection);
                jobsArray[i].Times = ChaosScheduleTimeRangeUtc.ToNativeList(pinCollection, jobs[i].Times);
            }

            var nativeChaosScheduleJobs = new NativeTypes.FABRIC_CHAOS_SCHEDULE_JOB_LIST
            {
                Count = (uint)jobsArray.Length,
                Items = pinCollection.AddBlittable(jobsArray)
            };

            return pinCollection.AddBlittable(nativeChaosScheduleJobs);
        }

        #endregion

    }
}
