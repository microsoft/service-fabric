// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Contains the recovered or full copied checkpoint lsn
    /// </summary>
    internal class RecoveredOrCopiedCheckpointLsn
    {
        public RecoveredOrCopiedCheckpointLsn()
        {
            this.Value = LogicalSequenceNumber.InvalidLsn;
        }

        public LogicalSequenceNumber Value
        {
            get; private set;
        }

        public void Update(LogicalSequenceNumber lsn)
        {
            this.Value = lsn;
        }
    }
}