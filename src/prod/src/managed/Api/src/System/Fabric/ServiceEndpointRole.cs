// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the kinds of possible roles of a service endpoint. </para>
    /// </summary>
    /// <remarks>
    ///   <para />
    /// </remarks>
    public enum ServiceEndpointRole
    {
        /// <summary>
        /// <para>Indicates that Service Fabric cannot classify the service endpoint role. </para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_SERVICE_ENDPOINT_ROLE.FABRIC_SERVICE_ROLE_INVALID,
        
        /// <summary>
        /// <para>Indicates that the service endpoint role is stateless. </para>
        /// </summary>
        /// <remarks>
        ///   <para />
        /// </remarks>
        Stateless = NativeTypes.FABRIC_SERVICE_ENDPOINT_ROLE.FABRIC_SERVICE_ROLE_STATELESS,
        
        /// <summary>
        /// <para>Indicates that the service endpoint role is a stateful Primary replica. </para>
        /// </summary>
        /// <remarks>
        ///   <para />
        /// </remarks>
        StatefulPrimary = NativeTypes.FABRIC_SERVICE_ENDPOINT_ROLE.FABRIC_SERVICE_ROLE_STATEFUL_PRIMARY,
        
        /// <summary>
        /// <para>Indicates that the service endpoint role is a stateful Secondary replica. </para>
        /// </summary>
        /// <remarks>
        ///   <para />
        /// </remarks>
        StatefulSecondary = NativeTypes.FABRIC_SERVICE_ENDPOINT_ROLE.FABRIC_SERVICE_ROLE_STATEFUL_SECONDARY
    }
}