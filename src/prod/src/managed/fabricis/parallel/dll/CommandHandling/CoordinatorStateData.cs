// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{    
    using Collections.Generic;

    /// <summary>
    /// Class that provides data to the Powershell window. The same structure is also duplicated
    /// by Service Fabric Explorer (SFE) for display purposes.
    /// TODO Update SFE code.
    /// </summary>
    internal sealed class CoordinatorStateData
    {
        private const string CoordinatorMode = "Parallel";

        /// <summary>
        /// The mode of operation of the Infrastructure Service coordinator.
        /// </summary>
        public string Mode { get; set; }

        public string AzureTenantId { get; set; }

        public string AssemblyFileVersion { get; set; }

        public string CoordinatorStartTime { get; set; }

        public string LastRunStartTime { get; set; }

        public string LastRunFinishTime { get; set; }

        public string JobBlockingPolicy { get; set; }

        public string JobDocumentIncarnation { get; set; }

        public List<JobSummary> Jobs { get; set; }

        // TODO: Expose these fields later, once support for job sets is added, and AllowAction command is ready
        // public List<List<string>> JobSetsRequiringApproval { get; set; }
        // public List<AllowActionRecord> OperatorAllowedActions { get; set; }

        public CoordinatorStateData()
        {
            Mode = CoordinatorMode;
            Jobs = new List<JobSummary>();
            //OperatorAllowedActions = new List<AllowActionRecord>();
        }
    }
}