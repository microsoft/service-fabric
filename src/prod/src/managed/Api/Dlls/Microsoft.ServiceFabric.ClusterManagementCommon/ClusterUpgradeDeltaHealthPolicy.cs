// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;

    public class ClusterUpgradeDeltaHealthPolicy
    {
        // Node health
        public byte MaxPercentDeltaUnhealthyNodes { get; set; }

        public byte MaxPercentUpgradeDomainDeltaUnhealthyNodes { get; set; }

        // Application health
        public byte MaxPercentDeltaUnhealthyApplications { get; set; }
    }
}