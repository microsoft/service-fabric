// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Collections.Generic;
    using Newtonsoft.Json;
    
    [JsonObject(IsReference = true)]
    public class ClusterNodeConfig
    {
        public ClusterNodeConfig()
        {
        }

        public ClusterNodeConfig(List<NodeStatus> nodesStatus, long version)
        {
            this.NodesStatus = nodesStatus;
            this.Version = version;
            this.IsUserSet = false;
        }

        public long Version { get; set; }

        public List<NodeStatus> NodesStatus { get; set; }

        public bool IsUserSet { get; set; }
    }
}