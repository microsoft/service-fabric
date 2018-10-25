//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System;
    using System.IO;
    using System.Runtime.Serialization;
    using Fabric.Chaos.Common;
    using Fabric.Common;
    using Globalization;
    using Collections.Generic;
    using Text;
    using Interop;

    /// <summary>
    /// <para>Represents which days of the week the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob"/> is active for.</para>
    /// </summary>
    [DataContract]
    public sealed class ChaosScheduleJobActiveDays : ByteSerializable
    {
        private const string TraceComponent = "Chaos.ChaosScheduleJobActiveDays";

        private Dictionary<DayOfWeek, bool> activeDays;

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJobActiveDays" /> class with all days set to false.</para>
        /// </summary>
        public ChaosScheduleJobActiveDays()
        {
            this.activeDays = new Dictionary<DayOfWeek, bool>();

            // set all days to false
            foreach (DayOfWeek day in Enum.GetValues(typeof(DayOfWeek)))
            {
                this.activeDays.Add(day, false);
            }
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJobActiveDays" /> class with all days set to false.</para>
        /// </summary>
        /// <param name="activeDaysSet">A set of days of the week that a job will be active for.</param>
        public ChaosScheduleJobActiveDays(HashSet<DayOfWeek> activeDaysSet)
            : this()
        {
            // all days are false prior to this foreach
            foreach (DayOfWeek day in Enum.GetValues(typeof(DayOfWeek)))
            {
                if (activeDaysSet.Contains(day))
                {
                    this.activeDays[day] = true;
                }
            }
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJobActiveDays" /> class by copying another set of active days.</para>
        /// </summary>
        /// <param name="other">Another set of days to copy.</param>
        public ChaosScheduleJobActiveDays(ChaosScheduleJobActiveDays other)
            : this()
        {
            foreach (DayOfWeek day in Enum.GetValues(typeof(DayOfWeek)))
            {
                    this.activeDays[day] = other.activeDays[day];
            }
        }

        #region Properties

        /// <summary>
        /// <para>Gets the value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Sunday.</para>
        /// </summary>
        /// <value>
        /// <para>The value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Sunday.</para>
        /// </value>
        [DataMember]
        public bool Sunday
        {
            get
            {
                return this.activeDays[DayOfWeek.Sunday];
            }

            private set
            {
                this.activeDays[DayOfWeek.Sunday] = value;
            }
        }

        /// <summary>
        /// <para>Gets the value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Monday.</para>
        /// </summary>
        /// <value>
        /// <para>The value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Monday.</para>
        /// </value>
        [DataMember]
        public bool Monday
        {
            get
            {
                return this.activeDays[DayOfWeek.Monday];
            }

            private set
            {
                this.activeDays[DayOfWeek.Monday] = value;
            }
        }

        /// <summary>
        /// <para>Gets the value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Tuesday.</para>
        /// </summary>
        /// <value>
        /// <para>The value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Tuesday.</para>
        /// </value>
        [DataMember]
        public bool Tuesday
        {
            get
            {
                return this.activeDays[DayOfWeek.Tuesday];
            }

            private set
            {
                this.activeDays[DayOfWeek.Tuesday] = value;
            }
        }

        /// <summary>
        /// <para>Gets the value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Wednesday.</para>
        /// </summary>
        /// <value>
        /// <para>The value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Wednesday.</para>
        /// </value>
        [DataMember]
        public bool Wednesday
        {
            get
            {
                return this.activeDays[DayOfWeek.Wednesday];
            }

            private set
            {
                this.activeDays[DayOfWeek.Wednesday] = value;
            }
        }

        /// <summary>
        /// <para>Gets the value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Thursday.</para>
        /// </summary>
        /// <value>
        /// <para>The value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Thursday.</para>
        /// </value>
        [DataMember]
        public bool Thursday
        {
            get
            {
                return this.activeDays[DayOfWeek.Thursday];
            }

            private set
            {
                this.activeDays[DayOfWeek.Thursday] = value;
            }
        }

        /// <summary>
        /// <para>Gets the value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Friday.</para>
        /// </summary>
        /// <value>
        /// <para>The value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Friday.</para>
        /// </value>
        [DataMember]
        public bool Friday
        {
            get
            {
                return this.activeDays[DayOfWeek.Friday];
            }

            private set
            {
                this.activeDays[DayOfWeek.Friday] = value;
            }
        }

        /// <summary>
        /// <para>Gets the value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Saturday.</para>
        /// </summary>
        /// <value>
        /// <para>The value of whether or not a <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleJob" /> will be active on Saturday.</para>
        /// </value>
        [DataMember]
        public bool Saturday
        {
            get
            {
                return this.activeDays[DayOfWeek.Saturday];
            }

            private set
            {
                this.activeDays[DayOfWeek.Saturday] = value;
            }
        }

        #endregion

        /// <summary>
        /// Get value of a day using the matching DayOfWeek enum.
        /// </summary>
        /// <param name="day">The day to get the status of.</param>
        public bool GetDayValueByEnum(DayOfWeek day)
        {
            return this.activeDays[day];
        }

        /// <summary>
        /// Determine if there are no days active.
        /// </summary>
        /// <returns>True if all the days have a false value, otherwise, return false.</returns>
        public bool NoActiveDays()
        {
            // return true if all the days are false. False otherwise
            return !(this.Sunday || this.Monday || this.Tuesday || this.Wednesday || this.Thursday || this.Friday || this.Saturday);
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
                this.Sunday = br.ReadBoolean();
                this.Monday = br.ReadBoolean();
                this.Tuesday = br.ReadBoolean();
                this.Wednesday = br.ReadBoolean();
                this.Thursday = br.ReadBoolean();
                this.Friday = br.ReadBoolean();
                this.Saturday = br.ReadBoolean();
            }
            else
            {
                TestabilityTrace.TraceSource.WriteError(TraceComponent, "Attempting to read a version of object below lowest version. Saw {0}. Expected >=6.2", objectVersion);
                ReleaseAssert.Fail("Failed to read byte serialization of ChaosScheduleJobActiveDays");
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

            bw.Write(Sunday);
            bw.Write(Monday);
            bw.Write(Tuesday);
            bw.Write(Wednesday);
            bw.Write(Thursday);
            bw.Write(Friday);
            bw.Write(Saturday);
        }
        #endregion

        /// <summary>
        /// Gets a string representation of this object.
        /// </summary>
        /// <returns>A string representation of the chaos schedule job active days object.</returns>
        public override string ToString()
        {
            StringBuilder message = new StringBuilder("ActiveDays: ");

            if (this.NoActiveDays())
            {
                message.Append("None");

                return message.ToString();
            }
            else
            {
                foreach (DayOfWeek day in Enum.GetValues(typeof(DayOfWeek)))
                {
                    if (this.GetDayValueByEnum(day))
                    {
                        message.Append(String.Format("{0}, ", Enum.GetName(typeof(DayOfWeek), day)));
                    }
                }

                // remove trailing ", "
                char[] trim = { ' ', ',' };
                return message.ToString().TrimEnd(trim);
            }
        }

        #region Interop Helpers

        internal static unsafe ChaosScheduleJobActiveDays FromNative(IntPtr pointer)
        {
            var activeDaysSet = new HashSet<DayOfWeek>();
            var nativeActiveDays = *(NativeTypes.FABRIC_CHAOS_SCHEDULE_JOB_ACTIVE_DAYS*)pointer;

            if (NativeTypes.FromBOOLEAN(nativeActiveDays.Sunday))
            {
                activeDaysSet.Add(DayOfWeek.Sunday);
            }

            if (NativeTypes.FromBOOLEAN(nativeActiveDays.Monday))
            {
                activeDaysSet.Add(DayOfWeek.Monday);
            }

            if (NativeTypes.FromBOOLEAN(nativeActiveDays.Tuesday))
            {
                activeDaysSet.Add(DayOfWeek.Tuesday);
            }

            if (NativeTypes.FromBOOLEAN(nativeActiveDays.Wednesday))
            {
                activeDaysSet.Add(DayOfWeek.Wednesday);
            }

            if (NativeTypes.FromBOOLEAN(nativeActiveDays.Thursday))
            {
                activeDaysSet.Add(DayOfWeek.Thursday);
            }

            if (NativeTypes.FromBOOLEAN(nativeActiveDays.Friday))
            {
                activeDaysSet.Add(DayOfWeek.Friday);
            }

            if (NativeTypes.FromBOOLEAN(nativeActiveDays.Saturday))
            {
                activeDaysSet.Add(DayOfWeek.Saturday);
            }

            return new ChaosScheduleJobActiveDays(activeDaysSet);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeActiveDays = new NativeTypes.FABRIC_CHAOS_SCHEDULE_JOB_ACTIVE_DAYS
            {
                Sunday = NativeTypes.ToBOOLEAN(this.Sunday),
                Monday = NativeTypes.ToBOOLEAN(this.Monday),
                Tuesday = NativeTypes.ToBOOLEAN(this.Tuesday),
                Wednesday = NativeTypes.ToBOOLEAN(this.Wednesday),
                Thursday = NativeTypes.ToBOOLEAN(this.Thursday),
                Friday = NativeTypes.ToBOOLEAN(this.Friday),
                Saturday = NativeTypes.ToBOOLEAN(this.Saturday),
            };

            return pinCollection.AddBlittable(nativeActiveDays);
        }
        #endregion
    }
}