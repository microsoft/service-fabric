// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Describes the partition information for the integer range that is based on partition schemes. Services observe this type of <see cref="System.Fabric.ServicePartitionInformation" /> when the service is created with the <see cref="System.Fabric.Description.UniformInt64RangePartitionSchemeDescription" /> class. <see cref="System.Fabric.Int64RangePartitionInformation" /> derives from <see cref="System.Fabric.IServicePartition" /> and is provided to services as part of the <see cref="System.Fabric.IStatefulServicePartition" /> or <see cref="System.Fabric.IStatelessServicePartition" />, which is passed in via the stateful <see cref="System.Fabric.IStatefulServiceReplica.OpenAsync(System.Fabric.ReplicaOpenMode,System.Fabric.IStatefulServicePartition,System.Threading.CancellationToken)" /> or the stateless <see cref="System.Fabric.IStatelessServiceInstance.OpenAsync(System.Fabric.IStatelessServicePartition,System.Threading.CancellationToken)" /> methods.</para>
    /// </summary>
    public sealed class Int64RangePartitionInformation : ServicePartitionInformation
    {
        internal Int64RangePartitionInformation():
            base(ServicePartitionKind.Int64Range)
        {
        }

        /// <summary>
        /// <para>Specifies the minimum key value for this partition.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Int64" />.</para>
        /// </value>
        public long LowKey { get; internal set; }

        /// <summary>
        /// <para>Specifies the maximum key value for this partition.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Int64" />.</para>
        /// </value>
        public long HighKey { get; internal set; }

        internal static unsafe Int64RangePartitionInformation FromNative(NativeTypes.FABRIC_INT64_RANGE_PARTITION_INFORMATION* nativePartition)
        {
            var int64RangePartitionInfo = new Int64RangePartitionInformation();
            int64RangePartitionInfo.Id = nativePartition->Id;
            int64RangePartitionInfo.LowKey = nativePartition->LowKey;
            int64RangePartitionInfo.HighKey = nativePartition->HighKey;

            return int64RangePartitionInfo;
        }

        internal override string GetPartitionInformationString()
        {
            return string.Format(
                        CultureInfo.InvariantCulture,
                        "Kind:{0} Highkey:{1} Lowkey:{2}",
                        ServicePartitionKind.Int64Range.ToString("G"),
                        this.HighKey,
                        this.LowKey);
        }
    }
}