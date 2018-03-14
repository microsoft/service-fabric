// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        class FakeLogicalLog 
            : public KAsyncServiceBase
            , public ILogicalLog
        {
            K_FORCE_SHARED(FakeLogicalLog)
            K_SHARED_INTERFACE_IMP(ILogicalLog)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:

            static NTSTATUS
                Create(
                    __out ILogicalLog::SPtr& Log,
                    __in KAllocator& Allocator)
            {
                NTSTATUS status;
                ILogicalLog::SPtr output;

                Log = nullptr;

                output = _new(KTL_TAG_TEST, Allocator) FakeLogicalLog();

                if (!output)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    return status;
                }

                status = static_cast<FakeLogicalLog&>(*output).Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                Log = Ktl::Move(output);
                return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> CloseAsync(
                __in ktl::CancellationToken const& cancellationToken
            ) override
            {
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }

            void Abort() override
            {
            }

            void Dispose() override
            {
                Abort();
            }

            BOOLEAN GetIsFunctional() override
            {
                return TRUE;
            }

            LONGLONG GetLength() const override
            {
                return 0;
            }

            LONGLONG GetWritePosition() const override
            {
                return 0;
            }

            LONGLONG GetReadPosition() const override
            {
                return 0;
            }

            LONGLONG GetHeadTruncationPosition() const override
            {
                return 0;
            }

            LONGLONG GetMaximumBlockSize() const override
            {
                return 0;
            }

            ULONG GetMetadataBlockHeaderSize() const override
            {
                return 0;
            }

            NTSTATUS CreateReadStream(__out ILogicalLogReadStream::SPtr& Stream, __in LONG SequentialAccessReadSize) override
            {
                UNREFERENCED_PARAMETER(Stream);
                UNREFERENCED_PARAMETER(SequentialAccessReadSize);
                return STATUS_SUCCESS;
            }

            void SetSequentialAccessReadSize(__in  ILogicalLogReadStream& LogStream, __in LONG SequentialAccessReadSize) override
            {
                UNREFERENCED_PARAMETER(LogStream);
                UNREFERENCED_PARAMETER(SequentialAccessReadSize);
            }

            ktl::Awaitable<NTSTATUS> ReadAsync(
                __out LONG& BytesRead,
                __in KBuffer& Buffer,
                __in ULONG Offset,
                __in ULONG Count,
                __in ULONG BytesToRead,
                __in ktl::CancellationToken const& cancellationToken
            ) override
            {
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }

            NTSTATUS SeekForRead(__in LONGLONG Offset, Common::SeekOrigin::Enum Origin) override
            {
                UNREFERENCED_PARAMETER(Offset);
                UNREFERENCED_PARAMETER(Origin);
                return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> AppendAsync(
                __in KBuffer const& Buffer,
                __in LONG Offset,
                __in ULONG Count,
                __in ktl::CancellationToken const& cancellationToken
            ) override
            {
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> FlushAsync(__in ktl::CancellationToken const& cancellationToken) override
            {
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> FlushWithMarkerAsync(__in ktl::CancellationToken const& cancellationToken) override
            {
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> TruncateHead(__in LONGLONG StreamOffset) override
            {
                UNREFERENCED_PARAMETER(StreamOffset);
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> TruncateTail(__in LONGLONG StreamOffset, __in ktl::CancellationToken const& cancellationToken) override
            {
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> WaitCapacityNotificationAsync(__in ULONG PercentOfSpaceUsed, __in ktl::CancellationToken const& cancellationToken) override
            {
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> WaitBufferFullNotificationAsync(__in ktl::CancellationToken const& cancellationToken) override
            {
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> ConfigureWritesToOnlyDedicatedLogAsync(__in ktl::CancellationToken const& cancellationToken) override
            {
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> ConfigureWritesToSharedAndDedicatedLogAsync(__in ktl::CancellationToken const& cancellationToken) override
            {
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> QueryLogUsageAsync(__out ULONG& PercentUsage, __in ktl::CancellationToken const& cancellationToken) override
            {
                co_await suspend_never{};
                co_return STATUS_SUCCESS;
            }
        };
    }
}
