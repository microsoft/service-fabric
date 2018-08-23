// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Security.Principal;

    /// <summary>
    /// <para>Represents a description for a security user.</para>
    /// </summary>
    public sealed class SecurityUserDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.SecurityUserDescription" /> class. </para>
        /// </summary>
        public SecurityUserDescription()
        {
            this.ParentSystemGroups = new ItemList<string>();
            this.ParentApplicationGroups = new ItemList<string>();
        }

        internal SecurityUserDescription(
            string name,
            SecurityIdentifier sid,
            IList<string> parentSystemGroups,
            IList<string> parentApplicationGroups)
        {
            this.Name = name;
            this.Sid = sid;
            this.ParentSystemGroups = parentSystemGroups;
            this.ParentApplicationGroups = parentApplicationGroups;
        }

        /// <summary>
        /// <para>Gets the name of the user to be created as part of environment setup for the application manifest. </para>
        /// </summary>
        /// <value>
        /// <para>The name of the user to be created as part of environment setup for the application manifest. </para>
        /// </value>
        public string Name
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>
        /// Gets the primary SecurityIdentifier for the SecurityUser.
        /// </para>
        /// </summary>
        /// <value>
        ///   The primary SecurityIdentifier for the SecurityUser
        /// </value>
        public SecurityIdentifier Sid
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the local groups to which this user is to be added.</para>
        /// </summary>
        /// <value>
        /// <para>The local groups to which this user is to be added.</para>
        /// </value>
        public IList<string> ParentSystemGroups
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the parent groups in the security group description, to which this user is to be added. </para>
        /// </summary>
        /// <value>
        /// <para>The parent groups in the security group description, to which this user is to be added. </para>
        /// </value>
        public IList<string> ParentApplicationGroups
        {
            get;
            private set;
        }

        internal static unsafe SecurityUserDescription CreateFromNative(IntPtr native)
        {
            var nativeDescription = (NativeTypes.FABRIC_SECURITY_USER_DESCRIPTION*)native;
            var name = NativeTypes.FromNativeString(nativeDescription->Name);
            var sid = new SecurityIdentifier(NativeTypes.FromNativeString(nativeDescription->Sid));

            var nativeParentSystemGroups = (NativeTypes.FABRIC_STRING_LIST*)nativeDescription->ParentSystemGroups;
            var nativeParentSystemGroupsStringArray = (char**)nativeParentSystemGroups->Items;
            var parentSystemGroups = new ItemList<string>();
            for (int i = 0; i < nativeParentSystemGroups->Count; i++)
            {
                var nativeItemPtr = (IntPtr)(nativeParentSystemGroupsStringArray + i);
                parentSystemGroups.Add(NativeTypes.FromNativeStringPointer(nativeItemPtr));
            }

            var nativeParentApplicationGroups = (NativeTypes.FABRIC_STRING_LIST*)nativeDescription->ParentApplicationGroups;
            var nativeParentApplicationGroupsStringArray = (char**)nativeParentApplicationGroups->Items;
            var parentApplicationGroups = new ItemList<string>();
            for (int i = 0; i < nativeParentApplicationGroups->Count; i++)
            {
                var nativeItemPtr = (IntPtr)(nativeParentApplicationGroupsStringArray + i);
                parentApplicationGroups.Add(NativeTypes.FromNativeStringPointer(nativeItemPtr));
            }

            return new SecurityUserDescription(
                name,
                sid,
                parentSystemGroups,
                parentApplicationGroups);
        }
    }
}