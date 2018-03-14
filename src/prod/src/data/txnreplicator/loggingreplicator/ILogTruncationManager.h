// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        interface ILogTruncationManager
        {
            K_SHARED_INTERFACE(ILogTruncationManager)

        public:

            virtual ktl::Awaitable<void> BlockSecondaryPumpIfNeeded(__in LONG64 lastStableLsn) = 0;

            virtual bool ShouldBlockOperationsOnPrimary() = 0;

            virtual KArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr> GetOldTransactions(__in TransactionMap & transactionMap) = 0;

            virtual bool ShouldIndex() = 0;

            virtual bool ShouldCheckpointOnPrimary(
                __in TransactionMap & transactionMap,
                __out KArray<LogRecordLib::BeginTransactionOperationLogRecord::SPtr> & abortTxList) = 0;

            virtual bool ShouldCheckpointOnSecondary(__in TransactionMap & transactionMap) = 0;

            virtual bool ShouldTruncateHead() = 0;
    
            virtual ReplicatedLogManager::IsGoodLogHeadCandidateCalculator GetGoodLogHeadCandidateCalculator() = 0;
        };
    }
}
