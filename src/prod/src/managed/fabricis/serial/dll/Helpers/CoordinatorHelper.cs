// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Fabric.Common;
    using System.Globalization;
    using System.Linq;
    using System.Text.RegularExpressions;
    using Microsoft.WindowsAzure.ServiceRuntime.Management;    

    /// <summary>
    /// Helper class for the infrastructure coordinator and related classes.
    /// </summary>
    internal static class CoordinatorHelper
    {
        private static readonly TraceType TraceType = new TraceType("WindowsAzureFabricCoordinator");

        public static RoleInstanceState ToRoleInstanceState(this ManagementRoleInstanceStatus managementRoleInstanceStatus)
        {            
            RoleInstanceState state;

            bool success = Enum.TryParse(managementRoleInstanceStatus.ToString(), out state);
            if (!success)
            {
                string message = "Unable to convert {0}.{1} to {2}".ToString(
                    typeof (ManagementRoleInstanceStatus).Name,
                    managementRoleInstanceStatus,
                    typeof (RoleInstanceState));

                TraceType.WriteError(message);
                throw new ArgumentException(message);
            }

            return state;
        }

        /// <summary>
        /// Inspects the impact on each role instance and determines if the impact is severe enough
        /// for manual approval.
        /// </summary>
        /// <param name="notification">The management notification provided by the management protocol (MR)</param>
        /// <returns>True if manual approval is required. False otherwise.</returns>
        public static bool IsManualApprovalRequired(this IManagementNotificationContext notification)
        {
            if (notification == null)
            {
                return false;
            }

            foreach (var impactReasons in notification.ImpactedInstances.Select(e => e.ImpactReasons))
            {
                foreach (var impactReason in impactReasons)
                {
                    switch (impactReason)
                    {
                        case ImpactReason.VendorRepairBegin:
                        case ImpactReason.VendorRepairEnd:
                            return true;
                    }
                }
            }

            return false;
        }

        public static bool Matches(this IManagementNotificationContext notification, string jobId, int upgradeDomain)
        {
            return
                notification != null &&
                notification.ActiveJobId.Equals(jobId, StringComparison.OrdinalIgnoreCase) &&
                notification.ActiveJobStepTargetUD == upgradeDomain;
        }

        public static string ToShortDisplayString(this IManagementNotificationContext notification)
        {
            if (notification == null)
            {
                return "<null>";
            }

            return String.Format(
                CultureInfo.InvariantCulture,
                "{0}:{1}:{2}:{3}",
                notification.NotificationType,
                notification.ActiveJobType,
                notification.ActiveJobId,
                notification.ActiveJobStepTargetUD);
        }
    }
}