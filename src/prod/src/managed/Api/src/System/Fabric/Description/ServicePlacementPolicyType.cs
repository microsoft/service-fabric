// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Indicates the type of the specific <see cref="System.Fabric.Description.ServicePlacementPolicyDescription" />.</para>
    /// </summary>
    public enum ServicePlacementPolicyType
    {
        /// <summary>
        /// <para>Invalid placement policy type. Indicates that the type of the policy specified was unknown or invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_INVALID,
        
        /// <summary>
        /// <para>Indicates that the <see cref="System.Fabric.Description.ServicePlacementPolicyDescription" /> is a 
        /// <see cref="System.Fabric.Description.ServicePlacementInvalidDomainPolicyDescription" />, which indicates that a particular fault or 
        /// upgrade domain cannot be used for placement of this service.</para>
        /// </summary>
        InvalidDomain = NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_INVALID_DOMAIN,
        
        /// <summary>
        /// <para>Indicates that the <see cref="System.Fabric.Description.ServicePlacementPolicyDescription" /> is a 
        /// <see cref="System.Fabric.Description.ServicePlacementRequireDomainDistributionPolicyDescription" /> indicating that the replicas of 
        /// the service must be placed in a specific domain.</para>
        /// </summary>
        RequireDomain = NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN,
        
        /// <summary>
        /// <para>Indicates that the <see cref="System.Fabric.Description.ServicePlacementPolicyDescription" /> is a 
        /// <see cref="System.Fabric.Description.ServicePlacementPreferPrimaryDomainPolicyDescription" />, which indicates that if possible 
        /// the Primary replica for the partitions of the service should be located in a particular domain as an optimization.</para>
        /// </summary>
        PreferPrimaryDomain = NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_PREFERRED_PRIMARY_DOMAIN,
        
        /// <summary>
        /// <para>Indicates that the <see cref="System.Fabric.Description.ServicePlacementPolicyDescription" /> is a 
        /// <see cref="System.Fabric.Description.ServicePlacementRequireDomainDistributionPolicyDescription" />, indicating that the system 
        /// will disallow placement of any two replicas from the same partition in the same domain at any time.</para>
        /// </summary>
        RequireDomainDistribution = NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_REQUIRED_DOMAIN_DISTRIBUTION,
        
        /// <summary>
        ///   <para />
        /// </summary>
        NonPartiallyPlaceService = NativeTypes.FABRIC_PLACEMENT_POLICY_TYPE.FABRIC_PLACEMENT_POLICY_NONPARTIALLY_PLACE_SERVICE
    }
}