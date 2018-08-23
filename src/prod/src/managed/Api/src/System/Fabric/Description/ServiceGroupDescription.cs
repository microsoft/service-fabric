// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Provides a collection of information that is necessary to create and describe a service group.  </para>
    /// </summary>
    /// <remarks>
    /// <para>A service group description contains a <see cref="System.Fabric.Description.ServiceGroupDescription" /> and a list of members in the 
    /// service group. The service description provides information, such as the metrics, application name, service group name, and initialization 
    /// information for this service group. The list of member definitions describes the services inside the service group.</para>
    /// </remarks>
    public sealed class ServiceGroupDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServiceGroupDescription" /> class.</para>
        /// </summary>
        public ServiceGroupDescription()
        {
            this.MemberDescriptions = new ItemList<ServiceGroupMemberDescription>(
                false /* mayContainNullValues */,
                true /* mayContainDuplicates */);
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServiceGroupDescription" /> class with the specified 
        /// <see cref="System.Fabric.Description.ServiceDescription" />.</para>
        /// </summary>
        /// <param name="serviceDescription">
        /// <para>The <see cref="System.Fabric.Description.ServiceDescription" /> to use as the basis for the <see cref="System.Fabric.Description.ServiceGroupDescription" />.</para>
        /// </param>
        public ServiceGroupDescription(ServiceDescription serviceDescription)
            : this()
        {
            this.ServiceDescription = serviceDescription;
        }

        /// <summary>
        /// <para>Specifies the list of <see cref="System.Fabric.Description.ServiceGroupMemberDescription" /> objects for the members of this service group.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Collections.Generic.IList{T}" /> of type <see cref="System.Fabric.Description.ServiceGroupMemberDescription" />.</para>
        /// </value>
        public IList<ServiceGroupMemberDescription> MemberDescriptions
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Describes the service group’s service.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Description.ServiceDescription" />.</para>
        /// </value>
        /// <remarks>
        /// <para>The service description describes how the system should partition the service group, including the partitioning scheme, such as the key, 
        /// the key range and other properties.</para>
        /// </remarks>
        public ServiceDescription ServiceDescription
        {
            get;
            set;
        }

        internal static void Validate(ServiceGroupDescription description)
        {
            Requires.Argument<ServiceDescription>("ServiceDescription", description.ServiceDescription).NotNull();
            ServiceDescription.Validate(description.ServiceDescription);
            foreach (ServiceGroupMemberDescription memberDescription in description.MemberDescriptions)
            {
                ServiceGroupMemberDescription.Validate(memberDescription);
            }
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ServiceGroupDescription CreateFromNative(NativeClient.IFabricServiceGroupDescriptionResult nativeResult)
        {
            if (nativeResult == null)
            {
                return null;
            }

            var nativeDescription = (NativeTypes.FABRIC_SERVICE_GROUP_DESCRIPTION*)nativeResult.get_Description();
            var nativeServiceDescription = (NativeTypes.FABRIC_SERVICE_DESCRIPTION*)nativeDescription->Description;

            ServiceDescription serviceDescription = ServiceDescription.CreateFromNative(nativeDescription->Description);

            bool isStateful = (serviceDescription.Kind == ServiceDescriptionKind.Stateful);

            ServiceGroupDescription description = new ServiceGroupDescription(serviceDescription);

            var members = (NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION*)nativeDescription->MemberDescriptions;
            for (int i = 0; i < nativeDescription->MemberCount; ++i)
            {
                var memberDescription = ServiceGroupMemberDescription.CreateFromNative((IntPtr)(members + i), isStateful);
                description.MemberDescriptions.Add(memberDescription);
            }

            GC.KeepAlive(nativeResult);

            return description;
        }
    }
}