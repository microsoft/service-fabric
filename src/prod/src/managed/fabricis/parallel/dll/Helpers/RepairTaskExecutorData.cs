// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Linq;

    /// <summary>
    /// A template for creating a repair task executor data so that the various 
    /// components of the infrastructure service can use it.
    /// </summary>
    /// <remarks>
    /// Setters are not strictly needed since we can initialize them with a constructor. 
    /// Keeping it this way for JSON serialization to work. It is okay since this is just
    /// an internal class.
    /// TODO, have a different Json serializersetting
    /// </remarks>
    internal class RepairTaskExecutorData
    {
        public const string VendorRepairFlag = "VendorRepair";

        /// <summary>
        /// The <see cref="ITenantJob"/> Id corresponding to the repair task.
        /// </summary>
        public string JobId { get; set; }

        /// <summary>
        /// The <see cref="ITenantJob"/> UD corresponding to the repair task.
        /// </summary>        
        public uint? UD { get; set; }

        public string StepId { get; set; }

        public string Flags { get; set; }

        public override string ToString()
        {
            string text = 
                "Job Id: {0}, UD: {1}, StepId: {2}, Flags: {3}".ToString(JobId, UD, StepId, Flags);
            return text;
        }

        public bool HasFlag(string flag)
        {
            if (this.Flags == null)
                return false;

            return this.Flags.Split(',').Contains(flag, StringComparer.OrdinalIgnoreCase);
        }
    }
}