// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the health report kind.</para>
    /// </summary>
    public enum HealthReportKind
    {
        /// <summary>
        /// <para>Indicates that the health report kind is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_HEALTH_REPORT_KIND.FABRIC_HEALTH_REPORT_KIND_INVALID,
        
        /// <summary>
        /// <para>Indicates that the health report is for a stateful service replica.</para>
        /// </summary>
        StatefulServiceReplica = NativeTypes.FABRIC_HEALTH_REPORT_KIND.FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA,
        
        /// <summary>
        /// <para>Indicates that the health report is for a stateless service instance.</para>
        /// </summary>
        StatelessServiceInstance = NativeTypes.FABRIC_HEALTH_REPORT_KIND.FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE,
        
        /// <summary>
        /// <para>Indicates that the health report is for a partition.</para>
        /// </summary>
        Partition = NativeTypes.FABRIC_HEALTH_REPORT_KIND.FABRIC_HEALTH_REPORT_KIND_PARTITION,
        
        /// <summary>
        /// <para>Indicates that the health report is for a node.</para>
        /// </summary>
        Node = NativeTypes.FABRIC_HEALTH_REPORT_KIND.FABRIC_HEALTH_REPORT_KIND_NODE,
        
        /// <summary>
        /// <para>Indicates that the health report is for a service.</para>
        /// </summary>
        Service = NativeTypes.FABRIC_HEALTH_REPORT_KIND.FABRIC_HEALTH_REPORT_KIND_SERVICE,
        
        /// <summary>
        /// <para>Indicates that the health report is for an application.</para>
        /// </summary>
        Application = NativeTypes.FABRIC_HEALTH_REPORT_KIND.FABRIC_HEALTH_REPORT_KIND_APPLICATION,
        
        /// <summary>
        /// <para>Indicates that the health report is for a deployed application.</para>
        /// </summary>
        DeployedApplication = NativeTypes.FABRIC_HEALTH_REPORT_KIND.FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION,
        
        /// <summary>
        /// <para>Indicates that the health report is for a deployed service package.</para>
        /// </summary>
        DeployedServicePackage = NativeTypes.FABRIC_HEALTH_REPORT_KIND.FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE,
        
        /// <summary>
        /// <para>Indicates that the health report is for cluster.</para>
        /// </summary>
        Cluster = NativeTypes.FABRIC_HEALTH_REPORT_KIND.FABRIC_HEALTH_REPORT_KIND_CLUSTER,
    }
}