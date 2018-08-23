//-----------------------------------------------------------------------
// <copyright file="ICheckpointManager.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    interface ICheckpointManager
    {
        BarrierLogRecord ReplicateBarrier();
    }
}
