// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    /// <remarks>
    /// We are copying the policy agent doc instead of just wrapping them (like in ServiceFabricRepairTask). 
    /// This is because of the foll.
    /// a. There are IBonded structures that need to be deserialized upfront.
    /// b. Bond lists return null. Here we are sanitizing them to empty lists.
    /// c. We are wrapping up lists with their wrapped up objects. E.g. this.Jobs contains 
    ///    new TenantJobWrapper objects and not the TenantJob objects anymore.
    /// </remarks>
    internal sealed class PolicyAgentDocumentForTenantWrapper : IPolicyAgentDocumentForTenant
    {
        public PolicyAgentDocumentForTenantWrapper(PolicyAgentDocumentForTenant doc)
        {
            doc.Validate("doc");

            this.Incarnation = doc.Incarnation;
            this.Jobs = new List<ITenantJob>();
            this.JobSetsRequiringApproval = new List<List<string>>();
            this.RoleInstanceHealthInfos = new List<RoleInstanceHealthInfo>();

            if (doc.JobInfo != null)
            {
                var tenantJobInfo = doc.JobInfo.Deserialize();
                this.JobDocumentIncarnation = tenantJobInfo.DocumentIncarnation;

                if (tenantJobInfo.Jobs != null)
                {
                    foreach (var job in tenantJobInfo.Jobs)
                    {
                        this.Jobs.Add(new TenantJobWrapper(job));
                    }
                }

                if (tenantJobInfo.SetOfJobListsForAnyOfApproval != null)
                {
                    foreach (var anyOneOfJobsToApprove in tenantJobInfo.SetOfJobListsForAnyOfApproval)
                    {
                        var jobset = new List<string>();

                        if (anyOneOfJobsToApprove.JobList != null)
                        {
                            foreach (var bondId in anyOneOfJobsToApprove.JobList)
                            {
                                var id = bondId.ToGuid().ToString();
                                jobset.Add(id);
                            }
                        }

                        JobSetsRequiringApproval.Add(jobset);
                    }
                }
            }

            if (doc.RoleInstanceHealthInfo != null)
            {
                var roleInstanceHealthInfo = doc.RoleInstanceHealthInfo.Deserialize();
                this.RoleInstanceHealthInfoIncarnation = roleInstanceHealthInfo.Incarnation;
                this.RoleInstanceHealthInfoTimestamp = roleInstanceHealthInfo.Timestamp;

                if (roleInstanceHealthInfo.RoleInstanceHealth != null)
                {
                    this.RoleInstanceHealthInfos = roleInstanceHealthInfo.RoleInstanceHealth;
                }
            }
        }

        public string Incarnation { get; private set; }

        public int JobDocumentIncarnation { get; private set; }
        
        public List<ITenantJob> Jobs { get; private set; }

        public List<List<string>> JobSetsRequiringApproval { get; private set; }

        public ulong RoleInstanceHealthInfoIncarnation { get; private set; }

        public string RoleInstanceHealthInfoTimestamp { get; private set; }

        public List<RoleInstanceHealthInfo> RoleInstanceHealthInfos { get; private set; }

        public override string ToString()
        {
            string text = "Incarnation: {0}, Jobs: {1}, Job doc incarnation: {2}, RIs: {3}, RI health info incarnation: {4}, RI health info timestamp: {5}".ToString(
                Incarnation, Jobs.Count, JobDocumentIncarnation, RoleInstanceHealthInfos, RoleInstanceHealthInfoIncarnation, RoleInstanceHealthInfoTimestamp);
            return text;
        }
    }
}