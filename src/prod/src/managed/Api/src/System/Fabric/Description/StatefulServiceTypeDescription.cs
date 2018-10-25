// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes the stateful service type.</para>
    /// </summary>
    public sealed class StatefulServiceTypeDescription : ServiceTypeDescription
    {
        /// <summary>
        /// <para>Initializes an instance of the <see cref="System.Fabric.Description.StatefulServiceTypeDescription" /> class. </para>
        /// </summary>
        public StatefulServiceTypeDescription()
            : base(ServiceDescriptionKind.Stateful)
        {
        }

        /// <summary>
        /// <para>Gets or sets a flag indicating whether this is a persistent service which stores states on the local disk.</para>
        /// </summary>
        /// <value>
        /// <para>A flag indicating whether this is a persistent service which stores states on the local disk.</para>
        /// </value>
        public bool HasPersistedState { get; set; }

        internal static unsafe new StatefulServiceTypeDescription CreateFromNative(IntPtr descriptionPtr)
        {
            NativeTypes.FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION* nativeDescription = (NativeTypes.FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION*)descriptionPtr;

            StatefulServiceTypeDescription description = new StatefulServiceTypeDescription();

            description.ReadCommonProperties(
                nativeDescription->ServiceTypeName,
                nativeDescription->PlacementConstraints,
                nativeDescription->LoadMetrics,
                nativeDescription->Extensions);

            if (nativeDescription->Reserved != IntPtr.Zero)
            {
                NativeTypes.FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION_EX1* ex1 = (NativeTypes.FABRIC_STATEFUL_SERVICE_TYPE_DESCRIPTION_EX1*)nativeDescription->Reserved;
                if (ex1 == null)
                {
                    throw new ArgumentException(StringResources.Error_UnknownReservedType);
                }

                if (ex1->PolicyList != IntPtr.Zero)
                {
                    NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_LIST* pList = (NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_LIST*)ex1->PolicyList;
                    description.ParsePlacementPolicies(pList->PolicyCount, pList->Policies);
                }
            }

            description.HasPersistedState = NativeTypes.FromBOOLEAN(nativeDescription->HasPersistedState);
            return description;
        }
    }
}