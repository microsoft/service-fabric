// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.ClusterAnalysis.Configuration
{
    using System.Collections.Generic;

    public sealed class ClusterAnalysisConfiguration
    {
        public ClusterAnalysisConfiguration()
        {
            this.AgentConfiguration = new List<AnalysisAgentConfiguration>();
        }

        public IList<AnalysisAgentConfiguration> AgentConfiguration { get; set; }
    }
}