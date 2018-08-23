// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// Class used for simplifying passing in parameters from clients. 
    /// Currently this exists just for testability client (test harness) to invoke via REST
    /// and hence is marked as internal
    /// </summary>
    internal sealed class RepairTaskHealthPolicyUpdateDescription
    {
        internal RepairTaskHealthPolicyUpdateDescription(
            RepairScopeIdentifier scope,
            string taskId,
            Int64 version,
            bool? performPreparingHealthCheck,
            bool? performRestoringHealthCheck)
        {
            this.Scope = scope;
            this.TaskId = taskId;
            this.Version = version;
            this.PerformPreparingHealthCheck = performPreparingHealthCheck;
            this.PerformRestoringHealthCheck = performRestoringHealthCheck;
        }

        public RepairScopeIdentifier Scope { get; set; }

        public string TaskId{ get; set; }

        public Int64 Version { get; set; }

        public bool? PerformPreparingHealthCheck { get; set; }

        public bool? PerformRestoringHealthCheck { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var requestDescription = new NativeTypes.FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_DESCRIPTION
            {
                Scope = this.Scope == null ? IntPtr.Zero : this.Scope.ToNative(pinCollection),
                RepairTaskId = pinCollection.AddBlittable(this.TaskId),
                Version = this.Version,
            };

            var flags =
                NativeTypes.FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_SETTINGS_FLAGS
                    .FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_SETTINGS_NONE;

            if (this.PerformPreparingHealthCheck.HasValue)
            {
                flags |= NativeTypes.FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_SETTINGS_FLAGS.FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_SETTINGS_HONOR_PERFORM_PREPARING_HEALTH_CHECK;
                requestDescription.PerformPreparingHealthCheck = NativeTypes.ToBOOLEAN(this.PerformPreparingHealthCheck.Value);
            }

            if (this.PerformRestoringHealthCheck.HasValue)
            {
                flags |= NativeTypes.FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_SETTINGS_FLAGS.FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_SETTINGS_HONOR_PERFORM_RESTORING_HEALTH_CHECK;
                requestDescription.PerformRestoringHealthCheck = NativeTypes.ToBOOLEAN(this.PerformRestoringHealthCheck.Value);
            }

            requestDescription.Flags = (uint)flags;
            return pinCollection.AddBlittable(requestDescription);
        }
    }
}