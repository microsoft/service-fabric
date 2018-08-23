// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Security.Cryptography.X509Certificates;

    internal class NativeX509CredentialConverter : INativeCredentialConverter
    {
        private X509Credentials x509Credentials;

        internal NativeX509CredentialConverter(X509Credentials x509Credentials)
        {
            this.x509Credentials = x509Credentials;
        }

        public IntPtr ToNative(PinCollection pin)
        {
            var nativeX509Credentials = new NativeTypes.FABRIC_X509_CREDENTIALS[1];
            nativeX509Credentials[0].Reserved = IntPtr.Zero;
            nativeX509Credentials[0].FindType = ToNativeX509FindType(this.x509Credentials.FindType);
            nativeX509Credentials[0].FindValue = pin.AddBlittable(this.x509Credentials.FindValue);
            nativeX509Credentials[0].StoreLocation = ToNativeStoreLocation(this.x509Credentials.StoreLocation);
            nativeX509Credentials[0].StoreName = pin.AddBlittable(this.x509Credentials.StoreName);
            nativeX509Credentials[0].ProtectionLevel = (NativeTypes.FABRIC_PROTECTION_LEVEL) this.x509Credentials.ProtectionLevel;
            nativeX509Credentials[0].RemoteCommonNameCount = (uint) this.x509Credentials.RemoteCommonNames.Count;

            if (nativeX509Credentials[0].RemoteCommonNameCount > 0)
            {
                var allowedCommonNamesArray = new IntPtr[this.x509Credentials.RemoteCommonNames.Count];
                for (var i = 0; i < this.x509Credentials.RemoteCommonNames.Count; i++)
                {
                    allowedCommonNamesArray[i] = pin.AddBlittable(this.x509Credentials.RemoteCommonNames[i]);
                }
                nativeX509Credentials[0].RemoteCommonNames = pin.AddBlittable(allowedCommonNamesArray);
            }

            if (this.x509Credentials.IssuerThumbprints.Count == 0 && this.x509Credentials.RemoteCertThumbprints.Count == 0 &&
                this.x509Credentials.RemoteX509Names.Count == 0 && this.x509Credentials.FindValueSecondary == null &&
                this.x509Credentials.RemoteCertIssuers.Count == 0)
                return pin.AddBlittable(nativeX509Credentials);

            var x509Ex1 = new NativeTypes.FABRIC_X509_CREDENTIALS_EX1[1];
            nativeX509Credentials[0].Reserved = pin.AddBlittable(x509Ex1);

            x509Ex1[0].IssuerThumbprintCount = (uint) this.x509Credentials.IssuerThumbprints.Count;
            var issuerThumbprintsArray = new IntPtr[this.x509Credentials.IssuerThumbprints.Count];
            for (var i = 0; i < this.x509Credentials.IssuerThumbprints.Count; ++i)
            {
                issuerThumbprintsArray[i] = pin.AddBlittable(this.x509Credentials.IssuerThumbprints[i]);
            }
            x509Ex1[0].IssuerThumbprints = pin.AddBlittable(issuerThumbprintsArray);

            if (this.x509Credentials.RemoteCertThumbprints.Count == 0 && this.x509Credentials.RemoteX509Names.Count == 0 &&
                this.x509Credentials.FindValueSecondary == null && this.x509Credentials.RemoteCertIssuers == null)
                return pin.AddBlittable(nativeX509Credentials);

            var x509Ex2 = new NativeTypes.FABRIC_X509_CREDENTIALS_EX2[1];
            x509Ex1[0].Reserved = pin.AddBlittable(x509Ex2);

            x509Ex2[0].RemoteCertThumbprintCount = (uint) this.x509Credentials.RemoteCertThumbprints.Count;
            var remoteCertThumbprints = new IntPtr[this.x509Credentials.RemoteCertThumbprints.Count];
            for (var i = 0; i < this.x509Credentials.RemoteCertThumbprints.Count; ++i)
            {
                remoteCertThumbprints[i] = pin.AddBlittable(this.x509Credentials.RemoteCertThumbprints[i]);
            }
            x509Ex2[0].RemoteCertThumbprints = pin.AddBlittable(remoteCertThumbprints);

            x509Ex2[0].RemoteX509NameCount = (uint) this.x509Credentials.RemoteX509Names.Count;
            var remoteX509Names = new NativeTypes.FABRIC_X509_NAME[this.x509Credentials.RemoteX509Names.Count];
            for (var i = 0; i < this.x509Credentials.RemoteX509Names.Count; ++i)
            {
                // Empty strings need to be converted to null, thus AddObject instead of AddBlittable
                remoteX509Names[i].Name = pin.AddObject(this.x509Credentials.RemoteX509Names[i].Name);
                remoteX509Names[i].IssuerCertThumbprint = pin.AddObject(this.x509Credentials.RemoteX509Names[i].IssuerCertThumbprint);
            }
            x509Ex2[0].RemoteX509Names = pin.AddBlittable(remoteX509Names);

            x509Ex2[0].FindValueSecondary = pin.AddBlittable(this.x509Credentials.FindValueSecondary);

            if (this.x509Credentials.RemoteCertIssuers == null)
            {
                return pin.AddBlittable(nativeX509Credentials);
            }

            var x509Ex3 = new NativeTypes.FABRIC_X509_CREDENTIALS_EX3[1];
            x509Ex2[0].Reserved = pin.AddBlittable(x509Ex3);

            x509Ex3[0].RemoteCertIssuerCount = (uint)this.x509Credentials.RemoteCertIssuers.Count;
            var remoteCertIssuers = new NativeTypes.FABRIC_X509_ISSUER_NAME[this.x509Credentials.RemoteCertIssuers.Count];
            for (var i = 0; i < this.x509Credentials.RemoteCertIssuers.Count; ++i)
            {
                var issuerStores = new IntPtr[this.x509Credentials.RemoteCertIssuers[i].IssuerStores.Count];
                for (var j = 0; j < this.x509Credentials.RemoteCertIssuers[i].IssuerStores.Count; ++j)
                {
                    issuerStores[j] = pin.AddBlittable(this.x509Credentials.RemoteCertIssuers[i].IssuerStores[j]);
                }
                remoteCertIssuers[i].Name = pin.AddBlittable(this.x509Credentials.RemoteCertIssuers[i].Name);
                remoteCertIssuers[i].IssuerStores = pin.AddBlittable(issuerStores);
                remoteCertIssuers[i].IssuerStoreCount = (uint)this.x509Credentials.RemoteCertIssuers[i].IssuerStores.Count;
            }
            x509Ex3[0].RemoteCertIssuers = pin.AddBlittable(remoteCertIssuers);

            return pin.AddBlittable(nativeX509Credentials);
        }


        private static NativeTypes.FABRIC_X509_STORE_LOCATION ToNativeStoreLocation(StoreLocation storeLocation)
        {
            switch (storeLocation)
            {
                case StoreLocation.CurrentUser:
                    return NativeTypes.FABRIC_X509_STORE_LOCATION.FABRIC_X509_STORE_LOCATION_CURRENTUSER;
                case StoreLocation.LocalMachine:
                    return NativeTypes.FABRIC_X509_STORE_LOCATION.FABRIC_X509_STORE_LOCATION_LOCALMACHINE;
                default:
                    throw new InvalidOperationException(StringResources.Error_InvalidX509StoreLocation);
            }
        }

        private static NativeTypes.FABRIC_X509_FIND_TYPE ToNativeX509FindType(X509FindType findType)
        {
            switch (findType)
            {
                case X509FindType.FindByThumbprint:
                    return NativeTypes.FABRIC_X509_FIND_TYPE.FABRIC_X509_FIND_TYPE_FINDBYTHUMBPRINT;
                case X509FindType.FindBySubjectName:
                    return NativeTypes.FABRIC_X509_FIND_TYPE.FABRIC_X509_FIND_TYPE_FINDBYSUBJECTNAME;
                default:
                    throw new InvalidOperationException(StringResources.Error_InvalidX509FindType);
            }
        }
    }
}