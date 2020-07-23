// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;
    using System.Collections.Generic;

    /// <summary>
    /// <para>Represents the partition load information.</para>
    /// </summary>
    public class PartitionLoadInformation
    {
        /// <summary>
        /// <para>Initializes an instance of the <see cref="System.Fabric.Query.PartitionLoadInformation" /> class.</para>
        /// </summary>
        public PartitionLoadInformation()
            : this(Guid.Empty)
        {
        }

        internal PartitionLoadInformation(Guid partitionId)
        {
            this.PartitionId = partitionId;

            // Initialize with empty list.
            this.PrimaryLoadMetricReports = new List<LoadMetricReport>();
            this.SecondaryLoadMetricReports = new List<LoadMetricReport>();
        }

        /// <summary>
        /// <para>Gets the partition ID. This command can be used as piping purpose.</para>
        /// </summary>
        /// <value>
        /// <para>The partition ID.</para>
        /// </value>
        public Guid PartitionId { get; private set; }

        /// <summary>
        /// <para>Gets the list of load reports for primary role of this partition.</para>
        /// </summary>
        /// <value>
        /// <para>The list of load reports for primary role of this partition.</para>
        /// </value>
        public IList<LoadMetricReport> PrimaryLoadMetricReports { get; private set; }

        /// <summary>
        /// <para>Gets the list of load reports for secondary role of this partition.</para>
        /// </summary>
        /// <value>
        /// <para>The list of load reports for secondary role of this partition.</para>
        /// </value>
        public IList<LoadMetricReport> SecondaryLoadMetricReports { get; private set; }

        internal static unsafe PartitionLoadInformation CreateFromNative(
            NativeClient.IFabricGetPartitionLoadInformationResult nativeResult)
        {
            var retval = CreateFromNative(*(NativeTypes.FABRIC_PARTITION_LOAD_INFORMATION*)nativeResult.get_PartitionLoadInformation());
            GC.KeepAlive(nativeResult);

            return retval;
        }

        internal static unsafe PartitionLoadInformation CreateFromNative(
            NativeTypes.FABRIC_PARTITION_LOAD_INFORMATION nativeLoadInformation)
        {
            IList<LoadMetricReport> primaryLoadsList;
            IList<LoadMetricReport> secondaryLoadsList;

            if (nativeLoadInformation.PrimaryLoadMetricReports == IntPtr.Zero)
            {
                primaryLoadsList = new List<LoadMetricReport>();
            }
            else
            {
                primaryLoadsList =
                    LoadMetricReport.CreateFromNativeList((NativeTypes.FABRIC_LOAD_METRIC_REPORT_LIST*)nativeLoadInformation.PrimaryLoadMetricReports);
            }

            if (nativeLoadInformation.SecondaryLoadMetricReports == IntPtr.Zero)
            {
                secondaryLoadsList = new List<LoadMetricReport>();
            }
            else
            {
                secondaryLoadsList =
                   LoadMetricReport.CreateFromNativeList((NativeTypes.FABRIC_LOAD_METRIC_REPORT_LIST*)nativeLoadInformation.SecondaryLoadMetricReports);
            }

            return new PartitionLoadInformation(nativeLoadInformation.PartitionId)
            {
                PrimaryLoadMetricReports = primaryLoadsList,
                SecondaryLoadMetricReports = secondaryLoadsList
            };
        }
    }
}