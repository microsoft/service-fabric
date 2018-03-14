// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Data
{
    namespace Log
    {
        class FileLogicalLog;

        //
        // Note that FileLogicalLogReadStream is strictly single
        // threaded, that is, only one operation (ReadAsync,
        // WriteAsync, FlushAsync or CloseAsync) can be outstanding at
        // one time. This is a restriction of the underlying
        // KFileStream as well as the delayed initialization
        //
        class FileLogicalLogReadStream 
            : public KObject<FileLogicalLogReadStream>
            , public KShared<FileLogicalLogReadStream>
            , public ILogicalLogReadStream
        {
            K_FORCE_SHARED(FileLogicalLogReadStream)
            K_SHARED_INTERFACE_IMP(KStream)
            K_SHARED_INTERFACE_IMP(ILogicalLogReadStream)
            K_SHARED_INTERFACE_IMP(IDisposable)

            friend FileLogicalLog;
            
        public:
            static NTSTATUS Create(
                __out FileLogicalLogReadStream::SPtr& readStream,
                __in KBlockFile& logFile,
                __in FileLogicalLog& fileLogicalLog,
                __in ktl::io::KFileStream& underlyingStream,
                __in KAllocator& allocator,
                __in ULONG allocationTag);

            // assumption: lock is taken outside
            NTSTATUS InvalidateForWrite(
                __in LONGLONG StreamOffset,
                __in LONGLONG Length);

            ktl::Awaitable<NTSTATUS> InvalidateForWriteAsync(
                __in LONGLONG StreamOffset,
                __in LONGLONG Length);

            // assumption: lock is taken outside
            NTSTATUS InvalidateForTruncate(
                __in LONGLONG StreamOffset);

            ktl::Awaitable<NTSTATUS> InvalidateForTruncateAsync(
                __in LONGLONG StreamOffset);

        protected:
            ktl::Awaitable<NTSTATUS> Initialize();

            //
            // KStream interface
            //
        public:
                
            VOID Dispose() override;

            ktl::Awaitable<NTSTATUS> CloseAsync() override;

            ktl::Awaitable<NTSTATUS> ReadAsync(
                __in KBuffer& Buffer,
                __out ULONG& BytesRead,
                __in ULONG Offset,
                __in ULONG Count) override;

            ktl::Awaitable<NTSTATUS> WriteAsync(
                __in KBuffer const & Buffer,
                __in ULONG Offset,
                __in ULONG Count) override;

            ktl::Awaitable<NTSTATUS> FlushAsync() override;
            
            LONGLONG GetPosition() const override;
            void SetPosition(__in LONGLONG Position) override;
            LONGLONG GetLength() const override;

            //
            // Implementation
            //
        private:
            FileLogicalLogReadStream(
                __in KBlockFile& logFile,
                __in FileLogicalLog& fileLogicalLog,
                __in ktl::io::KFileStream& underlyingStream
                );

            ktl::Task DisposeTask();

        private:

            volatile LONG closed_ = 0;
            static const ULONG defaultLockTimeoutMs_ = 20000;
            KListEntry listEntry_;
            KSharedPtr<FileLogicalLog> fileLogicalLog_;
            ktl::io::KFileStream::SPtr underlyingStream_;
            BOOLEAN initialized_;
            KBlockFile::SPtr logFile_;
            Data::Utilities::ReaderWriterAsyncLock::SPtr apiLock_;
        };                   
    }
}
