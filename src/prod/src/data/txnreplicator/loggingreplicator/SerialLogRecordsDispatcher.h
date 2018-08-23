// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class SerialLogRecordsDispatcher 
            : public LogRecordsDispatcher
        {
            K_FORCE_SHARED(SerialLogRecordsDispatcher)

        public:

            static LogRecordsDispatcher::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in IOperationProcessor & processor,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in KAllocator & allocator);

        protected:

            ktl::Awaitable<bool> ProcessLoggedRecords() override;

        private:

            SerialLogRecordsDispatcher(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in IOperationProcessor & processor,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig);
        };
    }
}
