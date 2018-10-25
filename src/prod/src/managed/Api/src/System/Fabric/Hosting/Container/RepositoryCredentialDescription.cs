// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class RepositoryCredentialDescription
    {
        internal RepositoryCredentialDescription()
        {
        }

        internal string AccountName { get; set; }

        internal string Password { get; set; }

        internal string Email { get; set; }

        internal bool IsPasswordEncrypted { get; set; }

        internal static unsafe RepositoryCredentialDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(
                nativePtr != IntPtr.Zero, 
                "RepositoryCredentialDescription.CreateFromNative() has null pointer.");

            var nativeCredential = *((NativeTypes.FABRIC_REPOSITORY_CREDENTIAL_DESCRIPTION*)nativePtr);

            return new RepositoryCredentialDescription
            {
                AccountName = NativeTypes.FromNativeString(nativeCredential.AccountName),
                Password = NativeTypes.FromNativeString(nativeCredential.Password),
                Email = NativeTypes.FromNativeString(nativeCredential.Email),
                IsPasswordEncrypted = NativeTypes.FromBOOLEAN(nativeCredential.IsPasswordEncrypted)
            };
        }
    }
}