// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    /// <summary>
    /// Returns Remove replica result object.
    /// </summary>
    /// <remarks>
    /// This class returns the selected replica for which remove action was called.
    /// </remarks>
    [Serializable]
    public class RemoveReplicaResult : ReplicaResult
    {
        /// <summary>
        /// Remove replica result constructor.
        /// </summary>
        /// <param name="selectedReplica">Selected replica</param>
        internal RemoveReplicaResult(SelectedReplica selectedReplica) : base(selectedReplica)
        {
        }
    }
}