// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        class LogicalLog;

        class LogicalLogStream 
            : public KObject<LogicalLogStream>
            , public KShared<LogicalLogStream>
            , public KWeakRefType<LogicalLogStream>
            , public ILogicalLogReadStream
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LogicalLog>
        {
            K_FORCE_SHARED(LogicalLogStream)
            K_SHARED_INTERFACE_IMP(ILogicalLogReadStream)
            K_SHARED_INTERFACE_IMP(KStream)
            K_SHARED_INTERFACE_IMP(IDisposable)

            friend class LogicalLog;

        public:

            static NTSTATUS Create(
                __in ULONG allocationTag,
                __in KAllocator& allocator,
                __in LogicalLog& parent,
                __in USHORT interfaceVersion,
                __in LONG sequentialAccessReadSize,
                __in ULONG streamArrayIndex,
                __in Data::Utilities::PartitionedReplicaId const & prId,
                __out LogicalLogStream::SPtr& stream);

            VOID InvalidateReadAhead();

            VOID SetSequentialAccessReadSize(__in LONG sequentialAccessReadSize);

            LONGLONG GetLength() const override;

            __declspec(property(get = GetCanRead)) BOOLEAN CanRead;
            BOOLEAN GetCanRead() const { return TRUE; }

            __declspec(property(get = GetCanSeek)) BOOLEAN CanSeek;
            BOOLEAN GetCanSeek() const { return TRUE; }

            __declspec(property(get = GetCanWrite)) BOOLEAN CanWrite;
            BOOLEAN GetCanWrite() const { return FALSE; }

            __declspec(property(get = GetPosition, put = SetPosition)) LONGLONG Position;
            LONGLONG GetPosition() const override { return readContext_.ReadLocation; }
            VOID SetPosition(__in LONGLONG value) override { Seek(value, Common::SeekOrigin::Begin); }

            VOID Dispose() override;

            ktl::Awaitable<NTSTATUS> CloseAsync() override;

            ktl::Awaitable<NTSTATUS> FlushAsync() override;

            // todo: fix Buffer SAL in KStream::ReadAsync
            ktl::Awaitable<NTSTATUS> ReadAsync(
                __out KBuffer& buffer,
                __out ULONG& bytesRead,
                __in ULONG offsetIntoBuffer = 0,
                __in ULONG count = 0) override;

            ktl::Awaitable<NTSTATUS> WriteAsync(
                __in KBuffer const & buffer,
                __in ULONG offsetIntoBuffer = 0,
                __in ULONG count = 0) override;

            LONGLONG Seek(
                __in LONGLONG offset,
                __in Common::SeekOrigin::Enum seekOrigin);

        private:

            VOID Cleanup();

            LogicalLogStream(
                __in LogicalLog& parent,
                __in USHORT interfaceVersion,
                __in LONG sequentialAccessReadSize,
                __in ULONG streamArrayIndex,
                __in Data::Utilities::PartitionedReplicaId const & prId);

            KSharedPtr<LogicalLog> parent_;
            LogicalLogReadContext readContext_;
            LONG sequentialAccessReadSize_;
            USHORT interfaceVersion_;
            ULONG streamArrayIndex_;
            BOOLEAN addedToStreamArray_;
        };
    }
}
