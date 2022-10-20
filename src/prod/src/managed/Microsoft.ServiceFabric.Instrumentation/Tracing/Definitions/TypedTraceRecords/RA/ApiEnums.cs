// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA
{
    public enum ApiInterface : uint
    {
        Invalid = 0,

        IStatefulServiceReplica = 1,

        IStatelessServiceInstance = 2,

        IStateProvider = 3,

        IReplicator = 4,
    }

    public enum ApiName : uint
    {
        Invalid = 0,

        // General lifecycle APIs
        Open = 1,
        Close = 2,
        Abort = 3,

        // IStateProvider Interface API's
        GetNextCopyState = 4,
        GetNextCopyContext = 5,
        UpdateEpoch = 6,
        OnDataLoss = 7,

        // IStatefulServiceReplica API's
        ChangeRole = 8,

        // IReplicator APIs 
        CatchupReplicaSet = 9,
        BuildReplica = 10,
        RemoveReplica = 11,
        GetStatus = 12,
        UpdateCatchupConfiguration = 13,
        UpdateCurrentConfiguration = 14,
        GetQuery = 15,
    }
}