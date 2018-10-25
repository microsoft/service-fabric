// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Data
{
    namespace Log
    {
        class FileLogicalLog 
            : public KObject<FileLogicalLog>
            , public KShared<FileLogicalLog>
            , public ILogicalLog
        {
            K_FORCE_SHARED_WITH_INHERITANCE(FileLogicalLog)
            K_SHARED_INTERFACE_IMP(ILogicalLog)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:

            static NTSTATUS Create(
                __in KAllocator& allocator,
                __out FileLogicalLog::SPtr& fileLogicalLog);

            ktl::Awaitable<NTSTATUS> OpenAsync(
                __in KWString& logFileName,
                __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None);
 
            VOID RemoveReadStreamFromList(__in FileLogicalLogReadStream& ReadStream);

            VOID Dispose() override;

#pragma region ILogicalLog
            ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken const & cancellationToken) override;

            void Abort() override;
            BOOLEAN GetIsFunctional() override;
            LONGLONG GetLength() const override;
            LONGLONG GetWritePosition() const override;
            LONGLONG GetReadPosition() const override;
            LONGLONG GetHeadTruncationPosition() const override;
            LONGLONG GetMaximumBlockSize() const override;
            ULONG GetMetadataBlockHeaderSize() const override;
            ULONGLONG GetSize() const override;
            ULONGLONG GetSpaceRemaining() const override;

            NTSTATUS CreateReadStream(__out ILogicalLogReadStream::SPtr& Stream, __in LONG SequentialAccessReadSize) override;
            void SetSequentialAccessReadSize(__in ILogicalLogReadStream& LogStream, __in LONG SequentialAccessReadSize) override;

            ktl::Awaitable<NTSTATUS> ReadAsync(
                __out LONG& BytesRead,
                __in KBuffer& Buffer,
                __in ULONG Offset,
                __in ULONG Count,
                __in ULONG BytesToRead,
                __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;

            NTSTATUS SeekForRead(__in LONGLONG Offset, __in Common::SeekOrigin::Enum Origin) override;

            ktl::Awaitable<NTSTATUS> AppendAsync(
                __in KBuffer const & Buffer,
                __in LONG OffsetIntoBuffer,
                __in ULONG Count,
                __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;

            ktl::Awaitable<NTSTATUS> FlushAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> FlushWithMarkerAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> TruncateHead(__in LONGLONG StreamOffset) override;
            ktl::Awaitable<NTSTATUS> TruncateTail(__in LONGLONG StreamOffset, __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> WaitCapacityNotificationAsync(__in ULONG PercentOfSpaceUsed, __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> WaitBufferFullNotificationAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> ConfigureWritesToOnlyDedicatedLogAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> ConfigureWritesToSharedAndDedicatedLogAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> QueryLogUsageAsync(__out ULONG& PercentUsage, __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
#pragma endregion

        private:

            ktl::Awaitable<void> InvalidateStreamsForWriteAsync(__in LONGLONG StreamOffset, __in LONGLONG Length);
            ktl::Awaitable<void> InvalidateStreamsForTruncateAsync(__in LONGLONG StreamOffset);
            VOID AddReadStreamToList(__in FileLogicalLogReadStream& ReadStream);
            ktl::Awaitable<NTSTATUS> InternalFlushWithMarkerAsync();

            ktl::Task AbortTask();

        private:

            static const ULONG defaultLockTimeoutMs_ = 20000;
            static const ULONG cacheSize_ = 128 * 1024; // Buffer size is 128K
            static const ULONG fileExtendSize_ = 4 * 1024 * 1024; // File extend size

        private:

            KBlockFile::SPtr logFile_;
            
            ktl::io::KFileStream::SPtr writeStream_;
            LONGLONG lastFlushStreamOffset_;

            KSpinLock readStreamListLock_;
            static const ULONG readStreamListOffset_;
            KNodeList<FileLogicalLogReadStream> readStreamList_;

            Data::Utilities::ReaderWriterAsyncLock::SPtr writeStreamLock_;
        };
    }
}
