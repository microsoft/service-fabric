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
        // File log based on Common::File sparse file implementation
        //
        class FileLog 
            : public KObject<FileLog>
            , public KShared<FileLog>
            , public Log::ILogicalLog
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED_WITH_INHERITANCE(FileLog)
            K_SHARED_INTERFACE_IMP(ILogicalLog)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:

            ktl::Awaitable<NTSTATUS> OpenAsync(
                __in KWString& logFileName,
                __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None);
 
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

            NTSTATUS CreateReadStream(__out Log::ILogicalLogReadStream::SPtr& Stream, __in LONG SequentialAccessReadSize) override;
            void SetSequentialAccessReadSize(__in Log::ILogicalLogReadStream& LogStream, __in LONG SequentialAccessReadSize) override;

            ktl::Awaitable<NTSTATUS> ReadAsync(
                __out LONG& BytesRead,
                __in KBuffer& Buffer,
                __in ULONG Offset,
                __in ULONG Count,
                __in ULONG BytesToRead,
                __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;

            NTSTATUS SeekForRead(__in LONGLONG Offset, __in Common::SeekOrigin::Enum Origin) override;

            virtual ktl::Awaitable<NTSTATUS> AppendAsync(
                __in KBuffer const & Buffer,
                __in LONG OffsetIntoBuffer,
                __in ULONG Count,
                __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;

            virtual ktl::Awaitable<NTSTATUS> FlushWithMarkerAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;

            ktl::Awaitable<NTSTATUS> FlushAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> TruncateHead(__in LONGLONG StreamOffset) override;
            ktl::Awaitable<NTSTATUS> TruncateTail(__in LONGLONG StreamOffset, __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> WaitCapacityNotificationAsync(__in ULONG PercentOfSpaceUsed, __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> WaitBufferFullNotificationAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> ConfigureWritesToOnlyDedicatedLogAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> ConfigureWritesToSharedAndDedicatedLogAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> QueryLogUsageAsync(__out ULONG& PercentUsage, __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
#pragma endregion
        
        protected:

            FileLog(
                __in Data::Utilities::PartitionedReplicaId const & traceId);

        private:

            class ReadStream;

            // Mutable to query size in const methods using setfilepointer API
            mutable Common::File logFile_;
        };
    }
}
