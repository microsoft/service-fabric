// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>Describes the application principals of the service.</para>
    /// </summary>
    public sealed class ApplicationPrincipalsDescription
    {
        /// <summary>
        /// <para>Creates and initializes an <see cref="System.Fabric.Description.ApplicationPrincipalsDescription" /> object. </para>
        /// </summary>
        public ApplicationPrincipalsDescription()
        {
            this.Users = new ItemList<SecurityUserDescription>();
            this.Groups = new ItemList<SecurityGroupDescription>();
        }

        internal ApplicationPrincipalsDescription(IList<SecurityUserDescription> users, IList<SecurityGroupDescription> groups)
        {
            this.Users = users;
            this.Groups = groups;
        }

        /// <summary>
        /// <para>Gets the users that must be created as a part of the application environment setup in the application manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The users that must be created as a part of the application environment setup in the application manifest.</para>
        /// </value>
        public IList<SecurityUserDescription> Users
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the groups that must be created as a part of the application environment setup in the application manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The groups that must be created as a part of the application environment setup in the application manifest.</para>
        /// </value>
        public IList<SecurityGroupDescription> Groups
        {
            get;
            private set;
        }

        internal static unsafe ApplicationPrincipalsDescription CreateFromNative(NativeTypes.FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION* nativeDescription)
        {
            var users = new ItemList<SecurityUserDescription>();
            var nativeUsers = (NativeTypes.FABRIC_SECURITY_USER_DESCRIPTION_LIST*)nativeDescription->Users;
            for (int i = 0; i < nativeUsers->Count; ++i)
            {
                users.Add(SecurityUserDescription.CreateFromNative(nativeUsers->Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_SECURITY_USER_DESCRIPTION)))));
            }

            var groups = new ItemList<SecurityGroupDescription>();
            var nativeGroups = (NativeTypes.FABRIC_SECURITY_GROUP_DESCRIPTION_LIST*)nativeDescription->Groups;
            for (int i = 0; i < nativeGroups->Count; ++i)
            {
                groups.Add(SecurityGroupDescription.CreateFromNative(nativeGroups->Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_SECURITY_GROUP_DESCRIPTION)))));
            }

            return new ApplicationPrincipalsDescription(users, groups);
        }
    }
}