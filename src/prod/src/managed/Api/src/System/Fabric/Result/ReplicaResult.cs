// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    using System.Fabric.Common;

    /// <summary>
    /// Replica result object. This class returns the SelectedReplica information. 
    /// </summary>
    [Serializable]
    public class ReplicaResult
    {
        /// <summary>
        /// Replica result constructor.
        /// </summary>
        /// <param name="selectedReplica">Selected replica</param>
        internal ReplicaResult(SelectedReplica selectedReplica)
        {
            ReleaseAssert.AssertIfNull(selectedReplica, "Selected replica cannot be null");
            this.SelectedReplica = selectedReplica;
        }

        /// <summary>
        /// Gets selected replica.
        /// </summary>
        /// <value>The SelectedReplica object.</value>
        public SelectedReplica SelectedReplica { get; private set; }
    }
}