// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        interface IPhysicalLogReader 
            : Utilities::IDisposable
        {
            K_SHARED_INTERFACE(IPhysicalLogReader)

        public:

            __declspec(property(get = get_StartingRecordPosition)) ULONG64 StartingRecordPosition;
            virtual ULONG64 get_StartingRecordPosition() const = 0;

            __declspec(property(get = get_EndingRecordPosition)) ULONG64 EndingRecordPosition;
            virtual ULONG64 get_EndingRecordPosition() const = 0;

            __declspec(property(get = get_IsValid)) bool IsValid;
            virtual bool get_IsValid() const = 0;

            virtual Utilities::IAsyncEnumerator<LogRecord::SPtr>::SPtr GetLogRecords(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KStringView const & readerName,
                __in LogReaderType::Enum readerType,
                __in LONG64 readAheadCacheSizeBytes,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig) = 0;
        };
    }
}
