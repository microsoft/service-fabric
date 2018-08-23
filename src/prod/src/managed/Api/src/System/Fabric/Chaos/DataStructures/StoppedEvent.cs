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
    /// <para>Represents the Chaos event that is created when Chaos is stopped for some reason.</para>
    /// </summary>
    [Serializable]
    public sealed class StoppedEvent : ChaosEvent
    {
        private const string TraceType = "Chaos.StoppedEvent";

        internal StoppedEvent()
        {
            base.Kind = "Stopped";
        }

        internal StoppedEvent(DateTime timeStamp, string reason)
            : base("Stopped", timeStamp)
        {
            this.Reason = reason;
        }

        /// <summary>
        /// Gets the reason for stopping.
        /// </summary>
        [DataMember]
        public string Reason
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string representation of the stopped event.
        /// </summary>
        /// <returns>A string representation of the stopped event.</returns>
        public override string ToString()
        {
            var description = new StringBuilder();
            description.AppendLine(base.ToString());
            description.AppendLine(string.Format(CultureInfo.InvariantCulture, "Reason for stop: {0}", this.Reason));

            return description.ToString();
        }

        internal static unsafe StoppedEvent FromNative(IntPtr nativeRaw)
        {
            if (nativeRaw == IntPtr.Zero)
            {
                return null;
            }

            NativeTypes.FABRIC_STOPPED_EVENT native = *(NativeTypes.FABRIC_STOPPED_EVENT*)nativeRaw;
            var timeStamp = NativeTypes.FromNativeFILETIME(native.TimeStampUtc);
            string reason = NativeTypes.FromNativeString(native.Reason);

            return new StoppedEvent(timeStamp, reason);
        }

        internal override unsafe IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeStoppedEvent = new NativeTypes.FABRIC_STOPPED_EVENT
                                         {
                                             TimeStampUtc = NativeTypes.ToNativeFILETIME(this.TimeStampUtc),
                                             Reason = pinCollection.AddObject(this.Reason)
                                         };

            return pinCollection.AddBlittable(nativeStoppedEvent);
        }

        /// <summary>
        /// Reads the state of this object from byte array.
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        /// <exception cref="EndOfStreamException">The end of the stream is reached. </exception>
        public override void Read(BinaryReader br)
        {
            this.ReadInheritedMembers(br);
            this.Reason = br.ReadString();
        }

        /// <summary>
        /// Writes the state of this object into a byte array.
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Write(BinaryWriter bw)
        {
            this.WriteInheritedMembers(bw);
            bw.Write(this.Reason);
        }
    }
}