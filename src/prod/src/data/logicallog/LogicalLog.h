// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        class LogicalLog
            : public KAsyncServiceBase
            , public KWeakRefType<LogicalLog>
            , public ILogicalLog
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LogicalLog>
        {
            K_FORCE_SHARED(LogicalLog)
            K_SHARED_INTERFACE_IMP(ILogicalLog)
            K_SHARED_INTERFACE_IMP(IDisposable)

            friend class LogicalLogStream;
            friend class LogTests::LogicalLogTest;

        public:

            static NTSTATUS Create(
                __in ULONG allocationTag,
                __in KAllocator& allocator,
                __in PhysicalLog& owner,
                __in LogManager& manager,
                __in LONGLONG owningHandleId,
                __in KGuid const & id,
                __in KtlLogStream& underlyingStream,
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __out LogicalLog::SPtr& logicalLog);

            ktl::Awaitable<NTSTATUS>
            OpenForCreate(__in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS>
            OpenForRecover(__in ktl::CancellationToken const & cancellationToken);

            __declspec(property(get = get_Id)) KGuid Id;
            KGuid get_Id() const { return id_; }

            __declspec(property(get = get_OwnerId)) KGuid OwnerId;
            KGuid get_OwnerId() const { return ownerId_; }

            __declspec(property(get = get_OwningHandleId)) LONGLONG OwningHandleId;
            LONGLONG const & get_OwningHandleId() const { return owningHandleId_; }

            // todo: VOID SetWriteDelay(LogWriteFaultInjectionParameters)

            ktl::Awaitable<NTSTATUS>
            DelayBeforeFlush(__in ktl::CancellationToken const & cancellationToken);

            VOID Dispose() override;

#pragma region ILogicalLog
            ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken const & cancellationToken) override;

            VOID Abort() override;

            LONGLONG GetLength() const override
            {
                return (nextWritePosition_ - headTruncationPoint_) - 1;
            }

            LONGLONG GetWritePosition() const override
            {
                return nextWritePosition_;
            }

            LONGLONG GetReadPosition() const override
            {
                return readContext_.ReadLocation;
            }

            LONGLONG GetHeadTruncationPosition() const override
            {
                return headTruncationPoint_;
            }

            LONGLONG GetMaximumBlockSize() const override
            {
                //
                // Maximum size that can be written is the Maximum IO buffer size plus the leftover room
                // in the metadata header
                //
                return maxBlockSize_ - recordOverhead_;
            }

            ULONG GetMetadataBlockHeaderSize() const override
            {
                return blockMetadataSize_;
            }

            ULONGLONG GetSize() const override
            {
                return logSize_;
            }

            ULONGLONG GetSpaceRemaining() const override
            {
                return logSpaceRemaining_;
            }

            NTSTATUS CreateReadStream(
                __out ILogicalLogReadStream::SPtr& stream,
                __in LONG sequentialAccessReadSize) override;

            VOID SetSequentialAccessReadSize(
                __in ILogicalLogReadStream& logStream,
                __in LONG sequentialAccessReadSize) override;

            ktl::Awaitable<NTSTATUS> ReadAsync(
                __out LONG& bytesRead,
                __in KBuffer& buffer,
                __in ULONG offset,
                __in ULONG count,
                __in ULONG bytesToRead,
                __in ktl::CancellationToken const & cancellationToken) override;

            NTSTATUS SeekForRead(
                __in LONGLONG Offset,
                __in Common::SeekOrigin::Enum Origin) override;

            ktl::Awaitable<NTSTATUS> AppendAsync(
                __in KBuffer const & buffer,
                __in LONG offsetIntoBuffer,
                __in ULONG count,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> FlushAsync(__in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> FlushWithMarkerAsync(__in ktl::CancellationToken const & cancellationToken) override;

            // todo: make this not a coroutine once I can change/delete filelogicallog
            ktl::Awaitable<NTSTATUS> TruncateHead(__in LONGLONG headTruncationPoint) override;

            ktl::Awaitable<NTSTATUS> TruncateTail(
                __in LONGLONG StreamOffset,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> WaitCapacityNotificationAsync(
                __in ULONG PercentOfSpaceUsed,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> WaitBufferFullNotificationAsync(__in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> ConfigureWritesToOnlyDedicatedLogAsync(__in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> ConfigureWritesToSharedAndDedicatedLogAsync(__in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> QueryLogUsageAsync(
                __out ULONG& PercentUsage,
                __in ktl::CancellationToken const & cancellationToken) override;
#pragma endregion

        private:

            // Test support
            BOOLEAN GetIsFunctional() override
            {
                if (TryAcquireServiceActivity())
                {
                    KFinally([&] { ReleaseServiceActivity(); });
                    return underlyingStream_->IsFunctional();
                }
                else
                {
                    return FALSE;
                }
            }

            FAILABLE
            LogicalLog::LogicalLog(
                __in PhysicalLog& owner,
                __in LogManager& manager,
                __in LONGLONG owningHandleId_,
                __in KGuid const & id,
                __in KtlLogStream& underlyingStream,
                __in Data::Utilities::PartitionedReplicaId const & prId);

            enum class OpenReason : short { Invalid = -1, Create = 0, Recover = 1};

            ktl::Awaitable<NTSTATUS> OpenAsync(OpenReason reason);
            VOID OnServiceOpen() override;
            ktl::Task OpenTask();

            VOID OnServiceClose() override;
            ktl::Task CloseTask();

            ktl::Awaitable<NTSTATUS>
            OnCreateAsync(__in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS>
            OnRecoverAsync(__in ktl::CancellationToken const & cancellationToken);

            ktl::Task AbortTask();

            ktl::Awaitable<NTSTATUS>
            QueryLogStreamRecoveryInfo(
                __in ktl::CancellationToken const & cancellationToken,
                __out KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation& recoveryInfo);

            ktl::Awaitable<NTSTATUS>
            QueryLogStreamReadInformation(
                __in ktl::CancellationToken const & cancellationToken,
                __out KLogicalLogInformation::LogicalLogReadInformation& readInformation);

            ktl::Awaitable<NTSTATUS>
            QueryDriverBuildInformation(
                __in ktl::CancellationToken const &  cancellationToken,
                __out KLogicalLogInformation::CurrentBuildInformation& driverBuildInformation);

            VOID
            QueryUserBuildInformation(__out ULONG& buildNumber, __out BYTE& isFreeBuild);

            ktl::Awaitable<NTSTATUS>
            QueryLogStreamUsageInternalAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out KLogicalLogInformation::CurrentLogUsageInformation& streamUsageInformation);

            ktl::Awaitable<NTSTATUS>
            QueryLogUsageAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out ULONG& percentageLogUsage);

            ktl::Awaitable<NTSTATUS>
            QueryLogSizeAndSpaceRemainingInternalAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out ULONGLONG& logSize,
                __out ULONGLONG& spaceRemaining);

            ktl::Awaitable<NTSTATUS>
            VerifyUserAndDriverBuildMatch();

            NTSTATUS
            CreateAndAddLogicalLogStream(
                __in LONG sequentialAccessReadSize,
                __out LogicalLogStream::SPtr& stream);

            VOID
            RemoveLogicalLogStream(LogicalLogStream const & stream);

            VOID
            InvalidateAllReads();

            NTSTATUS
            StartBackgroundRead(
                __in LONGLONG offset,
                __in LONG length,
                __out LogicalLogReadTask::SPtr& readTask);

            ktl::Awaitable<NTSTATUS>
            GetReadResultsAsync(
                __in LogicalLogReadTask& readTask,
                __out PhysicalLogStreamReadResults& results);

            ktl::Awaitable<NTSTATUS>
            InternalReadAsync(
                __in LogicalLogReadContext& readContext,
                __inout KBuffer& buffer,
                __in LONG offset,
                __in LONG count,
                __in LONG bytesToRead,
                __in ktl::CancellationToken const & cancellationToken,
                __out ReadAsyncResults& results);

            ktl::Awaitable<NTSTATUS>
            InternalFlushAsync(
                __in BOOLEAN isBarrier,
                __in ktl::CancellationToken const & cancellationToken);

            KGuid const id_;
            KGuid const ownerId_;
            KSharedPtr<LogManager> manager_;
            LONGLONG const owningHandleId_;
            KtlLogStream::SPtr underlyingStream_;
            ULONG const blockMetadataSize_;
            LogicalLogBuffer::SPtr writeBuffer_;

            LogicalLogReadContext readContext_;  // does this need to be a pointer type for atomic assignment/read?  it doesn't seem threadsafe...

            KArray<LogicalLogReadTask::SPtr> readTasks_;
            KSpinLock readTasksLock_;
            KArray<KWeakRef<LogicalLogStream>::SPtr> streams_;
            KSpinLock streamsLock_;

            volatile LONG internalFlushInProgress_;

            // Recoverable stream state
            LONGLONG nextWritePosition_;
            LONGLONG nextOperationNumber_;
            ULONG maxBlockSize_;
            LONGLONG headTruncationPoint_;
            ULONG maximumReadRecordSize_;
            LONGLONG recordOverhead_;
            USHORT interfaceVersion_;
            ULONGLONG logSize_;
            ULONGLONG logSpaceRemaining_;

            Common::Random randomGenerator_;
            // todo LogWriteFaultInjectionParameters logWriteFaultInjectionParameters

            OpenReason openReason_;
        };
    }
}
