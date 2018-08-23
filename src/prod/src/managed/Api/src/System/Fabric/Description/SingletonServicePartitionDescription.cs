// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes the partition scheme of a singleton-partitioned, or non-partitioned service.</para>
    /// </summary>
    public sealed class SingletonPartitionSchemeDescription : PartitionSchemeDescription
    {
        /// <summary>
        /// <para>Instantiates a <see cref="System.Fabric.Description.SingletonPartitionSchemeDescription" /> class.</para>
        /// </summary>
        public SingletonPartitionSchemeDescription()
            : base(PartitionScheme.Singleton)
        {
        }

        internal SingletonPartitionSchemeDescription(SingletonPartitionSchemeDescription other)
            : base(other)
        {
        }

        internal static unsafe SingletonPartitionSchemeDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr == IntPtr.Zero, StringResources.Error_NullNativePointer);
            return new SingletonPartitionSchemeDescription();
        }

        internal unsafe override IntPtr ToNative(PinCollection pin)
        {
            return IntPtr.Zero;
        }
    }
}