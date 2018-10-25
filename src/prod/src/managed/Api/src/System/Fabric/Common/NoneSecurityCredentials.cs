// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    ///   <para>SecurityCredentials subtype for non-secure mode</para>
    /// </summary>
    public sealed class NoneSecurityCredentials : SecurityCredentials
    {
        /// <summary>
        ///   <para>
        /// Creates SecurityCredentials object with CredentialType.None.
        /// </para>
        /// </summary>
        public NoneSecurityCredentials()
            : base(CredentialType.None)
        {            
        }

        internal static NoneSecurityCredentials CreateFromNative()
        {
            return new NoneSecurityCredentials();
        }
    }
}