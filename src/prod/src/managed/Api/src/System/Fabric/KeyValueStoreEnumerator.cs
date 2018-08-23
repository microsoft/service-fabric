// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Reads the local store contents of a secondary replica within the context of a copy completion callback.</para>
    /// <seealso cref="System.Fabric.KeyValueStoreReplica.OnCopyComplete(KeyValueStoreEnumerator)" />.
    /// </summary>
    public sealed class KeyValueStoreEnumerator
    {
        private NativeRuntime.IFabricKeyValueStoreEnumerator2 nativeEnumerator;

        internal KeyValueStoreEnumerator(NativeRuntime.IFabricKeyValueStoreEnumerator nativeEnumerator)
        {
            Utility.WrapNativeSyncInvokeInMTA(() => 
            { 
                this.nativeEnumerator = (NativeRuntime.IFabricKeyValueStoreEnumerator2)nativeEnumerator; 
            },
            "KeyValueStoreEnumerator.ctor");
        }

        /// <summary>
        /// <para>Enumerates the local store contents and includes the data value for all enumerated key-value pairs.</para>
        /// </summary>
        /// <param name="keyPrefix">
        /// <para>Specifies an optional key-prefix filter to apply during enumeration.</para>
        /// </param>
        /// <returns>
        /// <para>An enumerator over the local store contents.</para>
        /// </returns>
        public IEnumerator<KeyValueStoreItem> Enumerate(string keyPrefix)
        {
            return this.Enumerate(keyPrefix, true); // strictPrefix
        }

        /// <summary>
        /// <para>Enumerates the local store contents and includes the data value for all enumerated key-value pairs.</para>
        /// </summary>
        /// <param name="keyPrefix">
        /// <para>Specifies an optional key-prefix filter to apply during enumeration.</para>
        /// </param>
        /// <param name="strictPrefix">
        /// <para>When true, only keys prefixed by the value specified for <b>keyPrefix</b> are returned. Otherwise, enumeration starts at the first key matching or lexicographically greater than <b>keyPrefix</b> and continues until there are no more keys. The default is <b>true</b>.</para>
        /// </param>
        /// <returns>
        /// <para>An enumerator over the local store contents.</para>
        /// </returns>
        public IEnumerator<KeyValueStoreItem> Enumerate(string keyPrefix, bool strictPrefix)
        {
            return Utility.WrapNativeSyncInvoke(
                () => this.EnumerateHelper(keyPrefix, strictPrefix),
                "KeyValueStoreEnumerator.Enumerate");
        }

        /// <summary>
        /// <para>Enumerates the local store contents but excludes the data value for all enumerated key-value pairs.</para>
        /// </summary>
        /// <param name="keyPrefix">
        /// <para>Specifies an optional prefix filter to apply during enumeration.</para>
        /// </param>
        /// <returns>
        /// <para>An enumerator over the local store contents.</para>
        /// </returns>
        public IEnumerator<KeyValueStoreItemMetadata> EnumerateMetadata(string keyPrefix)
        {
            return this.EnumerateMetadata(keyPrefix, true); // strictPrefix
        }

        /// <summary>
        /// <para>Enumerates the local store contents but excludes the data value for all enumerated key-value pairs.</para>
        /// </summary>
        /// <param name="keyPrefix">
        /// <para>Specifies an optional key-prefix filter to apply during enumeration.</para>
        /// </param>
        /// <param name="strictPrefix">
        /// <para>When true, only keys prefixed by the value specified for <b>keyPrefix</b> are returned. Otherwise, enumeration starts at the first key matching or lexicographically greater than <b>keyPrefix</b> and continues until there are no more keys. The default is <b>true</b>.</para>
        /// </param>
        /// <returns>
        /// <para>An enumerator over the local store contents.</para>
        /// </returns>
        public IEnumerator<KeyValueStoreItemMetadata> EnumerateMetadata(string keyPrefix, bool strictPrefix)
        {
            return Utility.WrapNativeSyncInvoke(
                () => this.EnumerateMetadataHelper(keyPrefix, strictPrefix),
                "KeyValueStoreEnumerator.EnumerateMetadata");
        }

        private IEnumerator<KeyValueStoreItem> EnumerateHelper(string keyPrefix, bool strictPrefix)
        {
            return new KeyValueStoreItemEnumerator(
                null, // tx
                (tx) => this.CreateNativeEnumeratorByKey(keyPrefix, strictPrefix));
        }

        private NativeRuntime.IFabricKeyValueStoreItemEnumerator CreateNativeEnumeratorByKey(string keyPrefix, bool strictPrefix)
        {
            using (var pin = new PinCollection())
            {
                return this.nativeEnumerator.EnumerateByKey2(
                    (keyPrefix == null ? IntPtr.Zero : pin.AddBlittable(keyPrefix)),
                    NativeTypes.ToBOOLEAN(strictPrefix));
            }
        }

        private IEnumerator<KeyValueStoreItemMetadata> EnumerateMetadataHelper(string keyPrefix, bool strictPrefix)
        {
            return new KeyValueStoreItemMetadataEnumerator(
                null, // tx
                (tx) => this.CreateNativeMetadataEnumeratorByKey(keyPrefix, strictPrefix));
        }

        private NativeRuntime.IFabricKeyValueStoreItemMetadataEnumerator CreateNativeMetadataEnumeratorByKey(string keyPrefix, bool strictPrefix)
        {
            using (var pin = new PinCollection())
            {
                return this.nativeEnumerator.EnumerateMetadataByKey2(
                    (keyPrefix == null ? IntPtr.Zero : pin.AddBlittable(keyPrefix)),
                    NativeTypes.ToBOOLEAN(strictPrefix));
            }
        }
    }
}