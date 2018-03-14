// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        class LogManager;
        class PhysicalLogHandle;
        class LogicalLog;

        class PhysicalLog
            : public KAsyncServiceBase
            , public Common::TextTraceComponent<Common::TraceTaskCodes::LogicalLog>
        {
            K_FORCE_SHARED(PhysicalLog)

            friend class PhysicalLogHandle;
            friend class LogManager;
            friend class LogTests::LogicalLogTest;

        public:

            static NTSTATUS Create(
                __in ULONG allocationTag,
                __in KAllocator& allocator,
                __in LogManager& manager,
                __in KGuid const & id,
                __in KString const & path,
                __in KtlLogContainer& underlyingContainer,
                __out PhysicalLog::SPtr& physicalLog);

            ktl::Awaitable<NTSTATUS> OpenAsync(__in ktl::CancellationToken const & cancellationToken);
            ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS> GetHandle(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in ktl::CancellationToken const & cancellationToken,
                __out IPhysicalLogHandle::SPtr& physicalLog);

            __declspec(property(get = get_Id)) KGuid Id;
            KGuid get_Id() const { return id_; }

        private:

            // Test Support
            __declspec(property(get = get_HandlesCount)) LONG HandlesCount;
            LONG get_HandlesCount() { return handles_.Count(); }

            __declspec(property(get = get_LogsCount)) LONG LogsCount;
            LONG get_LogsCount() { return logicalLogs_.Count(); }

            //__declspec(property(get = get_IsFunctional)) BOOLEAN IsFunctional;
            //BOOLEAN get_IsFunctional() { return underlyingContainer_->IsFunctional(); }

            VOID OnServiceOpen() override;

            PhysicalLog(__in LogManager& manager, __in KGuid id, __in KString const & path, __in KtlLogContainer& underlyingContainer);

            ktl::Awaitable<NTSTATUS> OnCloseHandle(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in PhysicalLogHandle& toClose,
                __in ktl::CancellationToken const & cancellationToken,
                __out BOOLEAN& lastHandleClosed);

            ktl::Awaitable<NTSTATUS> OnCloseLogicalLog(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in LogicalLog& toClose,
                __in ktl::CancellationToken const & cancellationToken,
                __out BOOLEAN& lastHandleClosed);

            ktl::Awaitable<NTSTATUS> OnCreateAndOpenLogicalLogAsync(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in PhysicalLogHandle& owningHandle,
                __in KGuid const & logicalLogId,
                __in_opt KString::CSPtr & optionalLogStreamAlias,
                __in KString const & path,
                __in_opt KBuffer::SPtr securityDescriptor,
                __in LONGLONG maximumSize,
                __in ULONG maximumBlockSize,
                __in LogCreationFlags creationFlags,
                __in ktl::CancellationToken const & cancellationToken,
                __out ILogicalLog::SPtr& logicalLog);

            ktl::Awaitable<NTSTATUS> OnOpenLogicalLogAsync(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in PhysicalLogHandle& owningHandle,
                __in KGuid const & logicalLogId,
                __in ktl::CancellationToken const & cancellationToken,
                __out ILogicalLog::SPtr& logicalLog);

            ktl::Awaitable<NTSTATUS> OnRemoveAliasAsync(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in KString const & alias,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS> OnReplaceAliasLogsAsync(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in KString const & sourceLogAliasName,
                __in KString const & logAliasName,
                __in KString const & backupLogAliasName,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS> OnRecoverAliasLogsAsync(
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __in KString const & sourceLogAliasName,
                __in KString const & logAliasName,
                __in KString const & backupLogAliasName,
                __in ktl::CancellationToken const & cancellationToken,
                __out KGuid& logicalLogId);

            KSharedPtr<LogManager> const manager_;
            LONGLONG nextHandleId_;
            KAsyncEvent lock_;
            KGuid const id_;
            KString::CSPtr path_;
            KtlLogContainer::SPtr const underlyingContainer_;
            KHashTable<KGuid, LogicalLogInfo> logicalLogs_;
            KHashTable<LONGLONG, KWeakRef<PhysicalLogHandle>::SPtr> handles_;
        };
    }
}
