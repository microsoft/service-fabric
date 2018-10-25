// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// Identifies the underlying state provider type (implementation detail) of a key/value store.
    /// </summary>
    public enum KeyValueStoreProviderKind
    {
        /// <summary>
        /// The provider type is unknown
        /// </summary>
        Unknown = NativeTypes.FABRIC_KEY_VALUE_STORE_PROVIDER_KIND.FABRIC_KEY_VALUE_STORE_PROVIDER_KIND_UNKNOWN,
        
        /// <summary>
        /// The provider type is ESE
        /// </summary>
        ESE = NativeTypes.FABRIC_KEY_VALUE_STORE_PROVIDER_KIND.FABRIC_KEY_VALUE_STORE_PROVIDER_KIND_ESE,
        
        /// <summary>
        /// The provider type is TStore
        /// </summary>
        TStore = NativeTypes.FABRIC_KEY_VALUE_STORE_PROVIDER_KIND.FABRIC_KEY_VALUE_STORE_PROVIDER_KIND_TSTORE,
    }
}
