// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    /// <summary>
    /// <para>Enumerates the kinds of endpoint protocols.</para>
    /// </summary>
    public enum EndpointProtocol
    {
        /// <summary>
        /// <para>Indicates an HTTP endpoint.</para>
        /// </summary>
        Http,
        
        /// <summary>
        /// <para>Indicates an HTTPS endpoint.</para>
        /// </summary>
        Https,
        
        /// <summary>
        /// <para>Indicates a TCP endpoint.</para>
        /// </summary>
        Tcp,

        /// <summary>
        /// <para>Indicates a UDP endpoint.</para>
        /// </summary>
        Udp
    }
}