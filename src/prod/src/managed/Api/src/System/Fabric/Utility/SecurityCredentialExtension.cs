// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal static class SecurityCredentialExtension
    {
        internal static IntPtr ToNative(this SecurityCredentials securityCredentials, PinCollection pin)
        {
            var nativeCredentials = new NativeTypes.FABRIC_SECURITY_CREDENTIALS[1];

            nativeCredentials[0].Kind = NativeTypes.FABRIC_SECURITY_CREDENTIAL_KIND.FABRIC_SECURITY_CREDENTIAL_KIND_NONE;
            nativeCredentials[0].Value = IntPtr.Zero;

            switch (securityCredentials.CredentialType)
            {
                case CredentialType.X509:
                {
                    var converter = new NativeX509CredentialConverter((X509Credentials) securityCredentials);
                    nativeCredentials[0].Kind = NativeTypes.FABRIC_SECURITY_CREDENTIAL_KIND.FABRIC_SECURITY_CREDENTIAL_KIND_X509;
                    nativeCredentials[0].Value = converter.ToNative(pin);
                    break;
                }
                case CredentialType.Windows:
                {
                    var converter = new NativeWindowsCredentialConverter((WindowsCredentials) securityCredentials);
                    nativeCredentials[0].Kind = NativeTypes.FABRIC_SECURITY_CREDENTIAL_KIND.FABRIC_SECURITY_CREDENTIAL_KIND_WINDOWS;
                    nativeCredentials[0].Value = converter.ToNative(pin);
                    break;
                }
                case CredentialType.Claims:
                {
                    var converter = new NativeClaimsCredentialConverter((ClaimsCredentials) securityCredentials);
                    nativeCredentials[0].Kind = NativeTypes.FABRIC_SECURITY_CREDENTIAL_KIND.FABRIC_SECURITY_CREDENTIAL_KIND_CLAIMS;
                    nativeCredentials[0].Value = converter.ToNative(pin);
                    break;
                }
                case CredentialType.None:
                {
                    INativeCredentialConverter converter = new NativeNoneCredentialConverter();
                    nativeCredentials[0].Kind = NativeTypes.FABRIC_SECURITY_CREDENTIAL_KIND.FABRIC_SECURITY_CREDENTIAL_KIND_NONE;
                    nativeCredentials[0].Value = converter.ToNative(pin);
                    break;
                }
                default:
                    AppTrace.TraceSource.WriteError("SecurityCredentials.ToNative", "Unknown credential type: {0}", securityCredentials.CredentialType);
                    ReleaseAssert.Failfast("Unknown credential type: {0}", securityCredentials.CredentialType);
                    break;
            }

            return pin.AddBlittable(nativeCredentials);
        }
    }

    internal interface INativeCredentialConverter
    {
        IntPtr ToNative(PinCollection pin);
    }
}