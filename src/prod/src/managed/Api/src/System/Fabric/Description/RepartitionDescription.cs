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
    /// <para>Describes partitioning changes for an existing service.</para>
    /// </summary>
    [KnownType(typeof(NamedRepartitionDescription))]
    public abstract class RepartitionDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of this class.</para>
        /// </summary>
        /// <param name="kind">
        /// <para>
        /// The kind specifies the derived type of this instance.
        /// </para>
        /// </param>
        protected RepartitionDescription(PartitionScheme kind)
        {
            this.PartitionKind = kind;
        }

        /// <summary>
        /// <para>Gets a value indicating the derived type of this instance.</para>
        /// </summary>
        /// <value>
        /// <para>The derived type of this instance.</para>
        /// </value>
        public PartitionScheme PartitionKind { get; private set; }

        abstract internal unsafe IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_SERVICE_PARTITION_KIND kind);
    }
}