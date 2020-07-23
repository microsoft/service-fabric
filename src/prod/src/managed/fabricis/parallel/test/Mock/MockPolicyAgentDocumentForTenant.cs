// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    internal sealed class MockPolicyAgentDocumentForTenant : IPolicyAgentDocumentForTenant
    {
        public MockPolicyAgentDocumentForTenant()
        {
            Jobs = new List<ITenantJob>();
            JobSetsRequiringApproval = new List<List<string>>();
            RoleInstanceHealthInfos = new List<RoleInstanceHealthInfo>();
        }

        public string Incarnation { get; set; }
        public int JobDocumentIncarnation { get; set; }
        public List<ITenantJob> Jobs { get; set; }
        public List<List<string>> JobSetsRequiringApproval { get; set; }
        public ulong RoleInstanceHealthInfoIncarnation { get; set; }
        public string RoleInstanceHealthInfoTimestamp { get; set; }
        public List<RoleInstanceHealthInfo> RoleInstanceHealthInfos { get; set; }

        public override string ToString()
        {
            string text = "Incarnation: {0}, Jobs: {1}, Job doc incarnation: {2}, RIs: {3}, RI health info incarnation: {4}, RI health info timestamp: {5}".ToString(
                Incarnation, Jobs.Count, JobDocumentIncarnation, RoleInstanceHealthInfos.Count, RoleInstanceHealthInfoIncarnation, RoleInstanceHealthInfoTimestamp);
            return text;
        }
    }
}