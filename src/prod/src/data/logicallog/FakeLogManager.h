// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        class FakeLogManager 
            : public KObject<FakeLogManager>
            , public KShared<FakeLogManager>
            , public ILogManagerHandle
        {
            K_FORCE_SHARED(FakeLogManager)
            K_SHARED_INTERFACE_IMP(ILogManagerHandle)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:

            KDeclareDefaultCreate(FakeLogManager, fakeLogManager, KTL_TAG_TEST);

            KtlLoggerMode get_Mode() const override { return KtlLoggerMode::InProc; }

            ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken const & cancellationToken) override
            {
                co_return STATUS_SUCCESS;
            }

            VOID Abort() override
            {
            }

            VOID Dispose() override
            {
                Abort();
            }

            ktl::Awaitable<NTSTATUS> CreateAndOpenPhysicalLogAsync(
                __in KString const &,
                __in KGuid const &,
                __in LONGLONG,
                __in ULONG,
                __in ULONG,
                __in LogCreationFlags,
                __in ktl::CancellationToken const &,
                __out IPhysicalLogHandle::SPtr&) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> CreateAndOpenPhysicalLogAsync(
                __in ktl::CancellationToken const &,
                __out IPhysicalLogHandle::SPtr&) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> OpenPhysicalLogAsync(
                __in KString const &,
                __in KGuid const &,
                __in ktl::CancellationToken const &,
                __out IPhysicalLogHandle::SPtr&) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> OpenPhysicalLogAsync(
                __in ktl::CancellationToken const &,
                __out IPhysicalLogHandle::SPtr&) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> DeletePhysicalLogAsync(
                __in KString const &,
                __in KGuid const &,
                __in ktl::CancellationToken const &) override
            {
                co_return STATUS_SUCCESS;
            }

            ktl::Awaitable<NTSTATUS> DeletePhysicalLogAsync(__in ktl::CancellationToken const &) override
            {
                co_return STATUS_SUCCESS;
            }
        };
    }
}
