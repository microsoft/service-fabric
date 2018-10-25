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
    /// <para>Describes the type of stateless service.</para>
    /// </summary>
    public sealed class StatelessServiceTypeDescription : ServiceTypeDescription
    {
        /// <summary>
        /// <para>Creates and initializes an instance of the <see cref="System.Fabric.Description.StatelessServiceTypeDescription" /> object.</para>
        /// </summary>
        public StatelessServiceTypeDescription()
            : base(ServiceDescriptionKind.Stateless)
        {
        }

        /// <summary>
        /// <para>Specifies that the service does not implement Service Fabric interfaces. Service Fabric should start the specified executables (EXEs).</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Boolean" />.</para>
        /// </value>
        public bool UseImplicitHost { get; set; }

        internal static unsafe new StatelessServiceTypeDescription CreateFromNative(IntPtr descriptionPtr)
        {
            NativeTypes.FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION* nativeDescription = (NativeTypes.FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION*)descriptionPtr;

            StatelessServiceTypeDescription description = new StatelessServiceTypeDescription();

            description.ReadCommonProperties(
                nativeDescription->ServiceTypeName,
                nativeDescription->PlacementConstraints,
                nativeDescription->LoadMetrics,
                nativeDescription->Extensions);

            if (nativeDescription->Reserved != IntPtr.Zero)
            {
                NativeTypes.FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION_EX1* ex1 = (NativeTypes.FABRIC_STATELESS_SERVICE_TYPE_DESCRIPTION_EX1*)nativeDescription->Reserved;
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

            description.UseImplicitHost = NativeTypes.FromBOOLEAN(nativeDescription->UseImplicitHost);
            return description;
        }
    }
}