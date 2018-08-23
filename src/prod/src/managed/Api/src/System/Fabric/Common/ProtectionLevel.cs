// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    ///   <para>Enumerates how communication is protected.</para>
    /// </summary>
    public enum ProtectionLevel
    {
        /// <summary>
        ///   <para>Not protected.</para>
        /// </summary>
        None            = NativeTypes.FABRIC_PROTECTION_LEVEL.FABRIC_PROTECTION_LEVEL_NONE,     
         
        /// <summary>
        ///   <para>Only integrity is protected.</para>
        /// </summary>
        Sign            = NativeTypes.FABRIC_PROTECTION_LEVEL.FABRIC_PROTECTION_LEVEL_SIGN,

        /// <summary>
        ///   <para>Both confidentiality and integrity are protected.</para>
        /// </summary>
        EncryptAndSign  = NativeTypes.FABRIC_PROTECTION_LEVEL.FABRIC_PROTECTION_LEVEL_ENCRYPTANDSIGN
    }
}