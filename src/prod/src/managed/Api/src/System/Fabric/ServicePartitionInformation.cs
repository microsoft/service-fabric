// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Represents the base class for describing partitions.</para>
    /// </summary>
    /// <remarks>
    /// <para>
    ///     <see cref="System.Fabric.Int64RangePartitionInformation" />, <see cref="System.Fabric.NamedPartitionInformation" />, and <see cref="System.Fabric.SingletonPartitionInformation" /> 
    /// all derive from this type.</para>
    /// </remarks>
    [KnownType(typeof(SingletonPartitionInformation))]
    [KnownType(typeof(Int64RangePartitionInformation))]
    [KnownType(typeof(NamedPartitionInformation))]
    public abstract class ServicePartitionInformation
    {
        internal ServicePartitionInformation(ServicePartitionKind kind)
        {
            this.Kind = kind;
        }

        /// <summary>
        /// <para>Specifies the partition ID for this partition as a GUID.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Guid" />.</para>
        /// </value>
        public Guid Id { get; internal set; }

        /// <summary>
        /// <para>Describes the type of partition.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.ServicePartitionKind" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ServicePartitionKind, AppearanceOrder = -2)]
        public ServicePartitionKind Kind { get; private set; }

        internal static unsafe ServicePartitionInformation FromNative(IntPtr partitionIntPtr)
        {
            ReleaseAssert.AssertIf(partitionIntPtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "partitionInfo"));
            return FromNative(*(NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION*)partitionIntPtr);
        }

        internal static unsafe ServicePartitionInformation FromNative(NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION nativePartition)
        {
            ServicePartitionInformation servicePartitionInfo = null;

            switch (nativePartition.Kind)
            {
                case NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_SINGLETON:
                    servicePartitionInfo = SingletonPartitionInformation.FromNative((NativeTypes.FABRIC_SINGLETON_PARTITION_INFORMATION*)nativePartition.Value);
                    break;

                case NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
                    servicePartitionInfo = Int64RangePartitionInformation.FromNative((NativeTypes.FABRIC_INT64_RANGE_PARTITION_INFORMATION*)nativePartition.Value);
                    break;

                case NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_NAMED:
                    servicePartitionInfo = NamedPartitionInformation.FromNative((NativeTypes.FABRIC_NAMED_PARTITION_INFORMATION*)nativePartition.Value);
                    break;

                default:
                    AppTrace.TraceSource.WriteError("ServicePartitionInformation.FromNative", "Unknown partition kind: {0}", nativePartition.Kind);
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_PartitionKindUnknown_Formatted, nativePartition.Kind));
            }

            return servicePartitionInfo;
        }

        internal abstract string GetPartitionInformationString();

        // Method used by JsonSerializer to resolve derived type using json property "ServicePartitionKind".
        // Provide name of the json property which will be used as parameter to call this method.
        [DerivedTypeResolverAttribute("ServicePartitionKind")]
        internal static Type ResolveDerivedClass(ServicePartitionKind kind)
        {
            switch (kind)
            {
                case ServicePartitionKind.Singleton:
                    return typeof(SingletonPartitionInformation);

                case ServicePartitionKind.Int64Range:
                    return typeof(Int64RangePartitionInformation);

                case ServicePartitionKind.Named:
                    return typeof(NamedPartitionInformation);

                default:
                    return null;
            }
        }
    }
}