// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    internal sealed class MockImpactInfo : IImpactInfo
    {
        public MockImpactInfo()
        {
            ImpactedResources = new AffectedResourceImpact
            {
                ListOfImpactTypes = new List<ImpactTypeEnum>(),
            };
        }

        public ImpactActionEnum ImpactAction { get; set; }

        public AffectedResourceImpact ImpactedResources { get; set; }

        public override string ToString()
        {
            string text = "ImpactAction: {0}, ImpactedResources: [Disk: {1}, Compute: {2}, OS: {3}, Network: {4}, AppConfig: {5}, EstimatedDuration: {6}s, ImpactTypes: {7}]".ToString(
                ImpactAction, ImpactedResources.DiskImpact, ImpactedResources.ComputeImpact, ImpactedResources.OSImpact, ImpactedResources.NetworkImpact, ImpactedResources.ApplicationConfigImpact,
                ImpactedResources.EstimatedImpactDurationInSeconds, string.Join(",", ImpactedResources.ListOfImpactTypes));
            return text;
        }
    }
}