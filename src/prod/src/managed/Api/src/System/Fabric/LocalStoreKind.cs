// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the local store provider type.</para>
    /// </summary>
    public enum LocalStoreKind
    {
        /// <summary>
        /// <para>Reserved for future use. Does not indicate a valid database provider.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_LOCAL_STORE_KIND.FABRIC_LOCAL_STORE_KIND_INVALID,
        /// <summary>
        /// <para>Indicates an Extensible Store Engine (ESE) database provider. Please see http://msdn.microsoft.com/library/gg269245(v=exchg.10).aspx for documentation on ESE.</para>
        /// </summary>
        Ese = NativeTypes.FABRIC_LOCAL_STORE_KIND.FABRIC_LOCAL_STORE_KIND_ESE
    }
}