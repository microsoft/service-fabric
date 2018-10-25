// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Fabric.Chaos.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Runtime.Serialization;

    /// <summary>
    /// This is the base class for all the different types of events that Chaos generates
    /// </summary>
    [Serializable]
    public abstract class ChaosEvent : ByteSerializable
    {
        private const string TraceType = "Chaos.ChaosEvent";

        internal ChaosEvent()
        {
        }

        /// <summary>
        /// The base constructor for a Chaos event
        /// </summary>
        /// <param name="kind"></param>
        /// <param name="timeStamp">DateTime of when the event occurred</param>
        internal ChaosEvent(string kind, DateTime timeStamp)
        {
            this.Kind = kind;
            this.TimeStampUtc = timeStamp;
        }

        /// <summary>
        /// DateTime of when the event occurred
        /// </summary>
        [DataMember]
        public DateTime TimeStampUtc { get; internal set; }

        /// <summary>
        /// <para>Describes the type of ChaosEvent.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent" /> The type of derived ChaosEvent</para>
        /// </value>
        [DataMember]
        public string Kind { get; internal set; }

        /// <summary>
        /// In order to convert the Chaos event object into a byte array
        /// this override starts off by writing the event type and then
        /// calls the Write method of the derived class
        /// </summary>
        /// <returns>A byte array</returns>
        public override byte[] ToBytes()
        {
            using (MemoryStream mem = new MemoryStream())
            {
                using (BinaryWriter bw = new BinaryWriter(mem))
                {
                    this.WriteEventType(bw);

                    this.Write(bw);
                    return mem.ToArray();
                }
            }
        }

        /// <summary>
        /// This method is called from the derived class to convert the inherited members into bytes
        /// </summary>
        /// <param name="bw">A BinaryWriter object</param>
        protected void WriteInheritedMembers(BinaryWriter bw)
        {
            bw.Write(this.TimeStampUtc.Ticks);
        }

        /// <summary>
        /// This method is called from the derived class to convert back the inherited members from bytes
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        protected void ReadInheritedMembers(BinaryReader br)
        {
            long tickCount = br.ReadInt64();
            this.TimeStampUtc = new DateTime(tickCount);
        }

        /// <summary>
        /// This method is passed onto the derived classes for implementation
        /// </summary>
        /// <param name="bw">A BinaryWriter object</param>
        public abstract override void Write(BinaryWriter bw);

        /// <summary>
        /// This method is passed onto the derived classed for implementation
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        public abstract override void Read(BinaryReader br);

        internal abstract IntPtr ToNative(PinCollection pin);

        /// <summary>
        /// Returns a string representation of the class
        /// </summary>
        /// <returns>A string object</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "[{0}], Timestamp={1}", this.GetType().Name, this.TimeStampUtc);
        }

        private void WriteEventType(BinaryWriter bw)
        {
            bw.Write(ChaosUtility.ChaosEventTypeToIntMap[this.GetType()]);
        }
    }
}