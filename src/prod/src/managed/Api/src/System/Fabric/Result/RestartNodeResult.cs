// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    using System.Fabric.Common;
    using System.Runtime.Serialization;

    /// <summary>
    /// Returns Restart node result object.
    /// </summary>
    /// <remarks>
    /// This class returns the SelectedReplica and NodeResult for RestartNode Action.  
    /// </remarks>
    [DataContract]
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
        /// <value>The NodeResult object.</value>
        [DataMember]
        public NodeResult NodeResult { get; internal set; }

        /// <summary>
        /// Gets the SelectedReplica.
        /// SelectedReplica will be none in case the RestartNode Testability action was performed using node name.
        /// </summary>
        /// <value>The SelectedReplica object.</value>
        [DataMember]
        public SelectedReplica SelectedReplica { get; internal set; }

        /// <summary>
        /// Formats NodeResult and SelectedReplica into a string.
        /// </summary>
        /// <returns>The formatted string.</returns>
        public override string ToString()
        {
            return string.Format("NodeResult: {0}, SelectedReplica: {1}", this.NodeResult, this.SelectedReplica);
        }
    }
}