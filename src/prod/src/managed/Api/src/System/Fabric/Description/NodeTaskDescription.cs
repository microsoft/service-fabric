// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;

    internal class NodeTaskDescription
    {
        public string NodeName { get; set; }

        public NodeTask TaskType { get; set; }

        public static bool AreEqual(ReadOnlyCollection<NodeTaskDescription> left, ReadOnlyCollection<NodeTaskDescription> right)
        {
            bool equals = (left.Count == right.Count);
            if (!equals) { return equals; }

            var sortedLeft = new List<NodeTaskDescription>(left);
            var sortedRight = new List<NodeTaskDescription>(right);

            sortedLeft.Sort((NodeTaskDescription l, NodeTaskDescription r) => { return l.NodeName.CompareTo(r.NodeName); });
            sortedRight.Sort((NodeTaskDescription l, NodeTaskDescription r) => { return l.NodeName.CompareTo(r.NodeName); });

            for (int ix = 0; ix < sortedLeft.Count; ++ix)
            {
                if (sortedLeft[ix].NodeName != sortedRight[ix].NodeName ||
                    sortedLeft[ix].TaskType != sortedRight[ix].TaskType)
                {
                    return false;
                }
            }

            return true;
        }
    }
}