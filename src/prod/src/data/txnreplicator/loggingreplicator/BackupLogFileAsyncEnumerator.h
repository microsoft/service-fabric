// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        //
        // An abstraction for an iterator over log records that are created from a valid physical log reader
        //
        class BackupLogFileAsyncEnumerator
            : public KObject<BackupLogFileAsyncEnumerator>
            , public KShared<BackupLogFileAsyncEnumerator>
            , public Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr>
        {
            K_FORCE_SHARED(BackupLogFileAsyncEnumerator);
            K_SHARED_INTERFACE_IMP(IDisposable);
            K_SHARED_INTERFACE_IMP(IAsyncEnumerator);

        public: // Statics
            static SPtr Create(
                __in KWString const & fileName,
                __in ULONG32 logRecordCount,
                __in Utilities::BlockHandle & blockHandle,
                __in KAllocator & allocator);

        public: // IAsyncEnumerator
            LogRecordLib::LogRecord::SPtr GetCurrent();

            ktl::Awaitable<bool> MoveNextAsync(
                __in ktl::CancellationToken const & cancellationToken);

            void Reset();

        public: // IDisposable
            void Dispose();

        public:
            ktl::Awaitable<void> CloseAsync();

        private:
            ktl::Awaitable<bool> ReadBlockAsync();

        private: // Constructor
            BackupLogFileAsyncEnumerator(
                __in KWString const & fileName,
                __in ULONG32 logRecordCount,
                __in Utilities::BlockHandle & blockHandle) noexcept;

        private: // Static constants
            static const UINT8 BlockSizeSectionSize = sizeof(ULONG32);
            static const UINT8 CheckSumSectionSize = sizeof(ULONG64);

        private: // Initializer list initialized constants.
            const KWString fileName_;
            const ULONG32 logRecordCount_;
            const Utilities::BlockHandle::CSPtr blockHandeCSPtr_;
            const LogRecordLib::InvalidLogRecords::SPtr invalidLogRecords_;

        private: // Default initialized values
            LONG32 currentIndex_ = -1;
            bool isBlockDrained_ = false;
            bool isDisposed_ = false;

        private: // Constructor initialized
            KSharedArray<LogRecordLib::LogRecord::SPtr>::SPtr currentLogRecordsSPtr_;

        private:
            KBlockFile::SPtr fileSPtr_;
            ktl::io::KFileStream::SPtr fileStreamSPtr_;
        };
    }
}
