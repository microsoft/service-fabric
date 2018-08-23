// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Threading;
    using System.Threading.Tasks;

    internal class FabricCluster
    {
        private readonly FabricClient fabricClient;

        public FabricCluster(FabricClient fabricClient)
        {
            this.fabricClient = fabricClient;
        }

        public async Task<IEnumerable<NodeInfo>> GetLatestNodeInfoAsync(TimeSpan requestTimeout, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            // Get all current known nodes
            NodeList nodeList = new NodeList();
            string continuationToken = null;

            do
            {
                NodeList queryResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                            () =>
                                                this.fabricClient.QueryManager.GetNodeListAsync(null, continuationToken),
                                                operationTimeout,
                                                cancellationToken).ConfigureAwait(false);

                nodeList.AddRangeNullSafe(queryResult);

                continuationToken = queryResult.ContinuationToken;

            } while ( !string.IsNullOrEmpty(continuationToken) );

            if (nodeList.Count == 0)
            {
                throw new InvalidOperationException(StringHelper.Format(StringResources.Error_NotEnoughNodesForTestabilityAction, "GetNodes"));
            }

            var nodes = new List<NodeInfo>();
            foreach (Node node in nodeList)
            {
                NodeInfo nodeInfo = NodeInfo.CreateNodeInfo(node);
                nodes.Add(nodeInfo);
            }

            return nodes;
        }

        /// <summary>
        /// Computes the minimum majority based write quorum size
        /// </summary>
        /// <param name="replicaSetSize">The size of the replica set whose write quorum size is to be calculated</param>
        /// <returns>The size of the write quorum</returns>
        /// <remarks>'replicaSetSize' can be an even or odd positive integer. If it is even, it has the form 2*m and the write quorum size is m+1 -- the minimum majority;
        /// if the replicaSetSize is odd, it has the form 2*m+1 and the write quorum size is again m+1 -- the minimum majority.</remarks>
        public static long GetWriteQuorumSize(long replicaSetSize)
        {
            if (replicaSetSize <= 0)
            {
                throw new ArgumentOutOfRangeException("replicaSetSize", replicaSetSize, "Size of the replica set must be a positive integer.");
            }

            return (replicaSetSize / 2) + 1;
        }
    }
}