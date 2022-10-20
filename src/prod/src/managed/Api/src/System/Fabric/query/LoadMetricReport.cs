// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the load metric report.</para>
    /// </summary>
    public sealed class LoadMetricReport
    {   
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.LoadMetricReport" /> class.</para>
        /// </summary>
        public LoadMetricReport()
        {
        }

        /// <summary>
        /// <para>Gets the name of the metric.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the metric.</para>
        /// </value>
        public string Name { get; internal set; }

        /// <summary>
        /// <para>Gets the value of the load for the metric.</para>
        /// <para>
        /// In future releases of Service Fabric this parameter will be deprecated in favor of
        /// <see cref="System.Fabric.Query.LoadMetricReport.CurrentValue"/>.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The value of the load for the metric.</para>
        /// </value>
        public int Value { get; internal set; }

        /// <summary>
        /// <para>Gets the value of the load for the metric.</para>
        /// </summary>
        /// <value>
        /// <para>The value of the load for the metric.</para>
        /// </value>
        public double CurrentValue { get; internal set; }

        /// <summary>
        /// <para>Gets the UTC time when the load is reported.</para>
        /// </summary>
        /// <value>
        /// <para>The UTC time when the load is reported.</para>
        /// </value>
        public DateTime LastReportedUtc { get; internal set; }

        internal static unsafe LoadMetricReport CreateFromNative(NativeTypes.FABRIC_LOAD_METRIC_REPORT nativeResultItem)
        {
            double currentValue = 0;
            if (nativeResultItem.Reserved != IntPtr.Zero)
            {
                NativeTypes.FABRIC_LOAD_METRIC_REPORT_EX1* ext1 = 
                    (NativeTypes.FABRIC_LOAD_METRIC_REPORT_EX1*)nativeResultItem.Reserved;
                currentValue = ext1->CurrentValue;
            }

            return new LoadMetricReport
            {
                Name = NativeTypes.FromNativeString(nativeResultItem.Name),
                LastReportedUtc = NativeTypes.FromNativeFILETIME(nativeResultItem.LastReportedUtc),
                Value = (int)nativeResultItem.Value,
                CurrentValue = currentValue
            };
        }

        internal static unsafe IList<LoadMetricReport> CreateFromNativeList(NativeTypes.FABRIC_LOAD_METRIC_REPORT_LIST* list)
        {
            var rv = new List<LoadMetricReport>();

            var nativeArray = (NativeTypes.FABRIC_LOAD_METRIC_REPORT*)list->Items;
            for (int i = 0; i < list->Count; i++)
            {
                var nativeItem = *(nativeArray + i);
                rv.Add(LoadMetricReport.CreateFromNative(nativeItem));
            }

            return rv;
        }
    }
}