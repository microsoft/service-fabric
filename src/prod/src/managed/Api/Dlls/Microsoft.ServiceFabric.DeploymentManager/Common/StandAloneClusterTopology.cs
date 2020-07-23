// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Newtonsoft.Json;

    internal class StandAloneClusterTopology : IClusterTopology
    {
        [JsonConstructor]
        public StandAloneClusterTopology()
        {
        }

        public Dictionary<string, NodeDescription> Nodes
        {
            get;
            set;
        }

        internal List<string> Machines
        {
            get;
            set;
        }
    }
}