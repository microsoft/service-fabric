// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Threading;

    /// <summary>
    /// <para>Represents the status of running Chaos, possibly within a time range, as returned by <see cref="System.Fabric.FabricClient.TestManagementClient.GetChaosReportAsync(ChaosReportFilter, TimeSpan, CancellationToken)" />.</para>
    /// </summary>
    [Serializable]
    public sealed class ChaosReport
    {
        private const string TraceComponent = "ChaosReport";

        internal ChaosReport()
        {
            this.History = new List<ChaosEvent>();
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosReport" /> class.</para>
        /// </summary>
        /// <param name="chaosParameters">Parameters with which Chaos was started</param>
        /// <param name="status">If Chaos is currently running or stopped.</param>
        /// <param name="history">The list of ChaosEvent's that match the ChaosReportFilter in ChaosParameters</param>
        /// <param name="continuationToken">If too many events match the ChaosReportFilter, those will be returned in multiple batches, the batches are linked by this token.</param>
        public ChaosReport(
            ChaosParameters chaosParameters,
            ChaosStatus status,
            List<ChaosEvent> history,
            string continuationToken)
        {
            this.ChaosParameters = chaosParameters;
            this.Status = status;
            this.History = history;
            this.ContinuationToken = continuationToken;
        }

        /// <summary>
        /// <para>Gets the parameters that was passed into <see cref="System.Fabric.FabricClient.TestManagementClient.StartChaosAsync(ChaosParameters, TimeSpan, CancellationToken)" /> and with which Chaos was started.</para>
        /// </summary>
        [DataMember]
        public ChaosParameters ChaosParameters
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the current task (one of the enumerators in CurrentChaosTask <c>enum</c>) that Chaos is performing.</para>
        /// </summary>
        [DataMember]
        public ChaosStatus Status
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the list of Chaos events that were produced during the time of interest specified in <see cref="System.Fabric.FabricClient.TestManagementClient.GetChaosReportAsync(ChaosReportFilter, TimeSpan, CancellationToken)" />.
        /// </summary>
        [DataMember]
        public List<ChaosEvent> History
        {
            get;
            internal set;
        }

        /// <summary>
        /// If the number of Chaos events is too large, then those will be returned in batches; so, to get the
        /// next batch of events the ContinuationToken must be passed in the API call, <see cref="System.Fabric.FabricClient.TestManagementClient.GetChaosReportAsync(string, TimeSpan, CancellationToken)" />.
        /// </summary>
        [DataMember]
        public string ContinuationToken
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string representation of the chaos status object.
        /// </summary>
        /// <returns>A string representation of the chaos status object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "Parameters={0}", this.ChaosParameters));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "Status={0}", this.Status));
            if (this.History.Any())
            {
                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "History:"));
                foreach (ChaosEvent ce in this.History)
                {
                    sb.AppendLine(ce.ToString());
                }
            }

            if (!string.IsNullOrEmpty(this.ContinuationToken))
            {
                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ContinuationToken={0}", this.ContinuationToken));
            }

            return sb.ToString();
        }

        #region Interop Helpers
        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeReport = new NativeTypes.FABRIC_CHAOS_REPORT
            {
                ChaosParameters = this.ChaosParameters.ToNative(pinCollection),
                Status = (NativeTypes.FABRIC_CHAOS_STATUS)this.Status,
                ContinuationToken = pinCollection.AddObject(this.ContinuationToken),
                History = ToNativeHistory(pinCollection, this.History)
            };

            return pinCollection.AddBlittable(nativeReport);
        }

        private static IntPtr ToNativeHistory(PinCollection pin, List<ChaosEvent> history)
        {
            if (history == default(List<ChaosEvent>))
            {
                return IntPtr.Zero;
            }

            var eventArray = new NativeTypes.FABRIC_CHAOS_EVENT[history.Count];
            for (int i = 0; i < history.Count; ++i)
            {
                eventArray[i].Value = history[i].ToNative(pin);
                eventArray[i].Kind = ChaosUtility.GetNativeEventType(history[i]);
            }

            var nativeHistory = new NativeTypes.FABRIC_CHAOS_EVENT_LIST
            {
                Count = (uint)eventArray.Length,
                Items = pin.AddBlittable(eventArray)
            };

            return pin.AddBlittable(nativeHistory);
        }

        internal static unsafe ChaosReport FromNative(IntPtr pointer)
        {
            var nativeReport = *(NativeTypes.FABRIC_CHAOS_REPORT*)pointer;
            var chaosParameters = ChaosParameters.CreateFromNative(nativeReport.ChaosParameters);
            var status = (ChaosStatus)nativeReport.Status;
            var continuationToken = NativeTypes.FromNativeString(nativeReport.ContinuationToken);
            var history = FromNativeHistory(nativeReport.History);
            return new ChaosReport(chaosParameters, status, history, continuationToken);
        }

        private static unsafe List<ChaosEvent> FromNativeHistory(IntPtr pointer)
        {
            var nativeHistoryPtr = (NativeTypes.FABRIC_CHAOS_EVENT_LIST*)pointer;
            var retval = new List<ChaosEvent>();
            var nativeEventArrayPtr = (NativeTypes.FABRIC_CHAOS_EVENT*)nativeHistoryPtr->Items;
            for (int i = 0; i < nativeHistoryPtr->Count; ++i)
            {
                var nativeItem = *(nativeEventArrayPtr + i);
                ChaosEvent chaosEvent = ChaosUtility.FromNativeEvent(nativeItem);
                retval.Add(chaosEvent);
            }

            return retval;
        }

        #endregion
    }
}