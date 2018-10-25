// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a policy for the package sharing.</para>
    /// </summary>
    public class PackageSharingPolicy
    {
        /// <summary>
        /// <para>
        /// Creates PackageSharingPolicy object.
        /// </para>
        /// </summary>
        /// <param name="packageName">
        /// <para>Name of specific package that should be shared. </para>
        /// </param>
        /// <param name="sharingScope">
        /// <para>PackageSharingPolicyScope parameter to indicate whether Code, Config, Data or All packages should be shared. </para>
        /// </param>
        public PackageSharingPolicy(string packageName, PackageSharingPolicyScope sharingScope)
        {
            this.PackageName = packageName;
            this.SharingScope = sharingScope;
        }

        /// <summary>
        /// <para>Gets the name of code, configuration or data package that should be shared.</para>
        /// </summary>
        /// <value>
        /// <para>The name of code, configuration or data package that should be shared.</para>
        /// </value>
        public string PackageName
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the scope for package sharing policy.</para>
        /// </summary>
        /// <value>
        /// <para>The scope for package sharing policy.</para>
        /// </value>
        public PackageSharingPolicyScope SharingScope
        {
            get;
            private set;
        }

        internal void ToNative(PinCollection pinCollection, out NativeTypes.FABRIC_PACKAGE_SHARING_POLICY nativeSharingPolicy)
        {
            nativeSharingPolicy.Scope = (NativeTypes.FABRIC_PACKAGE_SHARING_POLICY_SCOPE)this.SharingScope;
            nativeSharingPolicy.PackageName = pinCollection.AddObject(this.PackageName);
            nativeSharingPolicy.Reserved = IntPtr.Zero;
        }

        internal static unsafe PackageSharingPolicy FromNative(NativeTypes.FABRIC_PACKAGE_SHARING_POLICY nativeSharingPolicy)
        {
            var packageName = NativeTypes.FromNativeString(nativeSharingPolicy.PackageName);
            var sharingScope = (PackageSharingPolicyScope)nativeSharingPolicy.Scope;
            return new PackageSharingPolicy(packageName, sharingScope);
        }
    }
}