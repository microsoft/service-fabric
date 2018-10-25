// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Collections.Generic;

    /// <summary>
    /// <para>Represents a list of PackageSharingPolicy objects.</para>
    /// </summary>
    public class PackageSharingPolicyList
    {
        /// <summary>
        /// <para>Initializes A new instance of the <see cref="System.Fabric.PackageSharingPolicyList" /> class.</para>
        /// </summary>
        public PackageSharingPolicyList()
            : this(new List<PackageSharingPolicy>())
        {
        }

        /// <summary>
        /// <para>Initializes A new instance of the <see cref="System.Fabric.PackageSharingPolicyList" /> class.</para>
        /// </summary>
        /// <param name="packageSharingPolicies">
        /// <para>The collection of policies for sharing packages.</para>
        /// </param>
        public PackageSharingPolicyList(IList<PackageSharingPolicy> packageSharingPolicies)
        {
            Requires.Argument<IList<PackageSharingPolicy>>("packageSharingPolicies", packageSharingPolicies).NotNull();
            this.PackageSharingPolicies = packageSharingPolicies;
        }

        /// <summary>
        /// <para>Gets the collection of policies for sharing packages.</para>
        /// </summary>
        /// <value>
        /// <para>The collection of policies for sharing packages.</para>
        /// </value>
        public IList<PackageSharingPolicy> PackageSharingPolicies
        {
            get;
            private set;
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeSharingArray = new NativeTypes.FABRIC_PACKAGE_SHARING_POLICY[this.PackageSharingPolicies.Count];
            for (int i = 0; i < this.PackageSharingPolicies.Count; ++i)
            {
                this.PackageSharingPolicies[i].ToNative(pinCollection, out nativeSharingArray[i]);
            }

            var nativeSharingList = new NativeTypes.FABRIC_PACKAGE_SHARING_POLICY_LIST();
            nativeSharingList.Count = (uint)nativeSharingArray.Length;
            nativeSharingList.Items = pinCollection.AddBlittable(nativeSharingArray);

            return pinCollection.AddBlittable(nativeSharingList);
        }

        internal static unsafe PackageSharingPolicyList FromNative(NativeTypes.FABRIC_PACKAGE_SHARING_POLICY_LIST* nativePtr)
        {
            var retval = new PackageSharingPolicyList();

            if (nativePtr->Count > 0)
            {
                var nativeSharingArray = (NativeTypes.FABRIC_PACKAGE_SHARING_POLICY*)nativePtr->Items;
                for (int i = 0; i < nativePtr->Count; ++i)
                {
                    var nativePolicy = *(nativeSharingArray + i);
                    retval.PackageSharingPolicies.Add(PackageSharingPolicy.FromNative(nativePolicy));
                }
            }

            return retval;
        }
    }
}