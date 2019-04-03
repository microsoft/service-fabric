// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    public interface IStatefulServicePartitionEx : IStatefulServicePartition
    {
        bool IsPersisted { get; }

        FabricReplicatorEx CreateReplicatorEx(
            IAtomicGroupStateProviderEx stateProvider,
            ReplicatorSettings replicatorSettings,
            ReplicatorLogSettings logSettings);
    }
}