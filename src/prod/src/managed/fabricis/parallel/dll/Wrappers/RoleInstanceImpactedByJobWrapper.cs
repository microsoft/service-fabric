// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    /// <remarks>
    /// <see cref="PolicyAgentDocumentForTenantWrapper"/> on why we are copying instead of wrapping.
    /// </remarks>
    internal sealed class RoleInstanceImpactedByJobWrapper : IRoleInstanceImpactedByJob
    {
        public string RoleInstanceName { get; private set; }

        public uint UpdateDomain { get; private set; }

        public AffectedResourceImpact ExpectedImpact { get; private set; }

        public RoleInstanceImpactedByJobWrapper()
        {
            
        }

        public RoleInstanceImpactedByJobWrapper(RoleInstanceImpactedByJob roleInstanceImpactedByJob)            
        {
            roleInstanceImpactedByJob.Validate("roleInstanceImpactedByJob");

            this.RoleInstanceName = roleInstanceImpactedByJob.RoleInstanceName;
            this.UpdateDomain = roleInstanceImpactedByJob.UpdateDomain;

            this.ExpectedImpact = roleInstanceImpactedByJob.ExpectedImpact != null
                ? roleInstanceImpactedByJob.ExpectedImpact.Deserialize()
                : new AffectedResourceImpact();

            if (this.ExpectedImpact.ListOfImpactTypes == null)
            {
                this.ExpectedImpact.ListOfImpactTypes = new List<ImpactTypeEnum>();
            }
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