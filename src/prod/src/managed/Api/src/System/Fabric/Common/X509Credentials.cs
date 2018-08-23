// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Security.Cryptography.X509Certificates;

    /// <summary>
    ///   <para>A type to identify X509 certificate with subject common name or DNS name</para>
    /// </summary>
    public class X509Name
    {
        /// <summary>
        ///   <para>Gets the subject common name or DNS name of X509 certificate</para>
        /// </summary>
        /// <value>
        ///   <para>Subject common name or DNS name of X509 certificate</para>
        /// </value>
        public string Name { get; private set; }

        /// <summary>
        ///   <para>Gets the certificate thumbprint to identify issuer</para>
        /// </summary>
        /// <value>
        ///   <para>Certificate thumbprint to identify issuer</para>
        /// </value>
        public string IssuerCertThumbprint { get; private set; }

        /// <summary>
        ///   <para>Construct an X509Name object that identifies an X509 certificate</para>
        /// </summary>
        /// <param name="name">
        ///   <para>Subject common name or DNS name of X509 certificate</para>
        /// </param>
        /// <param name="issuerCertThumbprint">
        ///   <para>Certificate thumbprint to identify issuer</para>
        /// </param>
        public X509Name(string name, string issuerCertThumbprint)
        {
            this.Name = name ?? string.Empty;
            this.IssuerCertThumbprint = issuerCertThumbprint ?? string.Empty;
        }

        /// <summary>
        ///   <para>Determines whether the specified object is equal to the current object</para>
        /// </summary>
        /// <param name="obj">
        ///   <para>The object to compare with the current object</para>
        /// </param>
        /// <returns>
        ///   <para>Returns true if the objects are equal, false otherwise.</para>
        /// </returns>
        public override bool Equals(object obj)
        {
            return this.Equals(obj as X509Name);
        }

        /// <summary>
        ///   <para>Compute hash code</para>
        /// </summary>
        /// <returns>
        ///   <para>Returns <see cref="System.Int32" /> representing the hash code.</para>
        /// </returns>
        public override int GetHashCode()
        {
            return this.Name.ToLower().GetHashCode() ^ this.IssuerCertThumbprint.ToLower().GetHashCode();
        }

        /// <summary>
        ///   <para>Determines whether the specified object is equal to the current object</para>
        /// </summary>
        /// <param name="other">
        ///   <para>The object to compare with the current object</para>
        /// </param>
        /// <returns>
        ///   <para>Returns true if the objects are equal, false otherwise.</para>
        /// </returns>
        public bool Equals(X509Name other)
        {
            if (other == null) return false;

            if (Object.ReferenceEquals(this, other)) return true;

            if (this.GetType() != other.GetType()) return false;

            return
                (string.Compare(this.Name, other.Name, StringComparison.OrdinalIgnoreCase) == 0) &&
                (string.Compare(this.IssuerCertThumbprint, other.IssuerCertThumbprint, StringComparison.OrdinalIgnoreCase) == 0);
        }
    };

    /// <summary>
    ///   <para>A type to identify X509 issuer store with issuer subject common name and stores</para>
    /// </summary>
    public class X509IssuerStore
    {
        /// <summary>
        ///   <para>Gets the issuer common name of X509 issuer certificate</para>
        /// </summary>
        /// <value>
        ///   <para>Issuer common name of X509 issuer certificate</para>
        /// </value>
        public string Name { get; private set; }

        /// <summary>
        ///   <para>Gets the certificate stores for the issuer X509 certificate.</para>
        /// </summary>
        /// <value>
        ///   <para>Certificate stores for issuer X509 certificate</para>
        /// </value>
        public IList<string> IssuerStores { get; private set; }

        /// <summary>
        ///   <para>Construct an X509IssuerStore object that identifies an X509 issuer certificate store</para>
        /// </summary>
        /// <param name="issuerStores">
        ///   <para>Issuer store names where issuer certificate can be found. All certificates in the mentioned stores will be whitelisted.</para>
        /// </param>
        public X509IssuerStore(IList<string> issuerStores)
        {
            this.Name = string.Empty;
            this.IssuerStores = issuerStores;
        }

        /// <summary>
        ///   <para>Construct an X509IssuerStore object that identifies an X509 issuer certificate store</para>
        /// </summary>
        /// <param name="name">
        ///   <para>Issuer common name for X509 issuer certificate</para>
        /// </param>
        /// <param name="issuerStores">
        ///   <para>Issuer store names where issuer certificate can be found</para>
        /// </param>
        public X509IssuerStore(string name, IList<string> issuerStores)
        {
            this.Name = name;
            this.IssuerStores = issuerStores;
        }

        /// <summary>
        ///   <para>Determines whether the specified object is equal to the current object</para>
        /// </summary>
        /// <param name="obj">
        ///   <para>The object to compare with the current object</para>
        /// </param>
        /// <returns>
        ///   <para>Returns true if the objects are equal, false otherwise.</para>
        /// </returns>
        public override bool Equals(object obj)
        {
            var other = obj as X509IssuerStore;
            if (other == null)
            {
                return false;
            }

            if (Object.ReferenceEquals(this, other))
            {
                return true;
            }

            if (this.GetType() != other.GetType())
            {
                return false;
            }

            return
                string.Compare(this.Name, other.Name, StringComparison.OrdinalIgnoreCase) == 0 &&
                this.IssuerStores.Count == other.IssuerStores.Count &&
                this.IssuerStores.All(x => other.IssuerStores.Contains(x
#if !DotNetCoreClrLinux
                , StringComparer.CurrentCultureIgnoreCase
#endif
                ));
        }

        /// <summary>
        ///   <para>Compute hash code</para>
        /// </summary>
        /// <returns>
        ///   <para>Returns <see cref="System.Int32" /> representing the hash code.</para>
        /// </returns>
        public override int GetHashCode()
        {
            return this.Name.ToLower().GetHashCode() ^ this.IssuerStores.GetHashCode();
        }
    };

    /// <summary>
    ///   <para>Specifies the security credentials that are based upon X.509 certificates.</para>
    /// </summary>
    public sealed class X509Credentials : SecurityCredentials
    {
        private IList<string> remoteCommonNames;
        private IList<string> issuerThumbprints;
        private X509FindType findType;
        private object findValue;
        private StoreLocation storeLocation;
        private string storeName;
        private ProtectionLevel protectionLevel;
        private IList<X509IssuerStore> remoteCertIssuers;

        /// <summary>
        ///   <para>Creates a new instance of the <see cref="System.Fabric.X509Credentials" /> class.</para>
        /// </summary>
        public X509Credentials() : base(CredentialType.X509)
        {
            this.RemoteX509Names = new List<X509Name>();
            this.remoteCommonNames = new List<string>();
            this.issuerThumbprints = new List<string>();
            this.RemoteCertThumbprints = new List<string>();
            this.remoteCertIssuers = new List<X509IssuerStore>();
            this.findType = X509FindType.FindBySubjectName;
            this.findValue = null;
            this.FindValueSecondary = null;
#if DotNetCoreClrLinux
            this.storeLocation = StoreLocation.LocalMachine;
#else
            this.storeLocation = StoreLocation.CurrentUser;
#endif
            this.storeName = StoreNameDefault;
            this.protectionLevel = ProtectionLevel.EncryptAndSign;
        }

        /// <summary>
        ///   <para>Deprecated by RemoteCommonNames.</para>
        /// </summary>
        /// <value>
        ///   <para>The allowed common names that you want Service Fabric to validate against.</para>
        /// </value>
        [Obsolete("Deprecated by RemoteCommonNames")]
        public IList<string> AllowedCommonNames
        {
            get
            {
                return this.remoteCommonNames;
            }
            private set
            {
                this.remoteCommonNames = value;
            }
        }

        /// <summary>
        ///   <para>Gets the list of X509Name to validate remote X509Credentials</para>
        /// </summary>
        /// <value>
        ///   <para>the list of X509Name to validate remote X509Credentials</para>
        /// </value>
        public IList<X509Name> RemoteX509Names { get; private set; }

        /// <summary>
        ///   <para>Gets the list of X509Issuers to validate remote X509Credentials</para>
        /// </summary>
        /// <value>
        ///   <para>The list of X509Issuers to validate remote X509Credentials</para>
        /// </value>
        public IList<X509IssuerStore> RemoteCertIssuers
        {
            get
            {
                return this.remoteCertIssuers;
            }

            set
            {
                this.remoteCertIssuers = value;
            }
        }

        /// <summary>
        ///   <para>Indicates the expected common names of remote certificates that you want Service Fabric to validate against.</para>
        /// </summary>
        /// <value>
        ///   <para>The expected common names of remote certificates that you want Service Fabric to validate against.</para>
        /// </value>
        public IList<string> RemoteCommonNames
        {
            get
            {
                return this.remoteCommonNames;
            }
            private set
            {
                this.remoteCommonNames = value;
            }
        }

        /// <summary>
        ///   <para>When not empty, this dictates the certificate thumbprints of direct issuer of remote certificates.</para>
        /// </summary>
        /// <value>
        ///   <para>The certificate thumbprints of direct issuer of remote certificates.</para>
        /// </value>
        public IList<string> IssuerThumbprints
        {
            get
            {
                return this.issuerThumbprints;
            }
            private set
            {
                this.issuerThumbprints = value;
            }
        }

        /// <summary>
        ///   <para>Gets the list of remote certificate thumbprints, used to validate remote X509Credentials</para>
        /// </summary>
        /// <value>
        ///   <para>List of remote certificate thumbprints, used to validate remote X509Credentials</para>
        /// </value>
        public IList<string> RemoteCertThumbprints { get; private set; }

        /// <summary>
        ///   <para> Specifies how to find local certificate in certificate store.Supported values:FindByThumbprint: find certificate by certificate thumbprintFindBySubjectName: find certificate by subject distinguished name or common name, when subject distinguished name is provided in FindValue, subject name in the certificate must be encoded in ASN encoding due to a restriction in native Windows crypto API. There is no such restriction when common name is provided in FindValue.</para>
        /// </summary>
        /// <value>
        ///   <para>The type of security credentials to use to secure the cluster.</para>
        /// </value>
        public X509FindType FindType
        {
            get
            {
                return this.findType;
            }

            set
            {
                this.findType = value;
            }
        }

        /// <summary>
        ///   <para>Specifies the filter value used to search local certificate in certificate store. FindType specifies the type of filter value.</para>
        /// </summary>
        /// <value>
        ///   <para>The value of security credentials to use to secure the cluster.</para>
        /// </value>
        public object FindValue
        {
            get
            {
                return this.findValue;
            }

            set
            {
                this.findValue = value;
            }
        }

        /// <summary>
        ///   <para>Gets or sets the secondary find value for loading local certificate credential.</para>
        /// </summary>
        /// <value>
        ///   <para>The secondary find value for loading local certificate credential.</para>
        /// </value>
        public object FindValueSecondary { get; set; }

        /// <summary>
        ///   <para>Indicates the location of the certificate store.</para>
        /// </summary>
        /// <value>
        ///   <para>The location of the certificate store.</para>
        /// </value>
        public StoreLocation StoreLocation
        {
            get
            {
                return this.storeLocation;
            }

            set
            {
                this.storeLocation = value;
            }
        }

        /// <summary>
        ///   <para>Indicates the default name of the store where the certificate is stored.</para>
        /// </summary>
        /// <value>
        ///   <para>The default name of the store where the certificate is stored.</para>
        /// </value>
        static public string StoreNameDefault
        {
            get
            {
#if DotNetCoreClrLinux
                return string.Empty;
#else
                return new X509Store(System.Security.Cryptography.X509Certificates.StoreName.My, StoreLocation.CurrentUser).Name;
#endif
            }
        }

        /// <summary>
        ///   <para>Indicates the name of the store where the certificate is stored.</para>
        /// </summary>
        /// <value>
        ///   <para>The name of the store where the certificate is stored.</para>
        /// </value>
        public string StoreName
        {
            get
            {
                return this.storeName;
            }

            set
            {
                Requires.Argument<object>("StoreName", value).NotNull();
                this.storeName = value;
            }
        }

        /// <summary>
        ///   <para>Indicates the string value that is used to specify whether the messages in the header and body have integrity and confidentiality guarantees applied to them when they are sent between the nodes of a cluster.</para>
        /// </summary>
        /// <value>
        ///   <para>The string value that is used to specify whether the messages in the header and body have integrity and confidentiality guarantees applied to them when they are sent between the nodes of a cluster.</para>
        /// </value>
        public ProtectionLevel ProtectionLevel
        {
            get
            {
                return this.protectionLevel;
            }

            set
            {
                this.protectionLevel = value;
            }
        }

        internal static unsafe X509Credentials CreateFromNative(NativeTypes.FABRIC_X509_CREDENTIALS* nativeCredentials)
        {
            X509Credentials managedCredentials = new X509Credentials();
            
            managedCredentials.FindType = CreateFromNative(nativeCredentials->FindType);
            
            // The only supported find values are currently of type string
            managedCredentials.FindValue = NativeTypes.FromNativeString(nativeCredentials->FindValue);

            managedCredentials.ProtectionLevel = CreateFromNative(nativeCredentials->ProtectionLevel);

            managedCredentials.StoreLocation = CreateFromNative(nativeCredentials->StoreLocation);

            managedCredentials.StoreName = NativeTypes.FromNativeString(nativeCredentials->StoreName);
            Requires.Argument<object>("StoreName", managedCredentials.StoreName).NotNullOrEmpty();

            var remoteCommonNames = new ItemList<string>();
            for (int i = 0; i < nativeCredentials->RemoteCommonNameCount; i++)
            {
                IntPtr location = nativeCredentials->RemoteCommonNames + (i * IntPtr.Size);
                IntPtr value = *((IntPtr*)location);
                remoteCommonNames.Add(NativeTypes.FromNativeString(value));
            }
            managedCredentials.RemoteCommonNames = remoteCommonNames;

            if (nativeCredentials->Reserved == IntPtr.Zero)
                return managedCredentials;

            NativeTypes.FABRIC_X509_CREDENTIALS_EX1* x509Ex1 = (NativeTypes.FABRIC_X509_CREDENTIALS_EX1*)(nativeCredentials->Reserved);
 
            var issuerThumbprints = new ItemList<string>();
            for (int i = 0; i < x509Ex1->IssuerThumbprintCount; i++)
            {
                IntPtr location = x509Ex1->IssuerThumbprints + (i * IntPtr.Size);
                IntPtr value = *((IntPtr*)location);
                issuerThumbprints.Add(NativeTypes.FromNativeString(value));
            }
            managedCredentials.IssuerThumbprints = issuerThumbprints;

            if (x509Ex1->Reserved == IntPtr.Zero)
                return managedCredentials;

            NativeTypes.FABRIC_X509_CREDENTIALS_EX2* x509Ex2 = (NativeTypes.FABRIC_X509_CREDENTIALS_EX2*)(x509Ex1->Reserved);
            var remoteCertThumbprints = new ItemList<string>();
            for (int i = 0; i < x509Ex2->RemoteCertThumbprintCount; ++i)
            {
                IntPtr location = x509Ex2->RemoteCertThumbprints + (i*IntPtr.Size);
                IntPtr value = *((IntPtr*)location);
                remoteCertThumbprints.Add(NativeTypes.FromNativeString(value));
            }
            managedCredentials.RemoteCertThumbprints = remoteCertThumbprints;

            NativeTypes.FABRIC_X509_NAME* x509Names = (NativeTypes.FABRIC_X509_NAME*) (x509Ex2->RemoteX509Names);
            for (int i = 0; i < x509Ex2->RemoteX509NameCount; ++i)
            {
                managedCredentials.RemoteX509Names.Add(new X509Name(
                    NativeTypes.FromNativeString(x509Names[i].Name),
                    NativeTypes.FromNativeString(x509Names[i].IssuerCertThumbprint)));
            }

            managedCredentials.FindValueSecondary = NativeTypes.FromNativeString(x509Ex2->FindValueSecondary);

            if (x509Ex2->Reserved == IntPtr.Zero)
                return managedCredentials;

            NativeTypes.FABRIC_X509_CREDENTIALS_EX3* x509Ex3 = (NativeTypes.FABRIC_X509_CREDENTIALS_EX3*)(x509Ex2->Reserved);
            NativeTypes.FABRIC_X509_ISSUER_NAME* x509Issuers = (NativeTypes.FABRIC_X509_ISSUER_NAME*)(x509Ex3->RemoteCertIssuers);
            for (int i = 0; i < x509Ex3->RemoteCertIssuerCount; ++i)
            {
                var issuerStores = new ItemList<string>();
                for (int j = 0; j < x509Issuers[i].IssuerStoreCount; ++j)
                {
                    IntPtr location = x509Issuers[i].IssuerStores + (j * IntPtr.Size);
                    IntPtr value = *((IntPtr*)location);
                    issuerStores.Add(NativeTypes.FromNativeString(value));
                }

                managedCredentials.RemoteCertIssuers.Add(new X509IssuerStore(
                    NativeTypes.FromNativeString(x509Issuers[i].Name),
                    issuerStores));
            }

            return managedCredentials;
        }

        private static unsafe X509FindType CreateFromNative(NativeTypes.FABRIC_X509_FIND_TYPE nativeType)
        {
            switch(nativeType)
            {
                case NativeTypes.FABRIC_X509_FIND_TYPE.FABRIC_X509_FIND_TYPE_FINDBYTHUMBPRINT:
                    return X509FindType.FindByThumbprint;
                case NativeTypes.FABRIC_X509_FIND_TYPE.FABRIC_X509_FIND_TYPE_FINDBYSUBJECTNAME:
                    return X509FindType.FindBySubjectName;
                default:
                    throw new InvalidOperationException(StringResources.Error_InvalidX509FindType);
            }
        }

        private static unsafe StoreLocation CreateFromNative(NativeTypes.FABRIC_X509_STORE_LOCATION nativeStoreLocation)
        {
            switch(nativeStoreLocation)
            {
                case NativeTypes.FABRIC_X509_STORE_LOCATION.FABRIC_X509_STORE_LOCATION_CURRENTUSER:
                    return StoreLocation.CurrentUser;
                case NativeTypes.FABRIC_X509_STORE_LOCATION.FABRIC_X509_STORE_LOCATION_LOCALMACHINE:
                    return StoreLocation.LocalMachine;
                default:
                    throw new InvalidOperationException(StringResources.Error_InvalidX509StoreLocation);
            }
        }
    }
}