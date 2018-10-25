// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    internal class NativeClaimsCredentialConverter : INativeCredentialConverter
    {
        private ClaimsCredentials claimsCredentials;

        public NativeClaimsCredentialConverter(ClaimsCredentials securityCredentials)
        {
            this.claimsCredentials = securityCredentials;
        }

        public IntPtr ToNative(PinCollection pin)
        {
            var nativeClaimsCredential = new NativeTypes.FABRIC_CLAIMS_CREDENTIALS[1];

            nativeClaimsCredential[0].ServerCommonNameCount = (uint) this.claimsCredentials.ServerCommonNames.Count;
            var serverCommonNameArray = new IntPtr[this.claimsCredentials.ServerCommonNames.Count];
            for (var i = 0; i < this.claimsCredentials.ServerCommonNames.Count; ++i)
            {
                serverCommonNameArray[i] = pin.AddBlittable(this.claimsCredentials.ServerCommonNames[i]);
            }
            nativeClaimsCredential[0].ServerCommonNames = pin.AddBlittable(serverCommonNameArray);

            nativeClaimsCredential[0].IssuerThumbprintCount = (uint) this.claimsCredentials.IssuerThumbprints.Count;
            var thumbprintsArray = new IntPtr[this.claimsCredentials.IssuerThumbprints.Count];
            for (var i = 0; i < this.claimsCredentials.IssuerThumbprints.Count; ++i)
            {
                thumbprintsArray[i] = pin.AddBlittable(this.claimsCredentials.IssuerThumbprints[i]);
            }
            nativeClaimsCredential[0].IssuerThumbprints = pin.AddBlittable(thumbprintsArray);

            nativeClaimsCredential[0].LocalClaims = pin.AddBlittable(this.claimsCredentials.LocalClaims);
            nativeClaimsCredential[0].ProtectionLevel = (NativeTypes.FABRIC_PROTECTION_LEVEL) this.claimsCredentials.ProtectionLevel;

            var ex1 = new NativeTypes.FABRIC_CLAIMS_CREDENTIALS_EX1[1];
            ex1[0].ServerThumbprintCount = (uint) this.claimsCredentials.ServerThumbprints.Count;
            var serverThumbprintsArray = new IntPtr[this.claimsCredentials.ServerThumbprints.Count];
            for (var i = 0; i < this.claimsCredentials.ServerThumbprints.Count; ++i)
            {
                serverThumbprintsArray[i] = pin.AddBlittable(this.claimsCredentials.ServerThumbprints[i]);
            }
            ex1[0].ServerThumbprints = pin.AddBlittable(serverThumbprintsArray);

            nativeClaimsCredential[0].Reserved = pin.AddBlittable(ex1);

            return pin.AddBlittable(nativeClaimsCredential);
        }
    }
}