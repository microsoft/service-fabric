// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Interop;
    using System.Globalization;

    /// <summary>
    /// <para>Describes the partition information for the name as a string that is based on partition schemes.
    /// Services observe this type of <see cref="System.Fabric.ServicePartitionInformation" /> when the service is created with the <see cref="System.Fabric.Description.NamedPartitionSchemeDescription" />. <see cref="System.Fabric.NamedPartitionInformation" /> derives from the <see cref="System.Fabric.IServicePartition" /> interface and is provided to services as part of the <see cref="System.Fabric.IStatefulServicePartition" /> or <see cref="System.Fabric.IStatelessServicePartition" /> interface, which is passed in during the stateful <see cref="System.Fabric.IStatefulServiceReplica.OpenAsync(System.Fabric.ReplicaOpenMode,System.Fabric.IStatefulServicePartition,System.Threading.CancellationToken)" /> or the stateless <see cref="System.Fabric.IStatelessServiceInstance.OpenAsync(System.Fabric.IStatelessServicePartition,System.Threading.CancellationToken)" />.</para>
    /// </summary>
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Preserve order of public members from V1.")]
    public sealed class NamedPartitionInformation : ServicePartitionInformation
    {
        internal NamedPartitionInformation()
            :base(ServicePartitionKind.Named)
        {
        }

        /// <summary>
        /// <para>Indicates the name of this partition of the service.</para>
        /// </summary>
        /// <value>
        /// <para>The name of this partition of the service.</para>
        /// </value>
        public string Name { get; internal set; }

        internal static unsafe NamedPartitionInformation FromNative(NativeTypes.FABRIC_NAMED_PARTITION_INFORMATION* nativePartition)
        {
            var namedPartitionInfo = new NamedPartitionInformation();
            namedPartitionInfo.Id = nativePartition->Id;
            namedPartitionInfo.Name = NativeTypes.FromNativeString(nativePartition->Name);

            return namedPartitionInfo;
        }

        internal override string GetPartitionInformationString()
        {
            return string.Format(
                        CultureInfo.InvariantCulture,
                        "Kind:{0} Name:{1}",
                        ServicePartitionKind.Named.ToString("G"),
                        this.Name);
        }
    }
}