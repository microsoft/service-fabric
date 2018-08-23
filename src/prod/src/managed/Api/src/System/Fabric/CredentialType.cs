// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>Defines the valid kinds of security credentials.</para>
    /// </summary>
    public enum CredentialType
    {
        /// <summary>
        /// <para>No credential defined (default).</para>
        /// </summary>
        None = 0,
        
        /// <summary>
        /// <para>The X509 certificate.</para>
        /// </summary>
        X509,
        
        /// <summary>
        /// <para>The active directory domain credential.</para>
        /// </summary>
        Windows,
        
        /// <summary>
        /// <para>The claims token acquired from STS (security token service).</para>
        /// </summary>
        Claims
    }
}