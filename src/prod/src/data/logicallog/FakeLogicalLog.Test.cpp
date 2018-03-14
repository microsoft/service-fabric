// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LogTests
{
    using namespace Common;
    using namespace Data::Log;
    using namespace Data::Utilities;

    class FakeStream 
        : public KAsyncServiceBase
        , public ILogicalLogReadStream
    {
        K_FORCE_SHARED(FakeStream)
        K_SHARED_INTERFACE_IMP(KStream)
        K_SHARED_INTERFACE_IMP(ILogicalLogReadStream)
        K_SHARED_INTERFACE_IMP(IDisposable)

    public:

        static NTSTATUS
            Create(
                __out ILogicalLogReadStream::SPtr& Stream,
                __in KAllocator& Allocator
            )
        {
            NTSTATUS status;
            ILogicalLogReadStream::SPtr output;

            Stream = nullptr;

            output = _new(KTL_TAG_TEST, Allocator) FakeStream();

            if (!output)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                return status;
            }

            status = static_cast<FakeStream&>(*output).Status();
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            Stream = Ktl::Move(output);
            return STATUS_SUCCESS;
        }

        VOID Dispose() override {}

        virtual ktl::Awaitable<NTSTATUS> ReadAsync(
            __in KBuffer& Buffer,
            __out ULONG& BytesRead,
            __in ULONG Offset,
            __in ULONG Count
        ) override
        {
            BytesRead = 0;
            co_await suspend_never{};
            co_return STATUS_SUCCESS;
        }

        virtual ktl::Awaitable<NTSTATUS> WriteAsync(
            __in KBuffer const & Buffer,
            __in ULONG Offset,
            __in ULONG Count
        ) override
        {
            co_await suspend_never{};
            co_return STATUS_SUCCESS;
        }

        virtual ktl::Awaitable<NTSTATUS> FlushAsync() override
        {
            co_await suspend_never{};
            co_return STATUS_SUCCESS;
        }

        virtual ktl::Awaitable<NTSTATUS> CloseAsync() override
        {
            co_await suspend_never{};
            co_return STATUS_SUCCESS;
        }

        LONGLONG GetPosition() const override
        {
            return(0);
        }

        void SetPosition(__in LONGLONG Position) override
        {
            UNREFERENCED_PARAMETER(Position);
            return;
        }

        virtual LONGLONG GetLength() const override
        {
            return 0;
        }

    };

    FakeStream::FakeStream()
    {
    }

    FakeStream::~FakeStream()
    {
    }

    using namespace Data::Utilities;
    using namespace std;
    using namespace ktl;
    using namespace ktl::io;
    using namespace ktl::kservice;

    Common::StringLiteral const TraceComponent("FakeLogicalLogTest");

    class FakeLogicalLogTest
    {
    protected:

        VOID BeginTest() {}
        VOID EndTest()
        {
            prId_ = nullptr;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
        ::FABRIC_REPLICA_ID rId_;
        KGuid pId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem* underlyingSystem_;
        KAllocator* allocator_;
    };

    BOOST_FIXTURE_TEST_SUITE(FakeLogicalLogTestSuite, FakeLogicalLogTest)

    BOOST_AUTO_TEST_CASE(CallApis)
    {
        TEST_TRACE_BEGIN("CallApis")
        {
            ILogicalLog::SPtr log;
            status = FakeLogicalLog::Create(log, *allocator_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            CancellationToken none;
            status = SyncAwait(log->CloseAsync(none));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            log->Abort();

            CODING_ERROR_ASSERT(0 == log->GetLength());
            CODING_ERROR_ASSERT(0 == log->GetWritePosition());
            CODING_ERROR_ASSERT(0 == log->GetReadPosition());
            CODING_ERROR_ASSERT(0 == log->GetHeadTruncationPosition());
            CODING_ERROR_ASSERT(0 == log->GetMaximumBlockSize());
            CODING_ERROR_ASSERT(0 == log->GetMetadataBlockHeaderSize());

            ILogicalLogReadStream::SPtr stream;
            status = FakeStream::Create(stream, *allocator_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = log->CreateReadStream(stream, 0);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            log->SetSequentialAccessReadSize(*stream, 0);

            LONG bytesRead;
            KBuffer::SPtr buffer;
            status = KBuffer::Create(256, buffer, *allocator_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            status = SyncAwait(log->ReadAsync(bytesRead, *buffer, 0, 0, 0, none));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            CODING_ERROR_ASSERT(NT_SUCCESS(log->SeekForRead(0, SeekOrigin::Begin)));

            status = SyncAwait(log->AppendAsync(*buffer, 0, 0, none));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(log->FlushAsync(none));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(log->FlushWithMarkerAsync(none));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(log->TruncateHead(0));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(log->TruncateTail(0, none));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(log->WaitCapacityNotificationAsync(0, none));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(log->WaitBufferFullNotificationAsync(none));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(log->ConfigureWritesToOnlyDedicatedLogAsync(none));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(log->ConfigureWritesToSharedAndDedicatedLogAsync(none));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ULONG p;
            status = SyncAwait(log->QueryLogUsageAsync(p, none));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
