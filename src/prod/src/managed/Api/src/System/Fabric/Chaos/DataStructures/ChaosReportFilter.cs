// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Text;

    /// <summary>
    ///  <para>Represents the filter to choose the interesting ChaosEvent's to include in the <see cref="System.Fabric.Chaos.DataStructures.ChaosReport" /></para>
    /// </summary>
    public sealed class ChaosReportFilter : ByteSerializable
    {
        private const string TraceComponent = "ChaosReportFilter";

        internal ChaosReportFilter()
        {
            this.StartTimeUtc = DateTime.MinValue;
            this.EndTimeUtc = DateTime.MaxValue;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="startTimeUtc"></param>
        /// <param name="endTimeUtc"></param>
        public ChaosReportFilter(
            DateTime startTimeUtc,
            DateTime endTimeUtc)
        {
            ThrowIf.IsTrue(startTimeUtc > endTimeUtc, "Start time must not be later than the end time.");

            this.StartTimeUtc = startTimeUtc;
            this.EndTimeUtc = endTimeUtc;
        }

        /// <summary>
        /// 
        /// </summary>
        public DateTime StartTimeUtc { get; internal set; }

        /// <summary>
        /// 
        /// </summary>
        public DateTime EndTimeUtc { get; private set; }

        /// <summary>
        /// Gets a string representation of the chaos report object.
        /// </summary>
        /// <returns>A string representation of the chaos status object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendFormat("startTime={0}, endTime={1}",
                this.StartTimeUtc.ToString(CultureInfo.InvariantCulture),
                this.EndTimeUtc.ToString(CultureInfo.InvariantCulture));

            return sb.ToString();
        }

        internal bool Match(long key)
        {
            var eventTimeStamp = new DateTime(key);

            return eventTimeStamp >= this.StartTimeUtc && eventTimeStamp <= this.EndTimeUtc;
        }

        #region Interop Helpers
        internal unsafe IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeReportFilter = new NativeTypes.FABRIC_CHAOS_REPORT_FILTER();

            nativeReportFilter.StartTimeUtc = NativeTypes.ToNativeFILETIME(this.StartTimeUtc);
            nativeReportFilter.EndTimeUtc = NativeTypes.ToNativeFILETIME(this.EndTimeUtc);

            return pinCollection.AddBlittable(nativeReportFilter);
        }

        internal static unsafe ChaosReportFilter FromNative(IntPtr pointer)
        {
            NativeTypes.FABRIC_CHAOS_REPORT_FILTER nativeReport = *(NativeTypes.FABRIC_CHAOS_REPORT_FILTER*)pointer;

            var startTimeUtc = NativeTypes.FromNativeFILETIME(nativeReport.StartTimeUtc);
            var endTimeUtc = NativeTypes.FromNativeFILETIME(nativeReport.EndTimeUtc);

            return new ChaosReportFilter(startTimeUtc, endTimeUtc);
        }

        /// <summary>
        /// Reads the state of this object from byte array.
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        public override void Read(BinaryReader br)
        {
            this.StartTimeUtc = new DateTime(br.ReadInt64());
            this.EndTimeUtc = new DateTime(br.ReadInt64());
        }

        /// <summary>
        /// Writes the state of this object into a byte array.
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        public override void Write(BinaryWriter bw)
        {
            bw.Write(this.StartTimeUtc.Ticks);
            bw.Write(this.EndTimeUtc.Ticks);
        }

        #endregion
    }
}