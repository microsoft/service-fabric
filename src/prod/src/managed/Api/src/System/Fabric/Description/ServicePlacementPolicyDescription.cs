// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Represents the base type for all PlacementPolicyDescription types in the cluster.</para>
    /// </summary>
    [KnownType(typeof(ServicePlacementInvalidDomainPolicyDescription))]
    [KnownType(typeof(ServicePlacementRequiredDomainPolicyDescription))]
    [KnownType(typeof(ServicePlacementPreferPrimaryDomainPolicyDescription))]
    [KnownType(typeof(ServicePlacementRequireDomainDistributionPolicyDescription))]
    [KnownType(typeof(ServicePlacementNonPartiallyPlaceServicePolicyDescription))]
    public abstract class ServicePlacementPolicyDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServicePlacementPolicyDescription" /> class.</para>
        /// </summary>
        /// <param name="type">
        /// <para>The service placement policy type.</para>
        /// </param>
        public ServicePlacementPolicyDescription(ServicePlacementPolicyType type)
        {
            this.Type = type;
        }

        /// <summary>
        /// <para> 
        /// Constructor for a ServicePlacementPolicyDescription
        /// </para>
        /// </summary>
        /// <param name="other">
        /// <para> The ServicePlacementPolicyDescription that the new object should be constructed from.</para>
        /// </param>
        protected ServicePlacementPolicyDescription(ServicePlacementPolicyDescription other)
        {
            this.Type = other.Type;
        }

        /// <summary>
        /// <para>Gets the service placement policy type.</para>
        /// </summary>
        /// <value>
        /// <para>The service placement policy type.</para>
        /// </value>
        public ServicePlacementPolicyType Type { get; set; }

        internal ServicePlacementPolicyDescription GetCopy()
        {
            ServicePlacementPolicyDescription copy;

            if (this is ServicePlacementInvalidDomainPolicyDescription)
            {
                copy = new ServicePlacementInvalidDomainPolicyDescription(this as ServicePlacementInvalidDomainPolicyDescription);
            }
            else if (this is ServicePlacementRequiredDomainPolicyDescription)
            {
                copy = new ServicePlacementRequiredDomainPolicyDescription(this as ServicePlacementRequiredDomainPolicyDescription);
            }
            else if (this is ServicePlacementPreferPrimaryDomainPolicyDescription)
            {
                copy = new ServicePlacementPreferPrimaryDomainPolicyDescription(this as ServicePlacementPreferPrimaryDomainPolicyDescription);
            }
            else if (this is ServicePlacementRequireDomainDistributionPolicyDescription)
            {
                copy = new ServicePlacementRequireDomainDistributionPolicyDescription(this as ServicePlacementRequireDomainDistributionPolicyDescription);
            }
            else
            {
                copy = new ServicePlacementNonPartiallyPlaceServicePolicyDescription(this as ServicePlacementNonPartiallyPlaceServicePolicyDescription);
            }

            return copy;
        }

        internal static unsafe ServicePlacementPolicyDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, StringResources.Error_NullNativePointer);

            NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION* casted = (NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION*)nativePtr;
            switch (casted->Type)
            {
                case NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN:
                    return ServicePlacementInvalidDomainPolicyDescription.CreateFromNative(casted->Value);
                case NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN:
                    return ServicePlacementRequiredDomainPolicyDescription.CreateFromNative(casted->Value);
                case NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN:
                    return ServicePlacementPreferPrimaryDomainPolicyDescription.CreateFromNative(casted->Value);
                case NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION:
                    return ServicePlacementRequireDomainDistributionPolicyDescription.CreateFromNative(casted->Value);
                case NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE:
                    return ServicePlacementNonPartiallyPlaceServicePolicyDescription.CreateFromNative(casted->Value);
                default:
                    AppTrace.TraceSource.WriteError("ServicePlacementPolicyDescription.CreateFromNative", "Invalid placement policy type {0}", casted->Type);
                    break;

            }
            return null;
        }

        internal unsafe IntPtr ToNative(PinCollection pin, ref NativeTypes.FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION nativePolicy)
        {
            NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE type;

            nativePolicy.Value = this.ToNative(pin, out type);
            nativePolicy.Type = type;

            return pin.AddBlittable(nativePolicy);

        }

        internal abstract IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE type);

        // Method used by JsonSerializer to resolve derived type using json property "Type".
        // The base class needs to have attributes [KnownType()]
        // This method must be static with one parameter input which represented by given json property.
        // Provide name of the json property which will be used as parameter to this method.
        [DerivedTypeResolverAttribute("Type")]
        internal static Type ResolveDerivedClass(ServicePlacementPolicyType type)
        {
            switch (type)
            {
                case ServicePlacementPolicyType.InvalidDomain:
                    return typeof(ServicePlacementInvalidDomainPolicyDescription);

                case ServicePlacementPolicyType.PreferPrimaryDomain:
                    return typeof(ServicePlacementPreferPrimaryDomainPolicyDescription);

                case ServicePlacementPolicyType.RequireDomain:
                    return typeof(ServicePlacementRequiredDomainPolicyDescription);

                case ServicePlacementPolicyType.RequireDomainDistribution:
                    return typeof(ServicePlacementRequireDomainDistributionPolicyDescription);

                case ServicePlacementPolicyType.NonPartiallyPlaceService:
                    return typeof(ServicePlacementNonPartiallyPlaceServicePolicyDescription);

                default:
                    return null;
            }
        }
    }
}