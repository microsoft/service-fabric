// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        class FakePhysicalLog
            : public KObject<FakePhysicalLog>
            , public KShared<FakePhysicalLog>
            , public IPhysicalLogHandle
        {
            K_FORCE_SHARED(FakePhysicalLog)
            K_SHARED_INTERFACE_IMP(IPhysicalLogHandle)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:

            KDeclareDefaultCreate(FakePhysicalLog, fakePhysicalLog, KTL_TAG_TEST);

            VOID Dispose() override
            {

            }

            VOID Abort() override
            {

            }

            ktl::Awaitable<NTSTATUS> CloseAsync(ktl::CancellationToken const & cancellationToken) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> CreateAndOpenLogicalLogAsync(
                __in KGuid const & logicalLogId,
                __in_opt KString::CSPtr & optionalLogStreamAlias,
                __in KString const & path,
                __in_opt KBuffer::SPtr securityDescriptor,
                __in LONGLONG maximumSize,
                __in ULONG maximumBlockSize,
                __in LogCreationFlags creationFlags,
                __in ktl::CancellationToken const & cancellationToken,
                __out ILogicalLog::SPtr& logicalLog) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> OpenLogicalLogAsync(
                __in KGuid const & logicalLogId,
                __in ktl::CancellationToken const & cancellationToken,
                __out ILogicalLog::SPtr& logicalLog) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> DeleteLogicalLogAsync(
                __in KGuid const & logicalLogId,
                __in ktl::CancellationToken const & cancellationToken) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> DeleteLogicalLogOnlyAsync(
                __in KGuid const & logicalLogId,
                __in ktl::CancellationToken const & cancellationToken) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> AssignAliasAsync(
                __in KGuid const & logicalLogId,
                __in KString const & alias,
                __in ktl::CancellationToken const & cancellationToken) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> ResolveAliasAsync(
                __in KString const & alias,
                __in ktl::CancellationToken const & cancellationToken,
                __out KGuid& logicalLogId) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> RemoveAliasAsync(
                __in KString const & alias,
                __in ktl::CancellationToken const & cancellationToken) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> ReplaceAliasLogsAsync(
                __in KString const & sourceLogAliasName,
                __in KString const & logAliasName,
                __in KString const & backupLogAliasName,
                __in ktl::CancellationToken const & cancellationToken) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> RecoverAliasLogsAsync(
                __in KString const & sourceLogAliasName,
                __in KString const & logAliasName,
                __in KString const & backupLogAliasName,
                __in ktl::CancellationToken const & cancellationToken,
                __out KGuid& logicalLogId) override
            {
                co_return STATUS_SUCCESS;
            }
        };
    }
}
