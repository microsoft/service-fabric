// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Contains all the information for a replicated operation received by a secondary replica.</para>
    /// </summary>
    /// <remarks>
    /// <para>See <see cref="System.Fabric.KeyValueStoreReplica.OnReplicationOperation(Collections.Generic.IEnumerator{KeyValueStoreNotification})" />.</para>
    /// </remarks>
    public sealed class KeyValueStoreNotification
    {
        private KeyValueStoreItem kvsItem;

        private KeyValueStoreNotification()
        {
        }

        /// <summary>
        /// <para>Gets or sets the metadata describing this replicated operation.</para>
        /// </summary>
        /// <value>
        /// <para>The metadata describing this replicated operation.</para>
        /// </value>
        public KeyValueStoreItemMetadata Metadata { get { return kvsItem.Metadata; } }

        /// <summary>
        /// <para>Gets or sets the data (if any) for this replicated operation. Null if this is a delete operation.</para>
        /// </summary>
        /// <value>
        /// <para>The data (if any) for this replicated operation. Null if this is a delete operation.</para>
        /// </value>
        public byte[] Value { get { return kvsItem.Value; } }

        /// <summary>
        /// <para>Indicates that this is a replicated delete operation.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if this is a replicated delete operation; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsDelete { get; private set; }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static KeyValueStoreNotification CreateFromNative(NativeRuntime.IFabricKeyValueStoreNotification nativeNotification)
        {
            var returnValue = new KeyValueStoreNotification()
            {
                IsDelete = NativeTypes.FromBOOLEAN(nativeNotification.IsDelete())
            };

            returnValue.kvsItem = KeyValueStoreItem.CreateFromNative(nativeNotification);

            GC.KeepAlive(nativeNotification);

            return returnValue;
        }
    }
}
