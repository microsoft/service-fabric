// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    [Flags]
    internal enum ValidationCheckFlag
    {
        Default = 0x0000,
        CheckError = 0x0001,
        CheckWarning = 0x0002,
        CheckTargetReplicaSetSize = 0x0004,
        CheckInBuildReplica = 0x0008,
        CheckQuorumLoss = 0x0010,

        All = CheckError | CheckWarning | CheckTargetReplicaSetSize | CheckInBuildReplica | CheckQuorumLoss,
        AllButQuorumLoss = CheckError | CheckWarning | CheckTargetReplicaSetSize | CheckInBuildReplica
    }
}