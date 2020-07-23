// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using RD.Fabric.PolicyAgent;

    internal interface IImpactInfo
    {
        ImpactActionEnum ImpactAction { get; }

        AffectedResourceImpact ImpactedResources { get; }
    }
}