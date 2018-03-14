// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        //
        // Implementation of the IPhysicalLogReader interface
        // Primarily used during copy/backup
        //
        class PhysicalLogReader
            : public IPhysicalLogReader
            , public KObject<PhysicalLogReader>
            , public KShared<PhysicalLogReader>
        {

            K_FORCE_SHARED_WITH_INHERITANCE(PhysicalLogReader)
            K_SHARED_INTERFACE_IMP(IDisposable)
            K_SHARED_INTERFACE_IMP(IPhysicalLogReader)

        public:

             static IPhysicalLogReader::SPtr Create(
                __in ILogManagerReadOnly & logManager,
                __in ULONG64 startingRecordPosition,
                __in ULONG64 endingRecordPosition,
                __in LONG64 startLsn,
                __in KStringView const & readerName,
                __in LogReaderType::Enum readerType,
                __in KAllocator & allocator);
 
            ULONG64 get_StartingRecordPosition() const override
            {
                return startingRecordPosition_;
            }

            ULONG64 get_EndingRecordPosition() const override
            {
                return endingRecordPosition_;
            }

            bool get_IsValid() const override
            {
                return isValid_;
            }

            virtual void Dispose() override;

            Utilities::IAsyncEnumerator<LogRecord::SPtr>::SPtr GetLogRecords(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KStringView const & readerName,
                __in LogReaderType::Enum readerType,
                __in LONG64 readAheadCacheSizeBytes,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig) override;
        
        protected:

            PhysicalLogReader(__in ILogManagerReadOnly & logManager);

            __declspec(property(get = get_IsDisposed)) bool IsDisposed;
            bool get_IsDisposed() const
            {
                return isDisposed_;
            }

            ILogManagerReadOnly::SPtr const logManager_;
            bool const isValid_;

            ULONG64 startingRecordPosition_;
            ULONG64 endingRecordPosition_;
            LONG64 startLsn_;
            KLocalString<Constants::LogReaderNameMaxLength> readerName_;
            LogReaderType::Enum readerType_;

        private:
            
            // Ensures derived classes do not set this variable
            bool isDisposed_;

            PhysicalLogReader(
                __in ILogManagerReadOnly & logManager,
                __in ULONG64 startingRecordPosition,
                __in ULONG64 endingRecordPosition,
                __in LONG64 startLsn,
                __in KStringView const & readerName,
                __in LogReaderType::Enum readerType);
        };
    }
}
