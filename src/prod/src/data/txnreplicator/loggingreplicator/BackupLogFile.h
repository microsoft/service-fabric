// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class BackupLogFile final
            : public KObject<BackupLogFile>
            , public KShared<BackupLogFile>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(BackupLogFile)

        public: // Static functions.
            static SPtr Create(
                __in Utilities::PartitionedReplicaId const& traceId,
                __in KWString const & filePath,
                __in KAllocator & allocator);

        public: // Properties
            __declspec(property(get = get_FileName)) KWString const FileName;
            KWString const BackupLogFile::get_FileName() const;

            __declspec(property(get = get_Size)) LONG64 Size;
            LONG64 BackupLogFile::get_Size() const;

            __declspec(property(get = get_SizeInKB)) ULONG32 SizeInKB;
            ULONG32 BackupLogFile::get_SizeInKB() const;

            __declspec(property(get = get_WriteTimeInMilliseconds)) LONG64 WriteTimeInMilliseconds;
            LONG64 BackupLogFile::get_WriteTimeInMilliseconds() const;

            __declspec(property(get = get_Count)) ULONG32 Count;
            ULONG32 BackupLogFile::get_Count() const;

            __declspec(property(get = get_IndexingRecordEpoch)) TxnReplicator::Epoch IndexingRecordEpoch;
            TxnReplicator::Epoch BackupLogFile::get_IndexingRecordEpoch() const;

            __declspec(property(get = get_IndexingRecordLSN)) FABRIC_SEQUENCE_NUMBER IndexingRecordLSN;
            FABRIC_SEQUENCE_NUMBER BackupLogFile::get_IndexingRecordLSN() const;

            __declspec(property(get = get_LastBackedUpEpoch)) TxnReplicator::Epoch LastBackedUpEpoch;
            TxnReplicator::Epoch BackupLogFile::get_LastBackedUpEpoch() const;

            __declspec(property(get = get_LastBackedUpLSN)) FABRIC_SEQUENCE_NUMBER LastBackedUpLSN;
            FABRIC_SEQUENCE_NUMBER BackupLogFile::get_LastBackedUpLSN() const;

        public: 
            // Create a new CheckpointFile and write it to the given file.    
            ktl::Awaitable<void> WriteAsync(
                __in Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr> & logRecords,
                __in LogRecordLib::BackupLogRecord const & backupLogRecord,
                __in ktl::CancellationToken const & cancellationToken);

            // Open a existing CheckpointFile
            ktl::Awaitable<void> ReadAsync(
                __in ktl::CancellationToken const & cancellationToken);

        public:
            BackupLogFileAsyncEnumerator::SPtr GetAsyncEnumerator() const;

            ktl::Awaitable<void> VerifyAsync();

        private: // Constructor
            BackupLogFile::BackupLogFile(
                __in Utilities::PartitionedReplicaId const& traceId,
                __in KWString const & filePath) noexcept;

        private: // Private functions
            ktl::Awaitable<void> WriteLogRecordsAsync(
                __in Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr> & logRecords,
                __in ktl::CancellationToken const& cancellationToken,
                __inout ktl::io::KStream & outputStream);

            ktl::Awaitable<void> WriteLogRecordsAsync(
                __in IncrementalBackupLogRecordsAsyncEnumerator & logRecords,
                __in ktl::CancellationToken const& cancellationToken,
                __inout ktl::io::KStream & outputStream);

            ktl::Awaitable<void> WriteLogRecordBlockAsync(
                __in ktl::CancellationToken const& cancellationToken,
                __in ULONG32 blockSize,
                __inout Utilities::BinaryWriter & binaryWriter,
                __inout KArray<Utilities::OperationData::CSPtr> & operationDataArray,
                __inout ktl::io::KStream & outputStream);

            ktl::Awaitable<void> WriteFooterAsync(
                __in ktl::io::KFileStream & fileStream,
                __in Data::Utilities::BlockHandle & propertiesBlockHandle,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<void> ReadFooterAsync(
                __in ktl::io::KFileStream & fileStream,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<Utilities::BlockHandle::SPtr> WritePropertiesAsync(
                __in ktl::io::KFileStream & fileStream,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<void> ReadPropertiesAsync(
                __in ktl::io::KFileStream & fileStream,
                __in ktl::CancellationToken const & cancellationToken);

        private: // Constants.
            static const ULONG32 NumberOfBytesInKB = 1024;
            static const ULONG32 InitialSizeOfMemoryStream = 32 * NumberOfBytesInKB;
            static const ULONG32 MinimumIntermediateFlushSize = InitialSizeOfMemoryStream;
            static const ULONG32 Version = 1;
            static const UINT8 BlockSizeSectionSize = sizeof(ULONG32);
            static const UINT8 CheckSumSectionSize = sizeof(ULONG64);

        private: // Initializer list initialized.
            KWString filePath_;

        private: // Default initialized
            LONG64 size_ = 0;
            LONG64 writeTimeInMilliseconds_ = 0;

        private: // Set later.
            // propertiesSPtr is the file properties
            BackupLogFileProperties::SPtr propertiesSPtr_;

            // footerSPtr has the infomation about version and offset and size of properties
            Utilities::FileFooter::SPtr footerSPtr_;
        };
    }
}
