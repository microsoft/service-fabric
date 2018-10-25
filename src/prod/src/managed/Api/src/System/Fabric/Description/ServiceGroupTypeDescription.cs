// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>Represents the type description of the Fabric service group.</para>
    /// </summary>
    public sealed class ServiceGroupTypeDescription
    {
        /// <summary>
        /// <para>Initialize an instance of <see cref="System.Fabric.Description.ServiceGroupTypeDescription" />.</para>
        /// </summary>
        public ServiceGroupTypeDescription()
        {
            this.Members = new Collection<ServiceGroupTypeMemberDescription>();
        }

        /// <summary>
        /// <para>Gets or sets the service group type.</para>
        /// </summary>
        /// <value>
        /// <para>The service group type.</para>
        /// </value>
        public ServiceTypeDescription ServiceTypeDescription
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the collection of members of this service group.</para>
        /// </summary>
        /// <value>
        /// <para>The collection of members of this service group.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ServiceGroupTypeMemberDescription)]
        public ICollection<ServiceGroupTypeMemberDescription> Members
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets or sets the flag indicates whether to use implicit factory.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> to use implicit factory; otherwise, <languageKeyword>false</languageKeyword>. </para>
        /// </value>
        public bool UseImplicitFactory
        {
            get;
            set;
        }

        internal static unsafe ServiceGroupTypeDescription CreateFromNative(IntPtr descriptionPtr)
        {
            NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION* nativeDescription = (NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION*)descriptionPtr;

            ServiceGroupTypeDescription description = new ServiceGroupTypeDescription();
            description.ServiceTypeDescription = ServiceTypeDescription.CreateFromNative(nativeDescription->Description);

            bool isStateful = (description.ServiceTypeDescription.ServiceTypeKind == ServiceDescriptionKind.Stateful);

            NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_LIST* nativeMemberList = (NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_LIST*)nativeDescription->Members;
            for (int i = 0; i < nativeMemberList->Count; ++i)
            {
                description.Members.Add(ServiceGroupTypeMemberDescription.CreateFromNative(nativeMemberList->Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION))), isStateful));
            }

            description.UseImplicitFactory = NativeTypes.FromBOOLEAN(nativeDescription->UseImplicitFactory);

            return description;
        }
    }
}