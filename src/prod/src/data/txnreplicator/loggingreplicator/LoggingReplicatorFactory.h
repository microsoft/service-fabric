// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class Factory
        {
        public:

            static TxnReplicator::ILoggingReplicator::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::IRuntimeFolders const & runtimeFolders,
                __in KString const & logDirectory,
                __in IStateReplicator & stateReplicator,
                __in TxnReplicator::IStateProviderManager & stateManager,
                __in Data::Utilities::IStatefulPartition & partition,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in TxnReplicator::SLInternalSettingsSPtr const & ktlLoggerSharedLogConfig,
                __in Data::Log::LogManager & logManager,
                __in TxnReplicator::IDataLossHandler & dataLossHandler,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
                __in bool hasPersistedState,
                __in KAllocator& allocator,
                __out IStateProvider::SPtr & stateProvider);
        };
    }
}
