// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define READER_WRITER_LOCK_TAG 'lwrA'

namespace Data 
{
    namespace Utilities
    {
        static const ULONGLONG ReaderWriterAsyncLockTypeName = K_MAKE_TYPE_TAG('  re', 'lwrA');

        class ReaderWriterAsyncLock : public KShared<ReaderWriterAsyncLock>, KObject<ReaderWriterAsyncLock>
        {
            K_FORCE_SHARED(ReaderWriterAsyncLock)

        public:
            typedef KDelegate<void()> SignalOwnersTestFunctionType;

            static NTSTATUS
                Create(
                    __in KAllocator& Allocator, 
                    __in ULONG Tag, 
                    __out ReaderWriterAsyncLock::SPtr& Result);

            // For testing purposes only!
            static NTSTATUS
                Create(
                    __in SignalOwnersTestFunctionType TestSignalOwnersAction,
                    __in KAllocator& Allocator, 
                    __in ULONG Tag, 
                    __out ReaderWriterAsyncLock::SPtr& Result);

            __declspec(property(get = get_IsClosed)) bool IsClosed;
            bool get_IsClosed() const
            {
                return isClosed_;
            }

            __declspec(property(get = get_IsActiveWriter)) bool IsActiveWriter;
            bool get_IsActiveWriter() const 
            {
                return (activeWriterAcs_.Get() != nullptr) == TRUE;
            }

            __declspec(property(get = get_ActiveReaderCount)) int ActiveReaderCount;
            int get_ActiveReaderCount() const
            {
                return activeReaderCount_;
            }

            __declspec(property(get = get_WaitingReaderCount)) int WaitingReaderCount;
            int get_WaitingReaderCount() const
            {
                return waitingReaderCount_;
            }

            __declspec(property(get = get_WaitingWriterCount)) int WaitingWriterCount;
            int get_WaitingWriterCount() const
            {
                int count = waitingWritersList_.Count();
                return count;
            }

            void Close();
            ktl::Awaitable<bool> WaitForLock(
                __in ktl::AwaitableCompletionSource<bool> & waitingCompletionSource, 
                __in bool writer, 
                __in int timeoutInMilliSeconds); //TODO: Make this private
            ktl::Awaitable<bool> ReaderWriterAsyncLock::AcquireReadLockAsync(__in int timeoutInMilliSeconds);
            ktl::Awaitable<bool> ReaderWriterAsyncLock::AcquireWriteLockAsync(__in int timeoutInMilliSeconds);
            void ReleaseReadLock();
            void ReleaseWriteLock();

        private:
            ReaderWriterAsyncLock(__in SignalOwnersTestFunctionType TestSignalOwnersAction);

            int ReaderWriterAsyncLock::GetIndex(__in KSharedPtr<ktl::AwaitableCompletionSource<bool>> waitingTcs);

            KArray<KSharedPtr<ktl::AwaitableCompletionSource<bool>>> waitingWritersList_;
            KSharedPtr<ktl::AwaitableCompletionSource<bool>> lastSignaledReaderAcs_ = nullptr;
            ThreadSafeSPtrCache<ktl::AwaitableCompletionSource<bool>> waitingReadersAcs_ = { nullptr };
            ThreadSafeSPtrCache<ktl::AwaitableCompletionSource<bool>> activeWriterAcs_ = { nullptr };

            int waitingReaderCount_ = 0;
            int activeReaderCount_ = 0;
            bool isClosed_ = false;
            KSpinLock readerWriterLock_;

            SignalOwnersTestFunctionType signalOwnersTestAction_;
            void SignalCompletionSource(__in ktl::AwaitableCompletionSource<bool> & tcs);
        };
    }
}
