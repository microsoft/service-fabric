// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;
    using Threading.Tasks;
    using System.Fabric.Query;    

    internal class ServiceFabricQueryClient : IQueryClient
    {
        private readonly FabricClient.QueryClient queryClient = new FabricClient().QueryManager;

        private readonly TraceType traceType;

        public ServiceFabricQueryClient(TraceType traceType)
        {
            this.traceType = traceType.Validate("traceType");
        }

        public async Task<IList<INode>> GetNodeListAsync(DateTimeOffset? referenceTime = null)
        {
            var now = referenceTime ?? DateTimeOffset.UtcNow;
            NodeList nodelist;

            try
            {
                nodelist = await queryClient.GetNodeListAsync().ConfigureAwait(false);            
            }
            catch (Exception ex)
            {
                traceType.WriteError("Unable to get node list. Errors: {0}", ex.GetMessage());
                throw;
            }

            var nodes = new List<INode>();
            foreach (var node in nodelist)
            {
                var simpleNode = new SimpleNode
                {
                    NodeName = node.NodeName,
                    NodeUpTimestamp = node.NodeUpAt == DateTime.Now
                        ? (DateTimeOffset?) null
                        : now - (DateTime.Now - node.NodeUpAt),
                };

                nodes.Add(simpleNode);
            }

            return nodes;
        }

        private class SimpleNode : INode
        {
            public string NodeName { get; set; }

            public DateTimeOffset? NodeUpTimestamp { get; set; }

            public override string ToString()
            {
                return "{0}, NodeUpTimestamp: {1}".ToString(NodeName, NodeUpTimestamp != null ? NodeUpTimestamp.Value.ToString("o") : "<null>");
            }
        }
    }
}