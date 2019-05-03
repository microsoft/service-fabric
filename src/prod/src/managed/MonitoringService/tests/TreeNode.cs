// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Tests
{
    using System.Collections.Generic;
    using Health;

    internal class TreeNode : List<TreeNode>
    {
        internal TreeNode(EntityHealth health)
        {
            this.Health = health;
        }

        internal EntityHealth Health { get; private set; }

        public void Add(EntityHealth health)
        {
            this.Add(new TreeNode(health));
        }
    }
}