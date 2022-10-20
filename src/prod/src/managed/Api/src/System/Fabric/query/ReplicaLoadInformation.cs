// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;

    /// <summary>
    /// <para>Represents the data structure that contains metric load information for a replica.</para>
    /// </summary>
    public class ReplicaLoadInformation
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.ReplicaLoadInformation" /> class.</para>
        /// </summary>
        public ReplicaLoadInformation()
            : this(Guid.Empty, 0)
        {
        }

        internal ReplicaLoadInformation(Guid partitionId, long replicaOrInstanceId)
        {
            this.PartitionId = partitionId;
            this.ReplicaOrInstanceId = replicaOrInstanceId;
            this.LoadMetricReports = new List<LoadMetricReport>();
        }

        /// <summary>
        /// <para>Gets the partition Identifier.</para>
        /// </summary>
        /// <value>
        /// <para>The partition Identifier.</para>
        /// </value>
        public Guid PartitionId { get; internal set; }

        /// <summary>
        /// <para>Gets the replica Identifer (stateful service), or instanceId (stateless service).</para>
        /// </summary>
        /// <value>
        /// <para>The replica Identifier (stateful service), or instanceId (stateless service).</para>
        /// </value>
        public long ReplicaOrInstanceId { get; internal set; }

        /// <summary>
        /// <para>Gets a list of metric and their load for a replica.</para>
        /// </summary>
        /// <value>
        /// <para>The list of metric and their load for a replica.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ReportedLoad)]
        public IList<LoadMetricReport> LoadMetricReports { get; internal set; }

        internal static unsafe ReplicaLoadInformation CreateFromNative(
            NativeClient.IFabricGetReplicaLoadInformationResult nativeResult)
        {
            var retval = CreateFromNative(*(NativeTypes.FABRIC_REPLICA_LOAD_INFORMATION*)nativeResult.get_ReplicaLoadInformation());
            GC.KeepAlive(nativeResult);

            return retval;
        }

        internal static unsafe ReplicaLoadInformation CreateFromNative(
            NativeTypes.FABRIC_REPLICA_LOAD_INFORMATION nativeLoadInformation)
        {
            IList<LoadMetricReport> LoadsList;

            if (nativeLoadInformation.LoadMetricReports == IntPtr.Zero)
            {
                LoadsList = new List<LoadMetricReport>();
            }
            else
            {
                LoadsList =
                    LoadMetricReport.CreateFromNativeList((NativeTypes.FABRIC_LOAD_METRIC_REPORT_LIST*)nativeLoadInformation.LoadMetricReports);
            }

            return new ReplicaLoadInformation(nativeLoadInformation.PartitionId, nativeLoadInformation.ReplicaOrInstanceId)
            {
                LoadMetricReports = LoadsList
            };
        }
    }
}