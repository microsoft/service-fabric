// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        class PhysicalLog;
        class LogManagerHandle;

        /// <summary>
        /// LogManager implementation
        /// 
        /// The implementation OM for the LogManager class is:
        ///     LogManager which wraps KtlLogManager instance and contains:
        ///         - LogManagerHandle instances which implement ILogManagerHandle
        ///         - PhysicalLog instances which contains:
        ///             - PhysicalLogHandle instances which implement IPhysicalLogHandle
        ///             - LogicalLog instances which implements ILogicalLog
        /// </summary>
        class LogManager 
            : public KAsyncServiceBase
            , public Common::TextTraceComponent<Common::TraceTaskCodes::LogicalLog>
        {
            K_FORCE_SHARED(LogManager)

            friend class LogManagerHandle;
            friend class PhysicalLog;
            friend class PhysicalLogHandle;
            friend class LogicalLog;
            friend class LogTests::LogicalLogTest;
            
        public:

            static NTSTATUS Create(__in KAllocator& allocator, __out LogManager::SPtr& logManager);

            Common::AsyncOperationSPtr BeginOpen(
                __in Common::AsyncCallback const & callback,
                __in Common::AsyncOperationSPtr const & parent,
                __in ktl::CancellationToken const & cancellationToken,
                __in_opt KtlLogger::SharedLogSettingsSPtr sharedLogSettings = nullptr,
                __in_opt KtlLoggerMode ktlLoggerMode = KtlLoggerMode::Default);
            Common::ErrorCode EndOpen(__in Common::AsyncOperationSPtr const & asyncOperation);

            ktl::Awaitable<NTSTATUS> OpenAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __in_opt KtlLogger::SharedLogSettingsSPtr sharedLogSettings = nullptr,
                __in_opt KtlLoggerMode ktlLoggerMode = KtlLoggerMode::Default);

            Common::AsyncOperationSPtr BeginClose(
                __in Common::AsyncCallback const & callback,
                __in Common::AsyncOperationSPtr const & parent,
                __in ktl::CancellationToken const & cancellationToken);
            Common::ErrorCode EndClose(__in Common::AsyncOperationSPtr const & asyncOperation);

            ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken const & cancellationToken);

            VOID Abort();

            ktl::Awaitable<NTSTATUS> GetHandle(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in LPCWSTR workDirectory,
                __in ktl::CancellationToken const & cancellationToken,
                __out ILogManagerHandle::SPtr& handle);

            // Debugging Breadcrumbs - try to catch the corruption during open
            __forceinline void SetBeginOpenStarted()
            {
                KInvariant(openState_ == 0);
                openState_ |= 1;
            }

            __forceinline void SetFabricOpenOperationExecuting()
            {
                KInvariant(openState_ == 1);
                openState_ |= 2;
            }

            __forceinline void SetOpenAsyncStartAwaiting()
            {
                // OpenAsync can happen without BeginOpen
                KInvariant(openState_ == 3 || openState_ == 0);
                openState_ |= 4;
            }

            __forceinline void SetOpenAsyncFinishAwaiting()
            {
                // OpenAsync can happen without BeginOpen
                KInvariant(openState_ == 7 || openState_ == 4);
                openState_ |= 8;
            }

            __forceinline void SetFabricOpenOpenAsyncCompleted()
            {
                KInvariant(openState_ == 15);
                openState_ |= 16;
            }

            __forceinline void SetFabricOpenOperationEnding()
            {
                KInvariant(openState_ == 31);
                openState_ |= 32;
            }

        private:

            // Test Support
            __declspec(property(get = get_IsLoaded)) BOOLEAN IsLoaded;
            BOOLEAN get_IsLoaded() const { return ktlLogManager_ != nullptr; }

            __declspec(property(get = get_LogsCount)) LONG LogsCount;
            LONG get_LogsCount() { return logs_.Count(); }

            __declspec(property(get = get_HandlesCount)) LONG HandlesCount;
            LONG get_HandlesCount() { return handles_.Count(); }

            VOID OnServiceOpen() override;
            ktl::Task OpenTask();
            VOID OnServiceClose() override;
            ktl::Task CloseTask();

            ktl::Task AbortTask();

            ktl::Awaitable<NTSTATUS> OnCloseHandle(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in ILogManagerHandle& toClose,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS> OnCreateAndOpenPhysicalLogAsync(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in KString const & pathToCommonContainer,
                __in KGuid const & physicalLogId,
                __in LONGLONG containerSize,
                __in ULONG maximumNumberStreams,
                __in ULONG maximumLogicalLogBlockSize,
                __in LogCreationFlags creationFlags,
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog);

            ktl::Awaitable<NTSTATUS> OnOpenPhysicalLogAsync(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in KString const & pathToCommonContainer,
                __in KGuid const & physicalLogId,
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog);

            ktl::Awaitable<NTSTATUS> OnDeletePhysicalLogAsync(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in KString const & pathToCommonPhysicalLog,
                __in KGuid const & physicalLogId,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS> OnPhysicalLogHandleClose(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in IPhysicalLogHandle& toClose,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS> OnLogicalLogClose(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in ILogicalLog& toClose,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS> DeleteLogicalLogAndMaybeDeletePhysicalLog(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in IPhysicalLogHandle& physicalLogHandle,
                __in KtlLogContainer& container,
                __in KString const & containerPath,
                __in KGuid const & containerId,
                __in KGuid const & logicalLogId,
                __in ktl::CancellationToken const & cancellationToken);

        private:

            unsigned short openState_ = 0;

            KAsyncEvent lock_;
            KHashTable<LONG, KSharedPtr<KWeakRef<LogManagerHandle>>> handles_;
            KHashTable<KGuid, KSharedPtr<PhysicalLog>> logs_;
            LONG nextHandleId_;
            KtlLogManager::SPtr ktlLogManager_;
            KtlLogger::SharedLogSettingsSPtr sharedLogSettings_;
            KString::SPtr sharedLogName_;
            KtlLoggerMode ktlLoggerMode_;
        };
    }
}
