// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Text;

    /// <summary>
    /// <para>Represents the event that is created when Chaos is started for the first time or following a stop.</para>
    /// </summary>
    [Serializable]
    public sealed class StartedEvent : ChaosEvent
    {
        private const string TraceType = "StartedEvent";

        /// <summary>
        ///
        /// </summary>
        public StartedEvent()
        {
            base.Kind = "Started";
            this.ChaosParameters = new ChaosParameters();
        }

        /// <summary>
        ///
        /// </summary>
        /// <param name="timeStamp"></param>
        /// <param name="chaosParameters"></param>
        public StartedEvent(DateTime timeStamp, ChaosParameters chaosParameters)
            : base("Started", timeStamp)
        {
            this.ChaosParameters = chaosParameters;
        }

        /// <summary>
        /// Gets the object that encapsulate the parameters with which Chaos was started.
        /// </summary>
        [DataMember]
        public ChaosParameters ChaosParameters
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string representation of the started event.
        /// </summary>
        /// <returns>A string representation of the started event.</returns>
        public override string ToString()
        {
            var description = new StringBuilder();

            description.AppendLine(base.ToString());

            description.AppendLine(string.Format(CultureInfo.InvariantCulture, "ChaosParameters: {0}", this.ChaosParameters));

            return description.ToString();
        }

        internal static unsafe StartedEvent FromNative(IntPtr nativeRaw)
        {
            if (nativeRaw == IntPtr.Zero)
            {
                return null;
            }

            var native = *(NativeTypes.FABRIC_STARTED_EVENT*)nativeRaw;
            var timeStamp = NativeTypes.FromNativeFILETIME(native.TimeStampUtc);
            var parameters = ChaosParameters.CreateFromNative(native.ChaosParameters);

            return new StartedEvent(timeStamp, parameters);
        }

        internal override IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeStartedEvent = new NativeTypes.FABRIC_STARTED_EVENT
	             {
	                 TimeStampUtc = NativeTypes.ToNativeFILETIME(this.TimeStampUtc),
	                 ChaosParameters = this.ChaosParameters.ToNative(pinCollection)
	             };

            return pinCollection.AddBlittable(nativeStartedEvent);
        }

        /// <summary>
        /// Reads the state of this object from byte array.
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        public override void Read(BinaryReader br)
        {
            this.ReadInheritedMembers(br);
            this.ChaosParameters.Read(br);
        }

        /// <summary>
        /// Writes the state of this object into a byte array.
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        public override void Write(BinaryWriter bw)
        {
            this.WriteInheritedMembers(bw);
            this.ChaosParameters.Write(bw);
        }
    }
}