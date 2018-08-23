//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos
{
    using System;
    using System.Fabric.Chaos.DataStructures;
    using System.IO;
    using Runtime.Serialization;

    /// <summary>
    /// <para>Represents a ChaosSchedule</para>
    /// </summary>
    [Serializable]
    [DataContract]
    internal sealed class ChaosScheduleItem : ByteSerializable
    {
        private const string TraceType = "Chaos.ChaosScheduleItem";

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="chaosParameters">Name of the chaos parameters</param>
        /// <param name="time">ChaosScheduleTimeRangeUtc of when chaos will run for.</param>
        public ChaosScheduleItem(int jobIndex, string chaosParameters, ChaosScheduleTimeRangeUtc time)
        {
            this.JobIndex = jobIndex;
            this.ChaosParameters = chaosParameters;
            this.Time = time;
        }

        /// <summary>
        /// Array index of which Job this item originates from.
        /// </summary>
        [DataMember]
        public int JobIndex
        {
            get;
            set;
        }

        /// <summary>
        /// Time range in a day the schedule item is scheduled for.
        /// </summary>
        [DataMember]
        public ChaosScheduleTimeRangeUtc Time
        {
            get;
            set;
        }

        /// <summary>
        /// Parameters to start chaos with for the scheduled time.
        /// </summary>
        public string ChaosParameters
        {
            get;
            set;
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
            this.ChaosParameters = br.ReadString();
            this.Time.Read(br);
        }

        /// <summary>
        /// Writes the state of this object into a byte array.
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Write(BinaryWriter bw)
        {
            bw.Write(this.ChaosParameters);
            this.Time.Write(bw);
        }
        #endregion
    }
}