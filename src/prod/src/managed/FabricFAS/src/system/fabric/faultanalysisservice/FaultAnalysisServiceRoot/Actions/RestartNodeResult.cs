// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Service
{
    using System.Fabric.Common;
    using System.Fabric.Testability2;

    /// <summary>
    /// Returns Restart node result object.
    /// </summary>
    /// <remarks>
    /// This class returns the SelectedReplica and NodeResult for RestartNode Action.  
    /// </remarks>
    [Serializable]
    public class RestartNodeResult
    {
        internal RestartNodeResult(SelectedReplica selectedReplica, NodeResult nodeResult)
        {
            ReleaseAssert.AssertIfNull(nodeResult, "Node result cannot be null");
            ReleaseAssert.AssertIfNull(selectedReplica, "Selected replica cannot be null"); 
            this.NodeResult = nodeResult;
            this.SelectedReplica = selectedReplica;
        }

        /// <summary>
        /// Gets the NodeResult.
        /// </summary>
        public NodeResult NodeResult { get; private set; }

        /// <summary>
        /// Gets the SelectedReplica.
        /// SelectedReplica will be none in case the RestartNode Testability action was performed using node name.
        /// </summary>
        public SelectedReplica SelectedReplica { get; private set; }
    }
}