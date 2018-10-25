// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Indicates the type of partitioning scheme that is used. </para>
    /// </summary>
    /// <remarks>
    /// <para>
    ///     <see cref="System.Fabric.ServicePartitionKind" /> defines the value of the <see cref="System.Fabric.ServicePartitionInformation.Kind" /> property of the 
    /// <see cref="System.Fabric.ServicePartitionInformation" /> class.</para>
    /// </remarks>
    public enum ServicePartitionKind
    {
        /// <summary>
        /// <para>Indicates the partition kind is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_INVALID,
        /// <summary>
        /// <para>Indicates that the partition is based on string names, and is a <see cref="System.Fabric.SingletonPartitionInformation" /> object, that was originally 
        /// created via <see cref="System.Fabric.Description.SingletonPartitionSchemeDescription" />.</para>
        /// </summary>
        Singleton = NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_SINGLETON,
        /// <summary>
        /// <para>Indicates that the partition is based on Int64 key ranges, and is an <see cref="System.Fabric.Int64RangePartitionInformation" /> object that was 
        /// originally created via <see cref="System.Fabric.Description.UniformInt64RangePartitionSchemeDescription" />.</para>
        /// </summary>
        Int64Range = NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE,
        /// <summary>
        /// <para>Indicates that the partition is based on string names, and is a <see cref="System.Fabric.NamedPartitionInformation" /> object, that was originally 
        /// created via <see cref="System.Fabric.Description.NamedPartitionSchemeDescription" />.</para>
        /// </summary>
        Named = NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_NAMED,
    }
}