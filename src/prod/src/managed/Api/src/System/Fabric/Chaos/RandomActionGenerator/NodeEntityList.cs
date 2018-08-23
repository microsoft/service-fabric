// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;
    using System.Text;

    internal class NodeEntityList : IReadOnlyCollection<NodeEntity>
    {
        private IList<NodeEntity> list;
        private ClusterStateSnapshot stateSnapshot;

        public NodeEntityList(ClusterStateSnapshot stateSnapshot)
        {
            this.list = new List<NodeEntity>();
            this.stateSnapshot = stateSnapshot;
        }

        public void AddNodes(IEnumerable<NodeInfo> nodeList)
        {
            if (nodeList != null)
            {
                foreach (var node in nodeList)
                {
                    this.list.Add(new NodeEntity(node, this.stateSnapshot));
                }
            }
        }

        public NodeEntity this[int index]
        {
            get
            {
                return this.list[index];
            }
        }

        public int Count
        {
            get { return this.list.Count; }
        }

        public IEnumerator<NodeEntity> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            foreach (var node in this.list)
            {
                sb.AppendLine(node.ToString());
            }

            return sb.ToString();
        }

        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        internal NodeEntity FindMatchingNodeEntity(string nodeName)
        {
            Requires.Argument("nodeName", nodeName).NotNullOrEmpty();

            return this.list.FirstOrDefault(n => ClusterStateSnapshot.MatchNodesByNameOrId(n.CurrentNodeInfo.NodeName, nodeName));
        }
    }
}