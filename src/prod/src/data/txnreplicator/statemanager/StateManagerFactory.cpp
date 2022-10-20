// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;
using namespace Data::StateManager;

NTSTATUS Factory::Create(
    __in PartitionedReplicaId const & traceId, 
    __in TxnReplicator::IRuntimeFolders & runtimeFolders, 
    __in IStatefulPartition & partition, 
    __in IStateProvider2Factory & stateProviderFactory, 
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in bool hasPersistedState,
    __in KAllocator & allocator, 
    __out IStateManager::SPtr & stateManager)
{
    StateManager::SPtr tmpStateManager = nullptr;
    NTSTATUS status = StateManager::Create(
        traceId, 
        runtimeFolders, 
        partition, 
        stateProviderFactory, 
        transactionalReplicatorConfig,
        hasPersistedState,
        allocator, 
        tmpStateManager);

    if (NT_SUCCESS(status))
    {
        stateManager = tmpStateManager.RawPtr();
    }

    return status;
}
