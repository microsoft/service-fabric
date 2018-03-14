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

        class PhysicalLogHandle
            : public KAsyncServiceBase
            , public KWeakRefType<PhysicalLogHandle>
            , public IPhysicalLogHandle
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LogicalLog>
        {
            K_FORCE_SHARED(PhysicalLogHandle)
            K_SHARED_INTERFACE_IMP(IDisposable)
            K_SHARED_INTERFACE_IMP(IPhysicalLogHandle)

            friend class LogTests::LogicalLogTest;

        public:

            __declspec(property(get = get_OwnerId)) KGuid OwnerId;
            KGuid get_OwnerId() const { return owner_->Id; }

            __declspec(property(get = get_Owner)) PhysicalLog& Owner;
            PhysicalLog& get_Owner() const { return *owner_; }

            static NTSTATUS Create(
                __in ULONG allocationTag,
                __in KAllocator& allocator,
                __in LogManager& manager,
                __in PhysicalLog& owner,
                __in LONGLONG id,
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __out PhysicalLogHandle::SPtr& handle);

            ktl::Awaitable<NTSTATUS> OpenAsync(ktl::CancellationToken const & cancellationToken);

            __declspec(property(get = get_Id)) LONGLONG Id;
            LONGLONG get_Id() const { return id_; }

            VOID Dispose() override;

            VOID Abort() override;

            ktl::Awaitable<NTSTATUS> CloseAsync(ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> CreateAndOpenLogicalLogAsync(
                __in KGuid const & logicalLogId,
                __in_opt KString::CSPtr & optionalLogStreamAlias,
                __in KString const & path,
                __in_opt KBuffer::SPtr securityDescriptor,
                __in LONGLONG maximumSize,
                __in ULONG maximumBlockSize,
                __in LogCreationFlags creationFlags,
                __in ktl::CancellationToken const & cancellationToken,
                __out ILogicalLog::SPtr& logicalLog) override;

            ktl::Awaitable<NTSTATUS> OpenLogicalLogAsync(
                __in KGuid const & logicalLogId,
                __in ktl::CancellationToken const & cancellationToken,
                __out ILogicalLog::SPtr& logicalLog) override;

            ktl::Awaitable<NTSTATUS> DeleteLogicalLogAsync(
                __in KGuid const & logicalLogId,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> DeleteLogicalLogOnlyAsync(
                __in KGuid const & logicalLogId,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<NTSTATUS> AssignAliasAsync(
                __in KGuid const & logicalLogId,
                __in KString const & alias,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> ResolveAliasAsync(
                __in KString const & alias,
                __in ktl::CancellationToken const & cancellationToken,
                __out KGuid& logicalLogId) override;

            ktl::Awaitable<NTSTATUS> RemoveAliasAsync(
                __in KString const & alias,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> ReplaceAliasLogsAsync(
                __in KString const & sourceLogAliasName,
                __in KString const & logAliasName,
                __in KString const & backupLogAliasName,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> RecoverAliasLogsAsync(
                __in KString const & sourceLogAliasName,
                __in KString const & logAliasName,
                __in KString const & backupLogAliasName,
                __in ktl::CancellationToken const & cancellationToken,
                __out KGuid& logicalLogId) override;

        private:

            // Test support
            __declspec(property(get = get_IsFunctional)) BOOLEAN IsFunctional;
            BOOLEAN get_IsFunctional()
            {
                if (TryAcquireServiceActivity())
                {
                    KFinally([&] { ReleaseServiceActivity(); });
                    return owner_->underlyingContainer_->IsFunctional();
                }
                else
                {
                    return FALSE;
                }
            }

            PhysicalLogHandle(
                __in LogManager& manager,
                __in PhysicalLog& owner,
                __in LONGLONG id,
                __in Data::Utilities::PartitionedReplicaId const & prId);

            VOID OnServiceOpen() override;
            VOID OnServiceClose() override;

            ktl::Task CloseTask();
            ktl::Task AbortTask();

            KSharedPtr<LogManager> manager_;
            KSharedPtr<PhysicalLog> owner_;
            LONGLONG id_;
        };
    }
}

