// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Text;

    /// <summary>
    /// <para>Represents the event that encapsulates the faults that are being executed by Chaos.</para>
    /// </summary>
    [Serializable]
    public class ExecutingFaultsEvent : ChaosEvent
    {
        private const string TraceType = "Chaos.ExecutingFaultsEvent";

        private List<string> faultList = null;

        /// <summary>
        ///
        /// </summary>
        /// <param name="timeStampUtc"></param>
        /// <param name="faults"></param>
        public ExecutingFaultsEvent(DateTime timeStampUtc, List<string> faults)
            : base("ExecutingFaults", timeStampUtc)
        {
            Requires.Argument("faults", faults).NotNull();

            this.faultList = faults;
        }

        /// <summary>
        ///
        /// </summary>
        public ExecutingFaultsEvent()
        {
            base.Kind = "ExecutingFaults";
            this.faultList = new List<string>();
        }

        /// <summary>
        /// Gets the list of faults that Chaos is executing.
        /// </summary>
        [DataMember]
        public List<string> Faults
        {
            get
            {
                return this.faultList;
            }

            internal set
            {
                this.faultList = value;
            }
        }

        /// <summary>
        /// Gets a string representation of the executing faults event.
        /// </summary>
        /// <returns>A string representation of the executing faults event.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine(base.ToString());
            sb.AppendFormat(CultureInfo.InvariantCulture, "{0} Faults: \n", this.Faults.Count);
            foreach (var fault in this.Faults)
            {
                sb.AppendFormat(CultureInfo.InvariantCulture, "\tFault: {0}\n", fault);
            }

            return sb.ToString();
        }

        internal static unsafe ExecutingFaultsEvent FromNative(IntPtr nativeRaw)
        {
            if (nativeRaw == IntPtr.Zero)
            {
                return null;
            }

            var native = *(NativeTypes.FABRIC_EXECUTING_FAULTS_EVENT*)nativeRaw;

            var timeStamp = NativeTypes.FromNativeFILETIME(native.TimeStampUtc);

            var nativeFaults = *(NativeTypes.FABRIC_STRING_LIST*)native.Faults;
            var managedFaults = NativeTypes.FromNativeStringList(nativeFaults);

            return new ExecutingFaultsEvent(timeStamp, managedFaults);
        }

        internal override IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeExecutingFaultsEvent = new NativeTypes.FABRIC_EXECUTING_FAULTS_EVENT
                                                 {
                                                     TimeStampUtc = NativeTypes.ToNativeFILETIME(this.TimeStampUtc),
                                                     Faults = NativeTypes.ToNativeStringList(pinCollection, this.Faults)
                                                 };

            return pinCollection.AddBlittable(nativeExecutingFaultsEvent);
        }

        /// <summary>
        /// Reads the state of this object from byte array.
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        /// <exception cref="EndOfStreamException">The end of the stream is reached. </exception>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Read(BinaryReader br)
        {
            this.ReadInheritedMembers(br);

            int numberOfFaults = br.ReadInt32();
            for (int i = 0; i < numberOfFaults; ++i)
            {
                this.faultList.Add(br.ReadString());
            }
        }

        /// <summary>
        /// Writes the state of this object into a byte array.
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        /// <exception cref="IOException">An I/O error occurs. </exception>
        public override void Write(BinaryWriter bw)
        {
            this.WriteInheritedMembers(bw);

            bw.Write(this.Faults == null ? 0 : this.Faults.Count);
            this.faultList.ForEach(bw.Write);
        }
    }
}