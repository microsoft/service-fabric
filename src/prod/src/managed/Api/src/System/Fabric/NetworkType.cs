// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Indicates the type of container network. </para>
    /// </summary>
    public enum NetworkType
    {
        /// <summary>
        /// <para>Indicates the network type is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_NETWORK_TYPE.FABRIC_NETWORK_TYPE_INVALID,

        /// <summary>
        /// <para>Indicates a local container network.</para>
        /// </summary>
        Local = NativeTypes.FABRIC_NETWORK_TYPE.FABRIC_NETWORK_TYPE_LOCAL,

        /// <summary>
        /// <para>Indicates a federated container network.</para>
        /// </summary>
        Federated = NativeTypes.FABRIC_NETWORK_TYPE.FABRIC_NETWORK_TYPE_FEDERATED,
    }
}
