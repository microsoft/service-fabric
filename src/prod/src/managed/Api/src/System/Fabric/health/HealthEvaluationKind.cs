// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the kinds of health evaluation.</para>
    /// </summary>
    public enum HealthEvaluationKind
    {
        /// <summary>
        /// <para>Indicates that the health evaluation is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_INVALID,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for a health event.</para>
        /// </summary>
        Event = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_EVENT,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for the replicas of a partition.</para>
        /// </summary>
        Replicas = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_REPLICAS,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for the partitions of a service.</para>
        /// </summary>
        Partitions = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_PARTITIONS,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for the deployed service packages of a deployed application.</para>
        /// </summary>
        DeployedServicePackages = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGES,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for the deployed applications of an application.</para>
        /// </summary>
        DeployedApplications = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for services of an application.</para>
        /// </summary>
        Services = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_SERVICES,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for the cluster nodes.</para>
        /// </summary>
        Nodes = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_NODES,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for the cluster applications.</para>
        /// </summary>
        Applications = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for the system application.</para>
        /// </summary>
        SystemApplication = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for the deployed applications of an application in an upgrade domain.</para>
        /// </summary>
        UpgradeDomainDeployedApplications = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for the cluster nodes in an upgrade domain.</para>
        /// </summary>
        UpgradeDomainNodes = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for a replica.</para>
        /// </summary>
        Replica = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_REPLICA,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for a partition.</para>
        /// </summary>
        Partition = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_PARTITION,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for a deployed service package.</para>
        /// </summary>
        DeployedServicePackage = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_SERVICE_PACKAGE,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for a deployed application.</para>
        /// </summary>
        DeployedApplication = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATION,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for a service.</para>
        /// </summary>
        Service = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_SERVICE,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for a node.</para>
        /// </summary>
        Node = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_NODE,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for an application.</para>
        /// </summary>
        Application = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_APPLICATION,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for the delta of unhealthy cluster nodes.</para>
        /// </summary>
        DeltaNodesCheck = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK,
        
        /// <summary>
        /// <para>Indicates that the health evaluation is for the delta of unhealthy upgrade domain cluster nodes.</para>
        /// </summary>
        UpgradeDomainDeltaNodesCheck = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK,
        
        /// <summary>
        /// <para>
        /// Indicates that the health evaluation is for the application type applications.
        /// </para>
        /// </summary>
        ApplicationTypeApplications = NativeTypes.FABRIC_HEALTH_EVALUATION_KIND.FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS,
    }
}