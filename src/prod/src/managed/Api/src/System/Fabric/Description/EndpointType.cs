// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    /// <summary>
    /// <para>Enumerates the kinds of endpoint types.</para>
    /// </summary>
    public enum EndpointType
    {
        /// <summary>
        /// <para>Indicates an external facing endpoint.</para>
        /// </summary>
        Input,
        
        /// <summary>
        /// <para>Indicates an internal facing endpoint.</para>
        /// </summary>
        Internal
    }
}