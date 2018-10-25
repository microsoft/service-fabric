// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the kinds of service descriptions.</para>
    /// </summary>
    public enum ServiceDescriptionKind
    {
        /// <summary>
        /// <para>All Service Fabric enumerations have a reserved Invalid flag.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND.FABRIC_SERVICE_DESCRIPTION_KIND_INVALID,
        
        /// <summary>
        /// <para>Indicates a stateless service.</para>
        /// </summary>
        Stateless = NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND.FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS,
        
        /// <summary>
        /// <para>Indicates a stateful service.</para>
        /// </summary>
        Stateful = NativeTypes.FABRIC_SERVICE_DESCRIPTION_KIND.FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL
    }
}