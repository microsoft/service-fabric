// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    ///   <para>Represents the claim based security credential acquired from STS (security token service).</para>
    /// </summary>
    public sealed class ClaimsCredentials : SecurityCredentials
    {
        private IList<string> serverCommonNames;
        private IList<string> serverThumbprints;
        private IList<string> issuerThumbprints;
        private string localClaims;
        private ProtectionLevel protectionLevel;

        /// <summary>
        ///   <para>Initializes a new instance of the <see cref="System.Fabric.ClaimsCredentials" /> class.</para>
        /// </summary>
        public ClaimsCredentials() : base(CredentialType.Claims)
        {
            this.serverCommonNames = new List<string>();
            this.serverThumbprints = new List<string>();
            this.issuerThumbprints = new List<string>();
            this.localClaims = string.Empty;
            this.protectionLevel = ProtectionLevel.EncryptAndSign;
        }

        /// <summary>
        ///   <para>Gets the string representation of claims token acquired from STS (security token service).</para>
        /// </summary>
        /// <value>
        ///   <para>The string representation of claims token acquired from STS (security token service).</para>
        /// </value>
        public string LocalClaims
        {
            get { return this.localClaims; }
            set
            {
                Requires.Argument<object>("localClaims", value).NotNull();
                this.localClaims = value;
            }
        }

        /// <summary>
        ///   <para>Gets the expected common names of server certificate.</para>
        /// </summary>
        /// <value>
        ///   <para>The expected common names of server certificate.</para>
        /// </value>
        public IList<string> ServerCommonNames
        {
            get { return this.serverCommonNames; }
            private set
            {
                this.serverCommonNames = value;
            }
        }

        /// <summary>
        /// FOR INTERNAL USE ONLY.
        /// </summary>
        /// <value>FOR INTERNAL USE ONLY.</value>
        public IList<string> ServerThumbprints
        {
            get { return this.serverThumbprints; }
            private set
            {
                this.serverThumbprints = value;
            }
        }

        /// <summary>
        ///   <para>Gets the certificate thumbprints of server certificate issuer.</para>
        /// </summary>
        /// <value>
        ///   <para>The certificate thumbprints of server certificate issuer.</para>
        /// </value>
        public IList<string> IssuerThumbprints
        {
            get { return this.issuerThumbprints; }
            private set
            {
                this.issuerThumbprints = value;
            }
        }

        /// <summary>
        ///   <para>Gets the protection level of communication with server; the default value is <see cref="System.Fabric.ProtectionLevel.EncryptAndSign" />.</para>
        /// </summary>
        /// <value>
        ///   <para>The protection level of communication with server.</para>
        /// </value>
        public ProtectionLevel ProtectionLevel
        {
            get { return this.protectionLevel; }
            set { this.protectionLevel = value; }
        }

      
        internal static unsafe ClaimsCredentials CreateFromNative(NativeTypes.FABRIC_CLAIMS_CREDENTIALS* nativeCredentials)
        {
            ClaimsCredentials managedCredentials = new ClaimsCredentials();

            managedCredentials.LocalClaims = NativeTypes.FromNativeString(nativeCredentials->LocalClaims);

            managedCredentials.ProtectionLevel = CreateFromNative(nativeCredentials->ProtectionLevel);

            var serverCommonNames = new ItemList<string>();
            for (int i = 0; i < nativeCredentials->ServerCommonNameCount; i++)
            {
                IntPtr location = nativeCredentials->ServerCommonNames + (i * IntPtr.Size);
                IntPtr value = *((IntPtr*)location);
                serverCommonNames.Add(NativeTypes.FromNativeString(value));
            }
            managedCredentials.ServerCommonNames = serverCommonNames;

            var issuerThumbprints = new ItemList<string>();
            for (int i = 0; i < nativeCredentials->IssuerThumbprintCount; i++)
            {
                IntPtr location = nativeCredentials->IssuerThumbprints + (i * IntPtr.Size);
                IntPtr value = *((IntPtr*)location);
                issuerThumbprints.Add(NativeTypes.FromNativeString(value));
            }
            managedCredentials.IssuerThumbprints = issuerThumbprints;

            if (nativeCredentials->Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_CLAIMS_CREDENTIALS_EX1*)nativeCredentials->Reserved;

                var serverThumbprints = new ItemList<string>();
                for (int i = 0; i < ex1->ServerThumbprintCount; i++)
                {
                    IntPtr location = ex1->ServerThumbprints + (i * IntPtr.Size);
                    IntPtr value = *((IntPtr*)location);
                    serverThumbprints.Add(NativeTypes.FromNativeString(value));
                }
                managedCredentials.ServerThumbprints = serverThumbprints;
            }

            return managedCredentials;
        }
    }
}