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
    /// <para>Represents a description of a security group.</para>
    /// </summary>
    public sealed class SecurityGroupDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.SecurityGroupDescription" /> class.</para>
        /// </summary>
        public SecurityGroupDescription()
        {
            this.DomainGroupMembers = new ItemList<string>();
            this.SystemGroupMembers = new ItemList<string>();
            this.DomainUserMembers = new ItemList<string>();
        }

        internal SecurityGroupDescription(
            string name,
            SecurityIdentifier sid,
            IList<string> domainGroupMembers,
            IList<string> systemGroupMembers,
            IList<string> domainUserMembers)
        {
            this.Name = name;
            this.Sid = sid;
            this.DomainGroupMembers = domainGroupMembers;
            this.SystemGroupMembers = systemGroupMembers;
            this.DomainUserMembers = domainUserMembers;
        }

        /// <summary>
        /// <para>Gets the name of the group to be created as part of environment setup for an application.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the group to be created as part of environment setup for an application.</para>
        /// </value>
        public string Name
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>
        /// Gets the primary SecurityIdentifier for the SecurityGroup.
        /// </para>
        /// </summary>
        /// <value>
        ///  The primary SecurityIdentifier for the SecurityGroup.
        /// </value>
        public SecurityIdentifier Sid
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the domain groups to be added as members to this group.</para>
        /// </summary>
        /// <value>
        /// <para>The domain groups to be added as members to this group.</para>
        /// </value>
        public IList<string> DomainGroupMembers
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the system groups to be added as members to this group.</para>
        /// </summary>
        /// <value>
        /// <para>The system groups to be added as members to this group.</para>
        /// </value>
        public IList<string> SystemGroupMembers
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the domain users to be added as members to this group.</para>
        /// </summary>
        /// <value>
        /// <para>The domain users to be added as members to this group.</para>
        /// </value>
        public IList<string> DomainUserMembers
        {
            get;
            private set;
        }

        internal static unsafe SecurityGroupDescription CreateFromNative(IntPtr native)
        {
            var nativeDescription = (NativeTypes.FABRIC_SECURITY_GROUP_DESCRIPTION*)native;

            var name = NativeTypes.FromNativeString(nativeDescription->Name);
            var sid = new SecurityIdentifier(NativeTypes.FromNativeString(nativeDescription->Sid));

            var nativeDomainGroupMembers = (NativeTypes.FABRIC_STRING_LIST*)nativeDescription->DomainGroupMembers;
            var domainGroupMembers = NativeTypes.FromNativeStringList(*nativeDomainGroupMembers);

            var nativeDomainUserMembers = (NativeTypes.FABRIC_STRING_LIST*)nativeDescription->DomainUserMembers;
            var systemGroupMembers = NativeTypes.FromNativeStringList(*nativeDomainUserMembers);

            var nativeSystemGroupMembers = (NativeTypes.FABRIC_STRING_LIST*)nativeDescription->SystemGroupMembers;
            var domainUserMembers = NativeTypes.FromNativeStringList(*nativeSystemGroupMembers);

            return new SecurityGroupDescription(
                name,
                sid,
                domainGroupMembers,
                systemGroupMembers,
                domainUserMembers);
        }
    }
}