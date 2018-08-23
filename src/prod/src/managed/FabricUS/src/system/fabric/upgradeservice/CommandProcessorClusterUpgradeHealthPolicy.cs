// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
namespace System.Fabric.UpgradeService
{
    public class CommandProcessorClusterUpgradeHealthPolicy
    {
        public byte MaxPercentUnhealthyNodes { get; set; }

        public byte MaxPercentUnhealthyApplications { get; set; }

        public Dictionary<string, CommandProcessorApplicationHealthPolicy> ApplicationHealthPolicies { get; set; }
    }

    public class CommandProcessorClusterUpgradeDeltaHealthPolicy
    {
        public byte MaxPercentDeltaUnhealthyNodes { get; set; }

        public byte MaxPercentUpgradeDomainDeltaUnhealthyNodes { get; set; }        
    }

    public class CommandProcessorApplicationHealthPolicy
    {
        public CommandProcessorServiceTypeHealthPolicy DefaultServiceTypeHealthPolicy { get; set; }

        public Dictionary<string, CommandProcessorServiceTypeHealthPolicy> SerivceTypeHealthPolicies { get; set; }
    }

    public class CommandProcessorServiceTypeHealthPolicy
    {
        public byte MaxPercentUnhealthyServices { get; set; }        
    }    
}