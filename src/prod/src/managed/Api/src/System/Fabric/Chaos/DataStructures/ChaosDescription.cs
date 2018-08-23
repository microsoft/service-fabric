//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using Interop;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Text;

    /// <summary>
    /// <para>Represents the status of Chaos.</para>
    /// </summary>
    [DataContract]
    public sealed class ChaosDescription
    {
        private const string TraceComponent = "Chaos.ChaosDescription";

        internal ChaosDescription()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosDescription" /> class.</para>
        /// </summary>
        /// <param name="chaosStatus">The status of Chaos. Can be Stopped, Running or Invalid</param>
        /// <param name="chaosParameters">The parameters that Chaos runs with if Chaos is currently running or the parameters if Chaos was running with if Chaos was running. If Chaos has never ran before, then the value here does not have a meaning.</param>
        /// <param name="chaosScheduleStatus">The state of the schedule.</param>
        public ChaosDescription(
            ChaosStatus chaosStatus,
            ChaosParameters chaosParameters,
            ChaosScheduleStatus chaosScheduleStatus)
        {
            this.Status = chaosStatus;
            this.ChaosParameters = chaosParameters;
            this.ScheduleStatus = chaosScheduleStatus;
        }

        /// <summary>
        /// <para>Gets the Chaos status.</para>
        /// </summary>
        [DataMember]
        public ChaosStatus Status
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the Chaos parameters.</para>
        /// </summary>
        [DataMember]
        public ChaosParameters ChaosParameters
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the Chaos schedule status.</para>
        /// </summary>
        [DataMember]
        public ChaosScheduleStatus ScheduleStatus
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets a string representation of the Chaos description object.</para>
        /// </summary>
        /// <returns>A string representation of the Chaos description object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ChaosStatus={0}", this.Status));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ChaosParameters={0}", this.ChaosParameters));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ChaosScheduleStatus={0}", this.ScheduleStatus));
            return sb.ToString();
        }

        #region Interop Helpers

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_CHAOS_DESCRIPTION
            {
                ChaosParameters = this.ChaosParameters.ToNative(pinCollection),
                Status = (NativeTypes.FABRIC_CHAOS_STATUS)this.Status,
                ScheduleStatus = (NativeTypes.FABRIC_CHAOS_SCHEDULE_STATUS)this.ScheduleStatus
            };

            return pinCollection.AddBlittable(nativeDescription);
        }

        internal static unsafe ChaosDescription FromNative(IntPtr pointer)
        {
            var nativeDescription = *(NativeTypes.FABRIC_CHAOS_DESCRIPTION*)pointer;
            var chaosParameters = ChaosParameters.CreateFromNative(nativeDescription.ChaosParameters);

            var chaosStatus = (ChaosStatus)nativeDescription.Status;
            var chaosScheduleStatus = (ChaosScheduleStatus)nativeDescription.ScheduleStatus;

            return new ChaosDescription(chaosStatus, chaosParameters, chaosScheduleStatus);
        }

        #endregion
    }
}
