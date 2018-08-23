// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        interface IReplicatedLogManager
        {
            K_SHARED_INTERFACE(IReplicatedLogManager);

        public:
            __declspec(property(get = get_CurrentLogTailLsn)) FABRIC_SEQUENCE_NUMBER CurrentLogTailLsn;
            virtual FABRIC_SEQUENCE_NUMBER get_CurrentLogTailLsn() const = 0;

            __declspec(property(get = get_CurrentLogTailEpoch)) TxnReplicator::Epoch CurrentLogTailEpoch;
            virtual TxnReplicator::Epoch get_CurrentLogTailEpoch() const = 0;

            __declspec(property(get = get_LastInformationRecord)) LogRecordLib::InformationLogRecord::SPtr LastInformationRecord;
            virtual LogRecordLib::InformationLogRecord::SPtr get_LastInformationRecord() const = 0;

            __declspec(property(get = get_LastCompletedEndCheckpointRecord)) LogRecordLib::EndCheckpointLogRecord::SPtr LastCompletedEndCheckpointRecord;
            virtual LogRecordLib::EndCheckpointLogRecord::SPtr get_LastCompletedEndCheckpointRecord() const = 0;

            __declspec(property(get = get_ProgressVectorValue)) LogRecordLib::ProgressVector::SPtr ProgressVectorValue;
            virtual LogRecordLib::ProgressVector::SPtr get_ProgressVectorValue() const = 0;

            virtual TxnReplicator::Epoch GetEpoch(
                __in FABRIC_SEQUENCE_NUMBER LSN) const = 0;

            virtual LogRecordLib::IndexingLogRecord::CSPtr GetIndexingLogRecordForBackup() const = 0;

            virtual NTSTATUS ReplicateAndLog(
                __in LogRecordLib::LogicalLogRecord & record,
                __out LONG64 & bufferedBytes,
                __in_opt TransactionManager * const transactionManager = nullptr) = 0;

            virtual void Information(
                __in LogRecordLib::InformationEvent::Enum type) = 0;
        };
    }
}
