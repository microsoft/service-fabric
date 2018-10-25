// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the entity kind.</para>
    /// </summary>
    public enum EntityKind
    {
        /// <summary>
        /// <para>Indicates that the entity kind is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_HEALTH_ENTITY_KIND.FABRIC_HEALTH_ENTITY_KIND_INVALID,
        
        /// <summary>
        /// <para>Indicates that the health entity is a replica, either a stateful service replica or a stateless service instance.</para>
        /// </summary>
        Replica = NativeTypes.FABRIC_HEALTH_ENTITY_KIND.FABRIC_HEALTH_ENTITY_KIND_REPLICA,
       
        /// <summary>
        /// <para>Indicates that the health entity is a partition.</para>
        /// </summary>
        Partition = NativeTypes.FABRIC_HEALTH_ENTITY_KIND.FABRIC_HEALTH_ENTITY_KIND_PARTITION,
        
        /// <summary>
        /// <para>Indicates that the health entity is a node.</para>
        /// </summary>
        Node = NativeTypes.FABRIC_HEALTH_ENTITY_KIND.FABRIC_HEALTH_ENTITY_KIND_NODE,
        
        /// <summary>
        /// <para>Indicates that the health entity is a service.</para>
        /// </summary>
        Service = NativeTypes.FABRIC_HEALTH_ENTITY_KIND.FABRIC_HEALTH_ENTITY_KIND_SERVICE,
        
        /// <summary>
        /// <para>Indicates that the health entity is an application.</para>
        /// </summary>
        Application = NativeTypes.FABRIC_HEALTH_ENTITY_KIND.FABRIC_HEALTH_ENTITY_KIND_APPLICATION,
        
        /// <summary>
        /// <para>Indicates that the health entity is a deployed application.</para>
        /// </summary>
        DeployedApplication = NativeTypes.FABRIC_HEALTH_ENTITY_KIND.FABRIC_HEALTH_ENTITY_KIND_DEPLOYED_APPLICATION,
        
        /// <summary>
        /// <para>Indicates that the health entity is a deployed service package.</para>
        /// </summary>
        DeployedServicePackage = NativeTypes.FABRIC_HEALTH_ENTITY_KIND.FABRIC_HEALTH_ENTITY_KIND_DEPLOYED_SERVICE_PACKAGE,
        
        /// <summary>
        /// <para>Indicates that the health entity is the cluster.</para>
        /// </summary>
        Cluster = NativeTypes.FABRIC_HEALTH_ENTITY_KIND.FABRIC_HEALTH_ENTITY_KIND_CLUSTER,
    }
}