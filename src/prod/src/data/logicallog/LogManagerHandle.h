// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        class LogManagerHandle
            : public KAsyncServiceBase
            , public KWeakRefType<LogManagerHandle>
            , public ILogManagerHandle
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LogicalLog>
        {
            K_FORCE_SHARED(LogManagerHandle)
            K_SHARED_INTERFACE_IMP(ILogManagerHandle)
            K_SHARED_INTERFACE_IMP(IDisposable)

            friend class LogTests::LogTestBase;
            friend class LogTests::LogicalLogTest;

        public:

            static NTSTATUS Create(
                __in KAllocator& allocator,
                __in long id,
                __in LogManager& owner,
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in LPCWSTR workDirectory,
                __out LogManagerHandle::SPtr& handle);

            __declspec(property(get = get_Id)) long Id;
            long get_Id() const { return id_; }

            KtlLoggerMode get_Mode() const override { return owner_->ktlLoggerMode_; }

            ktl::Awaitable<NTSTATUS> OpenAsync(__in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken const & cancellationToken) override;

            VOID Abort() override;

            VOID Dispose() override;

            ktl::Awaitable<NTSTATUS> CreateAndOpenPhysicalLogAsync(
                __in KString const & pathToCommonContainer,
                __in KGuid const & physicalLogId,
                __in LONGLONG containerSize,
                __in ULONG maximumNumberStreams,
                __in ULONG maximumLogicalLogBlockSize,
                __in LogCreationFlags creationFlags,
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog) override;

            ktl::Awaitable<NTSTATUS> CreateAndOpenPhysicalLogAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog) override;

            ktl::Awaitable<NTSTATUS> OpenPhysicalLogAsync(
                __in KString const & pathToCommonContainer,
                __in KGuid const & physicalLogId,
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog) override;

            ktl::Awaitable<NTSTATUS> OpenPhysicalLogAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog) override;

            ktl::Awaitable<NTSTATUS> DeletePhysicalLogAsync(
                __in KString const & pathToCommonPhysicalLog,
                __in KGuid const & physicalLogId,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> DeletePhysicalLogAsync(__in ktl::CancellationToken const & cancellationToken) override;

        private:

            FAILABLE
            LogManagerHandle(
                __in long id,
                __in LogManager& owner,
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in LPCWSTR workDirectory);

            VOID OnServiceOpen() override;
            VOID OnServiceClose() override;

            ktl::Awaitable<NTSTATUS> CreateAndOpenStagingLogAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog);

            ktl::Awaitable<NTSTATUS> OpenStagingLogAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog);

            ktl::Awaitable<NTSTATUS> DeleteStagingLogAsync(__in ktl::CancellationToken const & cancellationToken);

            ktl::Task CloseTask();
            ktl::Task AbortTask();

            LogManager::SPtr owner_;
            const long id_;
            KString::CSPtr stagingLogPath_;
        };
    }
}

