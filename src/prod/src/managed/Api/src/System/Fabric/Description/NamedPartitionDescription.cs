// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes the named partition scheme of the service.</para>
    /// </summary>
    public sealed class NamedPartitionSchemeDescription : PartitionSchemeDescription
    {
        /// <summary>
        /// <para>Instantiates a new instance of the <see cref="System.Fabric.Description.NamedPartitionSchemeDescription" /> class.</para>
        /// </summary>
        public NamedPartitionSchemeDescription()
            : base(PartitionScheme.Named)
        {
            this.PartitionNames = new ItemList<string>();
        }

        internal NamedPartitionSchemeDescription(IList<string> partitionNames)
            : base(PartitionScheme.Named)
        {
            this.PartitionNames = new ItemList<string>(partitionNames ?? new ItemList<string>());
        }

        internal NamedPartitionSchemeDescription(NamedPartitionSchemeDescription other)
            : base(other)
        {
            this.PartitionNames = new ItemList<string>(other.PartitionNames ?? new ItemList<string>());
        }

        /// <summary>
        /// <para>Gets the list of names that represent each partition.</para>
        /// </summary>
        /// <value>
        /// <para>The list of names that represent each partition.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Names)]
        public IList<string> PartitionNames
        {
            get;
            private set;
        }

        // Matching the properties with it's REST representation
        private int Count
        {
            get { return this.PartitionNames.Count; }
        }

        internal static unsafe NamedPartitionSchemeDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, StringResources.Error_NullNativePointer);

            var partitionNames = new ItemList<string>();
            // casted->Names is an array of IntPtr 
            // each of the elements is the pointer to the string
            NativeTypes.FABRIC_NAMED_PARTITION_DESCRIPTION* casted = (NativeTypes.FABRIC_NAMED_PARTITION_DESCRIPTION*)nativePtr;
            for (int i = 0; i < casted->PartitionCount; i++)
            {
                IntPtr nameLocation = casted->Names + (i * IntPtr.Size);
                IntPtr name = *((IntPtr*)nameLocation);
                partitionNames.Add(NativeTypes.FromNativeString(name));
            }

            return new NamedPartitionSchemeDescription(partitionNames);
        }

        internal override unsafe IntPtr ToNative(PinCollection pin)
        {
            var boxRoot = new NativeTypes.FABRIC_NAMED_PARTITION_DESCRIPTION[1];
            boxRoot[0].PartitionCount = this.PartitionNames.Count;

            var partitionNameArray = new IntPtr[this.PartitionNames.Count];
            for (int i = 0; i < this.PartitionNames.Count; i++)
            {
                partitionNameArray[i] = pin.AddBlittable(this.PartitionNames[i]);
            }

            boxRoot[0].Names = pin.AddBlittable(partitionNameArray);
            return pin.AddBlittable(boxRoot);
        }
    }
}