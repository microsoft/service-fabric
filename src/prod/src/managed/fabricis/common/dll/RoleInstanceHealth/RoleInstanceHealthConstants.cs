// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// Class for consolidating all constants related to health.
    /// </summary>
    internal static class RoleInstanceHealthConstants
    {
        public const string HealthReportRoleInstanceStatusPropertyName = "Azure:RoleInstanceStatus";
        public const string HealthReportRoleInstanceLastUpdateTimePropertyName = "Azure:RoleInstanceStatusLastUpdatedAt";

        /// <summary>
        /// The property name associated with warning the operator/admin that an Azure management notification requires
        /// manual approval. E.g. due to an impact reason of VendorRepairBegin
        /// </summary>
        public const string HealthReportManagementNotificationApprovalStatusPropertyName =
            "Azure:ManagementNotificationApprovalStatus";

        public const string HealthReportReplicaHealthWatchdogStatus = "HealthWatchdog";

        /// <summary>
        /// All health keys provided in clustermanifest.xml need to have this format in order to be used
        /// by the health system of Fabric infrastructure service. See <see cref="BaseRoleInstanceHealthPolicy.GetFullKeyName{T}(string, string, T)"/>
        /// </summary>
        public const string HealthKeyFormat = "{0}Health.{1}.{2}"; // e.g. 0 = "Azure.", 1 = "RoleInstanceStatusMap", 2 = "ReadyRole"

        /// <summary>
        /// The trace type used commonly by all health related classes.
        /// </summary>
        public static readonly TraceType TraceType = new TraceType("AzureHealthWatchdog");
    }
}