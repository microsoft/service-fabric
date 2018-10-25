// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Indicates that the service is Singleton-partitioned, effectively non-partitioned or with one partition only, and uses the partition 
    /// scheme of a Singleton service.</para>
    /// </summary>
    public sealed class SingletonPartitionInformation : ServicePartitionInformation
    {
        /// <summary>
        /// <para>Creates a new instance of the <see cref="System.Fabric.SingletonPartitionInformation" /> class.</para>
        /// </summary>
        public SingletonPartitionInformation()
            : base(ServicePartitionKind.Singleton)
        {
        }

        internal static unsafe SingletonPartitionInformation FromNative(NativeTypes.FABRIC_SINGLETON_PARTITION_INFORMATION* nativePartition)
        {
            var servicePartitionInfo = new SingletonPartitionInformation();
            servicePartitionInfo.Id = nativePartition->Id;

            return servicePartitionInfo;
        }

        internal override string GetPartitionInformationString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Kind:{0}", ServicePartitionKind.Singleton.ToString("G"));
        }
    }
}