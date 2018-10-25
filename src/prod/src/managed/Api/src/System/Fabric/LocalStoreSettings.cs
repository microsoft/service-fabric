// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the option settings for a local store.</para>
    /// </summary>
    public abstract class LocalStoreSettings
    {
        /// <summary>
        /// <para>Initializes a new instance of the class with the specified store provider type.</para>
        /// </summary>
        /// <param name="storeKind">
        /// <para>A <see cref="System.Fabric.LocalStoreKind" /> object representing the store provider type.</para>
        /// </param>
        protected LocalStoreSettings(LocalStoreKind storeKind)
        {
            this.StoreKind = storeKind;
        }

        /// <summary>
        /// <para>Gets the store provider type.</para>
        /// </summary>
        /// <value>
        /// <para>The store provider type as a <see cref="System.Fabric.LocalStoreKind" /> object.</para>
        /// </value>
        public LocalStoreKind StoreKind { get; private set; }

        internal abstract IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_LOCAL_STORE_KIND kind);
    }
}