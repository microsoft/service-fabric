// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    internal sealed class MockRoleInstanceImpactedByJob : IRoleInstanceImpactedByJob
    {
        public string RoleInstanceName { get; set; }

        public uint UpdateDomain { get; set; }

        public AffectedResourceImpact ExpectedImpact { get; set; }

        public MockRoleInstanceImpactedByJob()
        {
            this.ExpectedImpact = new AffectedResourceImpact { ListOfImpactTypes = new List<ImpactTypeEnum>() };
        }

        public override string ToString()
        {
            string text = "{0}, UD: {1}, ExpectedImpact: [Disk: {2}, Compute: {3}, OS: {4}, Network: {5}, AppConfig: {6}, EstimatedDuration: {7}s, ImpactTypes: {8}]".ToString(
                RoleInstanceName, UpdateDomain, ExpectedImpact.DiskImpact, ExpectedImpact.ComputeImpact, ExpectedImpact.OSImpact, ExpectedImpact.NetworkImpact, ExpectedImpact.ApplicationConfigImpact,
                ExpectedImpact.EstimatedImpactDurationInSeconds, string.Join(",", ExpectedImpact.ListOfImpactTypes));
            return text;
        }
    }
}