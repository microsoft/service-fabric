// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    /// <summary>
    /// Returns Restart replica result object.
    /// </summary>
    /// <remarks>
    /// This class returns the selected replica for which Restart Replica actions was called.
    /// </remarks>
    [Serializable]
    public class RestartReplicaResult : ReplicaResult
    {
        /// <summary>
        /// Restart replica result constructor.
        /// </summary>
        /// <param name="selectedReplica">Selected replica</param>
        internal RestartReplicaResult(SelectedReplica selectedReplica) : base(selectedReplica)
        {
        }
    }
}