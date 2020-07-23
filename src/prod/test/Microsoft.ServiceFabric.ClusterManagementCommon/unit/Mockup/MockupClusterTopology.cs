// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System.Collections.Generic;

    internal class MockupClusterTopology : IClusterTopology
    {
        public Dictionary<string, NodeDescription> Nodes
        {
            get;
            set;
        }
    }
}