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
    /// <para>Represents an item in the key/value store.</para>
    /// </summary>
    public sealed class KeyValueStoreItem
    {
        private KeyValueStoreItem()
        {
        }

        /// <summary>
        /// <para>Gets a <see cref="System.Fabric.KeyValueStoreItemMetadata" /> object that represents the metadata for an item in the key/value store. </para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.Fabric.KeyValueStoreItemMetadata" /> object that represents the metadata for an item in the key/value store.</para>
        /// </value>
        public KeyValueStoreItemMetadata Metadata { get; private set; }

        /// <summary>
        /// <para>Gets a <see cref="System.Byte" /> that represents the value of an item in the key/value store.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.Byte" /> that represents the value of an item in the key/value store.</para>
        /// </value>
        public byte[] Value { get; private set; }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static KeyValueStoreItem CreateFromNative(NativeRuntime.IFabricKeyValueStoreItemResult nativeItemResult)
        {
            if (nativeItemResult == null) { return null; }

            var item = CreateFromNative(nativeItemResult.get_Item());
            GC.KeepAlive(nativeItemResult);

            return item;
        }

        private static unsafe KeyValueStoreItem CreateFromNative(IntPtr nativeItem)
        {
            var item = (NativeTypes.FABRIC_KEY_VALUE_STORE_ITEM*)nativeItem;
            var metadata = KeyValueStoreItemMetadata.CreateFromNative(item->Metadata);

            var returnValue = new KeyValueStoreItem()
            {
                Metadata = metadata,
                Value = NativeTypes.FromNativeBytes(item->Value, (uint)metadata.ValueSizeInBytes)
            };

            return returnValue;
        }
    }
}