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
    ///   <para>Represents the active directory domain credential.</para>
    /// </summary>
    public sealed class WindowsCredentials : SecurityCredentials
    {
        private string remoteSpn;
        private IList<string> remoteIdentities;
        private ProtectionLevel protectionLevel;

        /// <summary>
        ///   <para>Initializes a new instance of the <see cref="System.Fabric.WindowsCredentials" /> class.</para>
        /// </summary>
        public WindowsCredentials() : base(CredentialType.Windows)
        {
            this.remoteSpn = string.Empty;
            this.remoteIdentities = new List<string>();
            this.protectionLevel = ProtectionLevel.EncryptAndSign;
        }

        /// <summary>
        ///   <para>Gets or sets the service principal name of remote listener, can be left empty if the remote listener runs as machine accounts.</para>
        /// </summary>
        /// <value>
        ///   <para>The service principal name of remote listener, can be left empty if the remote listener runs as machine accounts.</para>
        /// </value>
        public string RemoteSpn
        {
            get { return remoteSpn; }
            set
            {
                Requires.Argument<object>("RemoteSpn", value).NotNull();
                remoteSpn = value;
            }
        }

        /// <summary>
        ///   <para>Gets or sets the list of active directory domain identities of remote clients, each entry can be either account name or group name.</para>
        /// </summary>
        /// <value>
        ///   <para>The list of active directory domain identities of remote clients, each entry can be either account name or group name.</para>
        /// </value>
        public IList<string> RemoteIdentities
        {
            get { return this.remoteIdentities; }
            private set
            {
                this.remoteIdentities = value;
            }
        }

        /// <summary>
        ///   <para>Gets or sets how communication is protected, default value is <see cref="System.Fabric.ProtectionLevel.EncryptAndSign" />.</para>
        /// </summary>
        /// <value>
        ///   <para>The protection level of the credential.</para>
        /// </value>
        public ProtectionLevel ProtectionLevel
        {
            get { return this.protectionLevel; }
            set { this.protectionLevel = value; }
        }

        internal static unsafe WindowsCredentials CreateFromNative(NativeTypes.FABRIC_WINDOWS_CREDENTIALS* nativeCredentials)
        {
            WindowsCredentials managedCredentials = new WindowsCredentials();

            managedCredentials.ProtectionLevel = CreateFromNative(nativeCredentials->ProtectionLevel);

            managedCredentials.RemoteSpn = NativeTypes.FromNativeString(nativeCredentials->RemoteSpn);

            var remoteIdentities = new ItemList<string>();
            for (int i = 0; i < nativeCredentials->RemoteIdentityCount; i++)
            {
                IntPtr location = nativeCredentials->RemoteIdentities + (i * IntPtr.Size);
                IntPtr value = *((IntPtr*)location);
                remoteIdentities.Add(NativeTypes.FromNativeString(value));
            }
            managedCredentials.RemoteIdentities = remoteIdentities;

            return managedCredentials;
        }
    }
}