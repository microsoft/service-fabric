// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    internal sealed class MockJobStepInfo : IJobStepInfo
    {
        public MockJobStepInfo()
        {
            CurrentlyImpactedRoleInstances = new List<IRoleInstanceImpactedByJob>();
        }

        public ImpactStepEnum ImpactStep { get; set; }
        public AcknowledgementStatusEnum AcknowledgementStatus { get; set; }
        public string DeadlineForResponse { get; set; }
        
        public List<IRoleInstanceImpactedByJob> CurrentlyImpactedRoleInstances { get; set; }

        public ImpactActionStatus ActionStatus { get; set; }

        public override string ToString()
        {
            string text = "{0}, {1}, Deadline: {2}, Impacted RIs: {3}, ActionStatus: {4}".ToString(
                ImpactStep, AcknowledgementStatus, DeadlineForResponse, CurrentlyImpactedRoleInstances.Count, ActionStatus);
            return text;
        }
    }
}