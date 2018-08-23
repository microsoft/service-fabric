// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Text;

    /// <summary>
    /// <para>Represents a policy which indicates that a particular fault or upgrade domain should not be used for placement of the instances or 
    /// replicas of the service this policy is applied to.</para>
    /// </summary>
    /// <remarks>
    /// <para>As an example, in geographically distributed rings there may be a service which must not be run in a particular region due to political 
    /// or legal requirements. In this case that domain could be defined as invalid with this policy. </para>
    /// </remarks>
    public sealed class ServicePlacementInvalidDomainPolicyDescription : ServicePlacementPolicyDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServicePlacementInvalidDomainPolicyDescription" /> class.</para>
        /// </summary>
        public ServicePlacementInvalidDomainPolicyDescription()
            : base(ServicePlacementPolicyType.InvalidDomain)
        { }

        // Copy ctor
        internal ServicePlacementInvalidDomainPolicyDescription(ServicePlacementInvalidDomainPolicyDescription other)
            : base(other)
        {
            this.DomainName = other.DomainName;
        }

        /// <summary>
        /// <para>Gets or sets the name of the fault domain, as a string, that it is invalid to place this service in.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the fault domain, as a string, that it is invalid to place this service in.</para>
        /// </value>
        public string DomainName { get; set; }

        internal static new unsafe ServicePlacementInvalidDomainPolicyDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, StringResources.Error_NullNativePointer);
            NativeTypes.FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN_DESCRIPTION* casted =
                (NativeTypes.FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN_DESCRIPTION*)nativePtr;

            var policy = new ServicePlacementInvalidDomainPolicyDescription();
            policy.DomainName = NativeTypes.FromNativeString(casted->InvalidFaultDomain);

            return policy;
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE type)
        {
            var domain = new NativeTypes.FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN_DESCRIPTION[1];
            domain[0].InvalidFaultDomain = pin.AddBlittable(this.DomainName);
            type = NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN;

            return pin.AddBlittable(domain);
        }

        /// <summary>
        /// <para> 
        /// Return a string representation of the InvalidDomain Service Placement Policy in the form 'InvalidDomain, DomainName' 
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>A string representing the object.</para>
        /// </returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            if (this.DomainName != null)
            {
                sb.AppendFormat("InvalidDomain,{0}", this.DomainName);
            }

            return sb.ToString();
        }
    }

    /// <summary>
    /// <para>
    /// Placement policy description that requires a replica to be placed in a particular domain.
    /// </para>
    /// </summary>
    public sealed class ServicePlacementRequiredDomainPolicyDescription : ServicePlacementPolicyDescription
    {
        /// <summary>
        /// <para>
        /// Instantiates a new <see cref="System.Fabric.Description.ServicePlacementRequiredDomainPolicyDescription" /> object.
        /// </para>
        /// </summary>
        public ServicePlacementRequiredDomainPolicyDescription()
            : base(ServicePlacementPolicyType.RequireDomain)
        { }

        /// <summary> 
        /// A copy constructor for the ServicePlacementRequiredDomainPolicyDescription class
        /// </summary>
        internal ServicePlacementRequiredDomainPolicyDescription(ServicePlacementRequiredDomainPolicyDescription other)
            : base(other)
        {
            this.DomainName = other.DomainName;
        }

        /// <summary>
        /// <para> 
        /// Gets or sets the name of the domain specified in a ServicePlacementRequiredDomainPolicyDescription
        /// </para>
        /// </summary>
        /// <value>
        /// <para> A string containing the name of the domain that the ServicePlacementRequiredDomainPolicyDescription should respect.</para>
        /// </value>
        public string DomainName { get; set; }

        internal static new unsafe ServicePlacementRequiredDomainPolicyDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, StringResources.Error_NullNativePointer);
            NativeTypes.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DESCRIPTION* casted =
                (NativeTypes.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DESCRIPTION*)nativePtr;

            var policy = new ServicePlacementRequiredDomainPolicyDescription();
            policy.DomainName = NativeTypes.FromNativeString(casted->RequiredFaultDomain);

            return policy;
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE type)
        {
            var domain = new NativeTypes.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DESCRIPTION[1];
            domain[0].RequiredFaultDomain = pin.AddBlittable(this.DomainName);
            type = NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN;

            return pin.AddBlittable(domain);
        }


        /// <summary>
        /// <para> 
        /// Return a string representation of the RequiredDomain Service Placement Policy in the form 'RequiredDomain, DomainName' 
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>A string representing the object.</para>
        /// </returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            if (this.DomainName != null)
            {
                sb.AppendFormat("RequiredDomain,{0}", this.DomainName);
            }

            return sb.ToString();
        }
    }

    /// <summary>
    /// <para>Represents a <see cref="System.Fabric.Description.ServicePlacementPolicyDescription" /> which indicates that the service’s Primary replicas 
    /// should optimally be placed in a particular domain.</para>
    /// </summary>
    /// <remarks>
    /// <para>This constraint is usually used with fault domains in scenarios where the Service Fabric cluster is geographically distributed in order to 
    /// indicate that a service’s primary replica should be located in a particular fault domain, which in geo-distributed scenarios usually aligns with 
    /// regional or datacenter boundaries. Note that since this is an optimization it is possible that the Primary replica may not end up located in this 
    /// domain due to failures, capacity limits, or other constraints.</para>
    /// </remarks>
    /// <example>
    ///   <code>
    /// //create the service placement policy
    /// ServicePlacementPreferPrimaryDomainPolicyDescription placementPolicy = new ServicePlacementPreferPrimaryDomainPolicyDescription();
    /// placementPolicy.DomainName = @"fd:\Datacenter1";
    /// 
    /// //add it to the Stateful Service Description
    /// StatefulServiceDescription ssd = new StatefulServiceDescription();
    /// ssd.PlacementPolicies.Add(placementPolicy);</code>
    /// </example>
    public sealed class ServicePlacementPreferPrimaryDomainPolicyDescription : ServicePlacementPolicyDescription
    {
        /// <summary>
        /// <para>initializing a new instance of the <see cref="System.Fabric.Description.ServicePlacementPreferPrimaryDomainPolicyDescription" /> class.</para>
        /// </summary>
        public ServicePlacementPreferPrimaryDomainPolicyDescription()
            : base(ServicePlacementPolicyType.PreferPrimaryDomain)
        { }

        // Copy ctor
        internal ServicePlacementPreferPrimaryDomainPolicyDescription(ServicePlacementPreferPrimaryDomainPolicyDescription other)
            : base(other)
        {
            this.DomainName = other.DomainName;
        }

        /// <summary>
        /// <para>Gets or sets the string name of the domain in which the Primary replica should be preferentially located.</para>
        /// </summary>
        /// <value>
        /// <para>The string name of the domain in which the Primary replica should be preferentially located.</para>
        /// </value>
        public string DomainName { get; set; }

        internal static new unsafe ServicePlacementPreferPrimaryDomainPolicyDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, StringResources.Error_NullNativePointer);
            NativeTypes.FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN_DESCRIPTION* casted = (NativeTypes.FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN_DESCRIPTION*)nativePtr;

            var policy = new ServicePlacementPreferPrimaryDomainPolicyDescription();
            policy.DomainName = NativeTypes.FromNativeString(casted->PreferredPrimaryFaultDomain);

            return policy;
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE type)
        {
            var domain = new NativeTypes.FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN_DESCRIPTION[1];
            domain[0].PreferredPrimaryFaultDomain = pin.AddBlittable(this.DomainName);
            type = NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN;

            return pin.AddBlittable(domain);
        }


        /// <summary>
        /// <para> 
        /// Return a string representation of the PreferPrimaryDomain Service Placement Policy in the form 'PreferPrimaryDomain, DomainName' 
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>A string representing the object.</para>
        /// </returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            if (this.DomainName != null)
            {
                sb.AppendFormat("PreferredPrimaryDomain,{0}", this.DomainName);
            }

            return sb.ToString();
        }
    }

    /// <summary>
    /// <para>Specifies the placement policy which indicates that two replicas from the same partition should never be placed in the same fault or upgrade domain.  
    /// While this is not common it can expose the service to an increased risk of concurrent failures due to unplanned outages or other cases of subsequent/concurrent 
    /// failures. As an example, consider a case where replicas are deployed across different data center, with one replica per location. In the event that one of 
    /// the datacenters goes offline, normally the replica that was placed in that datacenter will be packed into one of the remaining datacenters. If this is not 
    /// desirable then this policy should be set.</para>
    /// </summary>
    public sealed class ServicePlacementRequireDomainDistributionPolicyDescription : ServicePlacementPolicyDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServicePlacementRequireDomainDistributionPolicyDescription" /> class.</para>
        /// </summary>
        public ServicePlacementRequireDomainDistributionPolicyDescription()
            : base(ServicePlacementPolicyType.RequireDomainDistribution)
        {
            DomainName = string.Empty;
        }

        // Copy ctor
        internal ServicePlacementRequireDomainDistributionPolicyDescription(ServicePlacementRequireDomainDistributionPolicyDescription other)
            : base(other)
        {
            this.DomainName = other.DomainName;
        }

        // Native/Rest has this property
        private string DomainName { get; set; }

        internal static new unsafe ServicePlacementRequireDomainDistributionPolicyDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, StringResources.Error_NullNativePointer);
            NativeTypes.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION_DESCRIPTION* casted =
                (NativeTypes.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION_DESCRIPTION*)nativePtr;

            var policy = new ServicePlacementRequireDomainDistributionPolicyDescription();

            return policy;
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE type)
        {
            var domain = new NativeTypes.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION_DESCRIPTION[1];
            type = NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION;

            return pin.AddBlittable(domain);
        }

        /// <summary>
        /// <para> 
        /// Return a string representation of the RequiredDomainDistribution Service Placement Policy in the form 'RequiredDomainDistribution, DomainName' 
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>A string representing the object.</para>
        /// </returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("RequiredDomainDistribution");

            return sb.ToString();
        }
    }

    /// <summary>
    /// <para>
    /// Placement policy description that describes a service placement where all replicas must be able to be placed in order for any replicas to be created.
    /// </para>
    /// </summary>
    public sealed class ServicePlacementNonPartiallyPlaceServicePolicyDescription : ServicePlacementPolicyDescription
    {
        /// <summary>
        ///   <para />
        /// </summary>
        public ServicePlacementNonPartiallyPlaceServicePolicyDescription()
            : base(ServicePlacementPolicyType.NonPartiallyPlaceService)
        { }

        // Copy ctor
        internal ServicePlacementNonPartiallyPlaceServicePolicyDescription(ServicePlacementNonPartiallyPlaceServicePolicyDescription other)
            : base(other)
        {
        }

        internal static new unsafe ServicePlacementNonPartiallyPlaceServicePolicyDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, StringResources.Error_NullNativePointer);
            NativeTypes.FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE_DESCRIPTION* CASTED =
                (NativeTypes.FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE_DESCRIPTION*)nativePtr;

            var policy = new ServicePlacementNonPartiallyPlaceServicePolicyDescription();

            return policy;
        }

        internal override IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE type)
        {
            var domain = new NativeTypes.FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE_DESCRIPTION[1];
            type = NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE;

            return pin.AddBlittable(domain);
        }

        /// <summary>
        /// <para> 
        /// Return a string representation of the ServicePlacementNonPartiallyPlaceServicePolicyDescription
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>A string representing the object.</para>
        /// </returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("NonPartiallyPlace");

            return sb.ToString();
        }
    }
}