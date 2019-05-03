// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;

ILoggingReplicator::SPtr Factory::Create(
    __in PartitionedReplicaId const & traceId,
    __in IRuntimeFolders const & runtimeFolders,
    __in KString const & logDirectory,
    __in IStateReplicator & stateReplicator,
    __in IStateProviderManager & stateManager,
    __in IStatefulPartition & partition,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in SLInternalSettingsSPtr const & ktlLoggerSharedLogConfig,
    __in Data::Log::LogManager & logManager,
    __in IDataLossHandler & dataLossHandler,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in ITransactionalReplicator & transactionalReplicator,
    __in bool hasPersistedState,
    __in KAllocator& allocator,
    __out IStateProvider::SPtr & stateProvider)
{
    LoggingReplicatorImpl::SPtr result = LoggingReplicatorImpl::Create(
        traceId,
        runtimeFolders,
        partition,
        stateReplicator,
        stateManager,
        logDirectory,
        transactionalReplicatorConfig,
        ktlLoggerSharedLogConfig,
        logManager,
        dataLossHandler,
        perfCounters,
        healthClient,
        transactionalReplicator,
        hasPersistedState,
        allocator,
        stateProvider);
    
    ASSERT_IFNOT(
        NT_SUCCESS(result->Status()),
        "{0}: Failed to create LoggingReplicatorImpl with {1:x}",
        traceId.TraceId,
        result->Status());

    return ILoggingReplicator::SPtr(result.RawPtr());
}
