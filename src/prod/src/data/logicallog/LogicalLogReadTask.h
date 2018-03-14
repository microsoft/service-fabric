// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        class LogicalLogReadTask 
            : public KObject<LogicalLogReadTask>
            , public KShared<LogicalLogReadTask>
        {
            K_FORCE_SHARED(LogicalLogReadTask)

        public:

            static NTSTATUS Create(
                __in ULONG allocationTag,
                __in KAllocator& allocator,
                __in KtlLogStream& underlyingStream,
                __out LogicalLogReadTask::SPtr& readTask);

            BOOLEAN IsInRange(__in LONGLONG offset) const;

            __declspec(property(get = GetOffset)) LONGLONG Offset;
            LONGLONG GetOffset() const;

            __declspec(property(get = GetLength)) LONG Length;
            LONG GetLength() const;

            BOOLEAN IsValid() const;

            VOID InvalidateRead();

            BOOLEAN HandleWriteThrough(__in LONGLONG writeOffset, __in LONG writeLength);

            VOID StartRead(__in LONGLONG offset, __in LONG length);

            ktl::Awaitable<NTSTATUS> GetReadResults(__out PhysicalLogStreamReadResults& results);

        private:

            ktl::Task StartReadInternal();

            LogicalLogReadTask(__in KtlLogStream& underlyingStream);

            KtlLogStream::SPtr const underlyingStream_;

            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr readTcs_;

            BOOLEAN isValid_;
            LONGLONG endOffset_;
            LONGLONG offset_;
            LONG length_;

            PhysicalLogStreamReadResults results_;
        };
    }
}
