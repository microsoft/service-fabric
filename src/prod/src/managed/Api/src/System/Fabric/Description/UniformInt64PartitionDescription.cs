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

    /// <summary>
    /// <para>Describes a partitioning scheme where an integer range is allocated evenly across a number of partitions.</para>
    /// </summary>
    public sealed class UniformInt64RangePartitionSchemeDescription : PartitionSchemeDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.UniformInt64RangePartitionSchemeDescription" /> class.</para>
        /// </summary>
        public UniformInt64RangePartitionSchemeDescription()
            : this(1)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.UniformInt64RangePartitionSchemeDescription" /> class by specifying the partition count.</para>
        /// </summary>
        /// <param name="partitionCount">
        /// <para>The number of partitions in the scheme.</para>
        /// </param>
        /// <remarks>The low key of the range defaults to long.MinValue and the high key defaults to long.MaxValue.</remarks>
        public UniformInt64RangePartitionSchemeDescription(int partitionCount)
            : this(partitionCount, long.MinValue, long.MaxValue)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.UniformInt64RangePartitionSchemeDescription" /> class 
        /// by specifying the partition count and the range values.</para>
        /// </summary>
        /// <param name="partitionCount">
        /// <para>The number of partitions in the scheme.</para>
        /// </param>
        /// <param name="lowKey">The low key of the range.</param>
        /// <param name="highKey">The high key of the range.</param>
        public UniformInt64RangePartitionSchemeDescription(int partitionCount, long lowKey, long highKey)
            : base(PartitionScheme.UniformInt64Range)
        {
            this.PartitionCount = partitionCount;
            this.LowKey = lowKey;
            this.HighKey = highKey;
        }

        internal UniformInt64RangePartitionSchemeDescription(UniformInt64RangePartitionSchemeDescription other)
            : base(other)
        {
            this.PartitionCount = other.PartitionCount;
            this.LowKey = other.LowKey;
            this.HighKey = other.HighKey;
        }

        /// <summary>
        /// <para>Gets or sets the partition count.</para>
        /// </summary>
        /// <value>
        /// <para>The partition count.</para>
        /// </value>
        /// <remarks>
        /// <para>Specifies the number of partitions into which this service is partitioned. Each partition receives approximately the same 
        /// number of keys. The number is determined by subtracting <languagekeyword>HighKey</languagekeyword> from <languagekeyword>LowKey</languagekeyword> and dividing 
        /// the sum by <languagekeyword>PartitionCount</languagekeyword>.</para>
        /// </remarks>
        [JsonCustomization(PropertyName = JsonPropertyNames.Count)]
        public int PartitionCount { get; set; }

        /// <summary>
        /// <para>Gets or sets the inclusive lower bound on the range of keys that is supported by the service.</para>
        /// </summary>
        /// <value>
        /// <para>The inclusive lower bound on the range of keys that is supported by the service.</para>
        /// </value>
        public long LowKey { get; set; }

        /// <summary>
        /// <para>Gets or sets the inclusive upper bound on the range of keys that is supported by the service.</para>
        /// </summary>
        /// <value>
        /// <para>The inclusive upper bound on the range of keys that is supported by the service.</para>
        /// </value>
        public long HighKey { get; set; }

        internal static unsafe UniformInt64RangePartitionSchemeDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, StringResources.Error_NullNativePointer);

            NativeTypes.FABRIC_UNIFORM_INT64_RANGE_PARTITION_DESCRIPTION* casted = (NativeTypes.FABRIC_UNIFORM_INT64_RANGE_PARTITION_DESCRIPTION*)nativePtr;

            return new UniformInt64RangePartitionSchemeDescription
            {
                HighKey = casted->HighKey,
                LowKey = casted->LowKey,
                PartitionCount = casted->PartitionCount,
            };
        }

        internal override unsafe IntPtr ToNative(PinCollection pin)
        {
            var boxRoot = new NativeTypes.FABRIC_UNIFORM_INT64_RANGE_PARTITION_DESCRIPTION[1];

            boxRoot[0].HighKey = this.HighKey;
            boxRoot[0].LowKey = this.LowKey;
            boxRoot[0].PartitionCount = this.PartitionCount;

            return pin.AddBlittable(boxRoot);
        }
    }
}