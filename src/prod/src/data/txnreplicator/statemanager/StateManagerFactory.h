// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        class Factory
        {
        public:
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::IRuntimeFolders & runtimeFolders, 
                __in Data::Utilities::IStatefulPartition & partition,
                __in IStateProvider2Factory & stateProviderFactory,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in bool hasPersistedState,
                __in KAllocator & allocator,
                __out IStateManager::SPtr & stateManager);
        };
    }
}
