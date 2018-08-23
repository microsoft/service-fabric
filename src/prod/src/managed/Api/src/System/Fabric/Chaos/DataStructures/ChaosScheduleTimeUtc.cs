//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Fabric.Strings;
    using System.Runtime.Serialization;
    using System.Text;
    using Fabric.Chaos.Common;
    using Fabric.Common;
    using Globalization;
    using IO;
    using Interop;

    /// <summary>
    /// <para>Represents a time of day in 24 hour time. Time is in UTC time.</para>
    /// </summary>
    [DataContract]
    public sealed class ChaosScheduleTimeUtc
        : ByteSerializable,
          IComparable<ChaosScheduleTimeUtc>,
          IEquatable<ChaosScheduleTimeUtc>
    {
        /// <summary>
        /// <para>A <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeUtc" /> representing the start of the day in UTC time.</para>
        /// </summary>
        public static readonly ChaosScheduleTimeUtc StartOfDay = new ChaosScheduleTimeUtc(0, 0);

        /// <summary>
        /// <para>A <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeUtc" /> representing the end of the day in UTC time.</para>
        /// </summary>
        public static readonly ChaosScheduleTimeUtc EndOfDay = new ChaosScheduleTimeUtc(23, 59);

        private const string TraceComponent = "Chaos.ChaosScheduleTimeUtc";

        internal ChaosScheduleTimeUtc()
        {
            this.Hour = 0;
            this.Minute = 0;
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeUtc" /> class.</para>
        /// </summary>
        /// <param name="hour">Hour of the day in 24 hour time format.</param>
        /// <param name="minute">Minute of the hour.</param>
        public ChaosScheduleTimeUtc(int hour, int minute)
        {
            if (hour < 0 || hour > 23)
            {
                throw new System.ArgumentException(StringResources.ChaosScheduler_TimeHourNotInLimit, "hour");
            }

            if (minute < 0 || minute > 59)
            {
                throw new System.ArgumentException(StringResources.ChaosScheduler_TimeMinuteNotInLimit, "minute");
            }

            this.Hour = hour;
            this.Minute = minute;
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosScheduleTimeUtc" /> class by copying another time.</para>
        /// </summary>
        /// <param name="other">Another ChaosScheduleTimeUtc to copy.</param>
        public ChaosScheduleTimeUtc(ChaosScheduleTimeUtc other)
        {
            this.Hour = other.Hour;
            this.Minute = other.Minute;
        }

        /// <summary>
        /// <para>Gets the integer representing the hour of the day in 24 hour format.</para>
        /// </summary>
        /// <value>
        /// <para>The integer representing the hour of the day in 24 hour format.</para>
        /// </value>
        [DataMember]
        public int Hour
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the integer representing the minute of the hour of the day.</para>
        /// </summary>
        /// <value>
        /// <para>Gets the integer representing the minute of the hour of the day.</para>
        /// </value>
        [DataMember]
        public int Minute
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Compares if left side time is later in the day than the right side.</para>
        /// </summary>
        /// <param name="operand1">Left side of operator.</param>
        /// <param name="operand2">Right side of operator.</param>
        public static bool operator >(ChaosScheduleTimeUtc operand1, ChaosScheduleTimeUtc operand2)
        {
            return operand1.CompareTo(operand2) == 1;
        }

        /// <summary>
        /// <para>Compares if left side time is earlier in the day than the right side.</para>
        /// </summary>
        /// <param name="operand1">Left side of operator.</param>
        /// <param name="operand2">Right side of operator.</param>
        public static bool operator <(ChaosScheduleTimeUtc operand1, ChaosScheduleTimeUtc operand2)
        {
            return operand1.CompareTo(operand2) == -1;
        }

        /// <summary>
        /// <para>Compares if left side time is later or at the same time in the day as the right side.</para>
        /// </summary>
        /// <param name="operand1">Left side of operator.</param>
        /// <param name="operand2">Right side of operator.</param>
        public static bool operator >=(ChaosScheduleTimeUtc operand1, ChaosScheduleTimeUtc operand2)
        {
            return operand1.CompareTo(operand2) >= 0;
        }

        /// <summary>
        /// <para>Compares if left side time is earlier in the day or at the same time as the right side.</para>
        /// </summary>
        /// <param name="operand1">Left side of operator.</param>
        /// <param name="operand2">Right side of operator.</param>
        public static bool operator <=(ChaosScheduleTimeUtc operand1, ChaosScheduleTimeUtc operand2)
        {
            return operand1.CompareTo(operand2) <= 0;
        }

        /// <summary>
        /// <para>Compares if left side time is the same time as the right side.</para>
        /// </summary>
        /// <param name="operand1">Left side of operator.</param>
        /// <param name="operand2">Right side of operator.</param>
        public static bool operator ==(ChaosScheduleTimeUtc operand1, ChaosScheduleTimeUtc operand2)
        {
            if (object.ReferenceEquals(operand1, null))
            {
                return object.ReferenceEquals(operand2, null);
            }

            return operand1.Equals(operand2);
        }

        /// <summary>
        /// <para>Compares if the left side time is not the same time as the right side.</para>
        /// </summary>
        /// <param name="operand1">Left side of operator.</param>
        /// <param name="operand2">Right side of operator.</param>
        public static bool operator !=(ChaosScheduleTimeUtc operand1, ChaosScheduleTimeUtc operand2)
        {
            return !operand1.Equals(operand2);
        }

        /// <summary>
        /// <para>Determine how this time relates to another time. </para>
        /// </summary>
        /// <returns>Returns 1 if this time is later than the other time, 0 if they are the same time and -1 if this time comes before the other time.</returns>
        public int CompareTo(ChaosScheduleTimeUtc other)
        {
            if (other == null)
            {
                return 1;
            }

            if (this.Hour > other.Hour || (this.Hour == other.Hour && this.Minute > other.Minute))
            {
                return 1;
            }
            else if (this.Hour == other.Hour && this.Minute == other.Minute)
            {
                return 0;
            }

            return -1;
        }

        /// <summary>
        /// <para>Compare if this time is equal to another object.</para>
        /// </summary>
        /// <returns>returns true if both are equal, false otherwise.</returns>
        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }

            ChaosScheduleTimeUtc timeObj = obj as ChaosScheduleTimeUtc;

            if (timeObj == null)
            {
                return false;
            }

            return this.Equals(timeObj);
        }

        /// <summary>
        /// <para>Compare if this time is equal to another time.</para>
        /// </summary>
        /// <returns>True if both times are equal. False otherwise.</returns>
        public bool Equals(ChaosScheduleTimeUtc other)
        {
            if (other == null)
            {
                return false;
            }

            return this.Hour == other.Hour && this.Minute == other.Minute;
        }

        /// <summary>
        /// <para>Get a hashcode for this object.</para>
        /// </summary>
        /// <returns>A hash value.</returns>
        public override int GetHashCode()
        {
            return (23 * this.Hour) + (29 * this.Minute);
        }

        /// <summary>
        /// Gets a string representation of this object.
        /// </summary>
        /// <returns>A string representation of the chaos schedule time object.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "Hour: {0}, Minute: {1}", this.Hour, this.Minute);
        }

        #region ByteSerializable
        /// <summary>
        /// Writes the state of this object into a byte array.
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Write(BinaryWriter bw)
        {
            bw.Write(ChaosConstants.ApiVersionCurrent);

            // Api Version >= 6.2
            bw.Write(this.Hour);
            bw.Write(this.Minute);
        }

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
                this.Hour = br.ReadInt32();
                this.Minute = br.ReadInt32();
            }
            else
            {
                TestabilityTrace.TraceSource.WriteError(TraceComponent, "Attempting to read a version of object below lowest version. Saw {0}. Expected >=6.2", objectVersion);
                ReleaseAssert.Fail("Failed to read byte serialization of ChaosScheduleTimeUtc");
            }
        }
        #endregion

        #region Interop Helpers

        internal static unsafe ChaosScheduleTimeUtc FromNative(IntPtr pointer)
        {
            var nativeTime = *(NativeTypes.FABRIC_CHAOS_SCHEDULE_TIME_UTC*)pointer;

            var hour = (int)nativeTime.Hour;
            var minute = (int)nativeTime.Minute;

            return new ChaosScheduleTimeUtc(hour, minute);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeTime = new NativeTypes.FABRIC_CHAOS_SCHEDULE_TIME_UTC
            {
                Hour = (uint)this.Hour,
                Minute = (uint)this.Minute
            };

            return pinCollection.AddBlittable(nativeTime);
        }

        #endregion
    }
}