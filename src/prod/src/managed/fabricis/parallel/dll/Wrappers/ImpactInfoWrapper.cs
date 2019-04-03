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
    internal sealed class ImpactInfoWrapper : IImpactInfo
    {
        public ImpactActionEnum ImpactAction { get; private set; }

        public AffectedResourceImpact ImpactedResources { get; private set; }

        public ImpactInfoWrapper(ImpactInfo impactInfo)
        {
            impactInfo.Validate("impactInfo");

            this.ImpactAction = impactInfo.ImpactAction;

            this.ImpactedResources = impactInfo.ImpactedResources != null 
                ? impactInfo.ImpactedResources.Deserialize() 
                : new AffectedResourceImpact();

            if (this.ImpactedResources.ListOfImpactTypes == null)
            {
                this.ImpactedResources.ListOfImpactTypes = new List<ImpactTypeEnum>();
            }
        }

        public override string ToString()
        {
            string text = "ImpactAction: {0}, ImpactedResources: [Disk: {1}, Compute: {2}, OS: {3}, Network: {4}, AppConfig: {5}, EstimatedDuration: {6}s, ImpactTypes: {7}]".ToString(
                ImpactAction, ImpactedResources.DiskImpact, ImpactedResources.ComputeImpact, ImpactedResources.OSImpact, ImpactedResources.NetworkImpact, ImpactedResources.ApplicationConfigImpact,
                ImpactedResources.EstimatedImpactDurationInSeconds, string.Join(",", ImpactedResources.ListOfImpactTypes));
            return text;
        }
    }
}