// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Enumerates the ways that a service can be partitioned.</para>
    /// </summary>
    public enum PartitionScheme
    {
        /// <summary>
        /// <para>All Service Fabric enumerations reserve the "Invalid" value.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_PARTITION_SCHEME.FABRIC_PARTITION_SCHEME_INVALID,
        
        /// <summary>
        /// <para>Indicates that the service is singleton-partitioned. This means that there is only one partition, or the service is not partitioned.</para>
        /// </summary>
        Singleton = NativeTypes.FABRIC_PARTITION_SCHEME.FABRIC_PARTITION_SCHEME_SINGLETON,
        
        /// <summary>
        /// <para>Indicates that the service is uniform int64 range-partitioned. This means that each partition owns a range of int64 keys.</para>
        /// </summary>
        UniformInt64Range = NativeTypes.FABRIC_PARTITION_SCHEME.FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE,
        
        /// <summary>
        /// <para>Indicates that the service is named-partitioned. This means that each partition is associated with a string name.</para>
        /// </summary>
        Named = NativeTypes.FABRIC_PARTITION_SCHEME.FABRIC_PARTITION_SCHEME_NAMED
    }

    /// <summary>
    /// <para>Describes how the service is partitioned. This is the parent entity from which the actual partitioning scheme descriptions are derived.</para>
    /// </summary>
    [KnownType(typeof(SingletonPartitionSchemeDescription))]
    [KnownType(typeof(UniformInt64RangePartitionSchemeDescription))]
    [KnownType(typeof(NamedPartitionSchemeDescription))]
    public abstract class PartitionSchemeDescription
    {
        private PartitionSchemeDescription()
        {
        }

        /// <summary>
        /// <para>Instantiates a <see cref="System.Fabric.Description.PartitionSchemeDescription" /> class. </para>
        /// </summary>
        /// <param name="scheme">
        /// <para>
        ///     <see cref="System.Fabric.Description.PartitionScheme" /> defines the kind of partition scheme.</para>
        /// </param>
        protected PartitionSchemeDescription(PartitionScheme scheme)
        {
            this.Scheme = scheme;
        }

        /// <summary>
        /// <para>
        /// Instantiates a <see cref="System.Fabric.Description.PartitionSchemeDescription" /> class with parameters from another 
        /// <see cref="System.Fabric.Description.PartitionSchemeDescription" /> object.
        /// </para>
        /// </summary>
        /// <param name="other">
        /// <para>The partition scheme description from which parameters are copied.</para>
        /// </param>
        protected PartitionSchemeDescription(PartitionSchemeDescription other)
        {
            this.Scheme = other.Scheme;
        }

        /// <summary>
        /// <para>Specifies how the service is partitioned. A common use is that it enables programmers to cast the description into 
        /// <see cref="System.Fabric.Description.SingletonPartitionSchemeDescription" />, <see cref="System.Fabric.Description.NamedPartitionSchemeDescription" />, 
        /// or <see cref="System.Fabric.Description.UniformInt64RangePartitionSchemeDescription" />.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Description.PartitionScheme" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.PartitionScheme)]
        public PartitionScheme Scheme { get; private set; }

        internal PartitionSchemeDescription GetCopy()
        {
            if (this is SingletonPartitionSchemeDescription)
            {
                return new SingletonPartitionSchemeDescription(this as SingletonPartitionSchemeDescription);
            }

            if (this is UniformInt64RangePartitionSchemeDescription)
            {
                return new UniformInt64RangePartitionSchemeDescription(this as UniformInt64RangePartitionSchemeDescription);
            }

            if (this is NamedPartitionSchemeDescription)
            {
                return new NamedPartitionSchemeDescription(this as NamedPartitionSchemeDescription);
            }

            return null;
        }

        internal static unsafe PartitionSchemeDescription CreateFromNative(NativeTypes.FABRIC_PARTITION_SCHEME scheme, IntPtr ptr)
        {
            ReleaseAssert.AssertIfNot(scheme != NativeTypes.FABRIC_PARTITION_SCHEME.FABRIC_PARTITION_SCHEME_INVALID, StringResources.Error_PartitionSchemeNotSupported);

            switch (scheme)
            {
                case NativeTypes.FABRIC_PARTITION_SCHEME.FABRIC_PARTITION_SCHEME_SINGLETON:
                    return SingletonPartitionSchemeDescription.CreateFromNative(ptr);
                case NativeTypes.FABRIC_PARTITION_SCHEME.FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
                    return UniformInt64RangePartitionSchemeDescription.CreateFromNative(ptr);
                case NativeTypes.FABRIC_PARTITION_SCHEME.FABRIC_PARTITION_SCHEME_NAMED:
                    return NamedPartitionSchemeDescription.CreateFromNative(ptr);
                default:
                    AppTrace.TraceSource.WriteError("ServicePartitionDescription.CreateFromNative", "Unknown scheme: {0}", scheme);
                    ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_PartitionSchemeUnknown_Formatted, scheme));
                    break;
            }

            return null;
        }

        internal abstract IntPtr ToNative(PinCollection pin);

        // Method used by JsonSerializer to resolve derived type using json property "PartitionScheme".
        // The base class needs to have attributes [KnownType()]
        // This method must be static with one parameter input which represented by given json property.
        // Provide name of the json property which will be used as parameter to this method.
        [DerivedTypeResolverAttribute("PartitionScheme")]
        internal static Type ResolveDerivedClass(PartitionScheme scheme)
        {
            switch (scheme)
            {
                case PartitionScheme.Singleton:
                    return typeof(SingletonPartitionSchemeDescription);

                case PartitionScheme.UniformInt64Range:
                    return typeof(UniformInt64RangePartitionSchemeDescription);

                case PartitionScheme.Named:
                    return typeof(NamedPartitionSchemeDescription);

                default:
                    return null;
            }
        }
    }
}