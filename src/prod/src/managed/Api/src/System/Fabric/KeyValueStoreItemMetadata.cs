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
    /// <para>Represents the metadata that are associated with a <see cref="System.Fabric.KeyValueStoreItem" /> object in the Key/Value store.</para>
    /// </summary>
    public sealed class KeyValueStoreItemMetadata
    {
        private KeyValueStoreItemMetadata()
        {
        }

        /// <summary>
        /// <para>Gets the key of the associated <see cref="System.Fabric.KeyValueStoreItem" /> object.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.String" /> that represents the key of the <see cref="System.Fabric.KeyValueStoreItem" /> object.</para>
        /// </value>
        public string Key { get; private set; }

        /// <summary>
        /// <para>Gets the size in bytes of the sequence number of the <see cref="System.Fabric.KeyValueStoreItem" /> object.</para>
        /// </summary>
        /// <value>
        /// <para>The size in bytes of the <see cref="System.Fabric.KeyValueStoreItem" /> object, as a <see cref="System.Int32" />.</para>
        /// </value>
        public int ValueSizeInBytes { get; private set; }

        /// <summary>
        /// <para>Gets the sequence number of the <see cref="System.Fabric.KeyValueStoreItem" /> object.</para>
        /// </summary>
        /// <value>
        /// <para>The sequence number of the <see cref="System.Fabric.KeyValueStoreItem" /> object, as a <see cref="System.Int64" />.</para>
        /// </value>
        public long SequenceNumber { get; private set; }

        /// <summary>
        /// <para>Gets the last modified time (UTC) of the <see cref="System.Fabric.KeyValueStoreItem" /> object.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.DateTime" /> that represents the last modified time of the <see cref="System.Fabric.KeyValueStoreItem" /> object.</para>
        /// </value>
        public DateTime LastModifiedUtc { get; private set; }

        /// <summary>
        /// <para>
        /// Gets the last modified time (UTC) of the <see cref="System.Fabric.KeyValueStoreItem" /> object
        /// by the primary replica of the service.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>
        /// A <see cref="System.DateTime" /> that represents the last modified time of the 
        /// <see cref="System.Fabric.KeyValueStoreItem" /> object on primary replica of the service.
        /// </para>
        /// </value>
        public DateTime LastModifiedOnPrimaryUtc { get; private set; }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static KeyValueStoreItemMetadata CreateFromNative(
           NativeRuntime.IFabricKeyValueStoreItemMetadataResult itemMetadataResult)
        {
            if (itemMetadataResult == null) { return null; }

            var itemMetadata = CreateFromNative(itemMetadataResult.get_Metadata());
            GC.KeepAlive(itemMetadataResult);

            return itemMetadata;
        }

        internal static unsafe KeyValueStoreItemMetadata CreateFromNative(IntPtr nativeMetadata)
        {
            var metadata = (NativeTypes.FABRIC_KEY_VALUE_STORE_ITEM_METADATA*)nativeMetadata;

            var returnValue = new KeyValueStoreItemMetadata()
            {
                Key = NativeTypes.FromNativeString(metadata->Key),
                ValueSizeInBytes = metadata->ValueSizeInBytes,
                LastModifiedUtc = NativeTypes.FromNativeFILETIME(metadata->LastModifiedUtc),
                SequenceNumber = metadata->SequenceNumber
            };

            if (metadata->Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_KEY_VALUE_STORE_ITEM_METADATA_EX1*) metadata->Reserved;
                returnValue.LastModifiedOnPrimaryUtc = NativeTypes.FromNativeFILETIME(ex1->LastModifiedOnPrimaryUtc);
            }

            return returnValue;
        }
    }
}