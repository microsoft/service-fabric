// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using Collections.Generic;
    using Threading.Tasks;

    internal class MockQueryClient : IQueryClient
    {
        public MockQueryClient(IList<INode> nodes = null)
        {
            Nodes = nodes ?? new List<INode>();
        }

        public IList<INode> Nodes { get; set; }

        public virtual async Task<IList<INode>> GetNodeListAsync(DateTimeOffset? referenceTime = null)
        {
            return await Task.FromResult(Nodes);
        }
    }
}